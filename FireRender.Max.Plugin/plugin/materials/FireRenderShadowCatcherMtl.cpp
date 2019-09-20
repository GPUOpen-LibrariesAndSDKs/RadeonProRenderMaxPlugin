/*********************************************************************************************************************************
* Radeon ProRender for 3ds Max plugin
* Copyright (c) 2017 AMD
* All Rights Reserved
*
* Radeon ProRender dedicated custom material plugin
*********************************************************************************************************************************/
#include "FireRenderShadowCatcherMtl.h"
#include "Resource.h"
#include "parser\MaterialParser.h"

FIRERENDER_NAMESPACE_BEGIN;

namespace
{
	static Color NormalColor(0.0f, 0.0f, 0.0f);

	static constexpr float NormalColorMulMin = 0.0f;
	static constexpr float NormalColorMulMax = 1.0f;
	static constexpr float NormalColorMulDefault = 0.0f;

	static constexpr float DisplaceColorMulMin = 0.0f;
	static constexpr float DisplaceColorMulMax = 1.0f;
	static constexpr float DisplaceColorMulDefault = 0.0f;

	static constexpr float ShadowColorMulMin = 0.0f;
	static constexpr float ShadowColorMulMax = 1.0f;
	static constexpr float ShadowColorMulDefault = 0.0f;

	static constexpr float ShadowWeightMulMin = 0.0f;
	static constexpr float ShadowWeightMulMax = 1.0f;
	static constexpr float ShadowWeightMulDefault = 1.0f;

	static constexpr float ShadowAlphaMulMin = 0.0f;
	static constexpr float ShadowAlphaMulMax = 1.0f;
	static constexpr float ShadowAlphaMulDefault = 0.0f;

	static constexpr float BackgroundColorMulMin = 0.0f;
	static constexpr float BackgroundColorMulMax = 1.0f;
	static constexpr float BackgroundColorMulDefault = 0.0f;

	static constexpr float BackgroundWeightMulMin = 0.0f;
	static constexpr float BackgroundWeightMulMax = 1.0f;
	static constexpr float BackgroundWeightMulDefault = 1.0f;

	static constexpr float BackgroundAlphaMulMin = 0.0f;
	static constexpr float BackgroundAlphaMulMax = 1.0f;
	static constexpr float BackgroundAlphaMulDefault = 0.0f;
}

IMPLEMENT_FRMTLCLASSDESC(ShadowCatcherMtl)

FRMTLCLASSDESCNAME(ShadowCatcherMtl) FRMTLCLASSNAME(ShadowCatcherMtl)::ClassDescInstance;

