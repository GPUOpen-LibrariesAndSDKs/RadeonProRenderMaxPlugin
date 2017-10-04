#pragma once

#include "FireRenderMtlBase.h"

FIRERENDER_NAMESPACE_BEGIN;

enum FRAvgMtl_TexmapId {
	FRAvgMtlMtl_TEXMAP_COLOR = 0
};

enum FRAvgMtl_ParamID : ParamID {
	FRAvgMtl_COLOR = 1000,
	FRAvgMtl_COLOR_TEXMAP = 1001
};

class FireRenderAvgMtlTraits
{
public:
	static const TCHAR* InternalName() { return _T("RPR Average"); }
	static Class_ID ClassId() { return FIRERENDER_AVERAGEMTL_CID; }
};

class FireRenderAvgMtl :
	public FireRenderTex<FireRenderAvgMtlTraits, FireRenderAvgMtl>
{
public:
	AColor EvalColor(ShadeContext& sc) override;
	void Update(TimeValue t, Interval& valid) override;
	frw::Value GetShader(const TimeValue t, class MaterialParser& mtlParser) override;
};

FIRERENDER_NAMESPACE_END;
