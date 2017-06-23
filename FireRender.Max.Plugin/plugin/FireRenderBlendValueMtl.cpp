/*********************************************************************************************************************************
* Radeon ProRender for 3ds Max plugin
* Copyright (c) 2017 AMD
* All Rights Reserved
*
* Radeon ProRender dedicated custom material plugin
*********************************************************************************************************************************/
#include "FireRenderBlendValueMtl.h"
#include "Resource.h"
#include "parser\MaterialParser.h"
#include "maxscript\mxsplugin\mxsPlugin.h"

FIRERENDER_NAMESPACE_BEGIN;

IMPLEMENT_FRMTLCLASSDESC(BlendValueMtl)

FRMTLCLASSDESCNAME(BlendValueMtl) FRMTLCLASSNAME(BlendValueMtl)::ClassDescInstance;

// All parameters of the material plugin. See FIRE_MAX_PBDESC definition for notes on backwards compatibility
static ParamBlockDesc2 pbDesc(
	0, _T("BlendValueMtlPbdesc"), 0, &FRMTLCLASSNAME(BlendValueMtl)::ClassDescInstance, P_AUTO_CONSTRUCT + P_AUTO_UI + P_VERSION, FIRERENDERMTLVER_LATEST, 0,
    //rollout
	IDD_FIRERENDER_BLENDVALUEMTL, IDS_FR_MTL_BLENDVALUE, 0, 0, NULL,

	FRBlendValueMtl_COLOR0, _T("Color1"), TYPE_RGBA, P_ANIMATABLE, 0,
	p_default, Color(0.5f, 0.5f, 0.5f), p_ui, TYPE_COLORSWATCH, IDC_BV_COLOR0, PB_END,

	FRBlendValueMtl_COLOR1, _T("Color2"), TYPE_RGBA, P_ANIMATABLE, 0,
	p_default, Color(0.5f, 0.5f, 0.5f), p_ui, TYPE_COLORSWATCH, IDC_BV_COLOR1, PB_END,

	FRBlendValueMtl_WEIGHT, _T("Weight"), TYPE_FLOAT, P_ANIMATABLE, 0,
	p_default, 0.5f, p_range, 0.f, 1.f, p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_BV_WEIGHT, IDC_BV_WEIGHT_S, SPIN_AUTOSCALE, PB_END,

	FRBlendValueMtl_COLOR0_TEXMAP, _T("color0Texmap"), TYPE_TEXMAP, 0, 0,
	p_subtexno, FRBlendValueMtl_TEXMAP_COLOR0, p_ui, TYPE_TEXMAPBUTTON, IDC_BV_COLOR0_TEXMAP, PB_END,

	FRBlendValueMtl_COLOR1_TEXMAP, _T("color1Texmap"), TYPE_TEXMAP, 0, 0,
	p_subtexno, FRBlendValueMtl_TEXMAP_COLOR1, p_ui, TYPE_TEXMAPBUTTON, IDC_BV_COLOR1_TEXMAP, PB_END,

	FRBlendValueMtl_WEIGHT_TEXMAP, _T("weightTexmap"), TYPE_TEXMAP, 0, 0,
	p_subtexno, FRBlendValueMtl_TEXMAP_WEIGHT, p_ui, TYPE_TEXMAPBUTTON, IDC_BV_WEIGHT_TEXMAP, PB_END,

    PB_END
    );

std::map<int, std::pair<ParamID, MCHAR*>> FRMTLCLASSNAME(BlendValueMtl)::TEXMAP_MAPPING = {
	{ FRBlendValueMtl_TEXMAP_COLOR0, { FRBlendValueMtl_COLOR0_TEXMAP, _T("Color 1 map") } },
	{ FRBlendValueMtl_TEXMAP_COLOR1, { FRBlendValueMtl_COLOR1_TEXMAP, _T("Color 2 map") } },
	{ FRBlendValueMtl_TEXMAP_WEIGHT, { FRBlendValueMtl_WEIGHT_TEXMAP, _T("Weight map") } }
};

FRMTLCLASSNAME(BlendValueMtl)::~FRMTLCLASSNAME(BlendValueMtl)()
{
}


frw::Value FRMTLCLASSNAME(BlendValueMtl)::getShader(const TimeValue t, MaterialParser& mtlParser)
{
	auto ms = mtlParser.materialSystem;
		
	const Color color0 = GetFromPb<Color>(pblock, FRBlendValueMtl_COLOR0);
	const Color color1 = GetFromPb<Color>(pblock, FRBlendValueMtl_COLOR1);
	const float weight = GetFromPb<float>(pblock, FRBlendValueMtl_WEIGHT);
	Texmap* color0Texmap = GetFromPb<Texmap*>(pblock, FRBlendValueMtl_COLOR0_TEXMAP);
	Texmap* color1Texmap = GetFromPb<Texmap*>(pblock, FRBlendValueMtl_COLOR1_TEXMAP);
	Texmap* weightTexmap = GetFromPb<Texmap*>(pblock, FRBlendValueMtl_WEIGHT_TEXMAP);
	
	frw::Value color0v(color0.r, color0.g, color0.b);
	if (color0Texmap)
		color0v = mtlParser.createMap(color0Texmap, 0);

	frw::Value color1v(color1.r, color1.g, color1.b);
	if (color1Texmap)
		color1v = mtlParser.createMap(color1Texmap, 0);

	frw::Value weightv(weight, weight, weight);
	if (weightTexmap)
		weightv = mtlParser.createMap(weightTexmap, MAP_FLAG_NOGAMMA);

	return mtlParser.materialSystem.ValueBlend(color0v, color1v, weightv);
}

void FRMTLCLASSNAME(BlendValueMtl)::Update(TimeValue t, Interval& valid) {
    for (int i = 0; i < NumSubTexmaps(); ++i) {
        // we are required to recursively call Update on all our submaps
        Texmap* map = GetSubTexmap(i);
        if (map != NULL) {
            map->Update(t, valid);
        }
    }
    this->pblock->GetValidity(t, valid);
}

FIRERENDER_NAMESPACE_END;
