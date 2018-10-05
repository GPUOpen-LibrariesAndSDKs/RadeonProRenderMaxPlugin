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
	static Color BaseColor(0.5f, 0.5f, 0.5f);

}

IMPLEMENT_FRMTLCLASSDESC(PbrMtl)

FRMTLCLASSDESCNAME(PbrMtl) FRMTLCLASSNAME(PbrMtl)::ClassDescInstance;

// All parameters of the material plugin. See FIRE_MAX_PBDESC definition for notes on backwards compatibility
static ParamBlockDesc2 pbDesc
(
	0, _T("PbrMtlPbdesc"), 0, &FRMTLCLASSNAME(PbrMtl)::ClassDescInstance, P_AUTO_CONSTRUCT + P_AUTO_UI + P_VERSION, FIRERENDERMTLVER_LATEST, 0,
    //rollout
	IDD_FIRERENDER_PBRMTL, IDS_FR_MTL_PBR, 0, 0, NULL,

	// BASE COLOR
	FRPBRMTL_BASE_COLOR_MUL, _T("BaseColorMul"), TYPE_FLOAT, P_OBSOLETE, 0,
	p_default, 0.5f, 
	p_range, 0.0f, 1.0f, 
	p_ui, TYPE_SPINNER, EDITTYPE_FLOAT,	IDC_PBR_BASE_COLOR_MUL, IDC_PBR_BASE_COLOR_MUL_S, 
	SPIN_AUTOSCALE, 
	PB_END,

	FRPBRMTL_BASE_COLOR, _T("BaseColor"), TYPE_RGBA, P_ANIMATABLE, 0,
	p_default, BaseColor, 
	p_ui, TYPE_COLORSWATCH, IDC_PBR_BASE_COLOR, 
	PB_END,

	FRPBRMTL_BASE_COLOR_MAP, _T("BaseColorTexmap"), TYPE_TEXMAP, 0, 0,
	p_subtexno, FRPBRMTL_MAP_BASE_COLOR, 
	p_ui, TYPE_TEXMAPBUTTON, IDC_PBR_BASE_COLOR_MAP, 
	PB_END,
	
	FRPBRMTL_BASE_COLOR_USEMAP, _T("BaseColorUseMap"), TYPE_BOOL, P_OBSOLETE, 0,
	p_default, TRUE, 
	p_ui, TYPE_SINGLECHEKBOX, IDC_PBR_BASE_COLOR_USEMAP, 
	PB_END,

	// ROUGHNESS
	FRPBRMTL_ROUGHNESS_MUL, _T("RoughnessMul"), TYPE_FLOAT, P_ANIMATABLE, 0,
	p_default, 0.25f, 
	p_range, 0.0f, 1.0f, 
	p_ui, TYPE_SPINNER, EDITTYPE_FLOAT,	IDC_PBR_ROUGHNESS_MUL, IDC_PBR_ROUGHNESS_MUL_S, 
	SPIN_AUTOSCALE, 
	PB_END,

	FRPBRMTL_ROUGHNESS, _T("Roughness"), TYPE_RGBA, P_OBSOLETE, 0,
	p_default, Color(1.0f, 1.0f, 1.0f), 
	p_ui, TYPE_COLORSWATCH, IDC_PBR_ROUGHNESS, 
	PB_END,

	FRPBRMTL_ROUGHNESS_MAP, _T("RoughnessTexmap"), TYPE_TEXMAP, 0, 0,
	p_subtexno, FRPBRMTL_MAP_ROUGHNESS, 
	p_ui, TYPE_TEXMAPBUTTON, IDC_PBR_ROUGHNESS_MAP, 
	PB_END,
	
	FRPBRMTL_ROUGHNESS_USEMAP, _T("RoughnessUseMap"), TYPE_BOOL, P_OBSOLETE, 0,
	p_default, TRUE, 
	p_ui, TYPE_SINGLECHEKBOX, IDC_PBR_ROUGHNESS_USEMAP, 
	PB_END,

	FRPBRMTL_ROUGHNESS_INVERT, _T("RoughnessInvert"), TYPE_BOOL, P_OBSOLETE, 0,
	p_default, FALSE, 
	p_ui, TYPE_SINGLECHEKBOX, IDC_PBR_ROUGHNESS_INVERT, 
	PB_END,

	// METALNESS
	FRPBRMTL_METALNESS_MUL, _T("MetalnessMul"), TYPE_FLOAT, P_ANIMATABLE, 0,
	p_default, 0.0f, 
	p_range, 0.0f, 1.0f, 
	p_ui, TYPE_SPINNER, EDITTYPE_FLOAT,	IDC_PBR_METALNESS_MUL, IDC_PBR_METALNESS_MUL_S, 
	SPIN_AUTOSCALE, 
	PB_END,

	FRPBRMTL_METALNESS_MAP, _T("MetalnessTexmap"), TYPE_TEXMAP, 0, 0,
	p_subtexno, FRPBRMTL_MAP_METALNESS, 
	p_ui, TYPE_TEXMAPBUTTON, IDC_PBR_METALNESS_MAP, 
	PB_END,

	FRPBRMTL_METALNESS_USEMAP, _T("MetalnessUseMap"), TYPE_BOOL, P_OBSOLETE, 0,
	p_default, TRUE, 
	p_ui, TYPE_SINGLECHEKBOX, IDC_PBR_METALNESS_USEMAP, 
	PB_END,

	// CAVITY
	FRPBRMTL_CAVITY_MUL, _T("CavityMul"), TYPE_FLOAT, P_OBSOLETE, 0,
	p_default, 0.0f, 
	p_range, 0.0f, 1.0f, 
	p_ui, TYPE_SPINNER, EDITTYPE_FLOAT,	IDC_PBR_CAVITY_MUL, IDC_PBR_CAVITY_MUL_S, 
	SPIN_AUTOSCALE, 
	PB_END,

	FRPBRMTL_CAVITY, _T("Cavity"), TYPE_RGBA, P_OBSOLETE, 0,
	p_default, Color(1.f, 1.f, 1.f), 
	p_ui, TYPE_COLORSWATCH, IDC_PBR_CAVITY, 
	PB_END,

	FRPBRMTL_CAVITY_MAP, _T("CavityTexmap"), TYPE_TEXMAP, P_OBSOLETE, 0,
	p_subtexno, FRPBRMTL_MAP_CAVITY, 
	p_ui, TYPE_TEXMAPBUTTON, IDC_PBR_CAVITY_MAP, 
	PB_END,

	FRPBRMTL_CAVITY_USEMAP, _T("CavityUseMap"), TYPE_BOOL, P_OBSOLETE, 0,
	p_default, TRUE, 
	p_ui, TYPE_SINGLECHEKBOX, IDC_PBR_CAVITY_USEMAP, 
	PB_END,

	FRPBRMTL_CAVITY_INVERT, _T("CavityInvert"), TYPE_BOOL, P_OBSOLETE, 0,
	p_default, FALSE, 
	p_ui, TYPE_SINGLECHEKBOX, IDC_PBR_CAVITY_INVERT, 
	PB_END,

	// NORMAL
	FRPBRMTL_NORMAL_MUL, _T("NormalMul"), TYPE_FLOAT, P_ANIMATABLE, 0,
	p_default, 0.0f, 
	p_range, 0.0f, 1.0f, 
	p_ui, TYPE_SPINNER, EDITTYPE_FLOAT,	IDC_PBR_NORMAL_MUL, IDC_PBR_NORMAL_MUL_S, 
	SPIN_AUTOSCALE, 
	PB_END,

	FRPBRMTL_NORMAL_MAP, _T("NormalTexmap"), TYPE_TEXMAP, 0, 0,
	p_subtexno, FRPBRMTL_MAP_NORMAL, 
	p_ui, TYPE_TEXMAPBUTTON, IDC_PBR_NORMAL_MAP,
	PB_END,

	FRPBRMTL_NORMAL_USEMAP, _T("NormalUseMap"), TYPE_BOOL, P_OBSOLETE, 0,
	p_default, TRUE, 
	p_ui, TYPE_SINGLECHEKBOX, IDC_PBR_NORMAL_USEMAP, 
	PB_END,

	// MATERIAL OPACITY
	FRPBRMTL_OPACITY_MUL, _T("OpacityMul"), TYPE_FLOAT, P_OBSOLETE, 0,
	p_default, 1.0f, 
	p_range, 0.0f, 1.0f, 
	p_ui, TYPE_SPINNER, EDITTYPE_FLOAT,	IDC_PBR_OPACITY_MUL, IDC_PBR_OPACITY_MUL_S, 
	SPIN_AUTOSCALE, 
	PB_END,

	FRPBRMTL_OPACITY_MAP, _T("OpacityTexmap"), TYPE_TEXMAP, P_OBSOLETE, 0,
	p_subtexno, FRPBRMTL_MAP_OPACITY, 
	p_ui, TYPE_TEXMAPBUTTON, IDC_PBR_OPACITY_MAP, 
	PB_END,

	FRPBRMTL_OPACITY_USEMAP, _T("OpacityUseMap"), TYPE_BOOL, P_OBSOLETE, 0,
	p_default, TRUE, 
	p_ui, TYPE_SINGLECHEKBOX, IDC_PBR_OPACITY_USEMAP, 
	PB_END,

	FRPBRMTL_OPACITY_INVERT, _T("OpacityInvert"), TYPE_BOOL, P_OBSOLETE, 0,
	p_default, FALSE, 
	p_ui, TYPE_SINGLECHEKBOX, IDC_PBR_OPACITY_INVERT, 
	PB_END,

	// EMISSIVE COLOR
	FRPBRMTL_EMISSIVE_COLOR_MUL, _T("EmissiveColorMul"), TYPE_FLOAT, P_OBSOLETE, 0,
	p_default, 0.0f, 
	p_range, 0.0f, 10.0f, 
	p_ui, TYPE_SPINNER, EDITTYPE_FLOAT,	IDC_PBR_EMISSIVE_COLOR_MUL, IDC_PBR_EMISSIVE_COLOR_MUL_S, 
	SPIN_AUTOSCALE, 
	PB_END,

	FRPBRMTL_EMISSIVE_COLOR, _T("EmissiveColor"), TYPE_RGBA, P_ANIMATABLE, 0,
	p_default, BaseColor, 
	p_ui, TYPE_COLORSWATCH, IDC_PBR_EMISSIVE_COLOR, 
	PB_END,

	FRPBRMTL_EMISSIVE_COLOR_MAP, _T("EmissiveColorTexmap"), TYPE_TEXMAP, 0, 0,
	p_subtexno, FRPBRMTL_MAP_EMISSIVE_COLOR, 
	p_ui, TYPE_TEXMAPBUTTON, IDC_PBR_EMISSIVE_COLOR_MAP, 
	PB_END,

	FRPBRMTL_EMISSIVE_COLOR_USEMAP, _T("EmissiveColorUseMap"), TYPE_BOOL, P_OBSOLETE, 0,
	p_default, TRUE, 
	p_ui, TYPE_SINGLECHEKBOX, IDC_PBR_EMISSIVE_COLOR_USEMAP, 
	PB_END,

	// EMISSIVE WEIGHT
	FRPBRMTL_EMISSIVE_WEIGHT_MUL, _T("EmissiveWeightMul"), TYPE_FLOAT, P_ANIMATABLE, 0,
	p_default, 0.0f, 
	p_range, 0.0f, FLT_MAX,
	p_ui, TYPE_SPINNER, EDITTYPE_FLOAT,	IDC_PBR_EMISSIVE_WEIGHT_MUL, IDC_PBR_EMISSIVE_WEIGHT_MUL_S, 
	SPIN_AUTOSCALE, 
	PB_END,

	FRPBRMTL_EMISSIVE_WEIGHT_MAP, _T("EmissiveWeightTexmap"), TYPE_TEXMAP, 0, 0,
	p_subtexno, FRPBRMTL_MAP_EMISSIVE_WEIGHT, 
	p_ui, TYPE_TEXMAPBUTTON, IDC_PBR_EMISSIVE_WEIGHT_MAP, 
	PB_END,

	FRPBRMTL_EMISSIVE_WEIGHT_USEMAP, _T("EmissiveWeightUseMap"), TYPE_BOOL, P_OBSOLETE, 0,
	p_default, TRUE, 
	p_ui, TYPE_SINGLECHEKBOX, IDC_PBR_EMISSIVE_WEIGHT_USEMAP, 
	PB_END,

	// GLASS (Refraction Weight)
	FRPBRMTL_GLASS_MUL, _T("GlassMul"), TYPE_FLOAT, P_ANIMATABLE, 0,
	p_default, 0.0f, 
	p_range, 0.0f, 1.0f, 
	p_ui, TYPE_SPINNER, EDITTYPE_FLOAT,	IDC_PBR_GLASS_MUL, IDC_PBR_GLASS_S, 
	SPIN_AUTOSCALE, 
	PB_END,

	FRPBRMTL_GLASS, _T("Glass"), TYPE_RGBA, P_OBSOLETE, 0,
	p_default, Color(1.f, 1.f, 1.f), 
	p_ui, TYPE_COLORSWATCH, IDC_PBR_GLASS, 
	PB_END,

	FRPBRMTL_GLASS_MAP, _T("GlassTexmap"), TYPE_TEXMAP, 0, 0,
	p_subtexno, FRPBRMTL_MAP_GLASS, 
	p_ui, TYPE_TEXMAPBUTTON, IDC_PBR_GLASS_MAP, 
	PB_END,

	FRPBRMTL_GLASS_USEMAP, _T("GlassUseMap"), TYPE_BOOL, P_OBSOLETE, 0,
	p_default, TRUE, 
	p_ui, TYPE_SINGLECHEKBOX, IDC_PBR_GLASS_USEMAP, 
	PB_END,

	// GLASS IOR
	FRPBRMTL_GLASS_IOR_MUL, _T("GlassIORMul"), TYPE_FLOAT, P_ANIMATABLE, 0,
	p_default, 1.5f, 
	p_range, 0.0f, 10.0f, 
	p_ui, TYPE_SPINNER, EDITTYPE_FLOAT,	IDC_PBR_GLASS_IOR_MUL, IDC_PBR_GLASS_IOR_S, 
	SPIN_AUTOSCALE, 
	PB_END,

	FRPBRMTL_GLASS_IOR, _T("GlassIOR"), TYPE_RGBA, P_OBSOLETE, 0,
	p_default, Color(1.f, 1.f, 1.f), 
	p_ui, TYPE_COLORSWATCH, IDC_PBR_GLASS_IOR, 
	PB_END,

	FRPBRMTL_GLASS_IOR_MAP, _T("GlassIORTexmap"), TYPE_TEXMAP, 0, 0,
	p_subtexno, FRPBRMTL_MAP_GLASS_IOR, 
	p_ui, TYPE_TEXMAPBUTTON, IDC_PBR_GLASS_IOR_MAP, 
	PB_END,

	FRPBRMTL_GLASS_IOR_USEMAP, _T("Glass IOR UseMap"), TYPE_BOOL, P_OBSOLETE, 0,
	p_default, TRUE, 
	p_ui, TYPE_SINGLECHEKBOX, IDC_PBR_GLASS_IOR_USEMAP, 
	PB_END,

	// SPECULAR
	FRPBRMTL_SPECULAR_MUL, _T("SpecularMul"), TYPE_FLOAT, P_ANIMATABLE, 0,
	p_default, 1.0f, 
	p_range, 0.0f, 1.0f, 
	p_ui, TYPE_SPINNER, EDITTYPE_FLOAT,	IDC_PBR_SPECULAR_MUL, IDC_PBR_SPECULAR_MUL_S, 
	SPIN_AUTOSCALE, 
	PB_END,

	FRPBRMTL_SPECULAR, _T("Specular"), TYPE_RGBA, P_OBSOLETE, 0,
	p_default, Color(1.f, 1.f, 1.f), 
	p_ui, TYPE_COLORSWATCH, IDC_PBR_SPECULAR, 
	PB_END,

	FRPBRMTL_SPECULAR_MAP, _T("SpecularTexmap"), TYPE_TEXMAP, 0, 0,
	p_subtexno, FRPBRMTL_MAP_SPECULAR, 
	p_ui, TYPE_TEXMAPBUTTON, IDC_PBR_SPECULAR_MAP, 
	PB_END,

	FRPBRMTL_SPECULAR_USEMAP, _T("SpecularUseMap"), TYPE_BOOL, P_OBSOLETE, 0,
	p_default, TRUE, 
	p_ui, TYPE_SINGLECHEKBOX, IDC_PBR_SPECULAR_USEMAP, 
	PB_END,

	// SSS
	FRPBRMTL_SSS_WEIGHT_MUL, _T("SSSWeightMul"), TYPE_FLOAT, P_ANIMATABLE, 0,
	p_default, 0.0f, 
	p_range, 0.0f, 1.0f, 
	p_ui, TYPE_SPINNER, EDITTYPE_FLOAT,	IDC_PBR_SSS_WEIGHT_MUL, IDC_PBR_SSS_WEIGHT_S, 
	SPIN_AUTOSCALE, 
	PB_END,

	FRPBRMTL_SSS_WEIGHT, _T("SSSWeight"), TYPE_RGBA, P_OBSOLETE, 0,
	p_default, Color(1.f, 1.f, 1.f), 
	p_ui, TYPE_COLORSWATCH, IDC_PBR_SSS_WEIGHT, 
	PB_END,

	FRPBRMTL_SSS_WEIGHT_MAP, _T("SSSWeightTexmap"), TYPE_TEXMAP, 0, 0,
	p_subtexno, FRPBRMTL_MAP_SSS_WEIGHT, 
	p_ui, TYPE_TEXMAPBUTTON, IDC_PBR_SSS_WEIGHT_MAP, 
	PB_END,

	FRPBRMTL_SSS_WEIGHT_USEMAP, _T("SSSUseWeightMap"), TYPE_BOOL, P_OBSOLETE, 0,
	p_default, TRUE, 
	p_ui, TYPE_SINGLECHEKBOX, IDC_PBR_SSS_WEIGHT_USEMAP, 
	PB_END,
		
	FRPBRMTL_SSS_COLOR_MUL, _T("SSSColorMul"), TYPE_FLOAT, P_OBSOLETE, 0,
	p_default, 0.0f, 
	p_range, 0.0f, 1.0f, 
	p_ui, TYPE_SPINNER, EDITTYPE_FLOAT,	IDC_PBR_SSS_COLOR_MUL, IDC_PBR_SSS_COLOR_S, 
	SPIN_AUTOSCALE, 
	PB_END,

	FRPBRMTL_SSS_COLOR, _T("SSSColor"), TYPE_RGBA, P_ANIMATABLE, 0,
	p_default, Color(0.436f, 0.227f, 0.131f),
	p_ui, TYPE_COLORSWATCH, IDC_PBR_SSS_COLOR, 
	PB_END,

	FRPBRMTL_SSS_COLOR_MAP, _T("SSSColorTexmap"), TYPE_TEXMAP, 0, 0,
	p_subtexno, FRPBRMTL_MAP_SSS_COLOUR, 
	p_ui, TYPE_TEXMAPBUTTON, IDC_PBR_SSS_COLOR_MAP, 
	PB_END,

	FRPBRMTL_SSS_COLOR_USEMAP, _T("SSSColorUseMap"), TYPE_BOOL, P_OBSOLETE, 0,
	p_default, TRUE, 
	p_ui, TYPE_SINGLECHEKBOX, IDC_PBR_SSS_COLOR_USEMAP, 
	PB_END,

	FRPBRMTL_SSS_SCATTER_DIST, _T("SSSRadius"), TYPE_POINT3, P_ANIMATABLE, 0,
	p_default, Point3(3.67f, 1.37f, 0.68f),
	p_range, 0.0f, 5.0f,
	p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_PBR_SSS_RADIUS_X, IDC_PBR_SSS_RADIUS_X_S,
	IDC_PBR_SSS_RADIUS_Y, IDC_PBR_SSS_RADIUS_Y_S, IDC_PBR_SSS_RADIUS_Z, IDC_PBR_SSS_RADIUS_Z_S,
	SPIN_AUTOSCALE,
	PB_END,
		
	PB_END
);

