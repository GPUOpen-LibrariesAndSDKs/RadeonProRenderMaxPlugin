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

#include "FireRenderMicrofacetMtl.h"
#include "FireRenderNormalMtl.h"
#include "Resource.h"
#include "parser\MaterialParser.h"
#include "maxscript\mxsplugin\mxsPlugin.h"

FIRERENDER_NAMESPACE_BEGIN;

IMPLEMENT_FRMTLCLASSDESC(MicrofacetMtl)

FRMTLCLASSDESCNAME(MicrofacetMtl) FRMTLCLASSNAME(MicrofacetMtl)::ClassDescInstance;

// All parameters of the material plugin. See FIRE_MAX_PBDESC definition for notes on backwards compatibility
static ParamBlockDesc2 pbDesc(
	0, _T("MicrofacetMtlPbdesc"), 0, &FRMTLCLASSNAME(MicrofacetMtl)::ClassDescInstance, P_AUTO_CONSTRUCT + P_AUTO_UI + P_VERSION, FIRERENDERMTLVER_LATEST, 0,
    //rollout
	IDD_FIRERENDER_MICROFACETMTL, IDS_FR_MTL_MICROFACET, 0, 0, NULL,

	FRMicrofacetMtl_COLOR, _T("Color"), TYPE_RGBA, P_ANIMATABLE, 0,
    p_default, Color(0.5f, 0.5f, 0.5f), p_ui, TYPE_COLORSWATCH, IDC_MF_COLOR, PB_END,

	FRMicrofacetMtl_ROUGHNESS, _T("Rougness"), TYPE_FLOAT, P_ANIMATABLE, 0,
	p_default, 1.f, p_range, 0.f, 1.f, p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_MF_ROUGHNESS, IDC_MF_ROUGHNESS_S, SPIN_AUTOSCALE, PB_END,

	FRMicrofacetMtl_IOR, _T("IOR"), TYPE_FLOAT, P_ANIMATABLE, 0,
	p_default, 1.f, p_range, 1.f, 999.f, p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_MF_IOR, IDC_MF_IOR_S, SPIN_AUTOSCALE, PB_END,

	FRMicrofacetMtl_COLOR_TEXMAP, _T("colorTexmap"), TYPE_TEXMAP, 0, 0,
	p_subtexno, FRMicrofacetMtl_TEXMAP_COLOR, p_ui, TYPE_TEXMAPBUTTON, IDC_MF_COLOR_TEXMAP, PB_END,

	FRMicrofacetMtl_ROUGHNESS_TEXMAP, _T("rougnessTexmap"), TYPE_TEXMAP, 0, 0,
	p_subtexno, FRMicrofacetMtl_TEXMAP_ROUGHNESS, p_ui, TYPE_TEXMAPBUTTON, IDC_MF_ROUGHNESS_TEXMAP, PB_END,

	FRMicrofacetMtl_NORMALMAP, _T("normalTexmap"), TYPE_TEXMAP, 0, 0,
	p_subtexno, FRMicrofacetMtl_TEXMAP_NORMAL, p_ui, TYPE_TEXMAPBUTTON, IDC_NORMAL_TEXMAP, PB_END,

    PB_END
    );

std::map<int, std::pair<ParamID, MCHAR*>> FRMTLCLASSNAME(MicrofacetMtl)::TEXMAP_MAPPING = {
	{ FRMicrofacetMtl_TEXMAP_COLOR, { FRMicrofacetMtl_COLOR_TEXMAP, _T("Color Map") } },
	{ FRMicrofacetMtl_TEXMAP_ROUGHNESS, { FRMicrofacetMtl_ROUGHNESS_TEXMAP, _T("Roughness Map") } },
	{ FRMicrofacetMtl_TEXMAP_NORMAL, { FRMicrofacetMtl_NORMALMAP, _T("Normal Map") } }
};

FRMTLCLASSNAME(MicrofacetMtl)::~FRMTLCLASSNAME(MicrofacetMtl)()
{
}


frw::Shader FRMTLCLASSNAME(MicrofacetMtl)::getShader(const TimeValue t, MaterialParser& mtlParser, INode* node)
{
	auto ms = mtlParser.materialSystem;

	frw::Shader material;
		
	Texmap* normalTexmap = GetFromPb<Texmap*>(pblock, FRMicrofacetMtl_NORMALMAP);
	const Color diffuseColor = GetFromPb<Color>(pblock, FRMicrofacetMtl_COLOR);
	Texmap* diffuseTexmap = GetFromPb<Texmap*>(pblock, FRMicrofacetMtl_COLOR_TEXMAP);
	const float roughness = GetFromPb<float>(pblock, FRMicrofacetMtl_ROUGHNESS);
	Texmap* roughnessTexmap = GetFromPb<Texmap*>(pblock, FRMicrofacetMtl_ROUGHNESS_TEXMAP);
	
	frw::Value roughnessv(roughness, roughness, roughness);
	if (roughnessTexmap)
	{
		material = frw::Shader(ms, frw::ShaderTypeMicrofacet);
		roughnessv = mtlParser.createMap(roughnessTexmap, MAP_FLAG_NOGAMMA);
		material.SetValue(RPR_MATERIAL_INPUT_ROUGHNESS, roughnessv);
	}
	else
	{
		if (roughness == 0.f)
			material = frw::Shader(ms, frw::ShaderTypeReflection);
		else if (roughness == 1.f)
			material = frw::Shader(ms, frw::ShaderTypeDiffuse);
		else
		{
			material = frw::Shader(ms, frw::ShaderTypeMicrofacet);
			material.SetValue(RPR_MATERIAL_INPUT_ROUGHNESS, roughnessv);
		}
	}
	
	frw::Value color(diffuseColor.r, diffuseColor.g, diffuseColor.b);
	if (diffuseTexmap)
		color = mtlParser.createMap(diffuseTexmap, 0);
	material.SetValue(RPR_MATERIAL_INPUT_COLOR, color);

	if (normalTexmap)
		material.SetValue(RPR_MATERIAL_INPUT_NORMAL, FRMTLCLASSNAME(NormalMtl)::translateGenericBump(t, normalTexmap, 1.f, mtlParser));
	
    return material;
}

FIRERENDER_NAMESPACE_END;
