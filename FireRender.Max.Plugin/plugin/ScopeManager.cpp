/*********************************************************************************************************************************
* Radeon ProRender for 3ds Max plugin
* Copyright (c) 2017 AMD
* All Rights Reserved
*
* Manages Scopes. Scopes include a Radeon ProRender context and material system, a collection of instantiated scene objects,
* a material translator instance. Scopes also deal with Radeon ProRender context initialization.
*********************************************************************************************************************************/

#include <math.h>
#include "resource.h"
#include "RprTools.h"
#include "plugin/ParamBlock.h"
#include "utils/Thread.h"
#include "ScopeManager.h"
#include "CoronaDeclarations.h"

#include <maxscript/maxscript.h>
#include <maxscript/compiler/parser.h>
#include <maxscript/macros/value_locals.h>
#include <maxscript/kernel/exceptions.h>

#define _WIN32_DCOM
#include <iostream>
#include <comdef.h>
#include <Wbemidl.h>

extern HINSTANCE hInstance;

FIRERENDER_NAMESPACE_BEGIN;

ScopeManagerMax ScopeManagerMax::TheManager;

GpuInfoArray ScopeManagerMax::gpuInfoArray;

bool ScopeManagerMax::CoronaOK = false;

class ScopeManagerMaxClassDesc : public ClassDesc
{
public:
	int             IsPublic() { return FALSE; }
	void*           Create(BOOL loading) { return &ScopeManagerMax::TheManager; }
	const TCHAR*    ClassName() { return _T("RPRScopeManager"); }
	SClass_ID       SuperClassID() { return GUP_CLASS_ID; }
	Class_ID        ClassID() { return SCOPEMANAGER_MAX_CLASSID; }
	const TCHAR*    Category() { return _T(""); }
};

static ScopeManagerMaxClassDesc ScopeManagerMaxCD;

ClassDesc* ScopeManagerMax::GetClassDesc()
{
	return &ScopeManagerMaxCD;
}

//////////////////////////////////////////////////////////////////////////////
//

// We are trying to make contexts until we find out max gpu count supported
// by getting the create call that returns error
// but if we have an active context already having this failure breaks 
// good context somehow - see AMDMAX-807
// so - calculating this only once, before any context was created
namespace
{
	static const rpr_creation_flags gpuIdFlags[] =
	{
		RPR_CREATION_FLAGS_ENABLE_GPU0,
		RPR_CREATION_FLAGS_ENABLE_GPU1,
		RPR_CREATION_FLAGS_ENABLE_GPU2,
		RPR_CREATION_FLAGS_ENABLE_GPU3,
		RPR_CREATION_FLAGS_ENABLE_GPU4,
		RPR_CREATION_FLAGS_ENABLE_GPU5,
		RPR_CREATION_FLAGS_ENABLE_GPU6,
		RPR_CREATION_FLAGS_ENABLE_GPU7
	};

	static const rpr_creation_flags gpuIdNames[] =
	{
		RPR_CONTEXT_GPU0_NAME,
		RPR_CONTEXT_GPU1_NAME,
		RPR_CONTEXT_GPU2_NAME,
		RPR_CONTEXT_GPU3_NAME,
		RPR_CONTEXT_GPU4_NAME,
		RPR_CONTEXT_GPU5_NAME,
		RPR_CONTEXT_GPU6_NAME,
		RPR_CONTEXT_GPU7_NAME
	};

	static const int gpuCountMax = sizeof(gpuIdFlags) / sizeof(gpuIdFlags[0]);
	static bool gpuComputed = false;
};


ScopeManagerMax::ScopeManagerMax()
{
	gpuComputed = false;

	char suffix[128 + 1] = {};
	sprintf_s(suffix, 128, "%x", RPR_API_VERSION);
	auto cacheFolder = ToAscii(GetDataStoreFolder());
	cacheFolder += "cache";
	rootCacheFolder = cacheFolder;
	cacheFolder += "\\";
	cacheFolder += suffix;
	cacheFolder += "\\";
	subCacheFolder = cacheFolder;

	CreateDirectoryA(rootCacheFolder.c_str(), NULL);
	CreateDirectoryA(subCacheFolder.c_str(), NULL);
}


