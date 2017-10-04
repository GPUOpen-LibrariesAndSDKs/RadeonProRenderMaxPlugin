/*********************************************************************************************************************************
* Radeon ProRender for 3ds Max plugin
* Copyright (c) 2017 AMD
* All Rights Reserved
*
* Radeon ProRender dedicated custom material plugin
*********************************************************************************************************************************/
#include "FireRenderRefractionMtl.h"
#include "FireRenderNormalMtl.h"
#include "Resource.h"
#include "parser\MaterialParser.h"
#include "maxscript\mxsplugin\mxsPlugin.h"

FIRERENDER_NAMESPACE_BEGIN;

// All parameters of the material plugin. See FIRE_MAX_PBDESC definition for notes on backwards compatibility
static ParamBlockDesc2 pbDesc(
	0, _T("RefractionMtlPbdesc"), 0, &FireRenderRefractionMtl::GetClassDesc(), P_AUTO_CONSTRUCT + P_AUTO_UI + P_VERSION, FIRERENDERMTLVER_LATEST, 0,
    //rollout
	IDD_FIRERENDER_REFRACTIONMTL, IDS_FR_MTL_REFRACTION, 0, 0, NULL,

	FRRefractionMtl_COLOR, _T("Color"), TYPE_RGBA, P_ANIMATABLE, 0,
    p_default, Color(0.5f, 0.5f, 0.5f), p_ui, TYPE_COLORSWATCH, IDC_REFR_COLOR, PB_END,

	FRRefractionMtl_IOR, _T("IOR"), TYPE_FLOAT, P_ANIMATABLE, 0,
	p_default, 1.52f, p_range, 1.f, 999.f, p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_REFR_IOR, IDC_REFR_IOR_S, SPIN_AUTOSCALE, PB_END,

	FRRefractionMtl_COLOR_TEXMAP, _T("colorTexmap"), TYPE_TEXMAP, 0, 0,
	p_subtexno, FRRefractionMtl_TEXMAP_COLOR, p_ui, TYPE_TEXMAPBUTTON, IDC_REFR_COLOR_TEXMAP, PB_END,

	FRRefractionMtl_NORMALMAP, _T("normalTexmap"), TYPE_TEXMAP, 0, 0,
	p_subtexno, FRRefractionMtl_TEXMAP_NORMAL, p_ui, TYPE_TEXMAPBUTTON, IDC_NORMAL_TEXMAP, PB_END,

    PB_END
    );

std::map<int, std::pair<ParamID, MCHAR*>> FireRenderRefractionMtl::TEXMAP_MAPPING = {
	{ FRRefractionMtl_TEXMAP_COLOR, { FRRefractionMtl_COLOR_TEXMAP, _T("Color Map") } },
	{ FRRefractionMtl_TEXMAP_NORMAL, { FRRefractionMtl_NORMALMAP, _T("Normal Map") } }
};

Color FireRenderRefractionMtl::GetDiffuse(int mtlNum, BOOL backFace)
{
	return GetFromPb<Color>(pblock, FRRefractionMtl_COLOR);
}

float FireRenderRefractionMtl::GetXParency(int mtlNum, BOOL backFace)
{
	return 0.5f;
}

frw::Shader FireRenderRefractionMtl::GetShader(const TimeValue t, MaterialParser& mtlParser, INode* node)
{
	auto ms = mtlParser.materialSystem;

	frw::Shader material(ms, frw::ShaderTypeRefraction);
		
	Texmap* normalTexmap = GetFromPb<Texmap*>(pblock, FRRefractionMtl_NORMALMAP);
	const Color diffuseColor = GetFromPb<Color>(pblock, FRRefractionMtl_COLOR);
	Texmap* diffuseTexmap = GetFromPb<Texmap*>(pblock, FRRefractionMtl_COLOR_TEXMAP);
	const float ior = GetFromPb<float>(pblock, FRRefractionMtl_IOR);
		
	frw::Value color(diffuseColor.r, diffuseColor.g, diffuseColor.b);
	if (diffuseTexmap)
		color = mtlParser.createMap(diffuseTexmap, 0);
	material.SetValue("color", color);

	frw::Value iorv(ior, ior, ior);
	material.SetValue("ior", iorv);
	
	if (normalTexmap)
		material.SetValue("normal", FireRenderNormalMtl::translateGenericBump(t, normalTexmap, 1.f, mtlParser));
	
    return material;
}

FIRERENDER_NAMESPACE_END;
