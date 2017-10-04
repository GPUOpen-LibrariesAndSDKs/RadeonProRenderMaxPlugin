/*********************************************************************************************************************************
 * Radeon ProRender for 3ds Max plugin
 * Copyright (c) 2017 AMD
 * All Rights Reserved 
 * 
 * DllMain and some basic functions loaded directly by 3ds Max plugin mechanism to initialize the plugin after the DLL is loaded
 *********************************************************************************************************************************/ 

#include "Common.h"
#include "ClassDescs.h"
#include "utils/Utils.h"
#include <iparamb2.h>
#include <direct.h>

#include "FireRenderDiffuseMtl.h"
#include "FireRenderBlendMtl.h"
#include "FireRenderAddMtl.h"
#include "FireRenderMicrofacetMtl.h"
#include "FireRenderReflectionMtl.h"
#include "FireRenderArithmMtl.h"
#include "FireRenderInputLUMtl.h"
#include "FireRenderBlendValueMtl.h"
#include "FireRenderRefractionMtl.h"
#include "FireRenderMFRefractionMtl.h"
#include "FireRenderTransparentMtl.h"
#include "FireRenderWardMtl.h"
#include "FireRenderEmissiveMtl.h"
#include "FireRenderFresnelMtl.h"
#include "FireRenderStandardMtl.h"
#include "FireRenderColorMtl.h"
#include "FireRenderAvgMtl.h"
#include "FireRenderOrenNayarMtl.h"
#include "FireRenderFresnelSchlickMtl.h"
#include "FireRenderDisplacementMtl.h"
#include "FireRenderNormalMtl.h"
#include "FireRenderDiffuseRefractionMtl.h"
#include "FireRenderMaterialMtl.h"
#include "FireRenderUberMtl.h"
#include "FireRenderUberMtlv2.h"
#include "FireRenderVolumeMtl.h"
#include "FireRenderPbrMtl.h"

#include "XMLMaterialExporter.h"
#include "FireRenderEnvironment.h"
#include "FireRenderAnalyticalSun.h"
#include "FireRenderPortalLight.h"
#include "FireRenderIESLight.h"

#include "BgManager.h"
#include "TmManager.h"
#include "CamManager.h"
#include "MPManager.h"
#include "ScopeManager.h"
#include "PRManager.h"

#ifdef FIREMAX_DEBUG
#pragma comment (lib, "ThirdParty/RadeonProRender SDK/Win/lib/RadeonProRender64.lib") // no 'D' suffix as no debug lib supplied
#pragma comment (lib, "ThirdParty/RadeonProRender SDK/Win/lib/RprLoadStore64.lib")
#pragma comment (lib, "ThirdParty/RadeonProRender SDK/Win/lib/RprSupport64.lib")
#else
#pragma comment (lib, "ThirdParty/RadeonProRender SDK/Win/lib/RadeonProRender64.lib") 
#pragma comment (lib, "ThirdParty/RadeonProRender SDK/Win/lib/RprLoadStore64.lib")
#pragma comment (lib, "ThirdParty/RadeonProRender SDK/Win/lib/RprSupport64.lib")
#endif

#pragma comment (lib, "ThirdParty/AxfPackage/ReleaseDll/AxfLib/AxFDecoding_r.lib")
#pragma comment (lib, "ThirdParty/AxfPackage/ReleaseDll/AxfLib/FreeImage.lib")
#pragma comment (lib, "ThirdParty/AxfPackage/ReleaseDll/AxfLib/libboost_filesystem-vc140-mt-1_62.lib")
#pragma comment (lib, "ThirdParty/AxfPackage/ReleaseDll/AxfLib/libboost_program_options-vc140-mt-1_62.lib")
#pragma comment (lib, "ThirdParty/AxfPackage/ReleaseDll/AxfLib/libboost_regex-vc140-mt-1_62.lib")
#pragma comment (lib, "ThirdParty/AxfPackage/ReleaseDll/AxfLib/libboost_system-vc140-mt-1_62.lib")