static void NotifyProc(void *param, NotifyInfo *info)
{
	if (info->intcode == NOTIFY_SYSTEM_STARTUP)
	{
		FPValue res = 0;

		FASSERT(ExecuteMAXScriptScript(
			L"quiet = GetQuietMode()\n"
			L"SetQuietMode true\n"
			L"r = renderers.current\n"
			L"coronaOK = 1\n"
			L"try\n"
			L"(renderers.current = CoronaRenderer())\n"
			L"catch\n"
			L"(\n"
			L"	coronaOK = 0\n"
			L")\n"
			L"renderers.current = r\n"
			L"SetQuietMode quiet\n"
			L"coronaOK;"
			, TRUE, &res));

		ScopeManagerMax::CoronaOK = (res.i == 1);
	}
}

// Activate and Stay Resident
//

DWORD ScopeManagerMax::Start()
{
	int res;
	res = RegisterNotification(NotifyProc, this, NOTIFY_SYSTEM_STARTUP);
	return GUPRESULT_KEEP;
}

void ScopeManagerMax::Stop()
{
	UnRegisterNotification(NotifyProc, this, NOTIFY_SYSTEM_STARTUP);
}

void ScopeManagerMax::DeleteThis()
{
}

ScopeID ScopeManagerMax::nextScopeId = 0;

ScopeID ScopeManagerMax::CreateScope(IParamBlock2 *pblock)
{
	static bool dumpTrace = false;
	if (GetFromPb<bool>(pblock, PARAM_TRACEDUMP_BOOL) != dumpTrace)
	{
		dumpTrace = !dumpTrace;
		EnableRPRTrace(pblock, dumpTrace);
	}

	// fix any GPU count issues to prevent errors when creating context
	ValidateParamBlock(pblock);

	auto renderDevice = GetFromPb<int>(pblock, PARAM_RENDER_DEVICE);

	auto useGpu = (renderDevice == RPR_RENDERDEVICE_GPUONLY) || (renderDevice == RPR_RENDERDEVICE_CPUGPU);
	bool useCpu = (renderDevice == RPR_RENDERDEVICE_CPUONLY) || (renderDevice == RPR_RENDERDEVICE_CPUGPU);

	int createFlags = getContextCreationFlags(useGpu, useCpu);

	rpr_context context;
	bool contextCreated = CreateContext(createFlags, context);
	FASSERT(contextCreated);

	frw::Scope theScope = frw::Scope(context, true);

	if (auto ctx = theScope.GetContext())
	{
		DebugPrint(L"Created Radeon ProRender Context\n");

		int stackSize = ctx.GetMaterialStackSize();
		DebugPrint(L"\tMaterial Stack Size: %d\n", stackSize);

		DebugPrint(L"\tAvailable Parameters: \n");

		// iterating one less because RPR 211 was returning error code on the last(n-1)
		for (int i = 0, n = ctx.GetParameterCount(); i < (n - 1); i++)
		{
			auto info = ctx.GetParameterInfo(i);
			DebugPrint(L"\t+ %s\t(%d)\t%s\n", ToUnicode(info.name).c_str(), info.type, ToUnicode(info.description).c_str());
		}

		BOOL useIrradianceClamp = FALSE;
		pblock->GetValue(PARAM_USE_IRRADIANCE_CLAMP, 0, useIrradianceClamp, Interval());
		if (useIrradianceClamp)
		{
			float irradianceClamp = FLT_MAX;
			pblock->GetValue(PARAM_IRRADIANCE_CLAMP, 0, irradianceClamp, Interval());
			ctx.SetParameter("radianceclamp", irradianceClamp);
		}
		else
			ctx.SetParameter("radianceclamp", FLT_MAX);
	}

	if (!theScope.IsValid())
		return -1;

	scopes.insert(std::make_pair(nextScopeId, theScope));

	ScopeID ret = nextScopeId;
	if (nextScopeId == INT_MAX)
		nextScopeId = 0;
	else
		nextScopeId++;

	return ret;
}

