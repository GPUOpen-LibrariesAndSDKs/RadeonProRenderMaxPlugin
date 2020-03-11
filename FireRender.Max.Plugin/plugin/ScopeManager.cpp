/**********************************************************************
Copyright 2020 Advanced Micro Devices, Inc
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at
    http://www.apache.org/licenses/LICENSE-2.0
Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
********************************************************************/

#include <math.h>
#include <algorithm>
#include "resource.h"
#include "RprTools.h"
#include "plugin/ParamBlock.h"
#include "plugin/FRSettingsFileHandler.h"
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

FIRERENDER_NAMESPACE_BEGIN

ScopeManagerMax ScopeManagerMax::TheManager;

GpuInfoArray ScopeManagerMax::gpuInfoArray;
CpuInfo ScopeManagerMax::cpuInfo;

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
	static bool prefsLoaded = false;
};


ScopeManagerMax::ScopeManagerMax()
{
	gpuComputed = false;
	prefsLoaded = false;

	SetupCacheFolder();
}

void ScopeManagerMax::SetupCacheFolder()
{
	std::ostringstream oss;

#ifdef RPR_VERSION_MAJOR_MINOR_REVISION
	oss << std::hex << RPR_VERSION_MAJOR_MINOR_REVISION;
#else
	oss << std::hex << RPR_API_VERSION;
#endif

	std::string versionedCacheFolder = oss.str();
	std::string dataStoreFolder = ToAscii(GetDataStoreFolder());
	std::string cacheRootFolder = dataStoreFolder + "cache\\";
	mCacheFolder = cacheRootFolder + versionedCacheFolder + "\\";

	BOOL rc = CreateDirectoryA(dataStoreFolder.c_str(), NULL);
	DWORD lastError = GetLastError();

	if (rc || ERROR_ALREADY_EXISTS == lastError)
	{
		rc = CreateDirectoryA(cacheRootFolder.c_str(), NULL);
		lastError = GetLastError();

		if (rc || ERROR_ALREADY_EXISTS == lastError)
		{
			rc = CreateDirectoryA(mCacheFolder.c_str(), NULL);
			lastError = GetLastError();
		}
	}

	if (!rc && lastError != ERROR_ALREADY_EXISTS)
		MessageBox(GetCOREInterface()->GetMAXHWnd(), L"Failed to create cache folder", L"Radeon ProRender", MB_OK | MB_ICONERROR);
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
DWORD ScopeManagerMax::Start()
{
	int res = RegisterNotification(NotifyProc, this, NOTIFY_SYSTEM_STARTUP);

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

	rpr_creation_flags createFlags = getContextCreationFlags();

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
			ctx.SetParameter(RPR_CONTEXT_RADIANCE_CLAMP, irradianceClamp);
		}
		else
			ctx.SetParameter(RPR_CONTEXT_RADIANCE_CLAMP, FLT_MAX);
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
			DWORD error = GetLastError();

			if (ERROR_ALREADY_EXISTS != error)
			{
				MessageBox(GetCOREInterface()->GetMAXHWnd(), L"Failed to create trace folder", L"Radeon ProRender", MB_OK | MB_ICONERROR);
			}
		}

		rpr_int res = rprContextSetParameterByKeyString(nullptr, RPR_CONTEXT_TRACING_PATH, ToAscii(traceFolder).c_str());
		FCHECK(res); // Able to call FCHECK_CONTEXT() will nullptr for context?
	}

	auto res = rprContextSetParameterByKey1u(nullptr, RPR_CONTEXT_TRACING_ENABLED, enable ? 1 : 0);
	FCHECK(res); // Able to call FCHECK_CONTEXT() will nullptr for context?
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

