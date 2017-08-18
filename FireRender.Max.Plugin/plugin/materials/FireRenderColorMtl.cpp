/*********************************************************************************************************************************
* Radeon ProRender for 3ds Max plugin
* Copyright (c) 2017 AMD
* All Rights Reserved
*
* Radeon ProRender dedicated custom material plugin
*********************************************************************************************************************************/
#include "FireRenderColorMtl.h"
#include "Resource.h"
#include "parser\MaterialParser.h"
#include "maxscript\mxsplugin\mxsPlugin.h"

FIRERENDER_NAMESPACE_BEGIN;

IMPLEMENT_FRMTLCLASSDESC(ColorMtl)

FRMTLCLASSDESCNAME(ColorMtl) FRMTLCLASSNAME(ColorMtl)::ClassDescInstance;

// All parameters of the material plugin. See FIRE_MAX_PBDESC definition for notes on backwards compatibility
static ParamBlockDesc2 pbDesc(
	0, _T("ColorMtlPbdesc"), 0, &FRMTLCLASSNAME(ColorMtl)::ClassDescInstance, P_AUTO_CONSTRUCT + P_AUTO_UI + P_VERSION, FIRERENDERMTLVER_LATEST, 0,
    //rollout
	IDD_FIRERENDER_COLORMTL, IDS_FR_MTL_COLOR, 0, 0, NULL,

	FRColorMtl_COLOR, _T("Color"), TYPE_RGBA, P_ANIMATABLE, 0,
	p_default, Color(1.0f, 0.0f, 0.0f), p_ui, TYPE_COLORSWATCH, IDC_COL_COLOR, PB_END,

	FRColorMtl_COLOR_TEXMAP, _T("colorTexmap"), TYPE_TEXMAP, 0, 0,
	p_subtexno, FRColorMtl_TEXMAP_COLOR, p_ui, TYPE_TEXMAPBUTTON, IDC_COL_COLOR_TEXMAP, PB_END,

    PB_END
    );

std::map<int, std::pair<ParamID, MCHAR*>> FRMTLCLASSNAME(ColorMtl)::TEXMAP_MAPPING = {
	{ FRColorMtl_TEXMAP_COLOR, { FRColorMtl_COLOR_TEXMAP, _T("Color Map") } }
};

FRMTLCLASSNAME(ColorMtl)::~FRMTLCLASSNAME(ColorMtl)()
{
}


frw::Value FRMTLCLASSNAME(ColorMtl)::getShader(const TimeValue t, MaterialParser& mtlParser)
{
	auto ms = mtlParser.materialSystem;
		
	const Color diffuseColor = GetFromPb<Color>(pblock, FRColorMtl_COLOR);
	Texmap* colorTexmap = GetFromPb<Texmap*>(pblock, FRColorMtl_COLOR_TEXMAP);
	
	frw::Value color(diffuseColor.r, diffuseColor.g, diffuseColor.b);
	if (colorTexmap)
		color = mtlParser.createMap(colorTexmap, 0);

	return color;
}

void FRMTLCLASSNAME(ColorMtl)::Update(TimeValue t, Interval& valid) {
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
