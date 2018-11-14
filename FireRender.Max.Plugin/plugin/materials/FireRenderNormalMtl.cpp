/*********************************************************************************************************************************
* Radeon ProRender for 3ds Max plugin
* Copyright (c) 2017 AMD
* All Rights Reserved
*
* Radeon ProRender dedicated custom material plugin
*********************************************************************************************************************************/
#include "FireRenderNormalMtl.h"
#include "Resource.h"
#include "parser\MaterialParser.h"
#include "ParamBlock.h"

#include <maxscript\mxsplugin\mxsPlugin.h>

FIRERENDER_NAMESPACE_BEGIN

IMPLEMENT_FRMTLCLASSDESC(NormalMtl)

FRMTLCLASSDESCNAME(NormalMtl) FRMTLCLASSNAME(NormalMtl)::ClassDescInstance;

// All parameters of the material plugin. See FIRE_MAX_PBDESC definition for notes on backwards compatibility
static ParamBlockDesc2 pbDesc(
	0, _T("NormalMtlPbdesc"), 0, &FRMTLCLASSNAME(NormalMtl)::ClassDescInstance, P_AUTO_CONSTRUCT + P_AUTO_UI + P_VERSION, FIRERENDERMTLVER_LATEST, 0,
	
	//rollout
	IDD_FIRERENDER_NORMALMTL, IDS_FR_MTL_NORMAL, 0, 0, NULL,

	FRNormalMtl_COLOR_TEXMAP, _T("Map"), TYPE_TEXMAP, 0, 0,
	p_subtexno, FRNormalMtl_TEXMAP_COLOR,
	p_ui, TYPE_TEXMAPBUTTON, IDC_NORMAL_TEXMAP,
	PB_END,

	FRNormalMtl_STRENGTH, _T("strength"), TYPE_FLOAT, P_ANIMATABLE, 0,
	p_default, 1.0f,
	p_range, -999.f, 999.f,
	p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_NORMAL_STRENGTH, IDC_NORMAL_STRENGTH_S, SPIN_AUTOSCALE,
	PB_END,

	FRNormalMtl_ISBUMP, _T("isBump"), TYPE_BOOL, P_ANIMATABLE, 0,
	p_default, FALSE,
	p_ui, TYPE_SINGLECHEKBOX, IDC_NORMAL_ISBUMP,
	PB_END,

	FRNormalMtl_ADDITIONALBUMP, _T("AdditionalBump"), TYPE_TEXMAP, P_OBSOLETE, 0,
	p_subtexno, FRNormalMtl_TEXMAP_ADDITIONALBUMP,
	p_ui, TYPE_TEXMAPBUTTON, IDC_NORMAL_ADDITIONALBUMP_TEXMAP,
	PB_END,

	FRNormalMtl_SWAPRG, _T("swapRG"), TYPE_BOOL, P_OBSOLETE | P_ANIMATABLE, 0,
	p_default, FALSE,
	p_ui, TYPE_SINGLECHEKBOX, IDC_NORMAL_SWAPRG,
	PB_END,

	FRNormalMtl_INVERTR, _T("invertR"), TYPE_BOOL, P_OBSOLETE | P_ANIMATABLE, 0,
	p_default, FALSE,
	p_ui, TYPE_SINGLECHEKBOX, IDC_NORMAL_INVERTR,
	PB_END,

	FRNormalMtl_INVERTG, _T("invertG"), TYPE_BOOL, P_OBSOLETE | P_ANIMATABLE, 0,
	p_default, FALSE,
	p_ui, TYPE_SINGLECHEKBOX, IDC_NORMAL_INVERTG,
	PB_END,

	FRNormalMtl_BUMPSTRENGTH, _T("bumpStrength"), TYPE_FLOAT, P_OBSOLETE | P_ANIMATABLE, 0,
	p_default, 1.0f,
	p_range, -999.f, 999.f,
	p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_BUMP_STRENGTH, IDC_BUMP_STRENGTH_S, SPIN_AUTOSCALE,
	PB_END,

	PB_END
);

std::map<int, std::pair<ParamID, MCHAR*>> FRMTLCLASSNAME(NormalMtl)::TEXMAP_MAPPING =
{
	{ FRNormalMtl_TEXMAP_COLOR,{ FRNormalMtl_COLOR_TEXMAP, _T("Map") } },
	//{ FRNormalMtl_TEXMAP_ADDITIONALBUMP,{ FRNormalMtl_ADDITIONALBUMP, _T("Additional Bump") } }
};

FRMTLCLASSNAME(NormalMtl)::~FRMTLCLASSNAME(NormalMtl)()
{
}

frw::Value FRMTLCLASSNAME(NormalMtl)::translateGenericBump(const TimeValue t, Texmap* bumpMap, const float& strength, MaterialParser& mtlParser)
{
	if (bumpMap)
	{
		if (bumpMap->ClassID() == FIRERENDER_NORMALMTL_CID)
		{
			return dynamic_cast<FRMTLCLASSNAME(NormalMtl)*>(bumpMap)->getShader(t, mtlParser);
		}

		frw::Value normalMap = mtlParser.createMap(bumpMap, MAP_FLAG_NORMALMAP);

		// create the actual shader node
		/*if (normalMap)
		{
			frw::BumpMapNode node(mtlParser.materialSystem);

			node.SetMap(normalMap);
			node.SetValue("bumpscale", frw::Value(strength));

			return node;
		}*/
		return normalMap;
	}

	return mtlParser.materialSystem.ValueLookupN();
}

frw::Value FRMTLCLASSNAME(NormalMtl)::getShader(const TimeValue t, MaterialParser& mtlParser)
{
	frw::MaterialSystem ms = mtlParser.materialSystem;

	Texmap* texmap = GetFromPb<Texmap*>(pblock, FRNormalMtl_COLOR_TEXMAP);

	if (texmap)
	{
		float weight = GetFromPb<float>(pblock, FRNormalMtl_STRENGTH);
		BOOL isBump = GetFromPb<BOOL>(pblock, FRNormalMtl_ISBUMP);

		// get the main normal map
		frw::Value normalMap = mtlParser.createMap(texmap, MAP_FLAG_NORMALMAP);

		// create the actual shader node
		frw::ValueNode res;

		if (isBump)
		{
			frw::BumpMapNode node(ms);
			node.SetMap(normalMap);
			res = node;
		}
		else
		{
			frw::NormalMapNode node(ms);
			node.SetMap(normalMap);
			res = node;
		}

		//res.SetValue("bumpscale", frw::Value(weight));

		return res;
	}

	return ms.ValueLookupN();
}

void FRMTLCLASSNAME(NormalMtl)::Update(TimeValue t, Interval& valid)
{
	for (int i = 0; i < NumSubTexmaps(); ++i)
	{
		// we are required to recursively call Update on all our submaps
		Texmap* map = GetSubTexmap(i);

		if (map != NULL)
			map->Update(t, valid);
	}

	this->pblock->GetValidity(t, valid);
}

FIRERENDER_NAMESPACE_END