ScopeID ScopeManagerMax::CreateLocalScope(ScopeID parent)
{
	frw::Scope theParent = GetScope(parent);
	if (!theParent.IsValid())
		return -1;
	frw::Scope theScope = theParent.CreateLocalScope();
	if (!theScope.IsValid())
		return -1;

	scopes.insert(std::make_pair(nextScopeId, theScope));

	ScopeID ret = nextScopeId;
	if (nextScopeId == INT_MAX)
		nextScopeId = 0;
	else
		nextScopeId++;
	return ret;
}

ScopeID ScopeManagerMax::CreateChildScope(ScopeID parent)
{
	frw::Scope theParent = GetScope(parent);
	if (!theParent.IsValid())
		return -1;
	frw::Scope theScope = theParent.CreateChildScope();
	if (!theScope.IsValid())
		return -1;

	scopes.insert(std::make_pair(nextScopeId, theScope));

	ScopeID ret = nextScopeId;
	if (nextScopeId == INT_MAX)
		nextScopeId = 0;
	else
		nextScopeId++;
	return ret;
}

void ScopeManagerMax::DestroyScope(ScopeID id)
{
	if (scopes.find(id) != scopes.end())
		scopes.erase(id);
}

void ScopeManagerMax::EnableRPRTrace(IParamBlock2 *pblock, bool enable)
{
	if (enable)
	{
		std::wstring traceFolder = GetRPRTraceDirectory(pblock);

		if (!CreateDirectory(traceFolder.c_str(), NULL))
		{
			auto error = GetLastError();
			if (ERROR_ALREADY_EXISTS != error)
			{
			}
		}

		auto res = rprContextSetParameterString(nullptr, "tracingfolder", ToAscii(traceFolder).c_str());
		FASSERT(RPR_SUCCESS == res);
	}

	auto res = rprContextSetParameter1u(nullptr, "tracing", enable ? 1 : 0);
	FASSERT(RPR_SUCCESS == res);
}

std::wstring ScopeManagerMax::GetRPRTraceDirectory(IParamBlock2 *pblock)
{
	const MCHAR* mpath = GetFromPb<const MCHAR*>(pblock, PARAM_TRACEDUMP_PATH);
	std::wstring path(mpath);

	if (path.empty())
	{
		path = GetDataStoreFolder();
	}
	path += L"trace\\";
	return path;
}

int ScopeManagerMax::GetCompiledShadersCount()
{
	bool x = true;
	int count = 0;
	std::string asd = subCacheFolder;
	asd += "*.bin";
	const char* file = asd.c_str();
	WIN32_FIND_DATAA FindFileData;
	HANDLE hFind;
	hFind = FindFirstFileA(file, &FindFileData);

	if (hFind != INVALID_HANDLE_VALUE)
	{
		count++;
		while ((x = FindNextFileA(hFind, &FindFileData)) == TRUE)
			count++;
		FindClose(hFind);
	}

	return count;
}

void ScopeManagerMax::ValidateParamBlock(IParamBlock2 *pblock)
{
	//set CPU mode if GPU is not compatible even before opening settings dialog
	int numCompatibleGPUs = 0;
	if (!hasGpuCompatibleWithFR(numCompatibleGPUs))
	{
		SetInPb(pblock, PARAM_RENDER_DEVICE, RPR_RENDERDEVICE_CPUONLY);
	}
}

