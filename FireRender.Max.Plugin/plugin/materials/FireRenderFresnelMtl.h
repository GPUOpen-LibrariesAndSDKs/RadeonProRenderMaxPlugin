#pragma once

#include "FireRenderMtlBase.h"

FIRERENDER_NAMESPACE_BEGIN;

enum FRFresnelMtl_TexmapId {
	FRFresnelMtl_TEXMAP_INVEC = 0,
	FRFresnelMtl_TEXMAP_N = 1,
	FRFresnelMtl_TEXMAP_IOR = 2,
};

enum FRFresnelMtl_ParamID : ParamID {
	FRFresnelMtl_IOR = 1001,
	FRFresnelMtl_INVEC_TEXMAP = 1003,
	FRFresnelMtl_N_TEXMAP = 1004,
	FRFresnelMtl_IOR_TEXMAP = 1005
};

BEGIN_DECLARE_FRTEXCLASSDESC(FresnelMtl, L"RPR Fresnel", FIRERENDER_FRESNELMTL_CID)
END_DECLARE_FRTEXCLASSDESC()

BEGIN_DECLARE_FRTEX(FresnelMtl)
END_DECLARE_FRTEX(FresnelMtl)


FIRERENDER_NAMESPACE_END;