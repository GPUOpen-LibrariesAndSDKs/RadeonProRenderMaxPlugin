#pragma once

#include "FireRenderMtlBase.h"

FIRERENDER_NAMESPACE_BEGIN;

enum FRBlendValueMtl_TexmapId {
	FRBlendValueMtl_TEXMAP_COLOR0 = 0,
	FRBlendValueMtl_TEXMAP_COLOR1 = 1,
	FRBlendValueMtl_TEXMAP_WEIGHT = 2
};

enum FRBlendValueMtl_ParamID : ParamID {
	FRBlendValueMtl_COLOR0 = 1000,
	FRBlendValueMtl_COLOR1 = 1001,
	FRBlendValueMtl_WEIGHT = 1002,
	FRBlendValueMtl_COLOR0_TEXMAP = 1003,
	FRBlendValueMtl_COLOR1_TEXMAP = 1004,
	FRBlendValueMtl_WEIGHT_TEXMAP = 1005
};

class FireRenderBlendValueMtlTraits
{
public:
	static const TCHAR* InternalName() { return _T("RPR Blend Value"); }
	static Class_ID ClassId() { return FIRERENDER_BLENDVALUEMTL_CID; }
};

class FireRenderBlendValueMtl :
	public FireRenderTex<FireRenderBlendValueMtlTraits, FireRenderBlendValueMtl>
{
public:
	AColor EvalColor(ShadeContext& sc) override;
	void Update(TimeValue t, Interval& valid) override;
	frw::Value GetShader(const TimeValue t, class MaterialParser& mtlParser) override;
};

FIRERENDER_NAMESPACE_END;
