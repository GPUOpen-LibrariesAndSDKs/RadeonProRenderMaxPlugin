#pragma once

#include "FireRenderMtlBase.h"

FIRERENDER_NAMESPACE_BEGIN;

enum FRPbrMtl_TexmapId
{
	FRPBRMTL_MAP_DIFFUSE_COLOR    = 0,
	FRPBRMTL_MAP_ROUGHNESS        = 1,
	FRPBRMTL_MAP_METALNESS        = 2,
	FRPBRMTL_MAP_CAVITY           = 3,
	FRPBRMTL_MAP_NORMAL           = 4,
	FRPBRMTL_MAP_OPACITY          = 5,
	FRPBRMTL_MAP_EMISSIVE         = 6,
};

enum FRPbrMtl_ParamID : ParamID
{
	FRPBRMTL_DIFFUSE_COLOR_MUL     = 1000,
	FRPBRMTL_DIFFUSE_COLOR         = 1001,
	FRPBRMTL_DIFFUSE_COLOR_MAP     = 1002,
	FRPBRMTL_DIFFUSE_COLOR_USEMAP  = 1003,

	FRPBRMTL_ROUGHNESS_MUL         = 1010,
	FRPBRMTL_ROUGHNESS             = 1011,
	FRPBRMTL_ROUGHNESS_MAP         = 1012,
	FRPBRMTL_ROUGHNESS_USEMAP      = 1013,
	FRPBRMTL_ROUGHNESS_INVERT      = 1014,

	FRPBRMTL_METALNESS_MUL         = 1020,
	FRPBRMTL_METALNESS             = 1021,
	FRPBRMTL_METALNESS_MAP         = 1022,
	FRPBRMTL_METALNESS_USEMAP      = 1023,

	FRPBRMTL_CAVITY_MUL            = 1030,
	FRPBRMTL_CAVITY                = 1031,
	FRPBRMTL_CAVITY_MAP            = 1032,
	FRPBRMTL_CAVITY_USEMAP         = 1033,
	FRPBRMTL_CAVITY_INVERT         = 1034,

	FRPBRMTL_NORMAL_MUL            = 1040,
	FRPBRMTL_NORMAL                = 1041,
	FRPBRMTL_NORMAL_MAP            = 1042,
	FRPBRMTL_NORMAL_USEMAP         = 1043,

	FRPBRMTL_OPACITY_MUL           = 1050,
	FRPBRMTL_OPACITY               = 1051,
	FRPBRMTL_OPACITY_MAP           = 1052,
	FRPBRMTL_OPACITY_USEMAP        = 1053,
	FRPBRMTL_OPACITY_INVERT        = 1054,

	FRPBRMTL_EMISSIVE_MUL          = 1060,
	FRPBRMTL_EMISSIVE              = 1061,
	FRPBRMTL_EMISSIVE_MAP          = 1062,
	FRPBRMTL_EMISSIVE_USEMAP       = 1063,
};

class FireRenderPbrMtlTraits
{
public:
	static const TCHAR* InternalName() { return _T("RPR PBR Material"); }
	static Class_ID ClassId() { return FIRERENDER_PBRMTL_CID; }
};

class FireRenderPbrMtl :
	public FireRenderMtl<FireRenderPbrMtlTraits, FireRenderPbrMtl>
{
public:
	Color GetDiffuse(int mtlNum, BOOL backFace) override;
	frw::Shader GetVolumeShader(const TimeValue t, MaterialParser& mtlParser, INode* node);
	frw::Shader GetShader(const TimeValue t, MaterialParser& mtlParser, INode* node) override;

private:
	std::tuple<bool, Texmap*, Color, float> GetParameters(FRPbrMtl_ParamID useMapId,
		FRPbrMtl_ParamID mapId, FRPbrMtl_ParamID colorId, FRPbrMtl_ParamID mulId);
};

FIRERENDER_NAMESPACE_END;
