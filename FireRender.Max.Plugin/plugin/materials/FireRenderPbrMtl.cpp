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
	static Color DiffuseColor(0.5f, 0.5f, 0.5f);

	static constexpr float DiffuseColorMulMin = 0.0f;
	static constexpr float DiffuseColorMulMax = 1.0f;
	static constexpr float DiffuseColorMulDefault = 1.0f;

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

	static constexpr float OpacityMulMin = 0.0f;
	static constexpr float OpacityMulMax = 1.0f;
	static constexpr float OpacityMulDefault = 1.0f;

	static constexpr float EmissiveMulMin = 0.0f;
	static constexpr float EmissiveMulMax = 1.0;
	static constexpr float EmissiveMulDefault = 0.0f;
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
	p_default, DiffuseColorMulDefault, p_range, DiffuseColorMulMin, DiffuseColorMulMax, p_ui, TYPE_SPINNER, EDITTYPE_FLOAT,
	IDC_PBR_DIFFUSE_COLOR_MUL, IDC_PBR_DIFFUSE_COLOR_MUL_S, SPIN_AUTOSCALE, PB_END,

	FRPBRMTL_DIFFUSE_COLOR, _T("DiffuseColor"), TYPE_RGBA, P_ANIMATABLE, 0,
	p_default, DiffuseColor, p_ui, TYPE_COLORSWATCH, IDC_PBR_DIFFUSE_COLOR, PB_END,

	FRPBRMTL_DIFFUSE_COLOR_MAP, _T("DiffuseColorTexmap"), TYPE_TEXMAP, 0, 0,
	p_subtexno, FRPBRMTL_MAP_DIFFUSE_COLOR, p_ui, TYPE_TEXMAPBUTTON, IDC_PBR_DIFFUSE_COLOR_MAP, PB_END,
	
	FRPBRMTL_DIFFUSE_COLOR_USEMAP, _T("DiffuseColorUseMap"), TYPE_BOOL, P_ANIMATABLE, 0,
	p_default, TRUE, p_ui, TYPE_SINGLECHEKBOX, IDC_PBR_DIFFUSE_COLOR_USEMAP, PB_END,

	// ROUGHNESS
	FRPBRMTL_ROUGHNESS_MUL, _T("RoughnessMul"), TYPE_FLOAT, P_ANIMATABLE, 0,
	p_default, RoughnessMulDefault, p_range, RoughnessMulMin, RoughnessMulMax, p_ui, TYPE_SPINNER, EDITTYPE_FLOAT,
	IDC_PBR_ROUGHNESS_MUL, IDC_PBR_ROUGHNESS_MUL_S, SPIN_AUTOSCALE, PB_END,

	FRPBRMTL_ROUGHNESS, _T("Roughness"), TYPE_RGBA, P_ANIMATABLE, 0,
	p_default, Color(1.0f, 1.0f, 1.0f), p_ui, TYPE_COLORSWATCH, IDC_PBR_ROUGHNESS, PB_END,

	FRPBRMTL_ROUGHNESS_MAP, _T("RoughnessTexmap"), TYPE_TEXMAP, 0, 0,
	p_subtexno, FRPBRMTL_MAP_ROUGHNESS, p_ui, TYPE_TEXMAPBUTTON, IDC_PBR_ROUGHNESS_MAP, PB_END,
	
	FRPBRMTL_ROUGHNESS_USEMAP, _T("RoughnessUseMap"), TYPE_BOOL, P_ANIMATABLE, 0,
	p_default, TRUE, p_ui, TYPE_SINGLECHEKBOX, IDC_PBR_ROUGHNESS_USEMAP, PB_END,

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
	p_default, TRUE, p_ui, TYPE_SINGLECHEKBOX, IDC_PBR_METALNESS_USEMAP, PB_END,

	// CAVITY
	FRPBRMTL_CAVITY_MUL, _T("CavityMul"), TYPE_FLOAT, P_ANIMATABLE, 0,
	p_default, CavityMulDefault, p_range, CavityMulMin, CavityMulMax, p_ui, TYPE_SPINNER, EDITTYPE_FLOAT,
	IDC_PBR_CAVITY_MUL, IDC_PBR_CAVITY_MUL_S, SPIN_AUTOSCALE, PB_END,

	FRPBRMTL_CAVITY, _T("Cavity"), TYPE_RGBA, P_ANIMATABLE, 0,
	p_default, Color(1.f, 1.f, 1.f), p_ui, TYPE_COLORSWATCH, IDC_PBR_CAVITY, PB_END,

	FRPBRMTL_CAVITY_MAP, _T("CavityTexmap"), TYPE_TEXMAP, 0, 0,
	p_subtexno, FRPBRMTL_MAP_CAVITY, p_ui, TYPE_TEXMAPBUTTON, IDC_PBR_CAVITY_MAP, PB_END,

	FRPBRMTL_CAVITY_USEMAP, _T("CavityUseMap"), TYPE_BOOL, P_ANIMATABLE, 0,
	p_default, TRUE, p_ui, TYPE_SINGLECHEKBOX, IDC_PBR_CAVITY_USEMAP, PB_END,

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
	p_default, TRUE, p_ui, TYPE_SINGLECHEKBOX, IDC_PBR_NORMAL_USEMAP, PB_END,

	// MATERIAL OPACITY
	FRPBRMTL_OPACITY_MUL, _T("OpacityMul"), TYPE_FLOAT, P_ANIMATABLE, 0,
	p_default, OpacityMulDefault, p_range, OpacityMulMin, OpacityMulMax, p_ui, TYPE_SPINNER, EDITTYPE_FLOAT,
	IDC_PBR_OPACITY_MUL, IDC_PBR_OPACITY_MUL_S, SPIN_AUTOSCALE, PB_END,

	FRPBRMTL_OPACITY, _T("Opacity"), TYPE_RGBA, P_ANIMATABLE, 0,
	p_default, Color(1.0f, 1.0f, 1.0f), p_ui, TYPE_COLORSWATCH, IDC_PBR_OPACITY, PB_END,

	FRPBRMTL_OPACITY_MAP, _T("OpacityTexmap"), TYPE_TEXMAP, 0, 0,
	p_subtexno, FRPBRMTL_MAP_OPACITY, p_ui, TYPE_TEXMAPBUTTON, IDC_PBR_OPACITY_MAP, PB_END,

	FRPBRMTL_OPACITY_USEMAP, _T("OpacityUseMap"), TYPE_BOOL, P_ANIMATABLE, 0,
	p_default, TRUE, p_ui, TYPE_SINGLECHEKBOX, IDC_PBR_OPACITY_USEMAP, PB_END,

	FRPBRMTL_OPACITY_INVERT, _T("OpacityInvert"), TYPE_BOOL, P_ANIMATABLE, 0,
	p_default, FALSE, p_ui, TYPE_SINGLECHEKBOX, IDC_PBR_OPACITY_INVERT, PB_END,

	// Emissive Color
	FRPBRMTL_EMISSIVE_MUL, _T("EmissiveMul"), TYPE_FLOAT, P_ANIMATABLE, 0,
	p_default, EmissiveMulDefault, p_range, EmissiveMulMin, EmissiveMulMax, p_ui, TYPE_SPINNER, EDITTYPE_FLOAT,
	IDC_PBR_EMISSIVE_MUL, IDC_PBR_EMISSIVE_MUL_S, SPIN_AUTOSCALE, PB_END,

	FRPBRMTL_EMISSIVE, _T("Emissive"), TYPE_RGBA, P_ANIMATABLE, 0,
	p_default, Color(1.f, 1.f, 1.f), p_ui, TYPE_COLORSWATCH, IDC_PBR_EMISSIVE, PB_END,

	FRPBRMTL_EMISSIVE_MAP, _T("EmissiveTexmap"), TYPE_TEXMAP, 0, 0,
	p_subtexno, FRPBRMTL_MAP_EMISSIVE, p_ui, TYPE_TEXMAPBUTTON, IDC_PBR_EMISSIVE_MAP, PB_END,

	FRPBRMTL_EMISSIVE_USEMAP, _T("EmissiveUseMap"), TYPE_BOOL, P_ANIMATABLE, 0,
	p_default, TRUE, p_ui, TYPE_SINGLECHEKBOX, IDC_PBR_EMISSIVE_USEMAP, PB_END,

	PB_END
);

