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

BEGIN_DECLARE_FRTEXCLASSDESC(DisplacementMtl, L"RPR Displacement", FIRERENDER_DISPLACEMENTMTL_CID)
END_DECLARE_FRTEXCLASSDESC()

BEGIN_DECLARE_FRTEX(DisplacementMtl)
	virtual  AColor EvalColor(ShadeContext& sc) override
	{
		Color color(0.f, 0.f, 0.f);
		Texmap* colorTexmap = GetFromPb<Texmap*>(pblock, FRDisplacementMtl_COLOR_TEXMAP);
		if (colorTexmap)
			color = colorTexmap->EvalColor(sc);
		return AColor(color.r, color.g, color.b);
	}

static Texmap *findDisplacementMap(const TimeValue t, MaterialParser& mtlParser, Mtl* material,
	float &minHeight, float &maxHeight, float &subdivision, float &creaseWeight, int &boundary, bool &notAccurate);

	static frw::Value translateDisplacement(const TimeValue t, MaterialParser& mtlParser, Mtl* material,
		float &minHeight, float &maxHeight, float &subdivision, float &creaseWeight, int &boundary, bool &notAccurate);

END_DECLARE_FRTEX(DisplacementMtl)


FIRERENDER_NAMESPACE_END;
