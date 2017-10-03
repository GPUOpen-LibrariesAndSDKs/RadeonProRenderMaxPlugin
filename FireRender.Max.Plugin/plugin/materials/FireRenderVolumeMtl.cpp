/*********************************************************************************************************************************
* Radeon ProRender for 3ds Max plugin
* Copyright (c) 2017 AMD
* All Rights Reserved
*
* Radeon ProRender dedicated custom material plugin
*********************************************************************************************************************************/
#include "FireRenderVolumeMtl.h"
#include "Resource.h"
#include "parser\MaterialParser.h"
#include "maxscript\mxsplugin\mxsPlugin.h"

FIRERENDER_NAMESPACE_BEGIN;

IMPLEMENT_FRMTLCLASSDESC(VolumeMtl)

FRMTLCLASSDESCNAME(VolumeMtl) FireRenderVolumeMtl::ClassDescInstance;


// All parameters of the material plugin. See FIRE_MAX_PBDESC definition for notes on backwards compatibility
static ParamBlockDesc2 pbDesc(
	0, _T("VolumeMtlPbdesc"), 0, &FireRenderVolumeMtl::ClassDescInstance, P_AUTO_CONSTRUCT + P_AUTO_UI + P_VERSION, FIRERENDERMTLVER_LATEST, 0,
    //rollout
	IDD_FIRERENDER_VOLUMEMTL, IDS_FR_MTL_VOLUME, 0, 0, NULL,

	FRVolumeMtl_Color, _T("color"), TYPE_RGBA, P_ANIMATABLE, 0,
	p_default, Color(1.0f, 1.0f, 1.0f), p_ui, TYPE_COLORSWATCH, IDC_ABSORPTION_COLOR,
	PB_END,

	FRVolumeMtl_ColorTexmap, _T("colorTexmap"), TYPE_TEXMAP, 0, 0,
	p_subtexno, FRVolumeMtl_TEXMAP_COLOR, p_ui, TYPE_TEXMAPBUTTON, IDC_ABSORPTION_TEXMAP, PB_END,
	
	FRVolumeMtl_Distance, _T("distance"), TYPE_FLOAT, P_ANIMATABLE, 0,
	p_default, 0.01f, p_range, 0.f, 100000.f, p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_DISTANCE, IDC_DISTANCE_S, SPIN_AUTOSCALE,
	PB_END,

	FRVolumeMtl_DistanceTexmap, _T("distanceTexmap"), TYPE_TEXMAP, 0, 0,
	p_subtexno, FRVolumeMtl_TEXMAP_DISTANCE, p_ui, TYPE_TEXMAPBUTTON, IDC_DISTANCE_TEXMAP, PB_END,

	FRVolumeMtl_EmissionMultiplier, _T("emissionMultiplier"), TYPE_FLOAT, P_ANIMATABLE, 0,
	p_default, 0.0f, p_range, 0.f, 9999999.f, p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_EMISSION_MULTIPLIER, IDC_EMISSION_MULTIPLIER_S, SPIN_AUTOSCALE,
	PB_END,

	FRVolumeMtl_EmissionColor, _T("emissionColor"), TYPE_RGBA, P_ANIMATABLE, 0,
	p_default, Color(1.0f, 1.0f, 1.0f), p_ui, TYPE_COLORSWATCH, IDC_EMISSION_COLOR,
	PB_END,

	FRVolumeMtl_EmissionColorTexmap, _T("emissionColorTexmap"), TYPE_TEXMAP, 0, 0,
	p_subtexno, FRVolumeMtl_TEXMAP_EMISSION, p_ui, TYPE_TEXMAPBUTTON, IDC_EMISSION_TEXMAP, PB_END,
	
	FRVolumeMtl_ScatteringDirection, _T("phase"), TYPE_FLOAT, P_ANIMATABLE, 0,
	p_default, 0.0f, p_range, -1.f, 1.f, p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_SCATTERING_PHASE, IDC_SCATTERING_PHASE_S, SPIN_AUTOSCALE,
	p_tooltip, IDS_FR_MTL_VOLUME_PHASE_TTP,
	PB_END,

	FRVolumeMtl_MultiScattering, _T("multiscatter"), TYPE_BOOL, P_ANIMATABLE, 0,
	p_default, TRUE, p_ui, TYPE_SINGLECHEKBOX, IDC_MULTISCATTER, 
	p_tooltip, IDS_FR_MTL_VOLUME_MULTISCATTER_TTP, 
	PB_END,

    PB_END
    );

std::map<int, std::pair<ParamID, MCHAR*>> FireRenderVolumeMtl::TEXMAP_MAPPING = {
	{ FRVolumeMtl_TEXMAP_COLOR,{ FRVolumeMtl_ColorTexmap, _T("Color Map") } },
	{ FRVolumeMtl_TEXMAP_DISTANCE,{ FRVolumeMtl_DistanceTexmap, _T("Distance Map") } },
	{ FRVolumeMtl_TEXMAP_EMISSION,{ FRVolumeMtl_EmissionColorTexmap, _T("Emission Map") } },
};

frw::Shader FireRenderVolumeMtl::getVolumeShader(const TimeValue t, MaterialParser& mtlParser, INode* node)
{
	auto ms = mtlParser.materialSystem;

	frw::Shader material(ms, frw::ShaderTypeVolume);

	const Color color = GetFromPb<Color>(pblock, FRVolumeMtl_Color);
	Texmap* colorTexmap = GetFromPb<Texmap*>(pblock, FRVolumeMtl_ColorTexmap);
	frw::Value theColor(color.r, color.g, color.b);
	if (colorTexmap)
		theColor = mtlParser.createMap(colorTexmap, 0);

	float distance = GetFromPb<float>(pblock, FRVolumeMtl_Distance);
	Texmap* distanceTexmap = GetFromPb<Texmap*>(pblock, FRVolumeMtl_DistanceTexmap);
	frw::Value theDistance(distance);
	if (distanceTexmap)
		theDistance = ms.ValueMul(mtlParser.createMap(distanceTexmap, MAP_FLAG_NOGAMMA), frw::Value(distance));
	
	// scattering
	material.SetValue("sigmas", ms.ValueDiv(theColor, theDistance));

	// absorption
	material.SetValue("sigmaa", ms.ValueDiv(ms.ValueSub(frw::Value(1), theColor), theDistance));

	// phase and multi on/off
	material.SetValue("g", frw::Value(GetFromPb<float>(pblock, FRVolumeMtl_ScatteringDirection)));
	material.SetValue("multiscatter", GetFromPb<BOOL>(pblock, FRVolumeMtl_MultiScattering) ? frw::Value(1.0f) : frw::Value(0.0f));

	// emission
	const Color emissionColor = GetFromPb<Color>(pblock, FRVolumeMtl_EmissionColor);
	Texmap* emissionTexmap = GetFromPb<Texmap*>(pblock, FRVolumeMtl_EmissionColorTexmap);
	frw::Value theEmission(emissionColor.r, emissionColor.g, emissionColor.b);
	if (emissionTexmap)
		theEmission = mtlParser.createMap(emissionTexmap, MAP_FLAG_WANTSHDR);

	material.SetValue("emission",
		ms.ValueMul(theEmission, frw::Value(GetFromPb<float>(pblock, FRVolumeMtl_EmissionMultiplier))));

    return material;
}

frw::Shader FireRenderVolumeMtl::getShader(const TimeValue t, MaterialParser& mtlParser, INode* node)
{
	auto ms = mtlParser.materialSystem;

	return frw::Shader(ms, frw::ShaderTypeTransparent);
}

FIRERENDER_NAMESPACE_END;