// All parameters of the material plugin. See FIRE_MAX_PBDESC definition for notes on backwards compatibility
static ParamBlockDesc2 pbDesc
(
	0, _T("ShadowCatcherMtlPbdesc"), 0, &FRMTLCLASSNAME(ShadowCatcherMtl)::ClassDescInstance, P_AUTO_CONSTRUCT + P_AUTO_UI + P_VERSION, FIRERENDERMTLVER_LATEST, 0,
	//rollout
	IDD_FIRERENDER_SCMTL, IDS_FR_MTL_SC, 0, 0, NULL,

	// Normal
	FRSHADOWCATCHER_NORMAL_MUL, _T("NormalMul"), TYPE_FLOAT, P_ANIMATABLE, 0,
	p_default, NormalColorMulDefault, p_range, NormalColorMulMin, NormalColorMulMax, p_ui, TYPE_SPINNER, EDITTYPE_FLOAT,
	IDC_SC_NORMAL_MUL, IDC_SC_NORMAL_MUL_S, SPIN_AUTOSCALE, PB_END,

	FRSHADOWCATCHER_NORMAL, _T("Normal"), TYPE_RGBA, P_ANIMATABLE, 0,
	p_default, NormalColor, p_ui, TYPE_COLORSWATCH, IDC_SC_NORMAL, PB_END,

	FRSHADOWCATCHER_NORMAL_MAP, _T("NormalTexmap"), TYPE_TEXMAP, 0, 0,
	p_subtexno, FRSHADOWCATCHERMTL_MAP_NORMAL, p_ui, TYPE_TEXMAPBUTTON, IDC_SC_NORMAL_MAP, PB_END,

	FRSHADOWCATCHER_NORMAL_USEMAP, _T("NormalUseMap"), TYPE_BOOL, P_ANIMATABLE, 0,
	p_default, TRUE, p_ui, TYPE_SINGLECHEKBOX, IDC_SC_NORMAL_USEMAP, PB_END,

	// Displace
	FRSHADOWCATCHER_DISPLACE_MUL, _T("DisplacementMul"), TYPE_FLOAT, P_ANIMATABLE, 0,
	p_default, DisplaceColorMulDefault, p_range, DisplaceColorMulMin, DisplaceColorMulMax, p_ui, TYPE_SPINNER, EDITTYPE_FLOAT,
	IDC_SC_DISPLACE_MUL, IDC_SC_DISPLACE_MUL_S, SPIN_AUTOSCALE, PB_END,

	FRSHADOWCATCHER_DISPLACE, _T("Displacement"), TYPE_RGBA, P_ANIMATABLE, 0,
	p_default, NormalColor, p_ui, TYPE_COLORSWATCH, IDC_SC_DISPLACE, PB_END,

	FRSHADOWCATCHER_DISPLACE_MAP, _T("DisplacementTexmap"), TYPE_TEXMAP, 0, 0,
	p_subtexno, FRSHADOWCATCHERMTL_MAP_DISPLACEMENT, p_ui, TYPE_TEXMAPBUTTON, IDC_SC_DISPLACE_MAP, PB_END,

	FRSHADOWCATCHER_DISPLACE_USEMAP, _T("DisplacementUseMap"), TYPE_BOOL, P_ANIMATABLE, 0,
	p_default, TRUE, p_ui, TYPE_SINGLECHEKBOX, IDC_SC_DISPLACE_USEMAP, PB_END,

	// Shadow
	// - Color
	FRSHADOWCATCHER_SHADOW_COLOR_MUL, _T("ShadowColorMul"), TYPE_FLOAT, P_ANIMATABLE, 0,
	p_default, ShadowColorMulDefault, p_range, ShadowColorMulMin, ShadowColorMulMax, p_ui, TYPE_SPINNER, EDITTYPE_FLOAT,
	IDC_SC_SHADOW_COLOR_MUL, IDC_SC_SHADOW_COLOR_MUL_S, SPIN_AUTOSCALE, PB_END,

	FRSHADOWCATCHER_SHADOW_COLOR, _T("ShadowColor"), TYPE_RGBA, P_ANIMATABLE, 0,
	p_default, NormalColor, p_ui, TYPE_COLORSWATCH, IDC_SC_SHADOW_COLOR, PB_END,

	FRSHADOWCATCHER_SHADOW_COLOR_MAP, _T("ShadowColorTexmap"), TYPE_TEXMAP, 0, 0,
	p_subtexno, FRSHADOWCATCHERMTL_MAP_SHADOW_COLOR, p_ui, TYPE_TEXMAPBUTTON, IDC_SC_SHADOW_COLOR_MAP, PB_END,

	FRSHADOWCATCHER_SHADOW_COLOR_USEMAP, _T("ShadowColorUseMap"), TYPE_BOOL, P_ANIMATABLE, 0,
	p_default, TRUE, p_ui, TYPE_SINGLECHEKBOX, IDC_SC_SHADOW_COLOR_USEMAP, PB_END,

	// - Weight
	FRSHADOWCATCHER_SHADOW_WEIGHT_MUL, _T("ShadowWeightMul"), TYPE_FLOAT, P_ANIMATABLE, 0,
	p_default, ShadowWeightMulDefault, p_range, ShadowWeightMulMin, ShadowWeightMulMax, p_ui, TYPE_SPINNER, EDITTYPE_FLOAT,
	IDC_SC_SHADOW_WEIGHT_MUL, IDC_SC_SHADOW_WEIGHT_MUL_S, SPIN_AUTOSCALE, PB_END,

	// - Alpha
	FRSHADOWCATCHER_SHADOW_ALPHA_MUL, _T("ShadowAlphaMul"), TYPE_FLOAT, P_ANIMATABLE, 0,
	p_default, ShadowAlphaMulDefault, p_range, ShadowAlphaMulMin, ShadowAlphaMulMax, p_ui, TYPE_SPINNER, EDITTYPE_FLOAT,
	IDC_SC_SHADOW_ALPHA_MUL, IDC_SC_SHADOW_ALPHA_MUL_S, SPIN_AUTOSCALE, PB_END,

	// Background Is Environment
	FRSHADOWCATCHER_IS_BACKGROUND, _T("BackgroundIsEnvironment"), TYPE_BOOL, P_ANIMATABLE, 0,
	p_default, TRUE, p_ui, TYPE_SINGLECHEKBOX, IDC_SC_IS_BACKGROUND, PB_END,

	// Background Color
	FRSHADOWCATCHER_BACKGROUND_COLOR_MUL, _T("BackgroundColorMul"), TYPE_FLOAT, P_ANIMATABLE, 0,
	p_default, BackgroundColorMulDefault, p_range, BackgroundColorMulMin, BackgroundColorMulMax, p_ui, TYPE_SPINNER, EDITTYPE_FLOAT,
	IDC_SC_BACKGROUND_COLOR_MUL, IDC_SC_BACKGROUND_COLOR_MUL_S, SPIN_AUTOSCALE, PB_END,

	FRSHADOWCATCHER_BACKGROUND_COLOR, _T("BackgroundColor"), TYPE_RGBA, P_ANIMATABLE, 0,
	p_default, NormalColor, p_ui, TYPE_COLORSWATCH, IDC_SC_BACKGROUND_COLOR, PB_END,

	FRSHADOWCATCHER_BACKGROUND_COLOR_MAP, _T("BackgroundColorTexmap"), TYPE_TEXMAP, 0, 0,
	p_subtexno, FRSHADOWCATCHERMTL_MAP_PRIMITIVE_COLOR, p_ui, TYPE_TEXMAPBUTTON, IDC_SC_BACKGROUND_COLOR_MAP, PB_END,

	FRSHADOWCATCHER_BACKGROUND_COLOR_USEMAP, _T("BackgroundColorUseMap"), TYPE_BOOL, P_ANIMATABLE, 0,
	p_default, TRUE, p_ui, TYPE_SINGLECHEKBOX, IDC_SC_BACKGROUND_COLOR_USEMAP, PB_END,

	// Background Weight
	FRSHADOWCATCHER_BACKGROUND_WEIGHT_MUL, _T("BackgroundWeightMul"), TYPE_FLOAT, P_ANIMATABLE, 0,
	p_default, BackgroundWeightMulDefault, p_range, BackgroundWeightMulMin, BackgroundWeightMulMax, p_ui, TYPE_SPINNER, EDITTYPE_FLOAT,
	IDC_SC_BACKGROUND_WEIGHT_MUL, IDC_SC_BACKGROUND_WEIGHT_MUL_S, SPIN_AUTOSCALE, PB_END,

	// Background Trancparency
	FRSHADOWCATCHER_BACKGROUND_ALPHA_MUL, _T("BackgroundTrancparencyMul"), TYPE_FLOAT, P_ANIMATABLE, 0,
	p_default, BackgroundAlphaMulDefault, p_range, BackgroundAlphaMulMin, BackgroundAlphaMulMax, p_ui, TYPE_SPINNER, EDITTYPE_FLOAT,
	IDC_SC_BACKGROUND_ALPHA_MUL, IDC_SC_BACKGROUND_ALPHA_MUL_S, SPIN_AUTOSCALE, PB_END,

	PB_END
);