std::map<int, std::pair<ParamID, MCHAR*>> FRMTLCLASSNAME(PbrMtl)::TEXMAP_MAPPING =
{
	{ FRPBRMTL_MAP_DIFFUSE_COLOR, { FRPBRMTL_DIFFUSE_COLOR_MAP, _T("Diffuse Color") } },
	{ FRPBRMTL_MAP_ROUGHNESS,     { FRPBRMTL_ROUGHNESS_MAP,     _T("Roughness") } },
	{ FRPBRMTL_MAP_METALNESS,     { FRPBRMTL_METALNESS_MAP,     _T("Metalness") } },
	{ FRPBRMTL_MAP_CAVITY,        { FRPBRMTL_CAVITY_MAP,        _T("Cavity Map") } },
	{ FRPBRMTL_MAP_NORMAL,        { FRPBRMTL_NORMAL_MAP,        _T("Normal Map") } },
	{ FRPBRMTL_MAP_OPACITY,       { FRPBRMTL_OPACITY_MAP,       _T("Opacity") } },
	{ FRPBRMTL_MAP_EMISSIVE,      { FRPBRMTL_EMISSIVE_MAP,      _T("Emissive") } },
};

FRMTLCLASSNAME(PbrMtl)::~FRMTLCLASSNAME(PbrMtl)()
{
}