std::map<int, std::pair<ParamID, MCHAR*>> FRMTLCLASSNAME(PbrMtl)::TEXMAP_MAPPING =
{
	{ FRPBRMTL_MAP_BASE_COLOR,        { FRPBRMTL_BASE_COLOR_MAP,         _T("Base Color") } },
	{ FRPBRMTL_MAP_ROUGHNESS,         { FRPBRMTL_ROUGHNESS_MAP,          _T("Roughness") } },
	{ FRPBRMTL_MAP_METALNESS,         { FRPBRMTL_METALNESS_MAP,          _T("Metalness") } },
	{ FRPBRMTL_MAP_CAVITY,            { FRPBRMTL_CAVITY_MAP,             _T("Cavity Map") } },
	{ FRPBRMTL_MAP_NORMAL,            { FRPBRMTL_NORMAL_MAP,             _T("Normal Map") } },
	{ FRPBRMTL_MAP_OPACITY,           { FRPBRMTL_OPACITY_MAP,            _T("Opacity") } },
	{ FRPBRMTL_MAP_EMISSIVE_COLOR,    { FRPBRMTL_EMISSIVE_COLOR_MAP,     _T("Emissive Color") } },
	{ FRPBRMTL_MAP_EMISSIVE_WEIGHT,   { FRPBRMTL_EMISSIVE_WEIGHT_MAP,    _T("Emissive Weight") } },
	{ FRPBRMTL_MAP_GLASS,			  { FRPBRMTL_GLASS_MAP,				 _T("Glass") } },
	{ FRPBRMTL_MAP_GLASS_IOR,		  { FRPBRMTL_GLASS_IOR_MAP,			 _T("Glass IOR") } },
	{ FRPBRMTL_MAP_SPECULAR,		  { FRPBRMTL_SPECULAR_MAP,			 _T("Specular")  } },
	{ FRPBRMTL_MAP_SSS_WEIGHT,		  { FRPBRMTL_SSS_WEIGHT_MAP,		 _T("SSS Weight") } },
	{ FRPBRMTL_MAP_SSS_COLOUR,		  { FRPBRMTL_SSS_COLOR_MAP,			 _T("SSS Color") } }
};

