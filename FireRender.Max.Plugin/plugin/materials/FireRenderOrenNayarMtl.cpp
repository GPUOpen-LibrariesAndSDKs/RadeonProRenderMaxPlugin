/*********************************************************************************************************************************
* Radeon ProRender for 3ds Max plugin
* Copyright (c) 2017 AMD
* All Rights Reserved
*
* Radeon ProRender dedicated custom material plugin
*********************************************************************************************************************************/
#include "FireRenderOrenNayarMtl.h"
#include "FireRenderNormalMtl.h"
#include "Resource.h"
#include "parser\MaterialParser.h"
#include "maxscript\mxsplugin\mxsPlugin.h"

FIRERENDER_NAMESPACE_BEGIN;

IMPLEMENT_FRMTLCLASSDESC(OrenNayarMtl)

FRMTLCLASSDESCNAME(OrenNayarMtl) FRMTLCLASSNAME(OrenNayarMtl)::ClassDescInstance;

// All parameters of the material plugin. See FIRE_MAX_PBDESC definition for notes on backwards compatibility
static ParamBlockDesc2 pbDesc(
	0, _T("OrenNayarMtlPbdesc"), 0, &FRMTLCLASSNAME(OrenNayarMtl)::ClassDescInstance, P_AUTO_CONSTRUCT + P_AUTO_UI + P_VERSION, FIRERENDERMTLVER_LATEST, 0,
    //rollout
	IDD_FIRERENDER_ORENNAYARMTL, IDS_FR_MTL_ORENNAYAR, 0, 0, NULL,

	FROrenNayarMtl_COLOR, _T("diffuseColor"), TYPE_RGBA, P_ANIMATABLE, 0,
    p_default, Color(0.5f, 0.5f, 0.5f), p_ui, TYPE_COLORSWATCH, IDC_DIFFUSE_COLOR, PB_END,

	FROrenNayarMtl_ROUGHNESS, _T("diffuseRougness"), TYPE_FLOAT, P_ANIMATABLE, 0,
    p_default, 1.f, p_range, 0.f, 1.f, p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_DIFFUSE_ROUGHNESS, IDC_DIFFUSE_ROUGHNESS_S, SPIN_AUTOSCALE, PB_END,

	FROrenNayarMtl_COLOR_TEXMAP, _T("diffuseTexmap"), TYPE_TEXMAP, 0, 0,
	p_subtexno, FROrenNayarMtl_TEXMAP_DIFFUSE, p_ui, TYPE_TEXMAPBUTTON, IDC_DIFFUSE_TEXMAP, PB_END,

	FROrenNayarMtl_ROUGHNESS_TEXMAP, _T("roughnessTexmap"), TYPE_TEXMAP, 0, 0,
	p_subtexno, FROrenNayarMtl_TEXMAP_ROUGHNESS, p_ui, TYPE_TEXMAPBUTTON, IDC_DIFFUSE_ROUGHNESS_TEXMAP, PB_END,

	FROrenNayarMtl_NORMALMAP, _T("normalTexmap"), TYPE_TEXMAP, 0, 0,
	p_subtexno, FROrenNayarMtl_TEXMAP_NORMAL, p_ui, TYPE_TEXMAPBUTTON, IDC_NORMAL_TEXMAP, PB_END,

    PB_END
    );

std::map<int, std::pair<ParamID, MCHAR*>> FRMTLCLASSNAME(OrenNayarMtl)::TEXMAP_MAPPING = {
	{ FROrenNayarMtl_TEXMAP_DIFFUSE, { FROrenNayarMtl_COLOR_TEXMAP, _T("Color Map") } },
	{ FROrenNayarMtl_TEXMAP_ROUGHNESS, { FROrenNayarMtl_ROUGHNESS_TEXMAP, _T("Roughness Map") } },
	{ FROrenNayarMtl_TEXMAP_NORMAL, { FROrenNayarMtl_NORMALMAP, _T("Normal map") } }
};

FRMTLCLASSNAME(OrenNayarMtl)::~FRMTLCLASSNAME(OrenNayarMtl)()
{
}


frw::Shader FRMTLCLASSNAME(OrenNayarMtl)::getShader(const TimeValue t, MaterialParser& mtlParser, INode* node)
{
	auto ms = mtlParser.materialSystem;

	frw::Shader material(ms, frw::ShaderTypeOrenNayer);
		
	Texmap* normalTexmap = GetFromPb<Texmap*>(pblock, FROrenNayarMtl_NORMALMAP);
	const Color diffuseColor = GetFromPb<Color>(pblock, FROrenNayarMtl_COLOR);
	const float roughness = GetFromPb<float>(pblock, FROrenNayarMtl_ROUGHNESS);
	Texmap* diffuseTexmap = GetFromPb<Texmap*>(pblock, FROrenNayarMtl_COLOR_TEXMAP);
	Texmap* roughnessTexmap = GetFromPb<Texmap*>(pblock, FROrenNayarMtl_ROUGHNESS_TEXMAP);
		
	frw::Value color(diffuseColor.r, diffuseColor.g, diffuseColor.b);
	if (diffuseTexmap)
		color = mtlParser.createMap(diffuseTexmap, 0);

	frw::Value roughnessv(roughness, roughness, roughness);
	if (roughnessTexmap)
		roughnessv = mtlParser.createMap(roughnessTexmap, MAP_FLAG_NOGAMMA);

	material.SetValue(RPR_MATERIAL_INPUT_COLOR, color);
	material.SetValue(RPR_MATERIAL_INPUT_ROUGHNESS, roughnessv);
	
	if (normalTexmap)
		material.SetValue(RPR_MATERIAL_INPUT_NORMAL, FRMTLCLASSNAME(NormalMtl)::translateGenericBump(t, normalTexmap, 1.f, mtlParser));
	
    return material;
}

FIRERENDER_NAMESPACE_END;
