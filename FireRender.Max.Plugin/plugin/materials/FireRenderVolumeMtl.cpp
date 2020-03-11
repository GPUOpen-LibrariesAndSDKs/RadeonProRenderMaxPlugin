/**********************************************************************
Copyright 2020 Advanced Micro Devices, Inc
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at
    http://www.apache.org/licenses/LICENSE-2.0
Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
********************************************************************/

#include "FireRenderVolumeMtl.h"
#include "Resource.h"
#include "parser\MaterialParser.h"
#include "maxscript\mxsplugin\mxsPlugin.h"

FIRERENDER_NAMESPACE_BEGIN

IMPLEMENT_FRMTLCLASSDESC(VolumeMtl)

FRMTLCLASSDESCNAME(VolumeMtl) FRMTLCLASSNAME(VolumeMtl)::ClassDescInstance;


// All parameters of the material plugin. See FIRE_MAX_PBDESC definition for notes on backwards compatibility
static ParamBlockDesc2 pbDesc(
	0, _T("VolumeMtlPbdesc"), 0, &FRMTLCLASSNAME(VolumeMtl)::ClassDescInstance, P_AUTO_CONSTRUCT + P_AUTO_UI + P_VERSION, FIRERENDERMTLVER_LATEST, 0,
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

std::map<int, std::pair<ParamID, MCHAR*>> FRMTLCLASSNAME(VolumeMtl)::TEXMAP_MAPPING = {
	{ FRVolumeMtl_TEXMAP_COLOR,{ FRVolumeMtl_ColorTexmap, _T("Color Map") } },
	{ FRVolumeMtl_TEXMAP_DISTANCE,{ FRVolumeMtl_DistanceTexmap, _T("Distance Map") } },
	{ FRVolumeMtl_TEXMAP_EMISSION,{ FRVolumeMtl_EmissionColorTexmap, _T("Emission Map") } },
};

FRMTLCLASSNAME(VolumeMtl)::~FRMTLCLASSNAME(VolumeMtl)()
{
}

frw::Shader FRMTLCLASSNAME(VolumeMtl)::getVolumeShader(const TimeValue t, MaterialParser& mtlParser, INode* node)
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
	material.SetValue(RPR_MATERIAL_INPUT_SCATTERING, ms.ValueDiv(theColor, theDistance));

	// absorption
	material.SetValue(RPR_MATERIAL_INPUT_ABSORBTION, ms.ValueDiv(ms.ValueSub(frw::Value(1), theColor), theDistance));

	// phase and multi on/off
	material.SetValue(RPR_MATERIAL_INPUT_G, frw::Value(GetFromPb<float>(pblock, FRVolumeMtl_ScatteringDirection)));
	material.SetValue(RPR_MATERIAL_INPUT_MULTISCATTER, GetFromPb<BOOL>(pblock, FRVolumeMtl_MultiScattering) ? frw::Value(1.0f) : frw::Value(0.0f));

	// emission
	const Color emissionColor = GetFromPb<Color>(pblock, FRVolumeMtl_EmissionColor);
	Texmap* emissionTexmap = GetFromPb<Texmap*>(pblock, FRVolumeMtl_EmissionColorTexmap);
	frw::Value theEmission(emissionColor.r, emissionColor.g, emissionColor.b);
	if (emissionTexmap)
		theEmission = mtlParser.createMap(emissionTexmap, MAP_FLAG_WANTSHDR);

	material.SetValue(RPR_MATERIAL_INPUT_EMISSION, ms.ValueMul(theEmission, frw::Value(GetFromPb<float>(pblock, FRVolumeMtl_EmissionMultiplier))));

    return material;
}

frw::Shader FRMTLCLASSNAME(VolumeMtl)::getShader(const TimeValue t, MaterialParser& mtlParser, INode* node)
{
	auto ms = mtlParser.materialSystem;

	return frw::Shader(ms, frw::ShaderTypeTransparent);
}

FIRERENDER_NAMESPACE_END
