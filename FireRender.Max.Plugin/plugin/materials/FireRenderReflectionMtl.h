#pragma once

#include "FireRenderMtlBase.h"

FIRERENDER_NAMESPACE_BEGIN;

enum FRReflectionMtl_TexmapId {
	FRReflectionMtl_TEXMAP_COLOR = 0,
	FRReflectionMtl_TEXMAP_NORMAL = 1
};

enum FRReflectionMtl_ParamID : ParamID {
	FRReflectionMtl_COLOR = 1000,
	FRReflectionMtl_COLOR_TEXMAP = 1002,
	FRReflectionMtl_NORMALMAP = 1003
};

class FireRenderReflectionMtlTraits
{
public:
	static const TCHAR* InternalName() { return _T("RPR Reflection Material"); }
	static Class_ID ClassId() { return FIRERENDER_REFLECTIONMTL_CID; }
};

class FireRenderReflectionMtl :
	public FireRenderMtl<FireRenderReflectionMtlTraits, FireRenderReflectionMtl>
{
public:
	Color GetDiffuse(int mtlNum, BOOL backFace) override;
	frw::Shader getShader(const TimeValue t, MaterialParser& mtlParser, INode* node);
};

FIRERENDER_NAMESPACE_END;
