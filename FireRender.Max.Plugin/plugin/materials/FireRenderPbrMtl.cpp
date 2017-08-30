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
	static constexpr float RoughnessMulMin = 0.0f;
	static constexpr float RoughnessMulMax = 1.0f;
	static constexpr float RoughnessMulDefault = 0.0f;

	static constexpr float MetalnessMulMin = 0.0f;
	static constexpr float MetalnessMulMax = 1.0f;
	static constexpr float MetalnessMulDefault = 0.0f;

	static constexpr float CavityMulMin = 0.0f;
	static constexpr float CavityMulMax = 1.0f;
	static constexpr float CavityMulDefault = 0.0f;

	static constexpr float NormalMulMin = 0.0f;
	static constexpr float NormalMulMax = 1.0f;
	static constexpr float NormalMulDefault = 0.0f;
}

IMPLEMENT_FRMTLCLASSDESC(PbrMtl)

FRMTLCLASSDESCNAME(PbrMtl) FRMTLCLASSNAME(PbrMtl)::ClassDescInstance;

// All parameters of the material plugin. See FIRE_MAX_PBDESC definition for notes on backwards compatibility
static ParamBlockDesc2 pbDesc
(
	0, _T("PbrMtlPbdesc"), 0, &FRMTLCLASSNAME(PbrMtl)::ClassDescInstance, P_AUTO_CONSTRUCT + P_AUTO_UI + P_VERSION, FIRERENDERMTLVER_LATEST, 0,
    //rollout
	IDD_FIRERENDER_PBRMTL, IDS_FR_MTL_PBR, 0, 0, NULL,

	// DIFFUSE COLOR
	FRPBRMTL_DIFFUSE_COLOR_MUL, _T("DiffuseColorMul"), TYPE_FLOAT, P_ANIMATABLE, 0,
	p_default, 1.0f, p_range, 0.0f, FLT_MAX, p_ui, TYPE_SPINNER, EDITTYPE_FLOAT,
	IDC_PBR_DIFFUSE_COLOR_MUL, IDC_PBR_DIFFUSE_COLOR_MUL_S, SPIN_AUTOSCALE, PB_END,

	FRPBRMTL_DIFFUSE_COLOR, _T("DiffuseColor"), TYPE_RGBA, P_ANIMATABLE, 0,
	p_default, Color(1.0f, 1.0f, 1.0f), p_ui, TYPE_COLORSWATCH, IDC_PBR_DIFFUSE_COLOR, PB_END,

	FRPBRMTL_DIFFUSE_COLOR_MAP, _T("DiffuseColorTexmap"), TYPE_TEXMAP, 0, 0,
	p_subtexno, FRPBRMTL_MAP_DIFFUSE_COLOR, p_ui, TYPE_TEXMAPBUTTON, IDC_PBR_DIFFUSE_COLOR_MAP, PB_END,
	
	FRPBRMTL_DIFFUSE_COLOR_USEMAP, _T("DiffuseColorUseMap"), TYPE_BOOL, P_ANIMATABLE, 0,
	p_default, FALSE, p_ui, TYPE_SINGLECHEKBOX, IDC_PBR_DIFFUSE_COLOR_USEMAP, PB_END,

	// ROUGHNESS
	FRPBRMTL_ROUGHNESS_MUL, _T("RoughnessMul"), TYPE_FLOAT, P_ANIMATABLE, 0,
	p_default, RoughnessMulDefault, p_range, RoughnessMulMin, RoughnessMulMax, p_ui, TYPE_SPINNER, EDITTYPE_FLOAT,
	IDC_PBR_ROUGHNESS_MUL, IDC_PBR_ROUGHNESS_MUL_S, SPIN_AUTOSCALE, PB_END,

	FRPBRMTL_ROUGHNESS, _T("Roughness"), TYPE_RGBA, P_ANIMATABLE, 0,
	p_default, Color(1.0f, 1.0f, 1.0f), p_ui, TYPE_COLORSWATCH, IDC_PBR_ROUGHNESS, PB_END,

	FRPBRMTL_ROUGHNESS_MAP, _T("RoughnessTexmap"), TYPE_TEXMAP, 0, 0,
	p_subtexno, FRPBRMTL_MAP_ROUGHNESS, p_ui, TYPE_TEXMAPBUTTON, IDC_PBR_ROUGHNESS_MAP, PB_END,
	
	FRPBRMTL_ROUGHNESS_USEMAP, _T("RoughnessUseMap"), TYPE_BOOL, P_ANIMATABLE, 0,
	p_default, FALSE, p_ui, TYPE_SINGLECHEKBOX, IDC_PBR_ROUGHNESS_USEMAP, PB_END,

	FRPBRMTL_ROUGHNESS_INVERT, _T("RoughnessInvert"), TYPE_BOOL, P_ANIMATABLE, 0,
	p_default, FALSE, p_ui, TYPE_SINGLECHEKBOX, IDC_PBR_ROUGHNESS_INVERT, PB_END,

	// METALNESS
	FRPBRMTL_METALNESS_MUL, _T("MetalnessMul"), TYPE_FLOAT, P_ANIMATABLE, 0,
	p_default, MetalnessMulDefault, p_range, MetalnessMulMin, MetalnessMulMax, p_ui, TYPE_SPINNER, EDITTYPE_FLOAT,
	IDC_PBR_METALNESS_MUL, IDC_PBR_METALNESS_MUL_S, SPIN_AUTOSCALE, PB_END,

	FRPBRMTL_METALNESS, _T("Metalness"), TYPE_RGBA, P_ANIMATABLE, 0,
	p_default, Color(1.0f, 1.0f, 1.0f), p_ui, TYPE_COLORSWATCH, IDC_PBR_METALNESS, PB_END,

	FRPBRMTL_METALNESS_MAP, _T("MetalnessTexmap"), TYPE_TEXMAP, 0, 0,
	p_subtexno, FRPBRMTL_MAP_METALNESS, p_ui, TYPE_TEXMAPBUTTON, IDC_PBR_METALNESS_MAP, PB_END,

	FRPBRMTL_METALNESS_USEMAP, _T("MetalnessUseMap"), TYPE_BOOL, P_ANIMATABLE, 0,
	p_default, FALSE, p_ui, TYPE_SINGLECHEKBOX, IDC_PBR_METALNESS_USEMAP, PB_END,

	// CAVITY
	FRPBRMTL_CAVITY_MUL, _T("CavityMul"), TYPE_FLOAT, P_ANIMATABLE, 0,
	p_default, CavityMulDefault, p_range, CavityMulMin, CavityMulMax, p_ui, TYPE_SPINNER, EDITTYPE_FLOAT,
	IDC_PBR_CAVITY_MUL, IDC_PBR_CAVITY_MUL_S, SPIN_AUTOSCALE, PB_END,

	FRPBRMTL_CAVITY, _T("Cavity"), TYPE_RGBA, P_ANIMATABLE, 0,
	p_default, Color(1.f, 1.f, 1.f), p_ui, TYPE_COLORSWATCH, IDC_PBR_CAVITY, PB_END,

	FRPBRMTL_CAVITY_MAP, _T("CavityTexmap"), TYPE_TEXMAP, 0, 0,
	p_subtexno, FRPBRMTL_MAP_CAVITY, p_ui, TYPE_TEXMAPBUTTON, IDC_PBR_CAVITY_MAP, PB_END,

	FRPBRMTL_CAVITY_USEMAP, _T("CavityUseMap"), TYPE_BOOL, P_ANIMATABLE, 0,
	p_default, FALSE, p_ui, TYPE_SINGLECHEKBOX, IDC_PBR_CAVITY_USEMAP, PB_END,

	FRPBRMTL_CAVITY_INVERT, _T("CavityInvert"), TYPE_BOOL, P_ANIMATABLE, 0,
	p_default, FALSE, p_ui, TYPE_SINGLECHEKBOX, IDC_PBR_CAVITY_INVERT, PB_END,

	// NORMAL
	FRPBRMTL_NORMAL_MUL, _T("NormalMul"), TYPE_FLOAT, P_ANIMATABLE, 0,
	p_default, NormalMulDefault, p_range, NormalMulMin, NormalMulMax, p_ui, TYPE_SPINNER, EDITTYPE_FLOAT,
	IDC_PBR_NORMAL_MUL, IDC_PBR_NORMAL_MUL_S, SPIN_AUTOSCALE, PB_END,

	FRPBRMTL_NORMAL, _T("Normal"), TYPE_RGBA, P_ANIMATABLE, 0,
	p_default, Color(1.0f, 1.0f, 1.0f), p_ui, TYPE_COLORSWATCH, IDC_PBR_NORMAL, PB_END,

	FRPBRMTL_NORMAL_MAP, _T("NormalTexmap"), TYPE_TEXMAP, 0, 0,
	p_subtexno, FRPBRMTL_MAP_NORMAL, p_ui, TYPE_TEXMAPBUTTON, IDC_PBR_NORMAL_MAP, PB_END,

	FRPBRMTL_NORMAL_USEMAP, _T("NormalUseMap"), TYPE_BOOL, P_ANIMATABLE, 0,
	p_default, FALSE, p_ui, TYPE_SINGLECHEKBOX, IDC_PBR_NORMAL_USEMAP, PB_END,

	PB_END
);

std::map<int, std::pair<ParamID, MCHAR*>> FRMTLCLASSNAME(PbrMtl)::TEXMAP_MAPPING =
{
	{ FRPBRMTL_MAP_DIFFUSE_COLOR, { FRPBRMTL_DIFFUSE_COLOR_MAP, _T("Diffuse Color") } },
	{ FRPBRMTL_MAP_ROUGHNESS,     { FRPBRMTL_ROUGHNESS_MAP,     _T("Roughness") } },
	{ FRPBRMTL_MAP_METALNESS,     { FRPBRMTL_METALNESS_MAP,     _T("Metalness") } },
	{ FRPBRMTL_MAP_CAVITY,        { FRPBRMTL_CAVITY_MAP,        _T("Cavity Map") } },
	{ FRPBRMTL_MAP_NORMAL,        { FRPBRMTL_NORMAL_MAP,        _T("Normal Map") } },
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
