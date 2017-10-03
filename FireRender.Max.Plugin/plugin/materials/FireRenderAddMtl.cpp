/*********************************************************************************************************************************
* Radeon ProRender for 3ds Max plugin
* Copyright (c) 2017 AMD
* All Rights Reserved
*
* Radeon ProRender dedicated custom material plugin
*********************************************************************************************************************************/
#include "FireRenderAddMtl.h"
#include "Resource.h"
#include "parser\MaterialParser.h"
#include "maxscript\mxsplugin\mxsPlugin.h"

FIRERENDER_NAMESPACE_BEGIN;

IMPLEMENT_FRMTLCLASSDESC(AddMtl)

FRMTLCLASSDESCNAME(AddMtl) FRMTLCLASSNAME(AddMtl)::ClassDescInstance;

// All parameters of the material plugin. See FIRE_MAX_PBDESC definition for notes on backwards compatibility
static ParamBlockDesc2 pbDesc(
	0, _T("AddMtlPbdesc"), 0, &FRMTLCLASSNAME(AddMtl)::ClassDescInstance, P_AUTO_CONSTRUCT + P_AUTO_UI + P_VERSION, FIRERENDERMTLVER_LATEST, 0,
    //rollout
	IDD_FIRERENDER_ADDMTL, IDS_FR_MTL_ADD, 0, 0, NULL,

	FRAddMtl_COLOR0, _T("material1"), TYPE_MTL, P_OWNERS_REF, IDS_FR_MTL_BLEND_MATERIAL1,
	p_refno, SUB1_REF,
	p_submtlno, 0,
	p_ui, TYPE_MTLBUTTON, IDC_BLEND_MAT0, PB_END,

	FRAddMtl_COLOR1, _T("material2"), TYPE_MTL, P_OWNERS_REF, IDS_FR_MTL_BLEND_MATERIAL2,
	p_refno, SUB2_REF,
	p_submtlno, 1,
	p_ui, TYPE_MTLBUTTON, IDC_BLEND_MAT1, PB_END,
    
	PB_END
    );

std::map<int, std::pair<ParamID, MCHAR*>> FireRenderAddMtl::TEXMAP_MAPPING;

FireRenderAddMtl::~FireRenderAddMtl() = default;

frw::Shader FireRenderAddMtl::getShader(const TimeValue t, MaterialParser& mtlParser, INode* node)
{
	auto ms = mtlParser.materialSystem;

	Mtl* material1 = GetFromPb<Mtl*>(pblock, FRAddMtl_COLOR0);
	Mtl* material2 = GetFromPb<Mtl*>(pblock, FRAddMtl_COLOR1);
			
	if (!material1 || !material2)
		return frw::DiffuseShader(mtlParser.materialSystem);
	
	auto mat0 = mtlParser.createShader(material1, node);
	auto mat1 = mtlParser.createShader(material2, node);

	if (!mat0 || !mat1)
		return frw::DiffuseShader(mtlParser.materialSystem);
		
	return mtlParser.materialSystem.ShaderAdd(mat0, mat1);
}

void FireRenderAddMtl::SetSubMtl(int i, Mtl *m)
{
	ReplaceReference(i + 1, m);
	if (i == 0)
	{
		pbDesc.InvalidateUI(FRAddMtl_COLOR0);
	}
	else if (i == 1)
	{
		pbDesc.InvalidateUI(FRAddMtl_COLOR1);
	}

}

RefTargetHandle FireRenderAddMtl::GetReference(int i)
{
	switch (i) {
		case 0: return pblock;
		case 1: return sub1;
		case 2: return sub2;
		default: return NULL;
	}
}

void FireRenderAddMtl::SetReference(int i, RefTargetHandle rtarg)
{
	switch (i) {
		case 0: pblock = (IParamBlock2*)rtarg; break;
		case 1: sub1 = (Mtl*)rtarg; break;
		case 2: sub2 = (Mtl*)rtarg; break;
	}
}

FIRERENDER_NAMESPACE_END;
