#pragma once

#include "FireRenderMtlBase.h"

FIRERENDER_NAMESPACE_BEGIN;

enum FRMFRefractionMtl_TexmapId {
	FRMFRefractionMtl_TEXMAP_COLOR = 0,
	FRMFRefractionMtl_TEXMAP_ROUGHNESS = 1,
	FRMFRefractionMtl_TEXMAP_NORMAL = 2
};

enum FRMFRefractionMtl_ParamID : ParamID {
	FRMFRefractionMtl_COLOR = 1000,
	FRMFRefractionMtl_IOR = 1001,
	FRMFRefractionMtl_ROUGHNESS = 1002,
	FRMFRefractionMtl_COLOR_TEXMAP = 1003,
	FRMFRefractionMtl_ROUGHNESS_TEXMAP = 1004,
	FRMFRefractionMtl_NORMALMAP = 1005,
};

BEGIN_DECLARE_FRMTLCLASSDESC(MFRefractionMtl, L"RPR Microfacet Refraction Material", FIRERENDER_MFREFRACTIONMTL_CID)
END_DECLARE_FRMTLCLASSDESC()

class FireRenderMFRefractionMtlTraits
{
public:
	using ClassDesc = FireRenderClassDescMFRefractionMtl;
};

class FireRenderMFRefractionMtl :
	public FireRenderMtl<FireRenderMFRefractionMtlTraits>
{
public:
	Color GetDiffuse(int mtlNum, BOOL backFace) override;
	float GetXParency(int mtlNum, BOOL backFace) override;
	frw::Shader getShader(const TimeValue t, class MaterialParser& mtlParser, INode* node);
};

FIRERENDER_NAMESPACE_END;
