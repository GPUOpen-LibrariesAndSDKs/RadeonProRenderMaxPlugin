/*********************************************************************************************************************************
* Radeon ProRender for 3ds Max plugin
* Copyright (c) 2017 AMD
* All Rights Reserved
*
* Radeon ProRender dedicated custom material plugin
*********************************************************************************************************************************/
#include "FireRenderStandardMtl.h"
#include "Resource.h"
#include "parser\MaterialParser.h"
#include "maxscript\mxsplugin\mxsPlugin.h"

FIRERENDER_NAMESPACE_BEGIN;

IMPLEMENT_FRMTLCLASSDESC(StandardMtl)

FRMTLCLASSDESCNAME(StandardMtl) FRMTLCLASSNAME(StandardMtl)::ClassDescInstance;

// All parameters of the material plugin. See FIRE_MAX_PBDESC definition for notes on backwards compatibility
static ParamBlockDesc2 pbDesc(
	0, _T("StandardMtlPbdesc"), 0, &FRMTLCLASSNAME(StandardMtl)::ClassDescInstance, P_AUTO_CONSTRUCT + P_AUTO_UI + P_VERSION, FIRERENDERMTLVER_LATEST, 0,
    //rollout
	IDD_FIRERENDER_STANDARDMTL, IDS_FR_MTL_STANDARD, 0, 0, NULL,

	// DIFFUSE

	FRStandardMtl_DIFFUSE_COLOR, _T("diffuseColor"), TYPE_RGBA, P_ANIMATABLE, 0,
	p_default, Color(0.5f, 0.5f, 0.5f), p_ui, TYPE_COLORSWATCH, IDC_SM_DIFFUSE_COLOR, PB_END,

	FRStandardMtl_DIFFUSE_ROUGHNESS, _T("diffuseRougness"), TYPE_FLOAT, P_ANIMATABLE, 0,
	p_default, 1.f, p_range, 0.f, 1.f, p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_SM_DIFFUSE_ROUGHNESS, IDC_SM_DIFFUSE_ROUGHNESS_S, SPIN_AUTOSCALE, PB_END,

	FRStandardMtl_DIFFUSE_COLOR_TEXMAP, _T("diffuseTexmap"), TYPE_TEXMAP, 0, 0,
	p_subtexno, FRStandardMtl_TEXMAP_DIFFUSE_COLOR, p_ui, TYPE_TEXMAPBUTTON, IDC_SM_DIFFUSE_TEXMAP, PB_END,

	FRStandardMtl_DIFFUSE_ROUGHNESS_TEXMAP, _T("diffuseRougnessTexmap"), TYPE_TEXMAP, 0, 0,
	p_subtexno, FRStandardMtl_TEXMAP_DIFFUSE_ROUGHNESS, p_ui, TYPE_TEXMAPBUTTON, IDC_SM_DIFFUSE_ROUGHNESS_TEXMAP, PB_END,

	FRStandardMtl_DIFFUSE_WEIGHT, _T("diffuseWeight"), TYPE_FLOAT, P_ANIMATABLE, 0,
	p_default, 1.f, p_range, 0.f, 1.f, p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_SM_DIFFUSE_WEIGHT, IDC_SM_DIFFUSE_WEIGHT_S, SPIN_AUTOSCALE, PB_END,

	FRStandardMtl_DIFFUSE_WEIGHT_TEXMAP, _T("diffuseWeightTexmap"), TYPE_TEXMAP, 0, 0,
	p_subtexno, FRStandardMtl_TEXMAP_DIFFUSE_WEIGHT, p_ui, TYPE_TEXMAPBUTTON, IDC_SM_DIFFUSE_WEIGHT_TEXMAP, PB_END,

	FRStandardMtl_DIFFUSE_NORMALMAP, _T("diffuseNormalTexmap"), TYPE_TEXMAP, 0, 0,
	p_subtexno, FRStandardMtl_TEXMAP_DIFFUSE_NORMAL, p_ui, TYPE_TEXMAPBUTTON, IDC_DIFFUSE_NORMAL_TEXMAP, PB_END,

	// REFLECTION
	
	FRStandardMtl_REFLECT_COLOR, _T("reflectionColor"), TYPE_RGBA, P_ANIMATABLE, 0,
	p_default, Color(0.0f, 0.0f, 0.0f), p_ui, TYPE_COLORSWATCH, IDC_SM_REFL_COLOR, PB_END,
	
	FRStandardMtl_REFLECT_IOR, _T("reflectionIOR"), TYPE_FLOAT, P_ANIMATABLE, 0,
	p_default, 1.f, p_range, 1.f, 999.f, p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_SM_REFL_IOR, IDC_SM_REFL_IOR_S, SPIN_AUTOSCALE, PB_END,
	
	FRStandardMtl_REFLECT_ROUGHNESS, _T("reflectionRougness"), TYPE_FLOAT, P_ANIMATABLE, 0,
	p_default, 1.f, p_range, 0.f, 1.f, p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_SM_REFL_ROUGHNESS, IDC_SM_REFL_ROUGHNESS_S, SPIN_AUTOSCALE, PB_END,

	FRStandardMtl_REFLECT_COLOR_TEXMAP, _T("reflectionColorTexmap"), TYPE_TEXMAP, 0, 0,
	p_subtexno, FRStandardMtl_TEXMAP_REFLECT_COLOR, p_ui, TYPE_TEXMAPBUTTON, IDC_SM_REFL_COLOR_TEXMAP, PB_END,

	FRStandardMtl_REFLECT_ROUGHNESS_TEXMAP, _T("reflectionRoughnessTexmap"), TYPE_TEXMAP, 0, 0,
	p_subtexno, FRStandardMtl_TEXMAP_REFLECT_ROUGHNESS, p_ui, TYPE_TEXMAPBUTTON, IDC_SM_REFL_ROUGHNESS_TEXMAP, PB_END,

	FRStandardMtl_REFLECT_WEIGHT, _T("reflectionWeight"), TYPE_FLOAT, P_ANIMATABLE, 0,
	p_default, 1.f, p_range, 0.f, 1.f, p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_SM_REFL_WEIGHT, IDC_SM_REFL_WEIGHT_S, SPIN_AUTOSCALE, PB_END,

	FRStandardMtl_REFLECT_WEIGHT_TEXMAP, _T("reflectionWeightTexmap"), TYPE_TEXMAP, 0, 0,
	p_subtexno, FRStandardMtl_TEXMAP_REFLECT_WEIGHT, p_ui, TYPE_TEXMAPBUTTON, IDC_SM_REFL_WEIGHT_TEXMAP, PB_END,

	FRStandardMtl_REFLECT_NORMALMAP, _T("reflectionNormalTexmap"), TYPE_TEXMAP, 0, 0,
	p_subtexno, FRStandardMtl_TEXMAP_REFLECT_NORMAL, p_ui, TYPE_TEXMAPBUTTON, IDC_REFLECT_NORMAL_TEXMAP, PB_END,

	// REFRACTION
	
	FRStandardMtl_REFRACT_COLOR, _T("refractionColor"), TYPE_RGBA, P_ANIMATABLE, 0,
	p_default, Color(0.0f, 0.0f, 0.0f), p_ui, TYPE_COLORSWATCH, IDC_SM_REFR_COLOR, PB_END,
	
	FRStandardMtl_REFRACT_IOR, _T("refractionIOR"), TYPE_FLOAT, P_ANIMATABLE, 0,
	p_default, 1.f, p_range, 1.f, 999.f, p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_SM_REFR_IOR, IDC_SM_REFR_IOR_S, SPIN_AUTOSCALE, PB_END,
	
	FRStandardMtl_REFRACT_COLOR_TEXMAP, _T("refractionColorTexmap"), TYPE_TEXMAP, 0, 0,
	p_subtexno, FRStandardMtl_TEXMAP_REFRACT_COLOR, p_ui, TYPE_TEXMAPBUTTON, IDC_SM_REFR_COLOR_TEXMAP, PB_END,

	FRStandardMtl_REFRACT_WEIGHT, _T("refractionWeight"), TYPE_FLOAT, P_ANIMATABLE, 0,
	p_default, 1.f, p_range, 0.f, 1.f, p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_SM_REFR_WEIGHT, IDC_SM_REFR_WEIGHT_S, SPIN_AUTOSCALE, PB_END,

	FRStandardMtl_REFRACT_WEIGHT_TEXMAP, _T("refractionWeightTexmap"), TYPE_TEXMAP, 0, 0,
	p_subtexno, FRStandardMtl_TEXMAP_REFRACT_WEIGHT, p_ui, TYPE_TEXMAPBUTTON, IDC_SM_REFR_WEIGHT_TEXMAP, PB_END,

	FRStandardMtl_REFRACT_NORMALMAP, _T("refractionNormalTexmap"), TYPE_TEXMAP, 0, 0,
	p_subtexno, FRStandardMtl_TEXMAP_REFRACT_NORMAL, p_ui, TYPE_TEXMAPBUTTON, IDC_REFRACT_NORMAL_TEXMAP, PB_END,

	// EMISSION

	FRStandardMtl_EMISSION_COLOR, _T("emissionColor"), TYPE_RGBA, P_ANIMATABLE, 0,
	p_default, Color(0.0f, 0.0f, 0.0f), p_ui, TYPE_COLORSWATCH, IDC_SM_EMISSION_COLOR, PB_END,
		
	FRStandardMtl_EMISSION_INTENSITY, _T("emissionIntensity"), TYPE_FLOAT, P_ANIMATABLE, 0,
	p_default, 0.f, p_range, 0.f, 999999.f, p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_SM_EMISSION_STRENGHT, IDC_SM_EMISSION_STRENGHT_S, SPIN_AUTOSCALE, PB_END,

	FRStandardMtl_EMISSION_WEIGHT, _T("emissionWeight"), TYPE_FLOAT, P_ANIMATABLE, 0,
	p_default, 0.f, p_range, 0.f, 1.f, p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_SM_EMISSION_WEIGHT, IDC_SM_EMISSION_WEIGHT_S, SPIN_AUTOSCALE, PB_END,

	FRStandardMtl_EMISSION_WEIGHT_TEXMAP, _T("emissionWeightTexmap"), TYPE_TEXMAP, 0, 0,
	p_subtexno, FRStandardMtl_TEXMAP_EMISSION_WEIGHT, p_ui, TYPE_TEXMAPBUTTON, IDC_SM_EMISSION_WEIGHT_TEXMAP, PB_END,

	// GENERAL

	FRStandardMtl_CASTS_SHADOWS, _T("castShadows"), TYPE_BOOL, P_ANIMATABLE, 0,
	p_default, TRUE, p_ui, TYPE_SINGLECHEKBOX, IDC_SM_CASTSHADOWS,
	p_tooltip, IDS_CASTSHADOWSTTP, PB_END,
		
    PB_END
    );

