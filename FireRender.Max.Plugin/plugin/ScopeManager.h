/*********************************************************************************************************************************
* Radeon ProRender for 3ds Max plugin
* Copyright (c) 2017 AMD
* All Rights Reserved
*
* Manages Scopes. Scopes include a Radeon ProRender context and material system, a collection of instantiated scene objects,
* a material translator instance. Scopes also deal with Radeon ProRender context initialization.
*********************************************************************************************************************************/

#pragma once

#include <max.h>
#include <guplib.h>
#include <notify.h>
#include <iparamb2.h>
#include "Common.h"
#include "frWrap.h"
#include "frScope.h"
#include <string>
#include <map>
#include <set>
#include <algorithm>

FIRERENDER_NAMESPACE_BEGIN;

#define SCOPEMANAGER_VERSION 1

typedef struct sGpuInfo
{
	std::string name;
	bool		isUsed;
	bool		isCompatible;
	bool		isWhiteListed;
	rpr_creation_flags frFlags;

	sGpuInfo() : isUsed(false), isCompatible(false), isWhiteListed(false), frFlags(0)
	{
	}

} GpuInfo;

typedef std::vector<GpuInfo> GpuInfoArray;

typedef int ScopeID;

class ScopeManagerMax : public GUP
{
public:
	static ScopeManagerMax TheManager;
	static GpuInfoArray gpuInfoArray;

	std::string rootCacheFolder;
	std::string subCacheFolder;

	static bool CoronaOK;

public:
	ScopeManagerMax();
	~ScopeManagerMax() {}

	// GUP Methods
	DWORD Start() override;
	void Stop() override;

	void DeleteThis() override;

	static ClassDesc* GetClassDesc();

	ScopeID CreateScope(IParamBlock2 *pblock);
	ScopeID CreateLocalScope(ScopeID parent);
	ScopeID CreateChildScope(ScopeID parent);
	void DestroyScope(ScopeID id);

	inline frw::Scope GetScope(ScopeID id)
	{
		auto it = scopes.find(id);
		if (it != scopes.end())
			return it->second;

		return frw::Scope();
	}
	
	static int getGpuUsedCount();
	static int getUncertifiedGpuCount();
	static const std::vector<std::string> &getGpuNamesThatHaveOldDriver();

	rpr_creation_flags getContextCreationFlags(bool useGpu, bool useCpu);
	bool hasGpuCompatibleWithFR(int& numberCompatibleGPUs);
	void ValidateParamBlock(IParamBlock2 *pblock);
	std::wstring GetRPRTraceDirectory(IParamBlock2 *pblock);

	int GetCompiledShadersCount();

private:
	void EnableRPRTrace(IParamBlock2 *pblock, bool enable);
	bool CreateContext(rpr_creation_flags createFlags, rpr_context& result);

	std::map<ScopeID, frw::Scope> scopes;
	static ScopeID nextScopeId;
};

FIRERENDER_NAMESPACE_END;
