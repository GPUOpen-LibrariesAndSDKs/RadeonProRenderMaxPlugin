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

BEGIN_DECLARE_FRMTLCLASSDESC(DiffuseRefractionMtl, L"RPR Diffuse Refraction Material", FIRERENDER_DIFFUSEREFRACTIONMTL_CID)
END_DECLARE_FRMTLCLASSDESC()

BEGIN_DECLARE_FRMTL(DiffuseRefractionMtl)

virtual Color GetDiffuse(int mtlNum, BOOL backFace) override {
	return GetFromPb<Color>(pblock, FRDiffuseRefractionMtl_COLOR);
}

END_DECLARE_FRMTL(DiffuseRefractionMtl)

FIRERENDER_NAMESPACE_END;
