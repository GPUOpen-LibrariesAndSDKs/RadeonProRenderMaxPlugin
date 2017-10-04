#pragma once

#include "FireRenderMtlBase.h"

FIRERENDER_NAMESPACE_BEGIN;

enum FRWardMtl_TexmapId {
	FRWardMtl_TEXMAP_COLOR = 0,
	FRWardMtl_TEXMAP_ROUGHNESSX = 1,
	FRWardMtl_TEXMAP_ROUGHNESSY = 2,
	FRWardMtl_TEXMAP_NORMAL = 3,
	FRWardMtl_TEXMAP_ROTATION = 4,
};

enum FRWardMtl_ParamID : ParamID {
	FRWardMtl_COLOR = 1000,
	FRWardMtl_ROTATION = 1001,
	FRWardMtl_ROUGHNESSX = 1002,
	FRWardMtl_ROUGHNESSY = 1003,
	FRWardMtl_COLOR_TEXMAP = 1004,
	FRWardMtl_ROUGHNESSX_TEXMAP = 1005,
	FRWardMtl_ROUGHNESSY_TEXMAP = 1006,
	FRWardMtl_NORMALMAP = 1007,
	FRWardMtl_ROTATION_TEXMAP = 1008,
};

class FireRenderWardMtlTraits
{
public:
	static const TCHAR* InternalName() { return _T("RPR Ward Material"); }
	static Class_ID ClassId() { return FIRERENDER_WARDMTL_CID; }
};

class FireRenderWardMtl :
	public FireRenderMtl<FireRenderWardMtlTraits, FireRenderWardMtl>
{
public:
	Color GetDiffuse(int mtlNum, BOOL backFace) override;
	frw::Shader GetShader(const TimeValue t, MaterialParser& mtlParser, INode* node) override;
};

FIRERENDER_NAMESPACE_END;