bool ScopeManagerMax::hasGpuCompatibleWithFR(int& numberCompatibleGPUs)
{
	if (!gpuComputed)
	{
		rpr_creation_flags devicesUsed = RPR_CREATION_FLAGS_ENABLE_GPU0 | RPR_CREATION_FLAGS_ENABLE_GPU1 | RPR_CREATION_FLAGS_ENABLE_GPU2 | RPR_CREATION_FLAGS_ENABLE_GPU3 |
			RPR_CREATION_FLAGS_ENABLE_GPU4 | RPR_CREATION_FLAGS_ENABLE_GPU5 | RPR_CREATION_FLAGS_ENABLE_GPU6 | RPR_CREATION_FLAGS_ENABLE_GPU7;
		rpr_creation_flags devicesCompatibleOut = 0;
		rprAreDevicesCompatible(
			"Tahoe64.dll",
			//NULL,
			false,
			devicesUsed,
			&devicesCompatibleOut,
#ifdef OSMac_
			RPRTOS_MACOS
#elif __linux__
			RPRTOS_LINUX
#else
			RPRTOS_WINDOWS
#endif
		);

		if (devicesCompatibleOut)
		{
			for (int i = 0; i < 8; i++)
			{
				if (devicesCompatibleOut & gpuIdFlags[i])
				{
					rpr_context temporaryContext = 0;
					if (CreateContext(gpuIdFlags[i], temporaryContext))
					{
						size_t size = 0;
						int status = rprContextGetInfo(temporaryContext, gpuIdNames[i], 0, 0, &size);
						if (status != RPR_SUCCESS) { throw  RPR_ERROR_INVALID_PARAMETER; }

						char* deviceName = new char[size];
						status = rprContextGetInfo(temporaryContext, gpuIdNames[i], size, deviceName, 0);
						if (status != RPR_SUCCESS) { throw  RPR_ERROR_INVALID_PARAMETER; }

						GpuInfo info;
						info.name = deviceName;
						info.isUsed = true;
						info.isCompatible = true;
						info.isWhiteListed = IsDeviceNameWhitelisted(
							deviceName,
#ifdef OSMac_
							RPRTOS_MACOS
#elif __linux__
							RPRTOS_LINUX
#else
							RPRTOS_WINDOWS
#endif
						);
						info.frFlags = gpuIdFlags[i];
						gpuInfoArray.push_back(info);

						delete[] deviceName; deviceName = NULL;

						auto res = rprObjectDelete(temporaryContext);
						FASSERT(RPR_SUCCESS == res);
					}
				}
			}
		}

		gpuComputed = true;
	}
	numberCompatibleGPUs = gpuInfoArray.size();
	return (numberCompatibleGPUs > 0);
}

int ScopeManagerMax::getGpuUsedCount()
{
	int count = 0;
	for (int i = 0; i < gpuInfoArray.size(); i++)
	{
		if (gpuInfoArray[i].isUsed)
			count++;
	}
	return count;
}

int ScopeManagerMax::getUncertifiedGpuCount()
{
	int count = 0;
	for (int i = 0; i < gpuInfoArray.size(); i++)
	{
		if ( ! gpuInfoArray[i].isWhiteListed)
			count++;
	}
	return count;
}

