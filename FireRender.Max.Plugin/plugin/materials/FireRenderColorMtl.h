#pragma once

#include "FireRenderMtlBase.h"

FIRERENDER_NAMESPACE_BEGIN;

enum FRColorMtl_TexmapId {
	FRColorMtl_TEXMAP_COLOR = 0
};

enum FRColorMtl_ParamID : ParamID {
	FRColorMtl_COLOR = 1000,
	FRColorMtl_COLOR_TEXMAP = 1001
};

class FireRenderColorMtlTraits
{
public:
	static const TCHAR* InternalName() { return _T("RPR Color Value"); }
	static Class_ID ClassId() { return FIRERENDER_COLORMTL_CID; }
};

class FireRenderColorMtl :
	public FireRenderTex<FireRenderColorMtlTraits, FireRenderColorMtl>
{
public:
	AColor EvalColor(ShadeContext& sc) override;
	void Update(TimeValue t, Interval& valid) override;
	frw::Value GetShader(const TimeValue t, class MaterialParser& mtlParser) override;
};

FIRERENDER_NAMESPACE_END;
