#pragma once

#include "FireRenderMtlBase.h"

FIRERENDER_NAMESPACE_BEGIN;

enum FRTransparentMtl_TexmapId {
	FRTransparentMtl_TEXMAP_COLOR = 0
};

enum FRTransparentMtl_ParamID : ParamID {
	FRTransparentMtl_COLOR = 1000,
	FRTransparentMtl_COLOR_TEXMAP = 1001
};

BEGIN_DECLARE_FRMTLCLASSDESC(TransparentMtl, L"RPR Transparent Material", FIRERENDER_TRANSPARENTMTL_CID)
END_DECLARE_FRMTLCLASSDESC()

BEGIN_DECLARE_FRMTL(TransparentMtl)

virtual Color GetDiffuse(int mtlNum, BOOL backFace) override {
	return GetFromPb<Color>(pblock, FRTransparentMtl_COLOR);
}

virtual float GetXParency(int mtlNum, BOOL backFace) override {
	Color c = GetFromPb<Color>(pblock, FRTransparentMtl_COLOR);
	return (c.r + c.b + c.g) * (1.f / 3.f);
}

END_DECLARE_FRMTL(TransparentMtl)

FIRERENDER_NAMESPACE_END;
