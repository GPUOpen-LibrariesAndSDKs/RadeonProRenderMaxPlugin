#pragma once

#include "FireRenderMtlBase.h"

FIRERENDER_NAMESPACE_BEGIN;

enum FRDisplacementMtl_TexmapId {
	FRDisplacementMtl_TEXMAP_COLOR = 0
};

enum FRDisplacementMtl_ParamID : ParamID {
	FRDisplacementMtl_COLOR_TEXMAP = 1000,
	FRDisplacementMtl_MINHEIGHT,
	FRDisplacementMtl_MAXHEIGHT,
	FRDisplacementMtl_SUBDIVISION,
	FRDisplacementMtl_CREASEWEIGHT,
	FRDisplacementMtl_BOUNDARY,
};

class FireRenderDisplacementMtlTraits
{
public:
	static const TCHAR* InternalName() { return _T("RPR Displacement"); }
	static Class_ID ClassId() { return FIRERENDER_DISPLACEMENTMTL_CID; }
};

class FireRenderDisplacementMtl :
	public FireRenderTex<FireRenderDisplacementMtlTraits, FireRenderDisplacementMtl>
{
public:

	static Texmap *findDisplacementMap(const TimeValue t, MaterialParser& mtlParser, Mtl* material,
		float &minHeight, float &maxHeight, float &subdivision, float &creaseWeight, int &boundary, bool &notAccurate);

	static frw::Value translateDisplacement(const TimeValue t, MaterialParser& mtlParser, Mtl* material,
		float &minHeight, float &maxHeight, float &subdivision, float &creaseWeight, int &boundary, bool &notAccurate);

	AColor EvalColor(ShadeContext& sc) override;
	void Update(TimeValue t, Interval& valid) override;
	frw::Value GetShader(const TimeValue t, class MaterialParser& mtlParser) override;
};

FIRERENDER_NAMESPACE_END;