FRMTLCLASSNAME(PbrMtl)::~FRMTLCLASSNAME(PbrMtl)()
{
}

Color FRMTLCLASSNAME(PbrMtl)::GetDiffuse(int mtlNum, BOOL backFace)
{
	return GetFromPb<Color>(pblock, FRPBRMTL_BASE_COLOR);
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

	Texmap* map = nullptr;
	Color color(0.0f, 0.0f, 0.0f);
	float mul = 0.0f;
	frw::Value value = frw::Value(color);
	frw::Value diffuseColorValue = frw::Value(color);
	bool toInvert = false;
	bool toUseMap = false;

	// DIFFUSE COLOR & CAVITY
	std::tie(map, color) = GetParameters(FRPBRMTL_BASE_COLOR_MAP,
		FRPBRMTL_BASE_COLOR);
	toUseMap = map != nullptr;
	
	diffuseColorValue = value = toUseMap ? mtlParser.createMap(map, MAP_FLAG_NOFLAGS) : color;

	shader.xSetValue(RPRX_UBER_MATERIAL_DIFFUSE_WEIGHT, 1.0f);
	shader.xSetValue(RPRX_UBER_MATERIAL_DIFFUSE_COLOR, value);

	// ROUGHNESS
	std::tie(map, mul) = GetParametersNoColor(FRPBRMTL_ROUGHNESS_MAP,
		FRPBRMTL_ROUGHNESS_MUL);
	toUseMap = map != nullptr;

	value = toUseMap ? materialSystem.ValueMul(mtlParser.createMap(map, MAP_FLAG_NOGAMMA), mul) : mul;
	shader.xSetValue(RPRX_UBER_MATERIAL_DIFFUSE_ROUGHNESS, value);
	shader.xSetValue(RPRX_UBER_MATERIAL_REFLECTION_ROUGHNESS, value);
	shader.xSetValue(RPRX_UBER_MATERIAL_REFRACTION_ROUGHNESS, value);

	// METALNESS
	std::tie(map, mul) = GetParametersNoColor(FRPBRMTL_METALNESS_MAP,
		FRPBRMTL_METALNESS_MUL);
	toUseMap = map != nullptr;

	value = toUseMap ? materialSystem.ValueMul(mtlParser.createMap(map, MAP_FLAG_NOGAMMA), mul) : mul;

	shader.xSetValue(RPRX_UBER_MATERIAL_REFLECTION_WEIGHT, value);
	
	if (mul > 0.0f)
	{
		shader.xSetParameterU(RPRX_UBER_MATERIAL_REFLECTION_MODE, RPRX_UBER_MATERIAL_REFLECTION_MODE_METALNESS);
		shader.xSetValue(RPRX_UBER_MATERIAL_REFLECTION_METALNESS, value);
	}
	else
	{
		shader.xSetParameterU(RPRX_UBER_MATERIAL_REFLECTION_MODE, RPRX_UBER_MATERIAL_REFLECTION_MODE_PBR);
		shader.xSetValue(RPRX_UBER_MATERIAL_REFLECTION_IOR, value);
	}

	// MATERIAL NORMAL
	std::tie(map, mul) = GetParametersNoColor(FRPBRMTL_NORMAL_MAP,
		FRPBRMTL_NORMAL_MUL);
	toUseMap = (map != nullptr);

#if (RPR_API_VERSION < 0x010031000)
	if (toUseMap && mul > 0.0f)
	{
		value = materialSystem.ValueMul(mtlParser.createMap(map, MAP_FLAG_NOGAMMA | MAP_FLAG_NORMALMAP), mul);
		shader.xSetValue(RPRX_UBER_MATERIAL_NORMAL, value);
	}
#else
	if (toUseMap)
	{
		value = materialSystem.ValueMul(mtlParser.createMap(map, MAP_FLAG_NOGAMMA | MAP_FLAG_NORMALMAP), 1.0f);
		shader.xSetValue(RPRX_UBER_MATERIAL_DIFFUSE_NORMAL, value);
		shader.xSetValue(RPRX_UBER_MATERIAL_REFLECTION_NORMAL, value);
	}
#endif

	// EMISSIVE
	std::tie(map, color) = GetParameters(FRPBRMTL_EMISSIVE_COLOR_MAP,
		FRPBRMTL_EMISSIVE_COLOR);
	toUseMap = map != nullptr;

	value = toUseMap ? mtlParser.createMap(map, MAP_FLAG_NOFLAGS) : color;

	shader.xSetValue(RPRX_UBER_MATERIAL_EMISSION_COLOR, value);


	std::tie(map, mul) = GetParametersNoColor(FRPBRMTL_EMISSIVE_WEIGHT_MAP,
		FRPBRMTL_EMISSIVE_WEIGHT_MUL);
	toUseMap = map != nullptr;

	value = toUseMap ? materialSystem.ValueMul(mtlParser.createMap(map, MAP_FLAG_NOGAMMA), mul) : mul;

	shader.xSetValue(RPRX_UBER_MATERIAL_EMISSION_WEIGHT, value);

	// GLASS
	std::tie(map, mul) = GetParametersNoColor(FRPBRMTL_GLASS_MAP,
		FRPBRMTL_GLASS_MUL);
	toUseMap = map != nullptr;

	value = toUseMap ? materialSystem.ValueMul(mtlParser.createMap(map, MAP_FLAG_NOGAMMA), mul) : mul;

	shader.xSetValue(RPRX_UBER_MATERIAL_REFRACTION_WEIGHT, value);
	shader.xSetValue(RPRX_UBER_MATERIAL_REFRACTION_COLOR, frw::Value(1.0f, 1.0f, 1.0f));

	std::tie(map, mul) = GetParametersNoColor(FRPBRMTL_GLASS_IOR_MAP,
		FRPBRMTL_GLASS_IOR_MUL);
	toUseMap = map != nullptr;

	value = toUseMap ? materialSystem.ValueMul(mtlParser.createMap(map, MAP_FLAG_NOGAMMA), mul) : mul;

	shader.xSetValue(RPRX_UBER_MATERIAL_REFRACTION_IOR, value);


	// SPECULAR
	std::tie(map, mul) = GetParametersNoColor(FRPBRMTL_SPECULAR_MAP,
		FRPBRMTL_SPECULAR_MUL);
	toUseMap = map != nullptr;

	value = toUseMap ? materialSystem.ValueMul(mtlParser.createMap(map, MAP_FLAG_NOGAMMA), mul) : mul;

	shader.xSetValue(RPRX_UBER_MATERIAL_REFLECTION_WEIGHT, value);
	shader.xSetValue(RPRX_UBER_MATERIAL_REFLECTION_COLOR, diffuseColorValue);

	// SSS
	std::tie(map, mul) = GetParametersNoColor(FRPBRMTL_SSS_WEIGHT_MAP,
		FRPBRMTL_SSS_WEIGHT_MUL);
	toUseMap = map != nullptr;

	value = toUseMap ? materialSystem.ValueMul(mtlParser.createMap(map, MAP_FLAG_NOGAMMA), mul) : mul;

	shader.xSetValue(RPRX_UBER_MATERIAL_SSS_WEIGHT, value);

	std::tie(map, color) = GetParameters(FRPBRMTL_SSS_COLOR_MAP,
		FRPBRMTL_SSS_COLOR);
	toUseMap = map != nullptr;

	value = toUseMap ? mtlParser.createMap(map, MAP_FLAG_NOFLAGS) : color;

	shader.xSetValue(RPRX_UBER_MATERIAL_SSS_SCATTER_COLOR, value);

	Point3 subsurfaceRadius = GetFromPb<Point3>(pblock, FRPBRMTL_SSS_SCATTER_DIST);
	value = frw::Value(subsurfaceRadius);
	shader.xSetValue(RPRX_UBER_MATERIAL_SSS_SCATTER_DISTANCE, value);

	return shader;
}

std::tuple<Texmap*, Color> FRMTLCLASSNAME(PbrMtl)::GetParameters(FRPbrMtl_ParamID mapId,
	FRPbrMtl_ParamID colorId)
{
	Texmap* map = GetFromPb<Texmap*>(pblock, mapId);
	Color color = GetFromPb<Color>(pblock, colorId);

	return std::make_tuple(map, color);
}

std::tuple<Texmap*, float> FRMTLCLASSNAME(PbrMtl)::GetParametersNoColor(FRPbrMtl_ParamID mapId,
	FRPbrMtl_ParamID mulId)
{
	Texmap* map = GetFromPb<Texmap*>(pblock, mapId);
	float mul = GetFromPb<float>(pblock, mulId);

	return std::make_tuple(map, mul);
}

FIRERENDER_NAMESPACE_END;
