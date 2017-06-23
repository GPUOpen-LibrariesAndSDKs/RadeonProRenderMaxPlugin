#pragma once

#include "FireRenderMtlBase.h"

FIRERENDER_NAMESPACE_BEGIN;

enum FRReflectionMtl_TexmapId {
	FRReflectionMtl_TEXMAP_COLOR = 0,
	FRReflectionMtl_TEXMAP_NORMAL = 1
};

enum FRReflectionMtl_ParamID : ParamID {
	FRReflectionMtl_COLOR = 1000,
	FRReflectionMtl_COLOR_TEXMAP = 1002,
	FRReflectionMtl_NORMALMAP = 1003
};

BEGIN_DECLARE_FRMTLCLASSDESC(ReflectionMtl, L"RPR Reflection Material", FIRERENDER_REFLECTIONMTL_CID)
END_DECLARE_FRMTLCLASSDESC()

BEGIN_DECLARE_FRMTL(ReflectionMtl)

virtual Color GetDiffuse(int mtlNum, BOOL backFace) override {
	return GetFromPb<Color>(pblock, FRReflectionMtl_COLOR);
}

END_DECLARE_FRMTL(ReflectionMtl)

FIRERENDER_NAMESPACE_END;
