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

BEGIN_DECLARE_FRMTLCLASSDESC(MicrofacetMtl, L"RPR Microfacet Material", FIRERENDER_MICROFACETMTL_CID)
END_DECLARE_FRMTLCLASSDESC()

BEGIN_DECLARE_FRMTL(MicrofacetMtl)

virtual Color GetDiffuse(int mtlNum, BOOL backFace) override {
	return GetFromPb<Color>(pblock, FRMicrofacetMtl_COLOR);
}

END_DECLARE_FRMTL(MicrofacetMtl)

FIRERENDER_NAMESPACE_END;
