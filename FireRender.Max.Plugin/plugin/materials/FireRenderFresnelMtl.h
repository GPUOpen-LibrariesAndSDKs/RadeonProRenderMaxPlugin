#pragma once

#include "FireRenderMtlBase.h"

FIRERENDER_NAMESPACE_BEGIN;;

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

class FireRenderFresnelMtlTraits
{
public:
	static const TCHAR* InternalName() { return _T("RPR Fresnel"); }
	static Class_ID ClassId() { return FIRERENDER_FRESNELMTL_CID; }
};

class FireRenderFresnelMtl :
	public FireRenderTex<FireRenderFresnelMtlTraits, FireRenderFresnelMtl>
{
public:
	void Update(TimeValue t, Interval& valid) override;
	frw::Value GetShader(const TimeValue t, class MaterialParser& mtlParser) override;
};

FIRERENDER_NAMESPACE_END;
