#pragma once

#include "FireRenderMtlBase.h"

FIRERENDER_NAMESPACE_BEGIN;

enum FRAmbientOcclusionMtl_TexMapID
{
	FRAmbientOcclusionMtl_TEXMAP_COLOR0 = 0, // Unoccluded Color Texmap
	FRAmbientOcclusionMtl_TEXMAP_COLOR1 = 1  // Occluded Color Texmap
};

enum FRAmbientOcclusionMTL_SIDEID
{
	FRAmbientOcclusionMtl_SIDE_FRONT,
	FRAmbientOcclusionMtl_SIDE_BACK
};

enum FRAmbientOcclusionMtl_ParamID : ParamID
{
	FRAmbientOcclusionMtl_RADIUS = 1000,
	FRAmbientOcclusionMtl_SIDE = 1001,
	FRAmbientOcclusionMtl_COLOR0 = 1002, // Unoccluded Color
	FRAmbientOcclusionMtl_COLOR0_TEXMAP = 1003, // Unoccluded  Color Texmap
	FRAmbientOcclusionMtl_COLOR1 = 1004, // Occluded Color
	FRAmbientOcclusionMtl_COLOR1_TEXMAP = 1005 //Occluded Color Texmap
};

BEGIN_DECLARE_FRTEXCLASSDESC(AmbientOcclusionMtl, L"RPR Ambient Occlusion", FIRERENDER_AMBIENTOCCLUSIONMTL_CID)
END_DECLARE_FRTEXCLASSDESC()

BEGIN_DECLARE_FRTEX(AmbientOcclusionMtl)
// EvalColor is used by software Renderers other than RPR, such as default Scanline Renderer
// This does not calculate Ambient Occlusion and instead always returns the Unoccluded Color 
virtual  AColor EvalColor(ShadeContext& sc) override
{
	Color color0 = GetFromPb<Color>(pblock, FRAmbientOcclusionMtl_COLOR0);
	Texmap* color0Texmap = GetFromPb<Texmap*>(pblock, FRAmbientOcclusionMtl_COLOR0_TEXMAP);
	if (color0Texmap)
		color0 = color0Texmap->EvalColor(sc);
	
	return color0;
}

END_DECLARE_FRTEX(AmbientOcclusionMtl)


FIRERENDER_NAMESPACE_END;