/*********************************************************************************************************************************
* Radeon ProRender for 3ds Max plugin
* Copyright (c) 2017 AMD
* All Rights Reserved
*
* Radeon ProRender dedicated custom material plugin
*********************************************************************************************************************************/
#include "FireRenderDisplacementMtl.h"
#include "Resource.h"
#include "parser\MaterialParser.h"
#include "maxscript\mxsplugin\mxsPlugin.h"
#include "CoronaDeclarations.h"
#include "FireRenderMaterialMtl.h"
#include "FireRenderUberMtl.h"
#include "FireRenderUberMtlv2.h"
#include "FireRenderUberMtlv3.h"
#include "plugin/ParamBlock.h"
#include "utils/Utils.h"

FIRERENDER_NAMESPACE_BEGIN;

IMPLEMENT_FRMTLCLASSDESC(DisplacementMtl)

FRMTLCLASSDESCNAME(DisplacementMtl) FRMTLCLASSNAME(DisplacementMtl)::ClassDescInstance;

// All parameters of the material plugin. See FIRE_MAX_PBDESC definition for notes on backwards compatibility
static ParamBlockDesc2 pbDesc(
	0, _T("DisplacementMtlPbdesc"), 0, &FRMTLCLASSNAME(DisplacementMtl)::ClassDescInstance, P_AUTO_CONSTRUCT + P_AUTO_UI + P_VERSION, FIRERENDERMTLVER_LATEST, 0,
    //rollout
	IDD_FIRERENDER_DISPLACEMENTMTL, IDS_FR_MTL_DISPLACEMENT, 0, 0, NULL,

	FRDisplacementMtl_COLOR_TEXMAP, _T("Map"), TYPE_TEXMAP, 0, 0,
	p_subtexno, FRDisplacementMtl_TEXMAP_COLOR, p_ui, TYPE_TEXMAPBUTTON, IDC_DISPLACEMENT_TEXMAP, PB_END,

	FRDisplacementMtl_MINHEIGHT, _T("Min Height"), TYPE_FLOAT, P_ANIMATABLE, 0,
	p_default, 0.f, p_range, .0f, 9999999.f, p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_DISPLACEMENT_MINHEIGHT, IDC_DISPLACEMENT_MINHEIGHT_S, SPIN_AUTOSCALE, PB_END,

	FRDisplacementMtl_MAXHEIGHT, _T("Max Height"), TYPE_FLOAT, P_ANIMATABLE, 0,
	p_default, 0.01f, p_range, .0f, 9999999.f, p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_DISPLACEMENT_MAXHEIGHT, IDC_DISPLACEMENT_MAXHEIGHT_S, SPIN_AUTOSCALE, PB_END,

	FRDisplacementMtl_SUBDIVISION, _T("Subdivision"), TYPE_FLOAT, P_ANIMATABLE, 0,
	p_default, 1.f, p_range, 0.f, 9999999.f, p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_DISPLACEMENT_SUBDIVISION, IDC_DISPLACEMENT_SUBDIVISION_S, SPIN_AUTOSCALE, PB_END,
	
	FRDisplacementMtl_CREASEWEIGHT, _T("CreaseWeight"), TYPE_FLOAT, P_ANIMATABLE, 0,
	p_default, 0.f, p_range, 0.f, 9999999.f, p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_DISPLACEMENT_CREASEWEIGHT, IDC_DISPLACEMENT_CREASEWEIGHT_S, SPIN_AUTOSCALE, PB_END,

	FRDisplacementMtl_BOUNDARY, _T("Boundary"), TYPE_INT, P_ANIMATABLE, 0,
	p_default, RPR_SUBDIV_BOUNDARY_INTERFOP_TYPE_EDGE_AND_CORNER, p_ui, TYPE_INT_COMBOBOX, IDC_DISPLACEMENT_BOUNDARY, 2,
		IDS_SUBDIV_BOUNDARY_INTEROP_EDGE, IDS_SUBDIV_BOUNDARY_INTEROP_EDGE_AND_CORNER,
	p_vals, RPR_SUBDIV_BOUNDARY_INTERFOP_TYPE_EDGE_ONLY, RPR_SUBDIV_BOUNDARY_INTERFOP_TYPE_EDGE_AND_CORNER, PB_END,

    PB_END
    );

