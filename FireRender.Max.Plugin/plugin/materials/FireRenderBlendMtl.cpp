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

#include "FireRenderBlendMtl.h"
#include "Resource.h"
#include "parser\MaterialParser.h"
#include "maxscript\mxsplugin\mxsPlugin.h"
#include "FireRenderDiffuseMtl.h"

FIRERENDER_NAMESPACE_BEGIN;

IMPLEMENT_FRMTLCLASSDESC(BlendMtl)

FRMTLCLASSDESCNAME(BlendMtl) FRMTLCLASSNAME(BlendMtl)::ClassDescInstance;

// All parameters of the material plugin. See FIRE_MAX_PBDESC definition for notes on backwards compatibility
static ParamBlockDesc2 pbDesc(
	0, _T("BlendMtlPbdesc"), 0, &FRMTLCLASSNAME(BlendMtl)::ClassDescInstance, P_AUTO_CONSTRUCT + P_AUTO_UI + P_VERSION, FIRERENDERMTLVER_LATEST, 0,
    //rollout
	IDD_FIRERENDER_BLENDMTL, IDS_FR_MTL_BLEND, 0, 0, NULL,

	FRBlendMtl_COLOR0, _T("material1"), TYPE_MTL, P_OWNERS_REF, IDS_FR_MTL_BLEND_MATERIAL1,
	p_refno, SUB1_REF,
	p_submtlno, 0,
	p_ui, TYPE_MTLBUTTON, IDC_BLEND_MAT0, PB_END,

	FRBlendMtl_COLOR1, _T("material2"), TYPE_MTL, P_OWNERS_REF, IDS_FR_MTL_BLEND_MATERIAL2,
	p_refno, SUB2_REF,
	p_submtlno, 1,
	p_ui, TYPE_MTLBUTTON, IDC_BLEND_MAT1, PB_END,

	FRBlendMtl_WEIGHT, _T("weight"), TYPE_FLOAT, P_ANIMATABLE, 0,
	p_default, 0.5f,
	p_range, 0.f, 1.f,
	p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_BLEND_WEIGHT, IDC_BLEND_WEIGHT_S, SPIN_AUTOSCALE, PB_END,

	FRBlendMtl_WEIGHT_TEXMAP, _T("weightMap"), TYPE_TEXMAP, 0, 0,
	p_refno, SUB3_REF,
	p_subtexno, 0, p_ui, TYPE_TEXMAPBUTTON, IDC_BLEND_WEIGHT_TEXMAP, PB_END,

    PB_END
    );

std::map<int, std::pair<ParamID, MCHAR*>> FRMTLCLASSNAME(BlendMtl)::TEXMAP_MAPPING = {
	{ 0, { FRBlendMtl_WEIGHT_TEXMAP, _T("Weight Map") } }
};

FRMTLCLASSNAME(BlendMtl)::~FRMTLCLASSNAME(BlendMtl)()
{
}


frw::Shader FRMTLCLASSNAME(BlendMtl)::getShader(const TimeValue t, MaterialParser& mtlParser, INode* node)
{
	auto ms = mtlParser.materialSystem;

	Mtl* material1 = GetFromPb<Mtl*>(pblock, FRBlendMtl_COLOR0);
	Mtl* material2 = GetFromPb<Mtl*>(pblock, FRBlendMtl_COLOR1);
	float weight = GetFromPb<float>(pblock, FRBlendMtl_WEIGHT);
	Texmap *weightMap = GetFromPb<Texmap*>(pblock, FRBlendMtl_WEIGHT_TEXMAP);
		
	if (!material1 || !material2)
		return frw::DiffuseShader(mtlParser.materialSystem);

	auto mat0 = mtlParser.createShader(material1, node);
	auto mat1 = mtlParser.createShader(material2, node);
	
	if (!mat0 || !mat1)
		return frw::DiffuseShader(mtlParser.materialSystem);

	frw::Value weightv(weight, weight, weight, 1.0);
	if (weightMap)
		weightv = mtlParser.createMap(weightMap, MAP_FLAG_NOGAMMA);
		
	return mtlParser.materialSystem.ShaderBlend(mat0, mat1, weightv);
}

void FRMTLCLASSNAME(BlendMtl)::SetSubMtl(int i, Mtl *m)
{
	ReplaceReference(i + 1, m);
	if (i == 0)
	{
		pbDesc.InvalidateUI(FRBlendMtl_COLOR0);
	}
	else if (i == 1)
	{
		pbDesc.InvalidateUI(FRBlendMtl_COLOR1);
	}

}

RefTargetHandle FRMTLCLASSNAME(BlendMtl)::GetReference(int i)
{
	switch (i) {
		case 0: return pblock;
		case 1: return sub1;
		case 2: return sub2;
		case 3: return sub3;
		default: return NULL;
	}
}

void FRMTLCLASSNAME(BlendMtl)::SetReference(int i, RefTargetHandle rtarg)
{
	switch (i) {
		case 0: pblock = (IParamBlock2*)rtarg; break;
		case 1: sub1 = (Mtl*)rtarg; break;
		case 2: sub2 = (Mtl*)rtarg; break;
		case 3: sub3 = (Texmap*)rtarg; break;
	}
}

FIRERENDER_NAMESPACE_END;
