/*********************************************************************************************************************************
* Radeon ProRender for 3ds Max plugin
* Copyright (c) 2017 AMD
* All Rights Reserved
*
* Radeon ProRender dedicated custom material plugin
*********************************************************************************************************************************/
#include "FireRenderMaterialMtl.h"
#include "Resource.h"
#include "parser\MaterialParser.h"
#include "maxscript\mxsplugin\mxsPlugin.h"
#include "FireRenderDiffuseMtl.h"

FIRERENDER_NAMESPACE_BEGIN;

IMPLEMENT_FRMTLCLASSDESC(MaterialMtl)

FRMTLCLASSDESCNAME(MaterialMtl) FRMTLCLASSNAME(MaterialMtl)::ClassDescInstance;

// All parameters of the material plugin. See FIRE_MAX_PBDESC definition for notes on backwards compatibility
static ParamBlockDesc2 pbDesc(
	0, _T("MaterialMtlPbdesc"), 0, &FRMTLCLASSNAME(MaterialMtl)::ClassDescInstance, P_AUTO_CONSTRUCT + P_AUTO_UI + P_VERSION, FIRERENDERMTLVER_LATEST, 0,
    //rollout
	IDD_FIRERENDER_MATERIALMTL, IDS_FR_MTL_MATERIAL, 0, 0, NULL,

	FRMaterialMtl_SURFACE, _T("surface"), TYPE_MTL, P_OWNERS_REF, IDS_FR_MTL_MATERIAL_SURFACE,
	p_refno, SUB1_REF,
	p_submtlno, 0,
	p_ui, TYPE_MTLBUTTON, IDC_MATERIAL_SURFACE, PB_END,

	FRMaterialMtl_VOLUME, _T("volume"), TYPE_MTL, P_OWNERS_REF, IDS_FR_MTL_MATERIAL_VOLUME,
	p_refno, SUB2_REF,
	p_submtlno, 1,
	p_ui, TYPE_MTLBUTTON, IDC_MATERIAL_VOLUME, PB_END,
	
	FRMaterialMtl_DISPLACEMENT, _T("displacement"), TYPE_TEXMAP, 0, 0,
	p_refno, SUB3_REF,
	p_subtexno, 0, p_ui, TYPE_TEXMAPBUTTON, IDC_MATERIAL_DISPLACEMENT, PB_END,
	
	FRMaterialMtl_CAUSTICS, _T("caustics"), TYPE_BOOL, P_ANIMATABLE, 0,
	p_default, TRUE, p_ui, TYPE_SINGLECHEKBOX, IDC_MATERIAL_CAUSTICS, PB_END,

	FRMaterialMtl_SHADOWCATCHER, _T("shadowCatcher"), TYPE_BOOL, P_ANIMATABLE, 0,
	p_default, FALSE, p_ui, TYPE_SINGLECHEKBOX, IDC_MATERIAL_SHADOWCATCHER, PB_END,

    PB_END
    );

std::map<int, std::pair<ParamID, MCHAR*>> FRMTLCLASSNAME(MaterialMtl)::TEXMAP_MAPPING = {
	{ 0,{ FRMaterialMtl_DISPLACEMENT, _T("Displacement") } }
};

FRMTLCLASSNAME(MaterialMtl)::~FRMTLCLASSNAME(MaterialMtl)()
{
}

frw::Shader FRMTLCLASSNAME(MaterialMtl)::getVolumeShader(const TimeValue t, MaterialParser& mtlParser, INode* node)
{
    Mtl* volume = GetFromPb<Mtl*>(pblock, FRMaterialMtl_VOLUME);
    if (!volume)
		return frw::Shader();

    return mtlParser.createVolumeShader(volume, node);
}

frw::Shader FRMTLCLASSNAME(MaterialMtl)::getShader(const TimeValue t, MaterialParser& mtlParser, INode* node)
{
	auto ms = mtlParser.materialSystem;
	
	Mtl* surface = GetFromPb<Mtl*>(pblock, FRMaterialMtl_SURFACE);

	if (!surface)
	{
		frw::DiffuseShader diffuseShader = frw::DiffuseShader(ms);
		diffuseShader.SetColor(frw::Value(0.5));
		return diffuseShader;
	}
		
	
	auto mat = mtlParser.createShader(surface, node);

	if (!mat)
	{
		frw::DiffuseShader diffuseShader = frw::DiffuseShader(ms);
		diffuseShader.SetColor(frw::Value(0.5));
		return diffuseShader;
	}

	return mat;
}

void FRMTLCLASSNAME(MaterialMtl)::SetSubMtl(int i, Mtl *m)
{
	ReplaceReference(i + 1, m);
	if (i == 0)
	{
		pbDesc.InvalidateUI(FRMaterialMtl_SURFACE);
	}
	else if (i == 1)
	{
		pbDesc.InvalidateUI(FRMaterialMtl_VOLUME);
	}
}

RefTargetHandle FRMTLCLASSNAME(MaterialMtl)::GetReference(int i)
{
	switch (i) {
		case 0: return pblock;
		case 1: return sub1;
		case 2: return sub2;
		case 3: return sub3;
	}
	return NULL;
}

void FRMTLCLASSNAME(MaterialMtl)::SetReference(int i, RefTargetHandle rtarg)
{
	switch (i) {
		case 0: pblock = (IParamBlock2*)rtarg; break;
		case 1: sub1 = (Mtl*)rtarg; break;
		case 2: sub2 = (Mtl*)rtarg; break;
		case 3: sub3 = (Texmap*)rtarg; break;
	}
}

FIRERENDER_NAMESPACE_END;