Color FRMTLCLASSNAME(PbrMtl)::GetDiffuse(int mtlNum, BOOL backFace)
{
	return GetFromPb<Color>(pblock, FRPBRMTL_DIFFUSE_COLOR);
}

frw::Shader FRMTLCLASSNAME(PbrMtl)::getVolumeShader(const TimeValue t, MaterialParser& mtlParser, INode* node)
{
	return frw::Shader();
}

frw::Shader FRMTLCLASSNAME(PbrMtl)::getShader(const TimeValue t, MaterialParser& mtlParser, INode* node)
{
	const frw::MaterialSystem& materialSystem = mtlParser.materialSystem;
	const frw::Scope& scope = mtlParser.GetScope();

	frw::Shader shader(scope.GetContext(), scope.GetContextEx(), RPRX_MATERIAL_UBER);

	bool useMap = false;
	Texmap* map = nullptr;
	Color color(0.0f, 0.0f, 0.0f);
	float mul = 0.0f;
	frw::Value value = frw::Value(color);
	bool toInvert = false;
	bool toUseMap = false;

	// DIFFUSE COLOR & CAVITY
	float diffuseMul = 0.0f;

	std::tie(useMap, map, color, diffuseMul) = GetParameters(FRPBRMTL_DIFFUSE_COLOR_USEMAP, FRPBRMTL_DIFFUSE_COLOR_MAP,
		FRPBRMTL_DIFFUSE_COLOR, FRPBRMTL_DIFFUSE_COLOR_MUL);
	toUseMap = (useMap && map != nullptr);
	
	value = toUseMap ? materialSystem.ValueMul(mtlParser.createMap(map, MAP_FLAG_NOFLAGS), color) : color;

	shader.xSetValue(RPRX_UBER_MATERIAL_DIFFUSE_COLOR, value);

	std::tie(useMap, map, color, mul) = GetParameters(FRPBRMTL_CAVITY_USEMAP, FRPBRMTL_CAVITY_MAP,
		FRPBRMTL_CAVITY, FRPBRMTL_CAVITY_MUL);
	toInvert = GetFromPb<bool>(pblock, FRPBRMTL_CAVITY_INVERT);
	toUseMap = (useMap && map != nullptr);

	mul = toInvert ? 1.0f - mul : mul;
	value = toUseMap ? mtlParser.createMap(map, MAP_FLAG_NOGAMMA) : color;
	value = materialSystem.ValueMul(value, mul);
	value = materialSystem.ValueSub(diffuseMul, value);

	shader.xSetValue(RPRX_UBER_MATERIAL_DIFFUSE_WEIGHT, value);

	// ROUGHNESS
	std::tie(useMap, map, color, mul) = GetParameters(FRPBRMTL_ROUGHNESS_USEMAP, FRPBRMTL_ROUGHNESS_MAP,
		FRPBRMTL_ROUGHNESS, FRPBRMTL_ROUGHNESS_MUL);
	toInvert = GetFromPb<bool>(pblock, FRPBRMTL_ROUGHNESS_INVERT);
	toUseMap = (useMap && map != nullptr);

	mul = toInvert ? 1.0f - mul : mul;
	value = toUseMap ? mtlParser.createMap(map, MAP_FLAG_NOGAMMA) : color;
	value = materialSystem.ValueMul(value, mul);
	shader.xSetValue(RPRX_UBER_MATERIAL_REFLECTION_ROUGHNESS, value);

	// METALNESS
	std::tie(useMap, map, color, mul) = GetParameters(FRPBRMTL_METALNESS_USEMAP, FRPBRMTL_METALNESS_MAP,
		FRPBRMTL_METALNESS, FRPBRMTL_METALNESS_MUL);
	toUseMap = (useMap && map != nullptr);

	value = toUseMap ? materialSystem.ValueMul(mtlParser.createMap(map, MAP_FLAG_NOGAMMA), color) : color;

	shader.xSetValue(RPRX_UBER_MATERIAL_REFLECTION_COLOR, value);
	shader.xSetValue(RPRX_UBER_MATERIAL_REFLECTION_WEIGHT, 1.0f);
	shader.xSetParameterU(RPRX_UBER_MATERIAL_REFLECTION_MODE, RPRX_UBER_MATERIAL_REFLECTION_MODE_METALNESS);
	shader.xSetValue(RPRX_UBER_MATERIAL_REFLECTION_METALNESS, mul);

	// MATERIAL NORMAL
	std::tie(useMap, map, color, mul) = GetParameters(FRPBRMTL_NORMAL_USEMAP, FRPBRMTL_NORMAL_MAP,
		FRPBRMTL_NORMAL, FRPBRMTL_NORMAL_MUL);
	toUseMap = (useMap && map != nullptr);

	if (toUseMap && mul > 0.0f)
	{
		value = materialSystem.ValueMul(mtlParser.createMap(map, MAP_FLAG_NOGAMMA | MAP_FLAG_NORMALMAP), mul);
		shader.xSetValue(RPRX_UBER_MATERIAL_NORMAL, value);
	}

	// OPACITY
	std::tie(useMap, map, color, mul) = GetParameters(FRPBRMTL_OPACITY_USEMAP, FRPBRMTL_OPACITY_MAP,
		FRPBRMTL_OPACITY, FRPBRMTL_OPACITY_MUL);
	toInvert = GetFromPb<bool>(pblock, FRPBRMTL_OPACITY_INVERT);
	toUseMap = (useMap && map != nullptr);

	value = toUseMap ? materialSystem.ValueMul(mtlParser.createMap(map, MAP_FLAG_NOGAMMA), mul) : mul;

	// If "invert" checkbox is selected treat the value as transparency (use as is).
	// in the other case convert opacity to transparency.
	if (!toInvert)
		value = mtlParser.materialSystem.ValueSub(frw::Value(1), value);

	shader.xSetValue(RPRX_UBER_MATERIAL_TRANSPARENCY, value);

	// EMISSIVE
	std::tie(useMap, map, color, mul) = GetParameters(FRPBRMTL_EMISSIVE_USEMAP, FRPBRMTL_EMISSIVE_MAP,
		FRPBRMTL_EMISSIVE, FRPBRMTL_EMISSIVE_MUL);
	toUseMap = (useMap && map != nullptr);

	value = toUseMap ? materialSystem.ValueMul(mtlParser.createMap(map, MAP_FLAG_WANTSHDR), color) : color;

	shader.xSetValue(RPRX_UBER_MATERIAL_EMISSION_COLOR, value);
	shader.xSetValue(RPRX_UBER_MATERIAL_EMISSION_WEIGHT, mul);

	if (mul > 0.0f)
		mtlParser.shaderData.mNumEmissive++;

	return shader;
}

std::tuple<bool, Texmap*, Color, float> FRMTLCLASSNAME(PbrMtl)::GetParameters(FRPbrMtl_ParamID useMapId,
	FRPbrMtl_ParamID mapId, FRPbrMtl_ParamID colorId, FRPbrMtl_ParamID mulId)
{
	bool useMap = GetFromPb<bool>(pblock, useMapId);
	Texmap* map = GetFromPb<Texmap*>(pblock, mapId);
	Color color = GetFromPb<Color>(pblock, colorId);
	float mul = GetFromPb<float>(pblock, mulId);

	return std::make_tuple(useMap, map, color, mul);
}

FIRERENDER_NAMESPACE_END;