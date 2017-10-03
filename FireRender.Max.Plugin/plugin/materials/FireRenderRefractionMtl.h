#pragma once

#include "FireRenderMtlBase.h"

FIRERENDER_NAMESPACE_BEGIN;

enum FRRefractionMtl_TexmapId {
	FRRefractionMtl_TEXMAP_COLOR = 0,
	FRRefractionMtl_TEXMAP_NORMAL = 1
};

enum FRRefractionMtl_ParamID : ParamID {
	FRRefractionMtl_COLOR = 1000,
	FRRefractionMtl_IOR = 1001,
	FRRefractionMtl_COLOR_TEXMAP = 1002,
	FRRefractionMtl_NORMALMAP = 1003
};

class FireRenderRefractionMtlTraits
{
public:
	static const TCHAR* InternalName() { return _T("RPR Refraction Material"); }
	static Class_ID ClassId() { return FIRERENDER_REFRACTIONMTL_CID; }
};

class FireRenderRefractionMtl :
	public FireRenderMtl<FireRenderRefractionMtlTraits, FireRenderRefractionMtl>
{
public:
	Color GetDiffuse(int mtlNum, BOOL backFace);
	float GetXParency(int mtlNum, BOOL backFace) override;
	frw::Shader getShader(const TimeValue t, MaterialParser& mtlParser, INode* node);
};

FIRERENDER_NAMESPACE_END;
