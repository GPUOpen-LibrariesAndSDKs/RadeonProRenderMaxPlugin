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

enum FRPbrMtl_TexmapId
{
	FRPBRMTL_MAP_BASE_COLOR,
	FRPBRMTL_MAP_ROUGHNESS,
	FRPBRMTL_MAP_METALNESS,
	FRPBRMTL_MAP_NORMAL,
	FRPBRMTL_MAP_EMISSIVE_COLOR,
	FRPBRMTL_MAP_EMISSIVE_WEIGHT,
	FRPBRMTL_MAP_GLASS,
	FRPBRMTL_MAP_GLASS_IOR,
	FRPBRMTL_MAP_SPECULAR,
	FRPBRMTL_MAP_SSS_WEIGHT,
	FRPBRMTL_MAP_SSS_COLOUR,
	// obsolete
	FRPBRMTL_MAP_CAVITY,
	FRPBRMTL_MAP_OPACITY,
};

enum FRPbrMtl_ParamID : ParamID
{
	FRPBRMTL_BASE_COLOR_MUL         = 1000,
	FRPBRMTL_BASE_COLOR             = 1001,
	FRPBRMTL_BASE_COLOR_MAP         = 1002,
	FRPBRMTL_BASE_COLOR_USEMAP      = 1003,

	FRPBRMTL_ROUGHNESS_MUL          = 1010,
	FRPBRMTL_ROUGHNESS              = 1011,
	FRPBRMTL_ROUGHNESS_MAP          = 1012,
	FRPBRMTL_ROUGHNESS_USEMAP       = 1013,
	FRPBRMTL_ROUGHNESS_INVERT       = 1014,

	FRPBRMTL_METALNESS_MUL          = 1020,
	FRPBRMTL_METALNESS              = 1021,
	FRPBRMTL_METALNESS_MAP          = 1022,
	FRPBRMTL_METALNESS_USEMAP       = 1023,

	FRPBRMTL_CAVITY_MUL             = 1030,
	FRPBRMTL_CAVITY                 = 1031,
	FRPBRMTL_CAVITY_MAP             = 1032,
	FRPBRMTL_CAVITY_USEMAP          = 1033,
	FRPBRMTL_CAVITY_INVERT          = 1034,

	FRPBRMTL_NORMAL_MUL             = 1040,
	FRPBRMTL_NORMAL                 = 1041,
	FRPBRMTL_NORMAL_MAP             = 1042,
	FRPBRMTL_NORMAL_USEMAP          = 1043,

	FRPBRMTL_OPACITY_MUL            = 1050,
	FRPBRMTL_OPACITY                = 1051,
	FRPBRMTL_OPACITY_MAP            = 1052,
	FRPBRMTL_OPACITY_USEMAP         = 1053,
	FRPBRMTL_OPACITY_INVERT         = 1054,

	FRPBRMTL_EMISSIVE_COLOR_MUL     = 1060,
	FRPBRMTL_EMISSIVE_COLOR         = 1061,
	FRPBRMTL_EMISSIVE_COLOR_MAP     = 1062,
	FRPBRMTL_EMISSIVE_COLOR_USEMAP  = 1063,

	FRPBRMTL_EMISSIVE_WEIGHT_MUL    = 1070,
	FRPBRMTL_EMISSIVE_WEIGHT        = 1071,
	FRPBRMTL_EMISSIVE_WEIGHT_MAP    = 1072,
	FRPBRMTL_EMISSIVE_WEIGHT_USEMAP = 1073,

	FRPBRMTL_GLASS_MUL				= 1080,
	FRPBRMTL_GLASS					,
	FRPBRMTL_GLASS_MAP				,
	FRPBRMTL_GLASS_USEMAP			,

	FRPBRMTL_GLASS_IOR_MUL			= 1090,
	FRPBRMTL_GLASS_IOR				,
	FRPBRMTL_GLASS_IOR_MAP			,
	FRPBRMTL_GLASS_IOR_USEMAP		,

	FRPBRMTL_SPECULAR_MUL			= 1100,
	FRPBRMTL_SPECULAR				,
	FRPBRMTL_SPECULAR_MAP			,
	FRPBRMTL_SPECULAR_USEMAP		,

	FRPBRMTL_SSS_WEIGHT_MUL			= 1200,
	FRPBRMTL_SSS_WEIGHT				,
	FRPBRMTL_SSS_WEIGHT_MAP			,
	FRPBRMTL_SSS_WEIGHT_USEMAP		,
	FRPBRMTL_SSS_COLOR_MUL			,
	FRPBRMTL_SSS_COLOR				,
	FRPBRMTL_SSS_COLOR_MAP			,
	FRPBRMTL_SSS_COLOR_USEMAP		,
	FRPBRMTL_SSS_SCATTER_DIST		,
};

BEGIN_DECLARE_FRMTLCLASSDESC(PbrMtl, L"RPR PBR Material", FIRERENDER_PBRMTL_CID)
END_DECLARE_FRMTLCLASSDESC()

BEGIN_DECLARE_FRMTL(PbrMtl)

public:
	virtual Color GetDiffuse(int mtlNum, BOOL backFace) override;

	frw::Shader getVolumeShader(const TimeValue t, MaterialParser& mtlParser, INode* node);

private:

	std::tuple<Texmap*, Color> GetParameters(FRPbrMtl_ParamID mapId,
		FRPbrMtl_ParamID colorId);

	std::tuple<Texmap*, float> GetParametersNoColor(FRPbrMtl_ParamID mapId,
		FRPbrMtl_ParamID mulId);

END_DECLARE_FRMTL(PbrMtl)

FIRERENDER_NAMESPACE_END;
