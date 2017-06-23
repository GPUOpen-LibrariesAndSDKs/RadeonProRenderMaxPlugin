/*********************************************************************************************************************************
* Radeon ProRender for 3ds Max plugin
* Copyright (c) 2017 AMD
* All Rights Reserved
*
* Radeon ProRender dedicated custom material plugin
*********************************************************************************************************************************/
#include "FireRenderAvgMtl.h"
#include "Resource.h"
#include "parser\MaterialParser.h"
#include "maxscript\mxsplugin\mxsPlugin.h"
#include "FireRenderDiffuseMtl.h"

FIRERENDER_NAMESPACE_BEGIN;

IMPLEMENT_FRMTLCLASSDESC(AvgMtl)

FRMTLCLASSDESCNAME(AvgMtl) FRMTLCLASSNAME(AvgMtl)::ClassDescInstance;

// All parameters of the material plugin. See FIRE_MAX_PBDESC definition for notes on backwards compatibility
static ParamBlockDesc2 pbDesc(
	0, _T("AvgMtlPbdesc"), 0, &FRMTLCLASSNAME(AvgMtl)::ClassDescInstance, P_AUTO_CONSTRUCT + P_AUTO_UI + P_VERSION, FIRERENDERMTLVER_LATEST, 0,
    //rollout
	IDD_FIRERENDER_COLORMTL, IDS_FR_MTLAVG, 0, 0, NULL,

	FRAvgMtl_COLOR, _T("Color"), TYPE_RGBA, P_ANIMATABLE, 0,
	p_default, Color(0.5f, 0.5f, 0.5f), p_ui, TYPE_COLORSWATCH, IDC_COL_COLOR, PB_END,

	FRAvgMtl_COLOR_TEXMAP, _T("colorTexmap"), TYPE_TEXMAP, 0, 0,
	p_subtexno, FRAvgMtlMtl_TEXMAP_COLOR, p_ui, TYPE_TEXMAPBUTTON, IDC_COL_COLOR_TEXMAP, PB_END,

    PB_END
    );

std::map<int, std::pair<ParamID, MCHAR*>> FRMTLCLASSNAME(AvgMtl)::TEXMAP_MAPPING = {
	{ FRAvgMtlMtl_TEXMAP_COLOR, { FRAvgMtl_COLOR_TEXMAP, _T("Color Map") } }
};

FRMTLCLASSNAME(AvgMtl)::~FRMTLCLASSNAME(AvgMtl)()
{
}


frw::Value FRMTLCLASSNAME(AvgMtl)::getShader(const TimeValue t, MaterialParser& mtlParser)
{
	auto ms = mtlParser.materialSystem;
		
	const Color color = GetFromPb<Color>(pblock, FRAvgMtl_COLOR);
	Texmap* colorTexmap = GetFromPb<Texmap*>(pblock, FRAvgMtl_COLOR_TEXMAP);

	frw::Value colorv(color.r, color.g, color.b);
	if (colorTexmap)
		colorv = mtlParser.createMap(colorTexmap, 0);

	return mtlParser.materialSystem.ValueDot(colorv, 1.0 / 3);
}

void FRMTLCLASSNAME(AvgMtl)::Update(TimeValue t, Interval& valid) {
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
