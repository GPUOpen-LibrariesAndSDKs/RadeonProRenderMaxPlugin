#pragma once

#include "FireRenderMtlBase.h"

FIRERENDER_NAMESPACE_BEGIN;

enum FRStandardMtl_TexmapId {
	FRStandardMtl_TEXMAP_DIFFUSE_COLOR = 0,
	FRStandardMtl_TEXMAP_DIFFUSE_ROUGHNESS = 1,
	FRStandardMtl_TEXMAP_DIFFUSE_WEIGHT = 2,
	FRStandardMtl_TEXMAP_DIFFUSE_NORMAL = 3,
	FRStandardMtl_TEXMAP_REFLECT_COLOR = 4,
	FRStandardMtl_TEXMAP_REFLECT_ROUGHNESS = 5,
	FRStandardMtl_TEXMAP_REFLECT_WEIGHT = 6,
	FRStandardMtl_TEXMAP_REFLECT_NORMAL = 7,
	FRStandardMtl_TEXMAP_REFRACT_COLOR = 8,
	FRStandardMtl_TEXMAP_REFRACT_WEIGHT = 9,
	FRStandardMtl_TEXMAP_REFRACT_NORMAL = 10,
	FRStandardMtl_TEXMAP_EMISSION_WEIGHT = 11
};

enum FRStandardMtl_ParamID : ParamID {
	FRStandardMtl_DIFFUSE_COLOR = 1000,
	FRStandardMtl_DIFFUSE_ROUGHNESS,
	FRStandardMtl_DIFFUSE_COLOR_TEXMAP,
	FRStandardMtl_DIFFUSE_ROUGHNESS_TEXMAP,
	FRStandardMtl_DIFFUSE_WEIGHT,
	FRStandardMtl_DIFFUSE_WEIGHT_TEXMAP,
	FRStandardMtl_DIFFUSE_NORMALMAP,

	FRStandardMtl_REFLECT_COLOR,
	FRStandardMtl_REFLECT_IOR,
	FRStandardMtl_REFLECT_ROUGHNESS,
	FRStandardMtl_REFLECT_COLOR_TEXMAP,
	FRStandardMtl_REFLECT_ROUGHNESS_TEXMAP,
	FRStandardMtl_REFLECT_WEIGHT,
	FRStandardMtl_REFLECT_WEIGHT_TEXMAP,
	FRStandardMtl_REFLECT_NORMALMAP,

	FRStandardMtl_REFRACT_COLOR,
	FRStandardMtl_REFRACT_IOR,
	FRStandardMtl_REFRACT_COLOR_TEXMAP,
	FRStandardMtl_REFRACT_WEIGHT,
	FRStandardMtl_REFRACT_WEIGHT_TEXMAP,
	FRStandardMtl_REFRACT_NORMALMAP,

	FRStandardMtl_EMISSION_COLOR,
	FRStandardMtl_EMISSION_INTENSITY,
	FRStandardMtl_EMISSION_WEIGHT,
	FRStandardMtl_EMISSION_WEIGHT_TEXMAP,

	/// Implemented at geometry level
	FRStandardMtl_CASTS_SHADOWS
};

BEGIN_DECLARE_FRMTLCLASSDESC(StandardMtl, L"RPR Legacy Material", FIRERENDER_STANDARDMTL_CID)
END_DECLARE_FRMTLCLASSDESC()

class FireRenderStandardMtlTraits
{
public:
	using ClassDesc = FireRenderClassDescStandardMtl;
};

class FireRenderStandardMtl :
	public FireRenderMtl<FireRenderStandardMtlTraits>
{
public:
	Color GetDiffuse(int mtlNum, BOOL backFace) override;
	Color GetSpecular(int mtlNum, BOOL backFace) override;
	float GetShininess(int mtlNum, BOOL backFace) override;
	float GetSelfIllum(int mtlNum, BOOL backFace) override;
	Color GetSelfIllumColor(int mtlNum, BOOL backFace) override;
	float GetXParency(int mtlNum, BOOL backFace) override;
	frw::Shader getShader(const TimeValue t, MaterialParser& mtlParser, INode* node);
};

FIRERENDER_NAMESPACE_END;
