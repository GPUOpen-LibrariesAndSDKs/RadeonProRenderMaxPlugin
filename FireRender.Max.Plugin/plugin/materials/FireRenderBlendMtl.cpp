/*********************************************************************************************************************************
* Radeon ProRender for 3ds Max plugin
* Copyright (c) 2017 AMD
* All Rights Reserved
*
* Radeon ProRender dedicated custom material plugin
*********************************************************************************************************************************/
#include "FireRenderBlendMtl.h"
#include "Resource.h"
#include "parser\MaterialParser.h"
#include "maxscript\mxsplugin\mxsPlugin.h"
#include "FireRenderDiffuseMtl.h"

FIRERENDER_NAMESPACE_BEGIN;

IMPLEMENT_FRMTLCLASSDESC(BlendMtl)

FRMTLCLASSDESCNAME(BlendMtl) FireRenderBlendMtl::ClassDescInstance;

// All parameters of the material plugin. See FIRE_MAX_PBDESC definition for notes on backwards compatibility
static ParamBlockDesc2 pbDesc(
	0, _T("BlendMtlPbdesc"), 0, &FireRenderBlendMtl::ClassDescInstance, P_AUTO_CONSTRUCT + P_AUTO_UI + P_VERSION, FIRERENDERMTLVER_LATEST, 0,
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

std::map<int, std::pair<ParamID, MCHAR*>> FireRenderBlendMtl::TEXMAP_MAPPING = {
	{ 0, { FRBlendMtl_WEIGHT_TEXMAP, _T("Weight Map") } }
};

frw::Shader FireRenderBlendMtl::getShader(const TimeValue t, MaterialParser& mtlParser, INode* node)
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

void FireRenderBlendMtl::SetSubMtl(int i, Mtl *m)
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

RefTargetHandle FireRenderBlendMtl::GetReference(int i)
{
	switch (i) {
		case 0: return pblock;
		case 1: return sub1;
		case 2: return sub2;
		case 3: return sub3;
		default: return NULL;
	}
}

Color FireRenderBlendMtl::GetDiffuse(int mtlNum, BOOL backFace)
{
	Mtl* m0 = GetFromPb<Mtl*>(pblock, FRBlendMtl_COLOR0);
	Mtl* m1 = GetFromPb<Mtl*>(pblock, FRBlendMtl_COLOR1);
	if (m0 && m1)
	{
		Color c0 = m0->GetDiffuse(0, backFace);
		Color c1 = m1->GetDiffuse(0, backFace);
		float w = GetFromPb<float>(pblock, FRBlendMtl_WEIGHT);
		return ((c0 * w) + (c1 * (1.f - w)));
	}
	if (m0 && !m1)
		return m0->GetDiffuse(0, backFace);
	if (m1 && !m0)
		return m1->GetDiffuse(0, backFace);
	return Color(0.8, 0.8, 0.8);
}

float FireRenderBlendMtl::GetXParency(int mtlNum, BOOL backFace)
{
	Mtl* m0 = GetFromPb<Mtl*>(pblock, FRBlendMtl_COLOR0);
	Mtl* m1 = GetFromPb<Mtl*>(pblock, FRBlendMtl_COLOR1);
	if (m0 && m1)
	{
		float t0 = m0->GetXParency(0, backFace);
		float t1 = m1->GetXParency(0, backFace);
		float w = GetFromPb<float>(pblock, FRBlendMtl_WEIGHT);
		return ((t0 * w) + (t1 * (1.f - w)));
	}
	if (m0 && !m1)
		return m0->GetXParency(0, backFace);
	if (m1 && !m0)
		return m1->GetXParency(0, backFace);
	return 0.f;
}

void FireRenderBlendMtl::SetReference(int i, RefTargetHandle rtarg)
{
	switch (i) {
		case 0: pblock = (IParamBlock2*)rtarg; break;
		case 1: sub1 = (Mtl*)rtarg; break;
		case 2: sub2 = (Mtl*)rtarg; break;
		case 3: sub3 = (Texmap*)rtarg; break;
	}
}

FIRERENDER_NAMESPACE_END;
