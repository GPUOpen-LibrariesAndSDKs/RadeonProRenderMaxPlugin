#pragma once

#include "FireRenderMtlBase.h"

FIRERENDER_NAMESPACE_BEGIN;

enum FRMicrofacetMtl_TexmapId {
	FRMicrofacetMtl_TEXMAP_COLOR = 0,
	FRMicrofacetMtl_TEXMAP_ROUGHNESS = 1,
	FRMicrofacetMtl_TEXMAP_NORMAL = 2
};

enum FRMicrofacetMtl_ParamID : ParamID {
	FRMicrofacetMtl_COLOR = 1000,
	FRMicrofacetMtl_ROUGHNESS = 1001,
	FRMicrofacetMtl_IOR = 1002,
	FRMicrofacetMtl_COLOR_TEXMAP = 1003,
	FRMicrofacetMtl_ROUGHNESS_TEXMAP = 1004,
	FRMicrofacetMtl_NORMALMAP = 1005
};

class FireRenderMicrofacetMtlTraits
{
public:
	static const TCHAR* InternalName() { return _T("RPR Microfacet Material"); }
	static Class_ID ClassId() { return FIRERENDER_MICROFACETMTL_CID; }
};

class FireRenderMicrofacetMtl :
	public FireRenderMtl<FireRenderMicrofacetMtlTraits, FireRenderMicrofacetMtl>
{
public:
	Color GetDiffuse(int mtlNum, BOOL backFace) override;
	frw::Shader GetShader(const TimeValue t, class MaterialParser& mtlParser, INode* node) override;
};

FIRERENDER_NAMESPACE_END;
