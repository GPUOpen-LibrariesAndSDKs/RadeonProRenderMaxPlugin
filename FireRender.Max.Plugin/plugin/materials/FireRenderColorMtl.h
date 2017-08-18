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

BEGIN_DECLARE_FRTEXCLASSDESC(ColorMtl, L"RPR Color Value", FIRERENDER_COLORMTL_CID)
END_DECLARE_FRTEXCLASSDESC()

BEGIN_DECLARE_FRTEX(ColorMtl)
	virtual  AColor EvalColor(ShadeContext& sc) override
	{
		Color color = GetFromPb<Color>(pblock, FRColorMtl_COLOR);
		Texmap* colorTexmap = GetFromPb<Texmap*>(pblock, FRColorMtl_COLOR_TEXMAP);
		if (colorTexmap)
			color = colorTexmap->EvalColor(sc);
		return color;
	}
END_DECLARE_FRTEX(ColorMtl)


FIRERENDER_NAMESPACE_END;
