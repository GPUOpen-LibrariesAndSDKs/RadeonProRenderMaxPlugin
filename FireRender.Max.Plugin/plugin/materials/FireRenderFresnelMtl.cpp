/*********************************************************************************************************************************
* Radeon ProRender for 3ds Max plugin
* Copyright (c) 2017 AMD
* All Rights Reserved
*
* Radeon ProRender dedicated custom material plugin
*********************************************************************************************************************************/
#include "FireRenderFresnelMtl.h"
#include "Resource.h"
#include "parser\MaterialParser.h"
#include "maxscript\mxsplugin\mxsPlugin.h"

FIRERENDER_NAMESPACE_BEGIN;

// All parameters of the material plugin. See FIRE_MAX_PBDESC definition for notes on backwards compatibility
static ParamBlockDesc2 pbDesc(
	0, _T("FresnelMtlPbdesc"), 0, &FireRenderFresnelMtl::GetClassDesc(), P_AUTO_CONSTRUCT + P_AUTO_UI + P_VERSION, FIRERENDERMTLVER_LATEST, 0,
    //rollout
	IDD_FIRERENDER_FRESNELMTL, IDS_FR_MTL_FRESNEL, 0, 0, NULL,

	FRFresnelMtl_IOR, _T("Ior"), TYPE_FLOAT, P_ANIMATABLE, 0,
	p_default, 1.52f, p_range, 1.0f, 999.f, p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_FRESNEL_IOR, IDC_FRESNEL_IOR_S, SPIN_AUTOSCALE, PB_END,

	FRFresnelMtl_IOR_TEXMAP, _T("iorTexmap"), TYPE_TEXMAP, 0, 0,
	p_subtexno, FRFresnelMtl_TEXMAP_IOR, p_ui, TYPE_TEXMAPBUTTON, IDC_FRESNEL_IOR_TEXMAP, PB_END,

	FRFresnelMtl_INVEC_TEXMAP, _T("invecTexmap"), TYPE_TEXMAP, 0, 0,
	p_subtexno, FRFresnelMtl_TEXMAP_INVEC, p_ui, TYPE_TEXMAPBUTTON, IDC_FRESNEL_INVEC_TEXMAP, PB_END,

	FRFresnelMtl_N_TEXMAP, _T("nTexmap"), TYPE_TEXMAP, 0, 0,
	p_subtexno, FRFresnelMtl_TEXMAP_N, p_ui, TYPE_TEXMAPBUTTON, IDC_FRESNEL_N_TEXMAP, PB_END,

    PB_END
    );

std::map<int, std::pair<ParamID, MCHAR*>> FireRenderFresnelMtl::TEXMAP_MAPPING = {
	{ FRFresnelMtl_TEXMAP_INVEC,{ FRFresnelMtl_INVEC_TEXMAP, _T("Invec map") } },
	{ FRFresnelMtl_TEXMAP_N,{ FRFresnelMtl_N_TEXMAP, _T("N map") } },
	{ FRFresnelMtl_TEXMAP_IOR,{ FRFresnelMtl_IOR_TEXMAP, _T("Ior map") } }
};


frw::Value FireRenderFresnelMtl::GetShader(const TimeValue t, MaterialParser& mtlParser)
{
	const float ior = GetFromPb<float>(pblock, FRFresnelMtl_IOR);
	Texmap* invecMap = GetFromPb<Texmap*>(pblock, FRFresnelMtl_INVEC_TEXMAP);
	Texmap* iorMap = GetFromPb<Texmap*>(pblock, FRFresnelMtl_IOR_TEXMAP);
	Texmap* nMap = GetFromPb<Texmap*>(pblock, FRFresnelMtl_N_TEXMAP);

	frw::Value iorValue(ior);
	if (iorMap)
		iorValue = mtlParser.createMap(iorMap, MAP_FLAG_NOGAMMA);

	auto nValue = mtlParser.createMap(nMap, MAP_FLAG_NOGAMMA);
	auto invecValue = mtlParser.createMap(invecMap, MAP_FLAG_NOGAMMA);
	
	return mtlParser.materialSystem.ValueFresnel(iorValue, nValue, invecValue);
}

void FireRenderFresnelMtl::Update(TimeValue t, Interval& valid) {
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