// Load preferences from attribute settings file, usually done once at startup
void ScopeManagerMax::LoadAttributeSettings()
{
	std::string gpuInfoNames = FRSettingsFileHandler::getAttributeSettingsFor(FRSettingsFileHandler::DevicesName);
	std::string gpuInfoUseds = FRSettingsFileHandler::getAttributeSettingsFor(FRSettingsFileHandler::DevicesSelected);
	std::string overrideCPUThreadCount = FRSettingsFileHandler::getAttributeSettingsFor(FRSettingsFileHandler::OverrideCPUThreadCount);
	std::string cpuThreadCount = FRSettingsFileHandler::getAttributeSettingsFor(FRSettingsFileHandler::CPUThreadCount);
	// Special case, if the prefs aren't created yet, don't overwrite current settings
	if( (gpuInfoNames.length()==0) || (gpuInfoUseds.length()==0) )
		return;

	// Initially set all gpus unused, and cpu used
	for( int i=0; i<gpuInfoArray.size(); i++ )
	{
		gpuInfoArray[i].isUsed = false;
	}
	cpuInfo.isUsed = true;

	// GPU selection is stored as two lists, a comma-separated list of GPU names,
	// plus comma-separated list of booleans (0 or 1) values,
	// with one extra boolean at the end for the CPU selection

	// Parse the input and toggle gpus on as required
	std::istringstream gpuInfoNamesStream(gpuInfoNames);
	std::istringstream gpuInfoUsedsStream(gpuInfoUseds);
	std::string gpuInfoName;
	std::string gpuInfoUsed;
	bool anyGPU = false;
	while( std::getline( gpuInfoNamesStream, gpuInfoName, ',' ) &&
		   std::getline( gpuInfoUsedsStream, gpuInfoUsed, ',' ) )
	{
		if( strtol(gpuInfoUsed.data(),nullptr,10)!=0 )
		{
			for( int i=0; i<gpuInfoArray.size(); i++ )
			{
				std::string item = gpuInfoArray[i].name;
				std::replace( item.begin(), item.end(), ',', '_'); // replace all ',' with '_'
				// If multiple GPUs of the same name are toggled within the preferences list,
				// then toggle an equal number of items in the gpuInfoArray list
				if( (item.compare(gpuInfoName)==0) && (gpuInfoArray[i].isUsed==false) )
				{
					gpuInfoArray[i].isUsed = true;
					anyGPU = true;
					continue;
				}
			}
		}
	}
	// Final list entry for the cpu
	if( std::getline( gpuInfoUsedsStream, gpuInfoUsed, ',' ) )
	{
		cpuInfo.isUsed = (strtol(gpuInfoUsed.data(),nullptr,10) != 0);
	}
	if( !anyGPU ) cpuInfo.isUsed = true;
	// CPU thread settings
	cpuInfo.isCpuThreadsNumOverriden = (strtol(overrideCPUThreadCount.data(),nullptr,10) != 0);
	cpuInfo.numCpuThreads = (strtol(cpuThreadCount.data(),nullptr,10));
}

// Save preferences to attribute settings file, done after relevant UI changes
void ScopeManagerMax::SaveAttributeSettings()
{
	// Special case, if the prefs have been loaded from disk yet, don't overwrite them
	// Can happen early after UI creation before context is ready
	if( !prefsLoaded )
		return;

	// GPU selection is stored as two lists, a comma-separated list of GPU names,
	// plus comma-separated list of booleans (0 or 1) values,
	// with one extra boolean at the end for the CPU selection

	std::string gpuInfoNames;
	std::string gpuInfoUseds;
	std::string overrideCPUThreadCount(cpuInfo.isCpuThreadsNumOverriden? "1":"0");
	std::string cpuThreadCount( std::to_string(cpuInfo.numCpuThreads) );
	int i=0;
	for( i=0; i<gpuInfoArray.size(); i++ )
	{
		if( i>0 )
		{
			gpuInfoNames.append(",");
			gpuInfoUseds.append(",");
		}
		std::string item = gpuInfoArray[i].name;
		std::replace( item.begin(), item.end(), ',', '_'); // replace all ',' with '_'
		gpuInfoNames.append(item);
		gpuInfoUseds.append(gpuInfoArray[i].isUsed? "1":"0" );
	}
	if( i>0 ) gpuInfoUseds.append(",");
	gpuInfoUseds.append(cpuInfo.isUsed? "1":"0" );
	FRSettingsFileHandler::setAttributeSettingsFor(FRSettingsFileHandler::DevicesName, gpuInfoNames);
	FRSettingsFileHandler::setAttributeSettingsFor(FRSettingsFileHandler::DevicesSelected, gpuInfoUseds);
	FRSettingsFileHandler::setAttributeSettingsFor(FRSettingsFileHandler::OverrideCPUThreadCount, overrideCPUThreadCount );
	FRSettingsFileHandler::setAttributeSettingsFor(FRSettingsFileHandler::CPUThreadCount, cpuThreadCount);
}


int ScopeManagerMax::GetCompiledShadersCount()
{
	std::string searchMask = mCacheFolder + "*.bin";

	WIN32_FIND_DATAA FindFileData{ 0 };
	HANDLE hFind = FindFirstFileA(searchMask.c_str(), &FindFileData);

	int count = 0;

	if (hFind != INVALID_HANDLE_VALUE)
	{
		count++;

		while (FindNextFileA(hFind, &FindFileData) != 0)
			count++;

		FindClose(hFind);
	}

	return count;
}

void ScopeManagerMax::ValidateParamBlock(IParamBlock2* pblock)
{
	// set CPU mode if GPU is not compatible even before opening settings dialog
	int numCompatibleGPUs = 0;

	if ( !hasGpuCompatibleWithFR(numCompatibleGPUs) )
	{
		cpuInfo.isUsed = true;
	}

	// Load gpu preferences; once at startup, but only after gpu initial settings
	// are applied via ValidateParamBlock() and hasGpuCompatibleWithFR()
	if( gpuComputed && !prefsLoaded )
	{	
		LoadAttributeSettings();
		prefsLoaded = true;
	}
}