std::map<int, std::pair<ParamID, MCHAR*>> FRMTLCLASSNAME(StandardMtl)::TEXMAP_MAPPING = {
	{ FRStandardMtl_TEXMAP_DIFFUSE_COLOR, { FRStandardMtl_DIFFUSE_COLOR_TEXMAP, _T("Diffuse Color Map") } },
	{ FRStandardMtl_TEXMAP_DIFFUSE_ROUGHNESS, { FRStandardMtl_DIFFUSE_ROUGHNESS_TEXMAP, _T("Diffuse Roughness Map") } },
	{ FRStandardMtl_TEXMAP_DIFFUSE_WEIGHT, { FRStandardMtl_DIFFUSE_WEIGHT_TEXMAP, _T("Diffuse Weight Map") } },
	{ FRStandardMtl_TEXMAP_DIFFUSE_NORMAL, { FRStandardMtl_DIFFUSE_NORMALMAP, _T("Diffuse Normal map") } },
	{ FRStandardMtl_TEXMAP_REFLECT_COLOR, { FRStandardMtl_REFLECT_COLOR_TEXMAP, _T("Reflection Color Map") } },
	{ FRStandardMtl_TEXMAP_REFLECT_ROUGHNESS, { FRStandardMtl_REFLECT_ROUGHNESS_TEXMAP, _T("Reflection Roughness Map") } },
	{ FRStandardMtl_TEXMAP_REFLECT_WEIGHT, { FRStandardMtl_REFLECT_WEIGHT_TEXMAP, _T("Reflection Weight Map") } },
	{ FRStandardMtl_TEXMAP_REFLECT_NORMAL, { FRStandardMtl_REFLECT_NORMALMAP, _T("Reflection Normal map") } },
	{ FRStandardMtl_TEXMAP_REFRACT_COLOR, { FRStandardMtl_REFRACT_COLOR_TEXMAP, _T("Refraction Color Map") } },
	{ FRStandardMtl_TEXMAP_REFRACT_WEIGHT, { FRStandardMtl_REFRACT_WEIGHT_TEXMAP, _T("Refraction Weight Map") } },
	{ FRStandardMtl_TEXMAP_REFRACT_NORMAL, { FRStandardMtl_REFRACT_NORMALMAP, _T("Refraction Normal map") } },
	{ FRStandardMtl_TEXMAP_EMISSION_WEIGHT, { FRStandardMtl_EMISSION_WEIGHT_TEXMAP, _T("Emission Weight Map") } }
};

