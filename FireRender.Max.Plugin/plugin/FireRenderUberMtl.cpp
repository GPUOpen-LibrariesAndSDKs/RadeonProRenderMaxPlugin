/*********************************************************************************************************************************
* Radeon ProRender for 3ds Max plugin
* Copyright (c) 2017 AMD
* All Rights Reserved
*
* Radeon ProRender dedicated custom material plugin
*********************************************************************************************************************************/
#include "FireRenderUberMtl.h"
#include "FireRenderNormalMtl.h"
#include "Resource.h"
#include "parser\MaterialParser.h"
#include "maxscript\mxsplugin\mxsPlugin.h"

FIRERENDER_NAMESPACE_BEGIN;

IMPLEMENT_FRMTLCLASSDESC(UberMtl)

FRMTLCLASSDESCNAME(UberMtl) FRMTLCLASSNAME(UberMtl)::ClassDescInstance;

// All parameters of the material plugin. See FIRE_MAX_PBDESC definition for notes on backwards compatibility
static ParamBlockDesc2 pbDesc(
	0, _T("UberMtlPbdesc"), 0, &FRMTLCLASSNAME(UberMtl)::ClassDescInstance, P_AUTO_CONSTRUCT + P_AUTO_UI + P_VERSION, FIRERENDERMTLVER_LATEST, 0,
    //rollout
	IDD_FIRERENDER_UBERMTL, IDS_FR_MTL_UBER, 0, 0, NULL,

	// DIFFUSE
	// DiffuseLevel is actually the refraction level, cause it mixes diffuse with refraction
	FRUBERMTL_DIFFLEVEL, _T("RefractionLevel"), TYPE_FLOAT, P_ANIMATABLE, 0,
	p_default, 0.f, p_range, 0.f, 1.f, p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_UBER_DIFFLEVEL, IDC_UBER_DIFFLEVEL_S, SPIN_AUTOSCALE, PB_END,
		
	FRUBERMTL_DIFFLEVEL_TEXMAP, _T("RefractionLevelTexmap"), TYPE_TEXMAP, 0, 0,
	p_subtexno, FRUBERMTL_TEXMAP_DIFFLEVEL, p_ui, TYPE_TEXMAPBUTTON, IDC_UBER_DIFFLEVEL_T, PB_END,
	
	FRUBERMTL_DIFFCOLOR, _T("DiffuseColor"), TYPE_RGBA, P_ANIMATABLE, 0,
	p_default, Color(0.5f, 0.5f, 0.5f), p_ui, TYPE_COLORSWATCH, IDC_UBER_DIFFCOLOR, PB_END,

	FRUBERMTL_DIFFCOLOR_TEXMAP, _T("DiffuseColorTexmap"), TYPE_TEXMAP, 0, 0,
	p_subtexno, FRUBERMTL_TEXMAP_DIFFCOLOR, p_ui, TYPE_TEXMAPBUTTON, IDC_UBER_DIFFCOLOR_T, PB_END,

	FRUBERMTL_DIFFNORMAL, _T("DiffuseNormal"), TYPE_TEXMAP, 0, 0,
	p_subtexno, FRUBERMTL_TEXMAP_DIFFNORMAL, p_ui, TYPE_TEXMAPBUTTON, IDC_UBER_DIFFNORMAL, PB_END,
	
	// GLOSSY REFLECTIONS
	FRUBERMTL_GLOSSYUSE, _T("UseGlossy"), TYPE_BOOL, P_ANIMATABLE, 0,
	p_default, FALSE, p_ui, TYPE_SINGLECHEKBOX, IDC_UBER_GLOSSYUSE, PB_END,

	FRUBERMTL_GLOSSYCOLOR, _T("ReflColor"), TYPE_RGBA, P_ANIMATABLE, 0,
	p_default, Color(1.0f, 1.0f, 1.0f), p_ui, TYPE_COLORSWATCH, IDC_UBER_GLOSSYCOLOR, PB_END,

	FRUBERMTL_GLOSSYCOLOR_TEXMAP, _T("ReflColorTexmap"), TYPE_TEXMAP, 0, 0,
	p_subtexno, FRUBERMTL_TEXMAP_GLOSSYCOLOR, p_ui, TYPE_TEXMAPBUTTON, IDC_UBER_GLOSSYCOLOR_T, PB_END,
	
	FRUBERMTL_GLOSSYROTATION, _T("ReflRotation"), TYPE_FLOAT, P_ANIMATABLE, 0,
	p_default, 0.f, p_range, 0.f, 1.f, p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_UBER_GLOSSYROTATION, IDC_UBER_GLOSSYROTATION_S, SPIN_AUTOSCALE, PB_END,

	FRUBERMTL_GLOSSYROTATION_TEXMAP, _T("ReflRotationTexmap"), TYPE_TEXMAP, 0, 0,
	p_subtexno, FRUBERMTL_TEXMAP_GLOSSYROTATION, p_ui, TYPE_TEXMAPBUTTON, IDC_UBER_GLOSSYROTATION_T, PB_END,
		
	FRUBERMTL_GLOSSYROUGHNESS_X, _T("ReflRoughnessX"), TYPE_FLOAT, P_ANIMATABLE, 0,
	p_default, 0.1f, p_range, 0.f, 1.f, p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_UBER_GLOSSYROUGHNESSX, IDC_UBER_GLOSSYROUGHNESSX_S, SPIN_AUTOSCALE, PB_END,

	FRUBERMTL_GLOSSYROUGHNESS_X_TEXMAP, _T("ReflRoughnessXTexmap"), TYPE_TEXMAP, 0, 0,
	p_subtexno, FRUBERMTL_TEXMAP_GLOSSYROUGHNESS_X, p_ui, TYPE_TEXMAPBUTTON, IDC_UBER_GLOSSYROUGHNESSX_T, PB_END,

	FRUBERMTL_GLOSSYROUGHNESS_Y, _T("ReflRoughnessY"), TYPE_FLOAT, P_ANIMATABLE, 0,
	p_default, 0.1f, p_range, 0.f, 1.f, p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_UBER_GLOSSYROUGHNESSY, IDC_UBER_GLOSSYROUGHNESSY_S, SPIN_AUTOSCALE, PB_END,

	FRUBERMTL_GLOSSYROUGHNESS_Y_TEXMAP, _T("ReflRoughnessYTexmap"), TYPE_TEXMAP, 0, 0,
	p_subtexno, FRUBERMTL_TEXMAP_GLOSSYROUGHNESS_Y, p_ui, TYPE_TEXMAPBUTTON, IDC_UBER_GLOSSYROUGHNESSY_T, PB_END,
	
	FRUBERMTL_GLOSSYIOR, _T("ReflIor"), TYPE_FLOAT, P_ANIMATABLE, 0,
	p_default, 1.5f, p_range, 1.f, 999.f, p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_UBER_GLOSSYIOR, IDC_UBER_GLOSSYIOR_S, SPIN_AUTOSCALE, PB_END,
		
	FRUBERMTL_GLOSSYNORMAL, _T("ReflNormal"), TYPE_TEXMAP, 0, 0,
	p_subtexno, FRUBERMTL_TEXMAP_GLOSSYNORMAL, p_ui, TYPE_TEXMAPBUTTON, IDC_UBER_GLOSSYNORMAL, PB_END,

	// CLEARCOAT
	FRUBERMTL_CCUSE, _T("UseClearCoat"), TYPE_BOOL, P_ANIMATABLE, 0,
	p_default, FALSE, p_ui, TYPE_SINGLECHEKBOX, IDC_UBER_CCUSE, PB_END,

	FRUBERMTL_CCCOLOR, _T("ClearCoatColor"), TYPE_RGBA, P_ANIMATABLE, 0,
	p_default, Color(1.0f, 1.0f, 1.0f), p_ui, TYPE_COLORSWATCH, IDC_UBER_CCCOLOR, PB_END,

	FRUBERMTL_CCCOLOR_TEXMAP, _T("ClearCoatColorTexmap"), TYPE_TEXMAP, 0, 0,
	p_subtexno, FRUBERMTL_TEXMAP_CCCOLOR, p_ui, TYPE_TEXMAPBUTTON, IDC_UBER_CCCOLOR_T, PB_END,

	FRUBERMTL_CCIOR, _T("ClearCoatIor"), TYPE_FLOAT, P_ANIMATABLE, 0,
	p_default, 2.4f, p_range, 1.f, 999.f, p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_UBER_CCIOR, IDC_UBER_CCIOR_S, SPIN_AUTOSCALE, PB_END,

	FRUBERMTL_CCNORMAL, _T("ClearCoatNormal"), TYPE_TEXMAP, 0, 0,
	p_subtexno, FRUBERMTL_TEXMAP_CCNORMAL, p_ui, TYPE_TEXMAPBUTTON, IDC_UBER_CCNORMAL, PB_END,

	// REFRACTION
	FRUBERMTL_REFRCOLOR, _T("RefrColor"), TYPE_RGBA, P_ANIMATABLE, 0,
	p_default, Color(1.0f, 1.0f, 1.0f), p_ui, TYPE_COLORSWATCH, IDC_UBER_REFRCOLOR, PB_END,

	FRUBERMTL_REFRCOLOR_TEXMAP, _T("RefrColorTexmap"), TYPE_TEXMAP, 0, 0,
	p_subtexno, FRUBERMTL_TEXMAP_REFRCOLOR, p_ui, TYPE_TEXMAPBUTTON, IDC_UBER_REFRCOLOR_T, PB_END,

	FRUBERMTL_REFRROUGHNESS, _T("RefrRoughness"), TYPE_FLOAT, P_ANIMATABLE, 0,
	p_default, 0.f, p_range, 0.f, 1.f, p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_UBER_REFRROUGHNESS, IDC_UBER_REFRROUGHNESS_S, SPIN_AUTOSCALE, PB_END,

	FRUBERMTL_REFRROUGHNESS_TEXMAP, _T("RefrRoughnessTexmap"), TYPE_TEXMAP, 0, 0,
	p_subtexno, FRUBERMTL_TEXMAP_REFRROUGHNESS, p_ui, TYPE_TEXMAPBUTTON, IDC_UBER_REFRROUGHNESS_T, PB_END,

	FRUBERMTL_REFRIOR, _T("RefrIor"), TYPE_FLOAT, P_ANIMATABLE, 0,
	p_default, 1.52f, p_range, 1.f, 999.f, p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_UBER_REFRIOR, IDC_UBER_REFRIOR_S, SPIN_AUTOSCALE, PB_END,

	FRUBERMTL_REFRNORMAL, _T("RefrNormal"), TYPE_TEXMAP, 0, 0,
	p_subtexno, FRUBERMTL_TEXMAP_REFRNORMAL, p_ui, TYPE_TEXMAPBUTTON, IDC_UBER_REFRNORMAL, PB_END,

	// TRANSPARENCY
	FRUBERMTL_TRANSPLEVEL, _T("TranspLevel"), TYPE_FLOAT, P_ANIMATABLE, 0,
	p_default, 0.f, p_range, 0.f, 1.f, p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_UBER_TRANSPLEVEL, IDC_UBER_TRANSPLEVEL_S, SPIN_AUTOSCALE, PB_END,

	FRUBERMTL_TRANSPLEVEL_TEXMAP, _T("TranspLevelTexmap"), TYPE_TEXMAP, 0, 0,
	p_subtexno, FRUBERMTL_TEXMAP_TRANSPLEVEL, p_ui, TYPE_TEXMAPBUTTON, IDC_UBER_TRANSPLEVEL_T, PB_END,

	FRUBERMTL_TRANSPCOLOR, _T("TranspColor"), TYPE_RGBA, P_ANIMATABLE, 0,
	p_default, Color(1.0f, 1.0f, 1.0f), p_ui, TYPE_COLORSWATCH, IDC_UBER_TRANSPCOLOR, PB_END,

	FRUBERMTL_TRANSPCOLOR_TEXMAP, _T("TranspColorTexmap"), TYPE_TEXMAP, 0, 0,
	p_subtexno, FRUBERMTL_TEXMAP_TRANSPCOLOR, p_ui, TYPE_TEXMAPBUTTON, IDC_UBER_TRANSPCOLOR_T, PB_END,

	// DISPLACEMENT
	FRUBERMTL_DISPLACEMENT, _T("Displacement"), TYPE_TEXMAP, 0, 0,
	p_subtexno, FRUBERMTL_TEXMAP_DISPLACEMENT, p_ui, TYPE_TEXMAPBUTTON, IDC_UBER_DISPLACEMENT, PB_END,

	// FLAGS
	FRUBERMTL_FRUBERCAUSTICS, _T("Caustics"), TYPE_BOOL, P_ANIMATABLE, 0,
	p_default, TRUE, p_ui, TYPE_SINGLECHEKBOX, IDC_UBERCAUSTICS, PB_END,

	FRUBERMTL_FRUBERSHADOWCATCHER, _T("ShadowCatcher"), TYPE_BOOL, P_ANIMATABLE, 0,
	p_default, FALSE, p_ui, TYPE_SINGLECHEKBOX, IDC_UBERSHADOWCATCHER, PB_END,

	// VOLUME AND SCATTERING
	FRUBERMTL_USEVOLUME, _T("UseVolume"), TYPE_BOOL, P_ANIMATABLE, 0,
	p_default, FALSE, p_ui, TYPE_SINGLECHEKBOX, IDC_USEVOLUME, PB_END,

	FRUBERMTL_COLOR, _T("absorptionColor"), TYPE_RGBA, P_ANIMATABLE, 0,
	p_default, Color(1.0f, 1.0f, 1.0f), p_ui, TYPE_COLORSWATCH, IDC_ABSORPTION_COLOR,
	PB_END,

	FRUBERMTL_COLORTEXMAP, _T("absorptionColorTexmap"), TYPE_TEXMAP, 0, 0,
	p_subtexno, FRUBERMTL_TEXMAP_COLOR, p_ui, TYPE_TEXMAPBUTTON, IDC_ABSORPTION_TEXMAP, PB_END,

	FRUBERMTL_DISTANCE, _T("absorptionDistance"), TYPE_WORLD, P_ANIMATABLE, 0,
	p_default, 0.01f, p_range, 0.f, 100000.f, p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_DISTANCE, IDC_DISTANCE_S, SPIN_AUTOSCALE,
	PB_END,

	FRUBERMTL_DISTANCETEXMAP, _T("absorptionDistanceTexmap"), TYPE_TEXMAP, 0, 0,
	p_subtexno, FRUBERMTL_TEXMAP_DISTANCE, p_ui, TYPE_TEXMAPBUTTON, IDC_DISTANCE_TEXMAP, PB_END,

	FRUBERMTL_EMISSIONMULTIPLIER, _T("emissionMultiplier"), TYPE_FLOAT, P_ANIMATABLE, 0,
	p_default, 0.0f, p_range, 0.f, 9999999.f, p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_EMISSION_MULTIPLIER, IDC_EMISSION_MULTIPLIER_S, SPIN_AUTOSCALE,
	PB_END,

	FRUBERMTL_EMISSIONCOLOR, _T("emissionColor"), TYPE_RGBA, P_ANIMATABLE, 0,
	p_default, Color(1.0f, 1.0f, 1.0f), p_ui, TYPE_COLORSWATCH, IDC_EMISSION_COLOR,
	PB_END,

	FRUBERMTL_EMISSIONCOLORTEXMAP, _T("emissionColorTexmap"), TYPE_TEXMAP, 0, 0,
	p_subtexno, FRUBERMTL_TEXMAP_EMISSION, p_ui, TYPE_TEXMAPBUTTON, IDC_EMISSION_TEXMAP, PB_END,

	FRUBERMTL_SCATTERINGDIRECTION, _T("phase"), TYPE_FLOAT, P_ANIMATABLE, 0,
	p_default, 0.0f, p_range, -1.f, 1.f, p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_SCATTERING_PHASE, IDC_SCATTERING_PHASE_S, SPIN_AUTOSCALE,
	p_tooltip, IDS_FR_MTL_VOLUME_PHASE_TTP,
	PB_END,

	FRUBERMTL_MULTISCATTERING, _T("multiscatter"), TYPE_BOOL, P_ANIMATABLE, 0,
	p_default, TRUE, p_ui, TYPE_SINGLECHEKBOX, IDC_MULTISCATTER,
	p_tooltip, IDS_FR_MTL_VOLUME_MULTISCATTER_TTP,
	PB_END,

	// invert transparency map (to use opacity maps)
	FRUBERMTL_INVERTTRANSPMAP, _T("invertTranspMap"), TYPE_BOOL, P_ANIMATABLE, 0,
	p_default, FALSE, p_ui, TYPE_SINGLECHEKBOX, IDC_UBER_INVERTMAP,
	p_tooltip, IDS_STRING265,
	PB_END,

    PB_END
    );