bool ScopeManagerMax::hasGpuCompatibleWithFR(int& numberCompatibleGPUs)
{
	try
	{
		if (!gpuComputed)
		{
			rpr_creation_flags devicesUsed = RPR_CREATION_FLAGS_ENABLE_GPU0 | RPR_CREATION_FLAGS_ENABLE_GPU1 | RPR_CREATION_FLAGS_ENABLE_GPU2 | RPR_CREATION_FLAGS_ENABLE_GPU3 |
				RPR_CREATION_FLAGS_ENABLE_GPU4 | RPR_CREATION_FLAGS_ENABLE_GPU5 | RPR_CREATION_FLAGS_ENABLE_GPU6 | RPR_CREATION_FLAGS_ENABLE_GPU7;
			rpr_creation_flags devicesCompatibleOut = 0;
			rprAreDevicesCompatible(
				"Tahoe64.dll",
				NULL,
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
							size_t deviceNameSize = 0;
							int res = rprContextGetInfo(temporaryContext, gpuIdNames[i], 0, 0, &deviceNameSize);
							FCHECK_CONTEXT(res, temporaryContext, "rprContextGetInfo");
							if (res != RPR_SUCCESS)
							{
								throw  RPR_ERROR_INVALID_PARAMETER;
							}

							std::unique_ptr<char[]> dataBuffer(new char[deviceNameSize]);
							res = rprContextGetInfo(temporaryContext, gpuIdNames[i], deviceNameSize, dataBuffer.get(), 0);
							FCHECK_CONTEXT(res, temporaryContext, "rprContextGetInfo");
							if (res != RPR_SUCCESS)
							{
								throw  RPR_ERROR_INVALID_PARAMETER;
							}

							gpuInfoArray.emplace_back();
							GpuInfo& info = gpuInfoArray.back();
							info.name.clear();
							info.name.append(dataBuffer.get());
							info.isUsed = true;
							info.isCompatible = true;
							info.isWhiteListed = IsDeviceNameWhitelisted(
								info.name.c_str(),
#ifdef OSMac_
								RPRTOS_MACOS
#elif __linux__
								RPRTOS_LINUX
#else
								RPRTOS_WINDOWS
#endif
							);
							info.frFlags = gpuIdFlags[i];

							res = rprObjectDelete(temporaryContext);
							FCHECK(res);
						}
					}
				}
			}

			gpuComputed = true;
		}

		numberCompatibleGPUs = int_cast(gpuInfoArray.size());
	}
	catch (int res)
	{
		FCHECK(res);
		return 0;
	}
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
		if (!gpuInfoArray[i].isWhiteListed)
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

rpr_creation_flags ScopeManagerMax::getContextCreationFlags()
{
	if (!gpuComputed)
	{
		int numCompatibleGPUs = 0;
		hasGpuCompatibleWithFR(numCompatibleGPUs);
	}

	rpr_creation_flags flags = 0;

	for (int i = 0; i < gpuInfoArray.size(); i++)
	{
		if (!gpuInfoArray[i].isUsed)
			continue;

		flags |= gpuIdFlags[i];
	}

	if (0 == flags)
	{
		ScopeManagerMax::TheManager.cpuInfo.isUsed = true;
	}
	
	if (ScopeManagerMax::TheManager.cpuInfo.isUsed)
	{
		flags |= RPR_CREATION_FLAGS_ENABLE_CPU;
	}

	return flags;
}

bool ScopeManagerMax::CreateContext(rpr_creation_flags createFlags, rpr_context& result)
{
	debugPrint(mCacheFolder + " <- cache folder\n");

	SetupCacheFolder();

	rpr_int tahoePluginID = rprRegisterPlugin("Tahoe64.dll");
	assert(tahoePluginID != -1);

	rpr_int plugins[] = { tahoePluginID };
	size_t pluginCount = sizeof(plugins) / sizeof(plugins[0]);

	std::vector<rpr_context_properties> ctxProperties;

	ctxProperties.push_back((rpr_context_properties) RPR_CONTEXT_SAMPLER_TYPE);
	ctxProperties.push_back((rpr_context_properties) RPR_CONTEXT_SAMPLER_TYPE_CMJ);

	if ( (createFlags & RPR_CREATION_FLAGS_ENABLE_CPU) && 
		cpuInfo.isCpuThreadsNumOverriden && cpuInfo.numCpuThreads > 0)
	{
		ctxProperties.push_back((rpr_context_properties) RPR_CONTEXT_CPU_THREAD_LIMIT);
		ctxProperties.push_back((rpr_context_properties) cpuInfo.numCpuThreads);
		debugPrint( "RPR_CONTEXT_CREATEPROP_CPU_THREAD_LIMIT : " + std::to_string(cpuInfo.numCpuThreads) );
	}

	ctxProperties.push_back((rpr_context_properties) 0);

	rpr_context context = nullptr;

	rpr_int res = rprCreateContext(RPR_VERSION_MAJOR_MINOR_REVISION, plugins, pluginCount, createFlags,
		ctxProperties.data(), mCacheFolder.c_str(), &result);
	
	if (res != RPR_SUCCESS)
	{
		AssertImpl((std::wstring(L"rprCreateContext returned error:") + std::to_wstring(res)).c_str(), _T(__FILE__), __LINE__);
	}

	return RPR_SUCCESS == res;
}

FIRERENDER_NAMESPACE_END