std::map<int, std::pair<ParamID, MCHAR*>> FRMTLCLASSNAME(ShadowCatcherMtl)::TEXMAP_MAPPING =
{
	{ FRSHADOWCATCHERMTL_MAP_PRIMITIVE_COLOR, { FRSHADOWCATCHER_BACKGROUND_COLOR_MAP, _T("Primitive Color") } },
	{ FRSHADOWCATCHERMTL_MAP_SHADOW_COLOR,    { FRSHADOWCATCHER_SHADOW_COLOR_MAP,     _T("Shadow Color") } },
	{ FRSHADOWCATCHERMTL_MAP_NORMAL,          { FRSHADOWCATCHER_NORMAL_MAP,           _T("Normal") } },
	{ FRSHADOWCATCHERMTL_MAP_DISPLACEMENT,    { FRSHADOWCATCHER_DISPLACE_MAP,         _T("Displacement") } },
};

FRMTLCLASSNAME(ShadowCatcherMtl)::~FRMTLCLASSNAME(ShadowCatcherMtl)() = default;

frw::Shader FRMTLCLASSNAME(ShadowCatcherMtl)::getVolumeShader(const TimeValue t, MaterialParser& mtlParser, INode* node)
{
	return frw::Shader();
}

frw::Shader FRMTLCLASSNAME(ShadowCatcherMtl)::getShader(const TimeValue t, MaterialParser& mtlParser, INode* node)
{
	const frw::MaterialSystem& materialSystem = mtlParser.materialSystem;
	const frw::Scope& scope = mtlParser.GetScope();

	frw::Shader shader(scope.GetContext(), scope.GetContextEx(), RPRX_MATERIAL_UBER);

	bool useMapFlag = false;
	Texmap* map = nullptr;
	Color color(0.0f, 0.0f, 0.0f);
	frw::Value value = frw::Value(color);
	bool canUseMap = false;
	std::tuple<bool, Texmap*, Color, float> parameters;

	// Normal Map
	float normalMul = 0.0f;
	parameters = GetParameters(FRSHADOWCATCHER_NORMAL_USEMAP, FRSHADOWCATCHER_NORMAL_MAP,
		FRSHADOWCATCHER_NORMAL, FRSHADOWCATCHER_NORMAL_MUL);

	value = SetupShaderOrdinary(mtlParser, parameters, MAP_FLAG_NOGAMMA | MAP_FLAG_NORMALMAP);

#if (RPR_API_COMPAT < 0x010031000)
	if (value.IsNode()) // a map must be connected
		shader.xSetValue(RPRX_UBER_MATERIAL_NORMAL, value);
#endif

	// Displacement Map
	float displacementMul = 0.0f;
	parameters = GetParameters(FRSHADOWCATCHER_DISPLACE_USEMAP, FRSHADOWCATCHER_DISPLACE_MAP,
		FRSHADOWCATCHER_DISPLACE, FRSHADOWCATCHER_DISPLACE_MUL);

	value = SetupShaderOrdinary(mtlParser, parameters, MAP_FLAG_NOGAMMA);

#if (RPR_API_COMPAT < 0x010034000)
	if (value.IsNode()) // a map must be connected
		shader.xSetValue(RPRX_UBER_MATERIAL_DISPLACEMENT, value);
#endif

	// Shadow
	// - Color
	float shadowColorMul = 0.0f;
	std::tie(useMapFlag, map, color, displacementMul) = GetParameters(FRSHADOWCATCHER_SHADOW_COLOR_USEMAP, FRSHADOWCATCHER_SHADOW_COLOR_MAP,
		FRSHADOWCATCHER_SHADOW_COLOR, FRSHADOWCATCHER_SHADOW_COLOR_MUL);
	canUseMap = (useMapFlag && map != nullptr); // map is ignored for now. maybe remove map completely?
	// - Alpha
	float shadowColorAlpha = GetFromPb<float>(pblock, FRSHADOWCATCHER_SHADOW_ALPHA_MUL);
	shader.SetShadowColor(color.r, color.g, color.b, shadowColorAlpha);

	// - Weight
	float shadowWeight = GetFromPb<float>(pblock, FRSHADOWCATCHER_SHADOW_WEIGHT_MUL);
	shader.SetShadowWeight(shadowWeight);

	// Background
	bool isBackroundEnvironmental = GetFromPb<bool>(pblock, FRSHADOWCATCHER_IS_BACKGROUND);
	shader.SetBackgroundIsEnvironment(isBackroundEnvironmental);

	if (isBackroundEnvironmental)
	{
		// - background color
		float backgroundColorMul = 0.0f;
		std::tie(useMapFlag, map, color, displacementMul) = GetParameters(FRSHADOWCATCHER_BACKGROUND_COLOR_USEMAP, FRSHADOWCATCHER_BACKGROUND_COLOR_MAP,
			FRSHADOWCATCHER_BACKGROUND_COLOR, FRSHADOWCATCHER_BACKGROUND_COLOR_MUL);
		canUseMap = (useMapFlag && map != nullptr);

		value = canUseMap ? materialSystem.ValueMul(mtlParser.createMap(map, MAP_FLAG_NOFLAGS), color) : color;

		shader.xSetValue(RPRX_UBER_MATERIAL_DIFFUSE_COLOR, value);

		// - background weight
		float backgroundColorWeight = GetFromPb<float>(pblock, FRSHADOWCATCHER_BACKGROUND_WEIGHT_MUL);
		shader.xSetValue(RPRX_UBER_MATERIAL_DIFFUSE_WEIGHT, value);

		// - background alpha
		float backgroundColorAlpha = GetFromPb<float>(pblock, FRSHADOWCATCHER_BACKGROUND_ALPHA_MUL);
		shader.xSetValue(RPRX_UBER_MATERIAL_TRANSPARENCY, value);
	}

	shader.SetShadowCatcher(true);
	

	return shader;
}