std::map<int, std::pair<ParamID, MCHAR*>> FRMTLCLASSNAME(DisplacementMtl)::TEXMAP_MAPPING = {
	{ FRDisplacementMtl_TEXMAP_COLOR,{ FRDisplacementMtl_COLOR_TEXMAP, _T("Map") } }
};

FRMTLCLASSNAME(DisplacementMtl)::~FRMTLCLASSNAME(DisplacementMtl)()
{
}


frw::Value FRMTLCLASSNAME(DisplacementMtl)::getShader(const TimeValue t, MaterialParser& mtlParser)
{
	Texmap* texmap = GetFromPb<Texmap*>(pblock, FRDisplacementMtl_COLOR_TEXMAP);

	frw::Value color(0.f, 0.f, 0.f);
	if (texmap)
		color = mtlParser.createMap(texmap, MAP_FLAG_NOGAMMA);

	return color;
}

void FRMTLCLASSNAME(DisplacementMtl)::Update(TimeValue t, Interval& valid) {
    for (int i = 0; i < NumSubTexmaps(); ++i) {
        // we are required to recursively call Update on all our submaps
        Texmap* map = GetSubTexmap(i);
        if (map != NULL) {
            map->Update(t, valid);
        }
    }
    this->pblock->GetValidity(t, valid);
}


Texmap *FRMTLCLASSNAME(DisplacementMtl)::findDisplacementMap(const TimeValue t, MaterialParser& mtlParser, Mtl* material,
	float &minHeight, float &maxHeight, float &subdivision, float &creaseWeight, int &boundary, bool &notAccurate)
{
	IParamBlock2* pb = material->GetParamBlock(0);

	Texmap *displacementMap = 0;

	if (material->ClassID() == Corona::MTL_CID)
	{
		if (GetFromPb<bool>(pb, Corona::MTLP_USEMAP_DISPLACE, t))
		{
			displacementMap = GetFromPb<Texmap*>(pb, Corona::MTLP_TEXMAP_DISPLACE, t);
			minHeight = GetFromPb<float>(pb, Corona::MTLP_DISPLACE_MIN, t);
			maxHeight = GetFromPb<float>(pb, Corona::MTLP_DISPLACE_MAX, t);
		}
	}
	else if (material->ClassID() == FIRERENDER_MATERIALMTL_CID)
	{
		displacementMap = GetFromPb<Texmap*>(pb, FRMaterialMtl_DISPLACEMENT, t);
	}
	else if (material->ClassID() == FIRERENDER_UBERMTL_CID)
	{
		displacementMap = GetFromPb<Texmap*>(pb, FRUBERMTL_DISPLACEMENT, t);
	}
	else if (material->ClassID() == FIRERENDER_UBERMTLV2_CID)
	{
		displacementMap = GetFromPb<Texmap*>(pb, FRUBERMTLV2_MAT_DISPLACE_MAP, t);
	}
	else if (material->ClassID() == FIRERENDER_UBERMTLV3_CID)
	{
		// TODO
		// displacementMap = GetFromPb<Texmap*>(pb, FRUBERMTLV3_XXXXX_DISPLACE_MAP, t);
	}
	else if (StdMat2 *stdmat = dynamic_cast<StdMat2*>(material))
	{
		if (stdmat->SubTexmapOn(stdmat->StdIDToChannel(ID_DP)))
		{
			displacementMap = stdmat->GetSubTexmap(ID_DP);
			maxHeight = stdmat->GetTexmapAmt(ID_DP, t);
		}
	}
	else if (material->ClassID() == PHYSICALMATERIAL_CID)
	{
		if (GetFromPb<BOOL>(pb, phm_displacement_map_on, t))
		{
			displacementMap = GetFromPb<Texmap*>(pb, phm_displacement_map, t);
			maxHeight = GetFromPb<float>(pb, phm_displacement_map_amt, t);
		}
	}
	
	if (displacementMap)
		return displacementMap;

	// crawl the shader graph
	int npb = material->NumParamBlocks();
	for (int j = 0; j < npb; j++)
	{
		if (auto pb = material->GetParamBlock(j))
		{
			auto pbd = pb->GetDesc();
			for (USHORT i = 0; i < pbd->count; i++)
			{
				ParamID id = pbd->IndextoID(i);
				ParamDef &pdef = pbd->GetParamDef(id);
				switch (pdef.type)
				{
					case TYPE_MTL:
					{
						Mtl *v = 0;
						pb->GetValue(id, t, v, FOREVER);
						if (v)
						{
							Texmap *res = findDisplacementMap(t, mtlParser, v, minHeight, maxHeight, subdivision, creaseWeight, boundary, notAccurate);
							if (res)
								return res;
						}
					}
					break;

					case TYPE_MTL_TAB:
					{
						int count = pb->Count(id);
						for (int i = 0; i < count; i++)
						{
							Mtl *v = 0;
							pb->GetValue(id, t, v, FOREVER, i);
							if (v)
							{
								Texmap *res = findDisplacementMap(t, mtlParser, v, minHeight, maxHeight, subdivision, creaseWeight, boundary, notAccurate);
								if (res)
									return res;
							}
						}
					}
					break;
				}
			}
		}
	}

	return 0;
}

