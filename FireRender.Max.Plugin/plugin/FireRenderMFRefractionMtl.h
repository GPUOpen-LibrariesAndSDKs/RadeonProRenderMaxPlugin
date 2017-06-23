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

BEGIN_DECLARE_FRMTL(MFRefractionMtl)

virtual Color GetDiffuse(int mtlNum, BOOL backFace) override {
	return GetFromPb<Color>(pblock, FRMFRefractionMtl_COLOR);
}

virtual float GetXParency(int mtlNum, BOOL backFace) override {
	return 0.5f;
}

END_DECLARE_FRMTL(MFRefractionMtl)

FIRERENDER_NAMESPACE_END;