std::map<int, std::pair<ParamID, MCHAR*>> FRMTLCLASSNAME(UberMtl)::TEXMAP_MAPPING = {
	{ FRUBERMTL_TEXMAP_DIFFCOLOR, { FRUBERMTL_DIFFCOLOR_TEXMAP, _T("Diffuse Color Map") } },
	{ FRUBERMTL_TEXMAP_DIFFNORMAL, { FRUBERMTL_DIFFNORMAL, _T("Diffuse Normal Map") } },
	{ FRUBERMTL_TEXMAP_GLOSSYCOLOR, { FRUBERMTL_GLOSSYCOLOR_TEXMAP, _T("Refl Color Map") } },
	{ FRUBERMTL_TEXMAP_GLOSSYROTATION,{ FRUBERMTL_GLOSSYROTATION_TEXMAP, _T("Refl Rotation Map") } },
	{ FRUBERMTL_TEXMAP_GLOSSYROUGHNESS_X, { FRUBERMTL_GLOSSYROUGHNESS_X_TEXMAP, _T("Refl Roughness X Map") } },
	{ FRUBERMTL_TEXMAP_GLOSSYROUGHNESS_Y,{ FRUBERMTL_GLOSSYROUGHNESS_Y_TEXMAP, _T("Refl Roughness Y Map") } },
	{ FRUBERMTL_TEXMAP_GLOSSYNORMAL, { FRUBERMTL_GLOSSYNORMAL, _T("Refl Normal Map") } },
	{ FRUBERMTL_TEXMAP_CCCOLOR, { FRUBERMTL_CCCOLOR_TEXMAP, _T("ClearCoat Color Map") } },
	{ FRUBERMTL_TEXMAP_CCNORMAL, { FRUBERMTL_CCNORMAL, _T("ClearCoat Normal Map") } },
	{ FRUBERMTL_TEXMAP_DIFFLEVEL,{ FRUBERMTL_DIFFLEVEL_TEXMAP, _T("Refr Level Map") } },
	{ FRUBERMTL_TEXMAP_REFRCOLOR, { FRUBERMTL_REFRCOLOR_TEXMAP, _T("Refr Color Map") } },
	{ FRUBERMTL_TEXMAP_REFRROUGHNESS, { FRUBERMTL_REFRROUGHNESS_TEXMAP, _T("Refr Roughness Map") } },
	{ FRUBERMTL_TEXMAP_REFRNORMAL, { FRUBERMTL_REFRNORMAL, _T("Refr Normal Map") } },
	{ FRUBERMTL_TEXMAP_TRANSPLEVEL, { FRUBERMTL_TRANSPLEVEL_TEXMAP, _T("Transp Level Map") } },
	{ FRUBERMTL_TEXMAP_TRANSPCOLOR, { FRUBERMTL_TRANSPCOLOR_TEXMAP, _T("Transp Color Map") } },
	{ FRUBERMTL_TEXMAP_DISPLACEMENT,{ FRUBERMTL_DISPLACEMENT, _T("Displacement Map") } },
	{ FRUBERMTL_TEXMAP_COLOR,{ FRUBERMTL_COLORTEXMAP, _T("Absorption Color Map") } },
	{ FRUBERMTL_TEXMAP_DISTANCE,{ FRUBERMTL_DISTANCETEXMAP, _T("Absorption Distance Map") } },
	{ FRUBERMTL_TEXMAP_EMISSION,{ FRUBERMTL_EMISSIONCOLORTEXMAP, _T("Emission Map") } }
};

