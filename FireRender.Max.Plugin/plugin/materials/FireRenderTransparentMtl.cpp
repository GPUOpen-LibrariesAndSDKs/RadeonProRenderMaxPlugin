/*********************************************************************************************************************************
* Radeon ProRender for 3ds Max plugin
* Copyright (c) 2017 AMD
* All Rights Reserved
*
* Radeon ProRender dedicated custom material plugin
*********************************************************************************************************************************/
#include "FireRenderTransparentMtl.h"
#include "Resource.h"
#include "parser\MaterialParser.h"
#include "maxscript\mxsplugin\mxsPlugin.h"

FIRERENDER_NAMESPACE_BEGIN;

// All parameters of the material plugin. See FIRE_MAX_PBDESC definition for notes on backwards compatibility
static ParamBlockDesc2 pbDesc(
	0, _T("TransparentMtlPbdesc"), 0, &FireRenderTransparentMtl::GetClassDesc(), P_AUTO_CONSTRUCT + P_AUTO_UI + P_VERSION, FIRERENDERMTLVER_LATEST, 0,
    //rollout
	IDD_FIRERENDER_TRANSPARENTMTL, IDS_FR_MTL_TRANSPARENT, 0, 0, NULL,

	FRTransparentMtl_COLOR, _T("Color"), TYPE_RGBA, P_ANIMATABLE, 0,
    p_default, Color(0.5f, 0.5f, 0.5f), p_ui, TYPE_COLORSWATCH, IDC_REFR_COLOR, PB_END,

	FRTransparentMtl_COLOR_TEXMAP, _T("colorTexmap"), TYPE_TEXMAP, 0, 0,
	p_subtexno, FRTransparentMtl_TEXMAP_COLOR, p_ui, TYPE_TEXMAPBUTTON, IDC_REFR_COLOR_TEXMAP, PB_END,

    PB_END
    );

std::map<int, std::pair<ParamID, MCHAR*>> FireRenderTransparentMtl::TEXMAP_MAPPING = {
	{ FRTransparentMtl_TEXMAP_COLOR, { FRTransparentMtl_COLOR_TEXMAP, _T("Color map") } }
};

Color FireRenderTransparentMtl::GetDiffuse(int mtlNum, BOOL backFace)
{
	return GetFromPb<Color>(pblock, FRTransparentMtl_COLOR);
}

float FireRenderTransparentMtl::GetXParency(int mtlNum, BOOL backFace)
{
	Color c = GetFromPb<Color>(pblock, FRTransparentMtl_COLOR);
	return (c.r + c.b + c.g) * (1.f / 3.f);
}

frw::Shader FireRenderTransparentMtl::GetShader(const TimeValue t, MaterialParser& mtlParser, INode* node)
{
	auto ms = mtlParser.materialSystem;

	frw::Shader material(ms, frw::ShaderTypeTransparent);
		
	const Color diffuseColor = GetFromPb<Color>(pblock, FRTransparentMtl_COLOR);
	Texmap* diffuseTexmap = GetFromPb<Texmap*>(pblock, FRTransparentMtl_COLOR_TEXMAP);

	frw::Value color(diffuseColor.r, diffuseColor.g, diffuseColor.b);
	if (diffuseTexmap)
		color = mtlParser.createMap(diffuseTexmap, 0);

	material.SetValue("color", color);
		
    return material;
}

FIRERENDER_NAMESPACE_END;
