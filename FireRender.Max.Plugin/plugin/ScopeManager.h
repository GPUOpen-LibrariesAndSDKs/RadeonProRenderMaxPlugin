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