FRMTLCLASSNAME(UberMtl)::~FRMTLCLASSNAME(UberMtl)()
{
}

frw::Shader FRMTLCLASSNAME(UberMtl)::getVolumeShader(const TimeValue t, MaterialParser& mtlParser, INode* node)
{
	auto ms = mtlParser.materialSystem;

	if (GetFromPb<BOOL>(pblock, FRUBERMTL_USEVOLUME))
	{
		auto ms = mtlParser.materialSystem;

		frw::Shader material(ms, frw::ShaderTypeVolume);

		const Color color = GetFromPb<Color>(pblock, FRUBERMTL_COLOR);
		Texmap* colorTexmap = GetFromPb<Texmap*>(pblock, FRUBERMTL_COLORTEXMAP);
		frw::Value theColor(color.r, color.g, color.b);
		if (colorTexmap)
			theColor = mtlParser.createMap(colorTexmap, 0);

		float distance = GetFromPb<float>(pblock, FRUBERMTL_DISTANCE);
		Texmap* distanceTexmap = GetFromPb<Texmap*>(pblock, FRUBERMTL_DISTANCETEXMAP);
		frw::Value theDistance(distance);
		if (distanceTexmap)
			theDistance = ms.ValueMul(mtlParser.createMap(distanceTexmap, MAP_FLAG_NOGAMMA), frw::Value(distance));

		// scattering
		material.SetValue("sigmas", ms.ValueDiv(theColor, theDistance));
		
		// absorption
		material.SetValue("sigmaa", ms.ValueDiv(ms.ValueSub(frw::Value(1), theColor), theDistance));

		// phase and multi on/off
		material.SetValue("g", frw::Value(GetFromPb<float>(pblock, FRUBERMTL_SCATTERINGDIRECTION)));
		material.SetValue("multiscatter", GetFromPb<BOOL>(pblock, FRUBERMTL_MULTISCATTERING) ? frw::Value(1.0f) : frw::Value(0.0f));

		// emission
		const Color emissionColor = GetFromPb<Color>(pblock, FRUBERMTL_EMISSIONCOLOR);
		Texmap* emissionTexmap = GetFromPb<Texmap*>(pblock, FRUBERMTL_EMISSIONCOLORTEXMAP);
		frw::Value theEmission(emissionColor.r, emissionColor.g, emissionColor.b);
		if (emissionTexmap)
			theEmission = mtlParser.createMap(emissionTexmap, MAP_FLAG_WANTSHDR);

		float emissionMultiplier = GetFromPb<float>(pblock, FRUBERMTL_EMISSIONMULTIPLIER);
		material.SetValue("emission",
			ms.ValueMul(theEmission, frw::Value(emissionMultiplier)));

		if (emissionMultiplier > 0.f && ((emissionColor.r > 0.f && emissionColor.g > 0.f && emissionColor.r > 0.f) || emissionTexmap))
			mtlParser.shaderData.mNumEmissive++;

		return material;
	}

	// rprShapeSetVolumeMaterial doesn't work when null material passed to it, so we should return default volume
	// material to be able to be able to remove volume parameters from shape.
	return frw::Shader(mtlParser.materialSystem, frw::ShaderTypeVolume);
}