std::tuple<bool, Texmap*, Color, float> FRMTLCLASSNAME(ShadowCatcherMtl)::GetParameters(FRShadowCatcherMtl_ParamID useMapId,
	FRShadowCatcherMtl_ParamID mapId, FRShadowCatcherMtl_ParamID colorId, FRShadowCatcherMtl_ParamID mulId)
{
	bool useMap = GetFromPb<bool>(pblock, useMapId);
	Texmap* map = GetFromPb<Texmap*>(pblock, mapId);
	Color color = GetFromPb<Color>(pblock, colorId);
	float mul = GetFromPb<float>(pblock, mulId);

	return std::make_tuple(useMap, map, color, mul);
}

frw::Value FRMTLCLASSNAME(ShadowCatcherMtl)::SetupShaderOrdinary(MaterialParser& mtlParser,
	std::tuple<bool, Texmap*, Color, float> parameters, int mapFlags)
{
	bool useMap = std::get<0>(parameters);
	Texmap* map = std::get<1>(parameters);
	Color color = std::get<2>(parameters);
	float mul = std::get<3>(parameters);

	frw::Value value = frw::Value(color);

	if (useMap && map != nullptr)
	{
		value = mtlParser.createMap(map, mapFlags);
	}

	value = mtlParser.materialSystem.ValueMul(value, mul);

	return std::move(value);
}

FIRERENDER_NAMESPACE_END;