#if MAX_PRODUCT_YEAR_NUMBER == 2013
#   pragma comment (lib, "plugins/maxsdk 2013/x64/lib/maxutil.lib")
#   pragma comment (lib, "plugins/maxsdk 2013/x64/lib/core.lib")
#   pragma comment (lib, "plugins/maxsdk 2013/x64/lib/paramblk2.lib")
#   pragma comment (lib, "plugins/maxsdk 2013/x64/lib/bmm.lib")
#   pragma comment (lib, "plugins/maxsdk 2013/x64/lib/geom.lib")
#   pragma comment (lib, "plugins/maxsdk 2013/x64/lib/mesh.lib")
#   pragma comment (lib, "plugins/maxsdk 2013/x64/lib/poly.lib")
#   pragma comment (lib, "plugins/maxsdk 2013/x64/lib/gup.lib")
#elif MAX_PRODUCT_YEAR_NUMBER == 2014
#   pragma comment (lib, "plugins/maxsdk 2014/lib/x64/Release/maxutil.lib")
#   pragma comment (lib, "plugins/maxsdk 2014/lib/x64/Release/core.lib")
#   pragma comment (lib, "plugins/maxsdk 2014/lib/x64/Release/paramblk2.lib")
#   pragma comment (lib, "plugins/maxsdk 2014/lib/x64/Release/bmm.lib")
#   pragma comment (lib, "plugins/maxsdk 2014/lib/x64/Release/geom.lib")
#   pragma comment (lib, "plugins/maxsdk 2014/lib/x64/Release/mesh.lib")
#   pragma comment (lib, "plugins/maxsdk 2014/lib/x64/Release/poly.lib")
#   pragma comment (lib, "plugins/maxsdk 2014/lib/x64/Release/gfx.lib")
#   pragma comment (lib, "plugins/maxsdk 2014/lib/x64/Release/gup.lib")
#elif MAX_PRODUCT_YEAR_NUMBER == 2015
#   pragma comment (lib, "plugins/maxsdk 2015/lib/x64/Release/maxutil.lib")
#   pragma comment (lib, "plugins/maxsdk 2015/lib/x64/Release/core.lib")
#   pragma comment (lib, "plugins/maxsdk 2015/lib/x64/Release/paramblk2.lib")
#   pragma comment (lib, "plugins/maxsdk 2015/lib/x64/Release/bmm.lib")
#   pragma comment (lib, "plugins/maxsdk 2015/lib/x64/Release/geom.lib")
#   pragma comment (lib, "plugins/maxsdk 2015/lib/x64/Release/mesh.lib")
#   pragma comment (lib, "plugins/maxsdk 2015/lib/x64/Release/poly.lib")
#   pragma comment (lib, "plugins/maxsdk 2015/lib/x64/Release/gfx.lib")
#   pragma comment (lib, "plugins/maxsdk 2015/lib/x64/Release/gup.lib")
#   pragma comment (lib, "plugins/maxsdk 2015/lib/x64/Release/GraphicsDriver.lib")
#   pragma comment (lib, "plugins/maxsdk 2015/lib/x64/Release/DefaultRenderItems.lib")
#else
#   pragma comment (lib, "Release/maxutil.lib")
#   pragma comment (lib, "Release/core.lib")
#   pragma comment (lib, "Release/paramblk2.lib")
#   pragma comment (lib, "Release/bmm.lib")
#   pragma comment (lib, "Release/geom.lib")
#   pragma comment (lib, "Release/mesh.lib")
#   pragma comment (lib, "Release/poly.lib")
#   pragma comment (lib, "Release/gfx.lib")
#   pragma comment (lib, "Release/GraphicsDriver.lib")
#   pragma comment (lib, "Release/DefaultRenderItems.lib")
#   pragma comment (lib, "Release/maxscrpt.lib")
#   pragma comment (lib, "Release/gup.lib")
#endif


HINSTANCE FireRender::fireRenderHInstance;

// This is the required signature for some of the functions to be loaded by 3ds Max when the DLL is loaded
#define EXPORT_TO_MAX extern "C" __declspec(dllexport)

namespace {
	std::vector<ClassDesc*> gClassInstances;

};

BOOL WINAPI DllMain(HINSTANCE hinstDLL, ULONG fdwReason, LPVOID /*lpvReserved*/) {
    if (fdwReason == DLL_PROCESS_ATTACH) {
        // Save the HINSTANCE of this plugin
        FireRender::fireRenderHInstance = hinstDLL;
        DisableThreadLibraryCalls(hinstDLL);
    }
    return true;
}

/// Returns a description that is displayed in 3ds Max plugin manager
EXPORT_TO_MAX const TCHAR* LibDescription() {
    return _T("Radeon ProRender for 3ds Max(R)");
}


/// Tells 3ds Max how many plugins are implemented in this DLL. Determines the indices with which LibClassDesc is called later
EXPORT_TO_MAX int LibNumberClasses() {
	return gClassInstances.size();
}

/// Returns the class descriptors for all plugins implemented in this DLL
EXPORT_TO_MAX ClassDesc* LibClassDesc(int i) {
	if (i < gClassInstances.size())
		return gClassInstances[i];
        FASSERT(false);
        return NULL;
    }

/// Has to always return Get3DSMAXVersion()
EXPORT_TO_MAX ULONG LibVersion() {
    return Get3DSMAXVersion();
}

int GetAOVElementClassDescCount();
ClassDesc2& GetAOVElementClassDesc(int);

