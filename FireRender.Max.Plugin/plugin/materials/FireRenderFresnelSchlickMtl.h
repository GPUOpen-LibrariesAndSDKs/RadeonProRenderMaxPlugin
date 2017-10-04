#pragma once

#include "FireRenderMtlBase.h"

FIRERENDER_NAMESPACE_BEGIN;

enum FRFresnelSchlickMtl_TexmapId {
	FRFresnelSchlickMtl_TEXMAP_INVEC = 0,
	FRFresnelSchlickMtl_TEXMAP_N = 1,
	FRFresnelSchlickMtl_TEXMAP_REFLECTANCE = 2,
};

enum FRFresnelSchlickMtl_ParamID : ParamID {
	FRFresnelSchlickMtl_REFLECTANCE = 1001,
	FRFresnelSchlickMtl_INVEC_TEXMAP = 1003,
	FRFresnelSchlickMtl_N_TEXMAP = 1004,
	FRFresnelSchlickMtl_REFLECTANCE_TEXMAP = 1005
};

class FireRenderFresnelSchlickMtlTraits
{
public:
	static const TCHAR* InternalName() { return _T("RPR Fresnel-Schlick"); }
	static Class_ID ClassId() { return FIRERENDER_FRESNELSCHLICKMTL_CID; }
};

class FireRenderFresnelSchlickMtl :
	public FireRenderTex<FireRenderFresnelSchlickMtlTraits, FireRenderFresnelSchlickMtl>
{
public:
	void Update(TimeValue t, Interval& valid) override;
	frw::Value GetShader(const TimeValue t, class MaterialParser& mtlParser) override;
};

FIRERENDER_NAMESPACE_END;
