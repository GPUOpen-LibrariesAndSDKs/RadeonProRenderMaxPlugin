#pragma once

#include "FireRenderMtlBase.h"

FIRERENDER_NAMESPACE_BEGIN;

enum FRDiffuseMtl_TexmapId {
	FRDiffuseMtl_TEXMAP_DIFFUSE = 0,
	FRDiffuseMtl_TEXMAP_NORMAL = 1
};

enum FRDiffuseMtl_ParamID : ParamID {
	FRDiffuseMtl_COLOR = 1000,
	FRDiffuseMtl_COLOR_TEXMAP = 1002,
	FRDiffuseMtl_NORMALMAP = 1003
};

BEGIN_DECLARE_FRMTLCLASSDESC(DiffuseMtl, L"RPR Diffuse Material", FIRERENDER_DIFFUSEMTL_CID)
END_DECLARE_FRMTLCLASSDESC()

BEGIN_DECLARE_FRMTL(DiffuseMtl)
public:
virtual Color GetDiffuse(int mtlNum, BOOL backFace) override {
	return GetFromPb<Color>(pblock, FRDiffuseMtl_COLOR);
}
END_DECLARE_FRMTL(DiffuseMtl)

FIRERENDER_NAMESPACE_END;