/// Called by 3ds Max immediately after 3ds Max loads the plugin. Any initialization should be done here (and not in DllMain)
EXPORT_TO_MAX int LibInitialize()
{
	// sanity check (AMDMAX-1029)
	wchar_t bufp[MAX_PATH + 1];
	int bytes = GetModuleFileName(NULL, bufp, MAX_PATH);
	std::wstring modulep = bufp;
	std::wstring::size_type pos = modulep.find_last_of('\\');

	if (pos != std::wstring::npos)
		modulep = modulep.substr(0, pos + 1);

	std::wstring checkp[4] =
	{
		{L"Tahoe64.dll" },
		{L"RprLoadStore64.dll" },
		{L"RadeonProRender64.dll" },
		{L"OpenImageIO_RPR.dll" },
	};

	for (int i = 0; i < 4; i++)
	{
		std::wstring toCheck = modulep + checkp[i];
		struct __stat64 buffer;
		if (_wstat64(toCheck.c_str(), &buffer) != 0)
		{
			std::wstring message = L"Required DLL ";
			message += checkp[i];
			message += L" is missing.\nCannot load Radeon ProRenderer.";
			MessageBox(0, message.c_str(), L"Radeon ProRender", MB_OK | MB_ICONEXCLAMATION);
			return false;
		}
	}
	
    // We create a dedicated folder for our logs/config in a location 3ds Max designates for us.
    const std::wstring folder = FireRender::GetDataStoreFolder();
    _wmkdir(folder.c_str());

	LoadLibrary(TEXT("Msftedit.dll"));

	// initialize class descriptors
	gClassInstances.clear();
	gClassInstances.push_back(&FireRender::fireRenderClassDesc);
	gClassInstances.push_back(&FireRender::autoTestingClassDesc);
	gClassInstances.push_back(&FireRender::FireRenderDiffuseMtl::GetClassDesc());
	gClassInstances.push_back(&FireRender::FireRenderBlendMtl::GetClassDesc());
	//gClassInstances.push_back(&FireRender::FireRenderAddMtl::GetClassDesc());
	gClassInstances.push_back(&FireRender::FireRenderMicrofacetMtl::GetClassDesc());
	gClassInstances.push_back(&FireRender::FireRenderReflectionMtl::GetClassDesc());
	gClassInstances.push_back(&FireRender::FireRenderArithmMtl::GetClassDesc());
	gClassInstances.push_back(&FireRender::FireRenderInputLUMtl::GetClassDesc());
	gClassInstances.push_back(&FireRender::FireRenderBlendValueMtl::GetClassDesc());
	gClassInstances.push_back(&FireRender::FireRenderRefractionMtl::GetClassDesc());
	gClassInstances.push_back(&FireRender::FireRenderMFRefractionMtl::GetClassDesc());
	gClassInstances.push_back(&FireRender::FireRenderTransparentMtl::GetClassDesc());
	gClassInstances.push_back(&FireRender::FireRenderWardMtl::GetClassDesc());
	gClassInstances.push_back(&FireRender::FireRenderEmissiveMtl::GetClassDesc());
	gClassInstances.push_back(&FireRender::FireRenderFresnelMtl::GetClassDesc());
	gClassInstances.push_back(&FireRender::fireRenderMaterialImporterClassDesc);
	gClassInstances.push_back(&FireRender::FireRenderColorMtl::GetClassDesc());
	gClassInstances.push_back(&FireRender::FireRenderAvgMtl::GetClassDesc());
	gClassInstances.push_back(&FireRender::FireRenderOrenNayarMtl::GetClassDesc());
	gClassInstances.push_back(&FireRender::FireRenderDiffuseRefractionMtl::GetClassDesc());
	
	if (RPR_API_VERSION > 0x010000094)
		gClassInstances.push_back(&FireRender::FireRenderFresnelSchlickMtl::GetClassDesc());
	
	gClassInstances.push_back(&FireRender::FireRenderDisplacementMtl::GetClassDesc());
	gClassInstances.push_back(&FireRender::FireRenderNormalMtl::GetClassDesc());
	gClassInstances.push_back(&FireRender::FireRenderMaterialMtl::GetClassDesc());
    //gClassInstances.push_back(&FireRender::FireRenderUberMtl::ClassDescInstance);
	gClassInstances.push_back(&FireRender::FireRenderUberMtlv2::GetClassDesc());
    gClassInstances.push_back(&FireRender::FireRenderVolumeMtl::GetClassDesc());
	gClassInstances.push_back(&FireRender::FireRenderPbrMtl::GetClassDesc());

	gClassInstances.push_back(FireRender::GetFireRenderEnvironmentDesc());

	gClassInstances.push_back(FireRender::GetFireRenderAnalyticalSunDesc());

	gClassInstances.push_back(FireRender::GetFireRenderPortalLightDesc());

	gClassInstances.push_back(FireRender::BgManagerMax::GetClassDesc());

	gClassInstances.push_back(FireRender::TmManagerMax::GetClassDesc());

	gClassInstances.push_back(FireRender::CamManagerMax::GetClassDesc());

	gClassInstances.push_back(FireRender::MPManagerMax::GetClassDesc());

	gClassInstances.push_back(FireRender::ScopeManagerMax::GetClassDesc());
	
	gClassInstances.push_back(FireRender::IFRMaterialExporter::GetClassDesc());

	gClassInstances.push_back(FireRender::PRManagerMax::GetClassDesc());

	gClassInstances.push_back(FireRender::FireRenderIESLight::GetClassDesc());

	for(int i=0;i<GetAOVElementClassDescCount();++i){
		gClassInstances.push_back(&GetAOVElementClassDesc(i));
	}

    return true;
}

/// Called by 3ds Max immediately before the plugin is unloaded (e.g. when closing the application)
EXPORT_TO_MAX int LibShutdown() {
    return true;
}