frw::Value FRMTLCLASSNAME(DisplacementMtl)::translateDisplacement(const TimeValue t, MaterialParser& mtlParser, Mtl* material,
	float &minHeight, float &maxHeight, float &subdivision, float &creaseWeight, int &boundary, bool &notAccurate)
{
	if (!material)
		return frw::Value();

	minHeight = 0.f;
	maxHeight = 0.01f;
	subdivision = 1;
	creaseWeight = 0.f;
	notAccurate = true;
	boundary = RPR_SUBDIV_BOUNDARY_INTERFOP_TYPE_EDGE_AND_CORNER;

	Texmap *displacementMap = findDisplacementMap(t, mtlParser, material, minHeight, maxHeight, subdivision, creaseWeight, boundary, notAccurate);
	
	if (displacementMap)
	{
		Texmap *dmap = displacementMap;
		if (displacementMap->ClassID() == FIRERENDER_DISPLACEMENTMTL_CID)
		{
			IParamBlock2* pblock = displacementMap->GetParamBlock(0);
			dmap = GetFromPb<Texmap*>(pblock, FRDisplacementMtl_COLOR_TEXMAP);
			maxHeight = GetFromPb<float>(pblock, FRDisplacementMtl_MAXHEIGHT, t);
			minHeight = GetFromPb<float>(pblock, FRDisplacementMtl_MINHEIGHT, t);
			subdivision = GetFromPb<float>(pblock, FRDisplacementMtl_SUBDIVISION, t);
			creaseWeight = GetFromPb<float>(pblock, FRDisplacementMtl_CREASEWEIGHT, t);
			boundary = GetFromPb<int>(pblock, FRDisplacementMtl_BOUNDARY, t);
			notAccurate = false;
		}

		if (dmap && ((minHeight || maxHeight) && (maxHeight > minHeight)))
		{
			frw::Image img = mtlParser.createImageFromMap(dmap, MAP_FLAG_NOGAMMA);
			frw::ImageNode imgNode = frw::ImageNode(mtlParser.materialSystem);
			imgNode.SetMap(img);
			return imgNode;
		}
	}

	return frw::Value();
}

FIRERENDER_NAMESPACE_END;
