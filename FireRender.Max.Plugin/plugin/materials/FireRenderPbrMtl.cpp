/*********************************************************************************************************************************
* Radeon ProRender for 3ds Max plugin
* Copyright (c) 2017 AMD
* All Rights Reserved
*
* Radeon ProRender dedicated custom material plugin
*********************************************************************************************************************************/
#include "FireRenderPbrMtl.h"

#include "Resource.h"

#include "parser\MaterialParser.h"

FIRERENDER_NAMESPACE_BEGIN;

namespace
{
	static constexpr float RoughnessMin = 0.f;
	static constexpr float RoughnessMax = 1.f;
}

IMPLEMENT_FRMTLCLASSDESC(PbrMtl)

FRMTLCLASSDESCNAME(PbrMtl) FRMTLCLASSNAME(PbrMtl)::ClassDescInstance;

// All parameters of the material plugin. See FIRE_MAX_PBDESC definition for notes on backwards compatibility
static ParamBlockDesc2 pbDesc
(
	0, _T("PbrMtlPbdesc"), 0, &FRMTLCLASSNAME(PbrMtl)::ClassDescInstance, P_AUTO_CONSTRUCT + P_AUTO_UI + P_VERSION, FIRERENDERMTLVER_LATEST, 0,
    //rollout
	IDD_FIRERENDER_PBRMTL, IDS_FR_MTL_PBR, 0, 0, NULL,

	// DIFFUSE
	FRPBRMTL_DIFFUSE_COLOR_MUL, _T("DiffuseColorMul"), TYPE_FLOAT, P_ANIMATABLE, 0,
	p_default, 1.0f, p_range, 0.f, FLT_MAX, p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_PBR_DIFFUSE_COLOR_MUL, IDC_PBR_DIFFUSE_COLOR_MUL_S, SPIN_AUTOSCALE, PB_END,

	FRPBRMTL_DIFFUSE_COLOR, _T("DiffuseColor"), TYPE_RGBA, P_ANIMATABLE, 0,
	p_default, Color(1.f, 1.f, 1.f), p_ui, TYPE_COLORSWATCH, IDC_PBR_DIFFUSE_COLOR, PB_END,

	FRPBRMTL_DIFFUSE_COLOR_MAP, _T("DiffuseColorTexmap"), TYPE_TEXMAP, 0, 0,
	p_subtexno, FRPBRMTL_MAP_DIFFUSE_COLOR, p_ui, TYPE_TEXMAPBUTTON, IDC_PBR_DIFFUSE_COLOR_MAP, PB_END,
	
    PB_END
);

std::map<int, std::pair<ParamID, MCHAR*>> FRMTLCLASSNAME(PbrMtl)::TEXMAP_MAPPING =
{
	{ FRPBRMTL_MAP_DIFFUSE_COLOR, { FRPBRMTL_DIFFUSE_COLOR_MAP, _T("Diffuse Color") } },
};

FRMTLCLASSNAME(PbrMtl)::~FRMTLCLASSNAME(PbrMtl)()
{
}

frw::Shader FRMTLCLASSNAME(PbrMtl)::getVolumeShader(const TimeValue t, MaterialParser& mtlParser, INode* node)
{
	return frw::Shader();
}

frw::Shader FRMTLCLASSNAME(PbrMtl)::getShader(const TimeValue t, MaterialParser& mtlParser, INode* node)
{
	auto ms = mtlParser.materialSystem;

	frw::Shader material(ms, frw::ShaderTypeVolume);

	const Color color = GetFromPb<Color>(pblock, FRPBRMTL_DIFFUSE_COLOR);
	Texmap* colorTexmap = GetFromPb<Texmap*>(pblock, FRPBRMTL_DIFFUSE_COLOR_MAP);
	frw::Value theColor(color.r, color.g, color.b);
	if (colorTexmap)
		theColor = mtlParser.createMap(colorTexmap, 0);

	return material;
}

FIRERENDER_NAMESPACE_END;