const std::vector<std::string> &ScopeManagerMax::getGpuNamesThatHaveOldDriver()
{
	static bool mInitialized = false;
	static std::vector<std::string> mResult;

	if (!mInitialized)
	{
		mInitialized = true;

		HRESULT hr;

		hr = CoInitializeEx(0, COINIT_MULTITHREADED);
		if (FAILED(hr) && hr != RPC_E_CHANGED_MODE)
			return mResult;

		IWbemLocator *pLoc = NULL;
		hr = CoCreateInstance(CLSID_WbemLocator,
			0,
			CLSCTX_INPROC_SERVER,
			IID_IWbemLocator, (LPVOID *)&pLoc);

		if (FAILED(hr))
		{
			CoUninitialize();
			return mResult;
		}

		// Connect to the root\cimv2 namespace with
		// the current user and obtain pointer pSvc
		// to make IWbemServices calls.
		IWbemServices *pSvc = NULL;
		hr = pLoc->ConnectServer(_bstr_t(L"ROOT\\CIMV2"), // Object path of WMI namespace
			NULL,                    // User name. NULL = current user
			NULL,                    // User password. NULL = current
			0,                       // Locale. NULL indicates current
			NULL,                    // Security flags.
			0,                       // Authority (for example, Kerberos)
			0,                       // Context object 
			&pSvc);                    // pointer to IWbemServices proxy


		if (FAILED(hr))
		{
			pLoc->Release();
			CoUninitialize();
			return mResult;
		}

		IEnumWbemClassObject* pEnumerator = NULL;
		hr = pSvc->ExecQuery(bstr_t("WQL"),
			bstr_t("SELECT * FROM Win32_VideoController"),
			WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
			NULL,
			&pEnumerator);

		if (FAILED(hr))
		{
			pSvc->Release();
			pLoc->Release();
			CoUninitialize();
			return mResult;
		}

		// Get data from query
		IWbemClassObject *pclsObj = NULL;
		ULONG uReturn = 0;

		bool foundOldDriver = false;
		while (pEnumerator)
		{

			HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);

			if (uReturn == 0)
				break;

			VARIANT vtProp;

			// Get the value of the Name property
			hr = pclsObj->Get(L"Name", 0, &vtProp, 0, 0);
			std::wstring unicodeName = vtProp.bstrVal;
			std::string name = std::string(unicodeName.begin(), unicodeName.end());

			hr = pclsObj->Get(L"DriverVersion", 0, &vtProp, 0, 0);
			std::wstring unicodedriverVersion = vtProp.bstrVal;;
			std::string driverVersion = std::string(unicodedriverVersion.begin(), unicodedriverVersion.end());

			bool oldDriver = false;

			// AMD card
			if (name.find("AMD") != std::string::npos || name.find("Radeon") != std::string::npos)
			{
				std::vector<int> version;
				std::vector<int> oldVersion = { 15, 201, 2401 };

				std::string subVer;
				for (int i = 0; i < driverVersion.length(); i++)
				{
					if (driverVersion[i] != '.')
					{
						subVer += driverVersion[i];
					}

					if (driverVersion[i] == '.' || i == driverVersion.length() - 1)
					{
						version.push_back(std::stoi(subVer));
						subVer = "";
					}
				}

				if (version.size() != oldVersion.size())
				{
					oldDriver = false;
				}
				else
				{
					for (int i = 0; i < version.size(); ++i)
					{
						if (version[i] < oldVersion[i])
						{
							oldDriver = true;
							break;
						}
						else if (version[i] > oldVersion[i])
						{
							oldDriver = false;
							break;
						}
						else if (version[i] == oldVersion[i])
						{
							oldDriver = true;
						}
					}
				}
			}
			else // Nvidia or Intel card or etc
			{

			}

			VariantClear(&vtProp);

			pclsObj->Release();

			if (oldDriver)
			{
				mResult.push_back(name);
			}
		}

		pSvc->Release();
		pLoc->Release();
		pEnumerator->Release();
		CoUninitialize();
	}
	return mResult;
}

rpr_creation_flags ScopeManagerMax::getContextCreationFlags(bool useGpu, bool useCpu)
{
	if (!gpuComputed)
	{
		int numCompatibleGPUs = 0;
		hasGpuCompatibleWithFR(numCompatibleGPUs);
	}

	rpr_creation_flags flags = 0;

	if(useGpu)
	{
		for (int i = 0; i < gpuInfoArray.size(); i++)
		{
			if (!gpuInfoArray[i].isUsed)
				continue;

			flags |= gpuIdFlags[i];
		}
	}
	
	if (useCpu || !useGpu) {
		flags |= RPR_CREATION_FLAGS_ENABLE_CPU;
	}

	return flags;
}

bool ScopeManagerMax::CreateContext(rpr_creation_flags createFlags, rpr_context& result)
{
	debugPrint(subCacheFolder + " <- cache folder\n");

	rpr_int tahoePluginID = rprRegisterPlugin("Tahoe64.dll");
	assert(tahoePluginID != -1);

	rpr_int plugins[] = { tahoePluginID };
	size_t pluginCount = sizeof(plugins) / sizeof(plugins[0]);

	rpr_context context = nullptr;
	int res = rprCreateContext(RPR_API_VERSION, plugins, pluginCount, createFlags, NULL, subCacheFolder.c_str(), &context);
	switch (res) {
	case RPR_SUCCESS:
		result = context;
		return true;
	case RPR_ERROR_UNSUPPORTED:
		return false;
	}
	AssertImpl((std::wstring(L"rprCreateContext returned error:") + std::to_wstring(res)).c_str(), _T(__FILE__), __LINE__);
	return false;
}

FIRERENDER_NAMESPACE_END;
