#pragma once

#include "FireRenderMtlBase.h"

FIRERENDER_NAMESPACE_BEGIN;

enum FRDiffuseRefractionMtl_TexmapId {
	FRDiffuseRefractionMtl_TEXMAP_DIFFUSE = 0,
	FRDiffuseRefractionMtl_TEXMAP_ROUGHNESS = 1,
	FRDiffuseRefractionMtl_TEXMAP_NORMAL = 2
};

enum FRDiffuseRefractionMtl_ParamID : ParamID {
	FRDiffuseRefractionMtl_COLOR = 1000,
	FRDiffuseRefractionMtl_ROUGHNESS = 1001,
	FRDiffuseRefractionMtl_COLOR_TEXMAP = 1002,
	FRDiffuseRefractionMtl_ROUGHNESS_TEXMAP = 1003,
	FRDiffuseRefractionMtl_NORMALMAP = 1004
};

class FireRenderDiffuseRefractionMtlTraits
{
public:
	static const TCHAR* InternalName() { return _T("RPR Diffuse Refraction Material"); }
	static Class_ID ClassId() { return FIRERENDER_DIFFUSEREFRACTIONMTL_CID; }
};

class FireRenderDiffuseRefractionMtl :
	public FireRenderMtl<FireRenderDiffuseRefractionMtlTraits, FireRenderDiffuseRefractionMtl>
{
public:
	frw::Shader GetShader(const TimeValue t, class MaterialParser& mtlParser, INode* node) override;
	Color GetDiffuse(int mtlNum, BOOL backFace) override;
};

FIRERENDER_NAMESPACE_END;
