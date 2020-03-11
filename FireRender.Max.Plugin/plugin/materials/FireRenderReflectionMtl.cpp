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

#include "FireRenderReflectionMtl.h"
#include "FireRenderNormalMtl.h"
#include "Resource.h"
#include "parser\MaterialParser.h"
#include "maxscript\mxsplugin\mxsPlugin.h"

FIRERENDER_NAMESPACE_BEGIN;

IMPLEMENT_FRMTLCLASSDESC(ReflectionMtl)

FRMTLCLASSDESCNAME(ReflectionMtl) FRMTLCLASSNAME(ReflectionMtl)::ClassDescInstance;

// All parameters of the material plugin. See FIRE_MAX_PBDESC definition for notes on backwards compatibility
static ParamBlockDesc2 pbDesc(
	0, _T("ReflectionMtlPbdesc"), 0, &FRMTLCLASSNAME(ReflectionMtl)::ClassDescInstance, P_AUTO_CONSTRUCT + P_AUTO_UI + P_VERSION, FIRERENDERMTLVER_LATEST, 0,
    //rollout
	IDD_FIRERENDER_REFLECTIONMTL, IDS_FR_MTL_REFLECTION, 0, 0, NULL,

	FRReflectionMtl_COLOR, _T("Color"), TYPE_RGBA, P_ANIMATABLE, 0,
    p_default, Color(0.5f, 0.5f, 0.5f), p_ui, TYPE_COLORSWATCH, IDC_REF_COLOR, PB_END,

	FRReflectionMtl_COLOR_TEXMAP, _T("colorTexmap"), TYPE_TEXMAP, 0, 0,
	p_subtexno, FRReflectionMtl_TEXMAP_COLOR, p_ui, TYPE_TEXMAPBUTTON, IDC_REF_COLOR_TEXMAP, PB_END,

	FRReflectionMtl_NORMALMAP, _T("normalTexmap"), TYPE_TEXMAP, 0, 0,
	p_subtexno, FRReflectionMtl_TEXMAP_NORMAL, p_ui, TYPE_TEXMAPBUTTON, IDC_NORMAL_TEXMAP, PB_END,

    PB_END
    );

std::map<int, std::pair<ParamID, MCHAR*>> FRMTLCLASSNAME(ReflectionMtl)::TEXMAP_MAPPING = {
	{ FRReflectionMtl_TEXMAP_COLOR, { FRReflectionMtl_COLOR_TEXMAP, _T("Color map") } },
	{ FRReflectionMtl_TEXMAP_NORMAL, { FRReflectionMtl_NORMALMAP, _T("Normal map") } }
};

FRMTLCLASSNAME(ReflectionMtl)::~FRMTLCLASSNAME(ReflectionMtl)()
{
}


frw::Shader FRMTLCLASSNAME(ReflectionMtl)::getShader(const TimeValue t, MaterialParser& mtlParser, INode* node)
{
	auto ms = mtlParser.materialSystem;

	frw::Shader material(ms, frw::ShaderTypeReflection);
		
	Texmap* normalTexmap = GetFromPb<Texmap*>(pblock, FRReflectionMtl_NORMALMAP);
	const Color diffuseColor = GetFromPb<Color>(pblock, FRReflectionMtl_COLOR);
	Texmap* diffuseTexmap = GetFromPb<Texmap*>(pblock, FRReflectionMtl_COLOR_TEXMAP);

	frw::Value color(diffuseColor.r, diffuseColor.g, diffuseColor.b);
	if (diffuseTexmap)
		color = mtlParser.createMap(diffuseTexmap, 0);

	material.SetValue(RPR_MATERIAL_INPUT_COLOR, color);
	if (normalTexmap)
		material.SetValue(RPR_MATERIAL_INPUT_NORMAL, FRMTLCLASSNAME(NormalMtl)::translateGenericBump(t, normalTexmap, 1.f, mtlParser));
		
    return material;
}

FIRERENDER_NAMESPACE_END;