frw::Shader FRMTLCLASSNAME(UberMtl)::getShader(const TimeValue t, MaterialParser& mtlParser, INode* node)
{
	auto ms = mtlParser.materialSystem;

	const float diffuseLevel = GetFromPb<float>(pblock, FRUBERMTL_DIFFLEVEL);
	Texmap* diffuseLevelTexmap = GetFromPb<Texmap*>(pblock, FRUBERMTL_DIFFLEVEL_TEXMAP);
	const Color diffuseColor = GetFromPb<Color>(pblock, FRUBERMTL_DIFFCOLOR);
	Texmap* diffuseColorTexmap = GetFromPb<Texmap*>(pblock, FRUBERMTL_DIFFCOLOR_TEXMAP);
	Texmap* diffuseNormalTexmap = GetFromPb<Texmap*>(pblock, FRUBERMTL_DIFFNORMAL);

	const bool reflUse = GetFromPb<bool>(pblock, FRUBERMTL_GLOSSYUSE);
	const Color reflColor = GetFromPb<Color>(pblock, FRUBERMTL_GLOSSYCOLOR);
	Texmap* reflColorTexmap = GetFromPb<Texmap*>(pblock, FRUBERMTL_GLOSSYCOLOR_TEXMAP);
	const float reflRotation = GetFromPb<float>(pblock, FRUBERMTL_GLOSSYROTATION);
	Texmap* reflRotationTexmap = GetFromPb<Texmap*>(pblock, FRUBERMTL_GLOSSYROTATION_TEXMAP);
	const float reflRoughnessX = GetFromPb<float>(pblock, FRUBERMTL_GLOSSYROUGHNESS_X);
	Texmap* reflRoughnessXTexmap = GetFromPb<Texmap*>(pblock, FRUBERMTL_GLOSSYROUGHNESS_X_TEXMAP);
	const float reflRoughnessY = GetFromPb<float>(pblock, FRUBERMTL_GLOSSYROUGHNESS_Y);
	Texmap* reflRoughnessYTexmap = GetFromPb<Texmap*>(pblock, FRUBERMTL_GLOSSYROUGHNESS_Y_TEXMAP);
	const float reflIOR = GetFromPb<float>(pblock, FRUBERMTL_GLOSSYIOR);
	Texmap* reflNormalTexmap = GetFromPb<Texmap*>(pblock, FRUBERMTL_GLOSSYNORMAL);

	const bool ccUse = GetFromPb<bool>(pblock, FRUBERMTL_CCUSE);
	const Color ccColor = GetFromPb<Color>(pblock, FRUBERMTL_CCCOLOR);
	Texmap* ccColorTexmap = GetFromPb<Texmap*>(pblock, FRUBERMTL_CCCOLOR_TEXMAP);
	const float ccIOR = GetFromPb<float>(pblock, FRUBERMTL_CCIOR);
	Texmap* ccNormalTexmap = GetFromPb<Texmap*>(pblock, FRUBERMTL_CCNORMAL);

	const Color refrColor = GetFromPb<Color>(pblock, FRUBERMTL_REFRCOLOR);
	Texmap* refrColorTexmap = GetFromPb<Texmap*>(pblock, FRUBERMTL_REFRCOLOR_TEXMAP);
	const float refrRoughness = GetFromPb<float>(pblock, FRUBERMTL_REFRROUGHNESS);
	Texmap* refrRoughnessTexmap = GetFromPb<Texmap*>(pblock, FRUBERMTL_REFRROUGHNESS_TEXMAP);
	const float refrIOR = GetFromPb<float>(pblock, FRUBERMTL_REFRIOR);
	Texmap* refrNormalTexmap = GetFromPb<Texmap*>(pblock, FRUBERMTL_REFRNORMAL);

	const float transpLevel = GetFromPb<float>(pblock, FRUBERMTL_TRANSPLEVEL); // weights.transparency
	Texmap* transpLevelTexmap = GetFromPb<Texmap*>(pblock, FRUBERMTL_TRANSPLEVEL_TEXMAP);
	const Color transpColor = GetFromPb<Color>(pblock, FRUBERMTL_TRANSPCOLOR);
	Texmap* transpColorTexmap = GetFromPb<Texmap*>(pblock, FRUBERMTL_TRANSPCOLOR_TEXMAP);

	//FRUBERMTL_DISPLACEMENT,
	//FRUBERMTL_FRUBERCAUSTICS,
	//FRUBERMTL_FRUBERSHADOWCATCHER
	
	frw::Shader material = frw::Shader(ms, frw::ShaderTypeUber);

	//DumpFRParms(material.Handle());

	// DIFFUSE
	{
		frw::Value color(diffuseColor.r, diffuseColor.g, diffuseColor.b);
		if (diffuseColorTexmap)
			color = mtlParser.createMap(diffuseColorTexmap, 0);
		material.SetValue("diffuse.color", color);

		if (diffuseNormalTexmap)
			material.SetValue("diffuse.normal", FRMTLCLASSNAME(NormalMtl)::translateGenericBump(t, diffuseNormalTexmap, 1.f, mtlParser));
	}

	// GLOSSY (uses microfacet for now, will use ward)
	{
		if (!reflUse)
			material.SetValue("weights.glossy2diffuse", frw::Value(0.f));
		else
		{
			frw::Value color(reflColor.r, reflColor.g, reflColor.b);
			if (reflColorTexmap)
				color = mtlParser.createMap(reflColorTexmap, 0);
			material.SetValue("glossy.color", color);
			
			frw::Value roughx(reflRoughnessX, reflRoughnessX, reflRoughnessX);
			if (reflRoughnessXTexmap)
				roughx = mtlParser.createMap(reflRoughnessXTexmap, MAP_FLAG_NOGAMMA);
			material.SetValue("roughness_x", ms.ValueAdd(roughx, 0.000001f));
			
			frw::Value roughy(reflRoughnessY, reflRoughnessY, reflRoughnessY);
			if (reflRoughnessYTexmap)
				roughy = mtlParser.createMap(reflRoughnessYTexmap, MAP_FLAG_NOGAMMA);
			material.SetValue("roughness_y", ms.ValueAdd(roughy, 0.000001f));

			if (reflNormalTexmap)
				material.SetValue("glossy.normal", FRMTLCLASSNAME(NormalMtl)::translateGenericBump(t, reflNormalTexmap, 1.f, mtlParser));

			material.SetValue("weights.glossy2diffuse", ms.ValueFresnel(reflIOR));
		}
	}

	// CLEARCOAT (specular reflections)
	{
		if (!ccUse)
			material.SetValue("weights.clearcoat2glossy", frw::Value(0.f));
		else
		{
			frw::Value color(ccColor.r, ccColor.g, ccColor.b);
			if (ccColorTexmap)
				color = mtlParser.createMap(ccColorTexmap, 0);
			material.SetValue("clearcoat.color", color);

			if (ccNormalTexmap)
				material.SetValue("clearcoat.normal", FRMTLCLASSNAME(NormalMtl)::translateGenericBump(t, ccNormalTexmap, 1.f, mtlParser));

			material.SetValue("weights.clearcoat2glossy", ms.ValueFresnel(ccIOR));
		}
	}

	// REFRACTION
	{
		frw::Value color(refrColor.r, refrColor.g, refrColor.b);
		if (refrColorTexmap)
			color = mtlParser.createMap(refrColorTexmap, 0);
		material.SetValue("refraction.color", color);
				
		if (refrNormalTexmap)
			material.SetValue("refraction.normal", FRMTLCLASSNAME(NormalMtl)::translateGenericBump(t, refrNormalTexmap, 1.f, mtlParser));

		material.SetValue("refraction.ior", frw::Value(refrIOR));

		frw::Value rough(refrRoughness, refrRoughness, refrRoughness);
		if (refrRoughnessTexmap)
			rough = mtlParser.createMap(refrRoughnessTexmap, MAP_FLAG_NOGAMMA);
		material.SetValue("refraction.roughness", rough);

		frw::Value weight(diffuseLevel, diffuseLevel, diffuseLevel);
		if (diffuseLevelTexmap)
			weight = mtlParser.createMap(diffuseLevelTexmap, MAP_FLAG_NOGAMMA);
		weight = ms.ValueSub(1.0, weight);
		material.SetValue("weights.diffuse2refraction", weight);
	}

	// TRANSPARENCY
	{
		frw::Value color(transpColor.r, transpColor.g, transpColor.b);
		if (transpColorTexmap)
			color = mtlParser.createMap(transpColorTexmap, 0);
		material.SetValue("transparency.color", color);

		frw::Value weight(transpLevel, transpLevel, transpLevel);
		if (transpLevelTexmap)
		{
			weight = mtlParser.createMap(transpLevelTexmap, MAP_FLAG_NOGAMMA);
			const bool invert = GetFromPb<bool>(pblock, FRUBERMTL_INVERTTRANSPMAP);
			if (invert)
				weight = ms.ValueSub(frw::Value(1.0), weight);
		}
		material.SetValue("weights.transparency", weight);
	}

    return material;
}

FIRERENDER_NAMESPACE_END;