FRMTLCLASSNAME(StandardMtl)::~FRMTLCLASSNAME(StandardMtl)()
{
}


frw::Shader FRMTLCLASSNAME(StandardMtl)::getShader(const TimeValue t, MaterialParser& mtlParser, INode* node)
{
	auto ms = mtlParser.materialSystem;

	currentTime = t;

	frw::Shader material(ms, frw::ShaderTypeStandard);

	IParamBlock2* pb = pblock; // for use with macros

	Texmap* diffuseNormalTexmap = FireRender::GetFromPb<Texmap*>(pb, FRStandardMtl_DIFFUSE_NORMALMAP, t);
	Texmap* reflectNormalTexmap = FireRender::GetFromPb<Texmap*>(pb, FRStandardMtl_REFLECT_NORMALMAP, t);
	Texmap* refractNormalTexmap = FireRender::GetFromPb<Texmap*>(pb, FRStandardMtl_REFRACT_NORMALMAP, t);

	GETSHADERCOLOR(mtlParser, diffuse, FRStandardMtl_DIFFUSE_COLOR, FRStandardMtl_DIFFUSE_COLOR_TEXMAP, t)
	GETSHADERFLOAT(mtlParser, diffuseRoughness, FRStandardMtl_DIFFUSE_ROUGHNESS, FRStandardMtl_DIFFUSE_ROUGHNESS_TEXMAP, t)
	GETSHADERFLOAT(mtlParser, diffuseWeight, FRStandardMtl_DIFFUSE_WEIGHT, FRStandardMtl_DIFFUSE_WEIGHT_TEXMAP, t)
		
	GETSHADERCOLOR(mtlParser, reflect, FRStandardMtl_REFLECT_COLOR, FRStandardMtl_REFLECT_COLOR_TEXMAP, t)
	GETSHADERFLOAT(mtlParser, reflectRoughness, FRStandardMtl_REFLECT_ROUGHNESS, FRStandardMtl_REFLECT_ROUGHNESS_TEXMAP, t)
	GETSHADERFLOAT_NOMAP(reflectIOR, FRStandardMtl_REFLECT_IOR, t)
	GETSHADERFLOAT(mtlParser, reflectWeight, FRStandardMtl_REFLECT_WEIGHT, FRStandardMtl_REFLECT_WEIGHT_TEXMAP, t)
	 
	GETSHADERCOLOR(mtlParser, refract, FRStandardMtl_REFRACT_COLOR, FRStandardMtl_REFRACT_COLOR_TEXMAP, t)
	GETSHADERFLOAT_NOMAP(refractIOR, FRStandardMtl_REFRACT_IOR, t)
	GETSHADERFLOAT(mtlParser, refractWeight, FRStandardMtl_REFRACT_WEIGHT, FRStandardMtl_REFRACT_WEIGHT_TEXMAP, t)
	
	GETSHADERCOLOR_NOMAP(emission, FRStandardMtl_EMISSION_COLOR, t)
	GETSHADERFLOAT_NOMAP(emissionIntensity, FRStandardMtl_EMISSION_INTENSITY, t)
	GETSHADERFLOAT(mtlParser, emissionWeight, FRStandardMtl_EMISSION_WEIGHT, FRStandardMtl_EMISSION_WEIGHT_TEXMAP, t)

	material.SetValue("diffuse.color", diffuse);
	material.SetValue("diffuse.roughness", diffuseRoughness);
	material.SetValue("diffuse.weight", diffuseWeight);
	if (diffuseNormalTexmap)
	{
		frw::Value normalMap = mtlParser.createMap(diffuseNormalTexmap, MAP_FLAG_NORMALMAP);
		material.SetValue("diffuse.normal", normalMap);
	}

	material.SetValue("reflect.color", reflect);
	material.SetValue("reflect.ior", reflectIOR);
	material.SetValue("reflect.roughness", reflectRoughness);
	material.SetValue("reflect.weight", reflectWeight);
	if (reflectNormalTexmap)
	{
		frw::Value normalMap = mtlParser.createMap(reflectNormalTexmap, MAP_FLAG_NORMALMAP);
		material.SetValue("reflect.normal", normalMap);
	}

	material.SetValue("refract.color", refract);
	material.SetValue("refract.ior", refractIOR);
	material.SetValue("refract.weight", refractWeight);
	if (refractNormalTexmap)
	{
		frw::Value normalMap = mtlParser.createMap(refractNormalTexmap, MAP_FLAG_NORMALMAP);
		material.SetValue("refract.normal", normalMap);
	}

	material.SetValue("emissive.color.color", ms.ValueMul(emissionIntensity, emission));
	material.SetValue("emissive.weight", emissionWeight);

	if (emissionWeight_direct > 0.f || emissionWeight_texmap)
		mtlParser.shaderData.mNumEmissive++;
		
    return material;
}

FIRERENDER_NAMESPACE_END;
