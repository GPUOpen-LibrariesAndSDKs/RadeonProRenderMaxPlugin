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

FIRERENDER_NAMESPACE_BEGIN

#define SCOPEMANAGER_VERSION 1

struct GpuInfo
{
	std::string name;
	bool isUsed = false;
	bool isCompatible = false;
	bool isWhiteListed = false;
	rpr_creation_flags frFlags = 0;
};

typedef std::vector<GpuInfo> GpuInfoArray;

struct CpuInfo
{
	bool isUsed = false;
	bool isCpuThreadsNumOverriden = false;
	std::uint64_t numCpuThreads = 0;
};

typedef int ScopeID;

class ScopeManagerMax : public GUP
{
public:
	static ScopeManagerMax TheManager;
	static GpuInfoArray gpuInfoArray;
	static CpuInfo cpuInfo;

	std::string mCacheFolder;

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

	rpr_creation_flags getContextCreationFlags();
	bool hasGpuCompatibleWithFR(int& numberCompatibleGPUs);
	void ValidateParamBlock(IParamBlock2 *pblock);
	std::wstring GetRPRTraceDirectory(IParamBlock2 *pblock);

	int GetCompiledShadersCount();

	void LoadAttributeSettings();
	void SaveAttributeSettings();

private:
	void EnableRPRTrace(IParamBlock2 *pblock, bool enable);
	bool CreateContext(rpr_creation_flags createFlags, rpr_context& result);
	void SetupCacheFolder();

	std::map<ScopeID, frw::Scope> scopes;
	static ScopeID nextScopeId;
};

FIRERENDER_NAMESPACE_END
