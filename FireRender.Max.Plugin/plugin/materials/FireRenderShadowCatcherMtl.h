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

#include "FireRenderMtlBase.h"

FIRERENDER_NAMESPACE_BEGIN;

enum FRShadowCatcherMtl_TexmapId
{
	FRSHADOWCATCHERMTL_MAP_PRIMITIVE_COLOR = 0,
	FRSHADOWCATCHERMTL_MAP_SHADOW_COLOR = 1,
	FRSHADOWCATCHERMTL_MAP_NORMAL = 2,
	FRSHADOWCATCHERMTL_MAP_DISPLACEMENT = 3,
};

enum FRShadowCatcherMtl_ParamID : ParamID
{
	FRSHADOWCATCHER_NORMAL_MUL = 1000,
	FRSHADOWCATCHER_NORMAL = 1001,
	FRSHADOWCATCHER_NORMAL_MAP = 1002,
	FRSHADOWCATCHER_NORMAL_USEMAP = 1003,

	FRSHADOWCATCHER_DISPLACE_MUL = 1010,
	FRSHADOWCATCHER_DISPLACE = 1011,
	FRSHADOWCATCHER_DISPLACE_MAP = 1012,
	FRSHADOWCATCHER_DISPLACE_USEMAP = 1013,

	FRSHADOWCATCHER_SHADOW_COLOR_MUL = 1020,
	FRSHADOWCATCHER_SHADOW_COLOR = 1021,
	FRSHADOWCATCHER_SHADOW_COLOR_MAP = 1022,
	FRSHADOWCATCHER_SHADOW_COLOR_USEMAP = 1023,

	FRSHADOWCATCHER_SHADOW_WEIGHT_MUL = 1030,

	FRSHADOWCATCHER_SHADOW_ALPHA_MUL = 1040,

	FRSHADOWCATCHER_IS_BACKGROUND = 1051,

	FRSHADOWCATCHER_BACKGROUND_COLOR_MUL = 1060,
	FRSHADOWCATCHER_BACKGROUND_COLOR = 1061,
	FRSHADOWCATCHER_BACKGROUND_COLOR_MAP = 1062,
	FRSHADOWCATCHER_BACKGROUND_COLOR_USEMAP = 1063,

	FRSHADOWCATCHER_BACKGROUND_WEIGHT_MUL = 1070,

	FRSHADOWCATCHER_BACKGROUND_ALPHA_MUL = 1080,
};

BEGIN_DECLARE_FRMTLCLASSDESC(ShadowCatcherMtl, L"RPR Shadow Catcher Material", FIRERENDER_SCMTL_CID)
END_DECLARE_FRMTLCLASSDESC()

BEGIN_DECLARE_FRMTL(ShadowCatcherMtl)

public:
	frw::Shader getVolumeShader(const TimeValue t, MaterialParser& mtlParser, INode* node);

private:
	std::tuple<bool, Texmap*, Color, float> GetParameters(FRShadowCatcherMtl_ParamID useMapId,
		FRShadowCatcherMtl_ParamID mapId, FRShadowCatcherMtl_ParamID colorId, FRShadowCatcherMtl_ParamID mulId);
	frw::Value SetupShaderOrdinary(MaterialParser& mtlParser, std::tuple<bool, Texmap*, Color, float> parameters, int mapFlags);

END_DECLARE_FRMTL(ShadowCatcherMtl)

FIRERENDER_NAMESPACE_END;
