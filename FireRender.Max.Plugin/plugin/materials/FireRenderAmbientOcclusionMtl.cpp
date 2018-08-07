
#include "FireRenderAmbientOcclusionMtl.h"
#include "Resource.h"
#include "parser\MaterialParser.h"
#include "maxscript\mxsplugin\mxsPlugin.h"

FIRERENDER_NAMESPACE_BEGIN;

IMPLEMENT_FRMTLCLASSDESC(AmbientOcclusionMtl)

FRMTLCLASSDESCNAME(AmbientOcclusionMtl) FRMTLCLASSNAME(AmbientOcclusionMtl)::ClassDescInstance;

static ParamBlockDesc2 pbDesc(
	0, _T("AmbientOcclusionMtlPbdesc"), 0, &FRMTLCLASSNAME(AmbientOcclusionMtl)::ClassDescInstance, P_AUTO_CONSTRUCT + P_AUTO_UI + P_VERSION, FIRERENDERMTLVER_LATEST, 0,
    //rollout
	IDD_FIRERENDER_AMBIENTOCCLUSIONMTL, IDS_FR_MTL_AMBIENTOCCLUSION, 0, 0, NULL,

	FRAmbientOcclusionMtl_RADIUS, _T("Radius"), TYPE_FLOAT, P_ANIMATABLE, 0,
	p_default, 0.1f, 
	p_range, 0.0f, 10.0f, 
	p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_AO_RADIUS, IDC_AO_RADIUS_S, SPIN_AUTOSCALE, 
	PB_END,

	FRAmbientOcclusionMtl_SIDE, _T("Side"), TYPE_INT, P_ANIMATABLE, 0,
	p_default, FRAmbientOcclusionMtl_SIDE_FRONT,
	p_ui, TYPE_INT_COMBOBOX, IDC_AO_SIDE, 2, IDS_AO_SIDE_FRONT, IDS_AO_SIDE_BACK,
	p_vals, FRAmbientOcclusionMtl_SIDE_FRONT, FRAmbientOcclusionMtl_SIDE_BACK,
	PB_END,

	FRAmbientOcclusionMtl_COLOR0, _T("Unoccluded Color"), TYPE_RGBA, P_ANIMATABLE, 0,
	p_default, Color(1.0f, 1.0f, 1.0f),
	p_ui, TYPE_COLORSWATCH, IDC_AO_COLOR0,
	PB_END,

	FRAmbientOcclusionMtl_COLOR1, _T("Occluded Color"), TYPE_RGBA, P_ANIMATABLE, 0,
	p_default, Color(0.0f, 0.0f, 0.0f),
	p_ui, TYPE_COLORSWATCH, IDC_AO_COLOR1,
	PB_END,

	FRAmbientOcclusionMtl_COLOR0_TEXMAP, _T("color0Texmap"), TYPE_TEXMAP, 0, 0,
	p_subtexno, FRAmbientOcclusionMtl_TEXMAP_COLOR0,
	p_ui, TYPE_TEXMAPBUTTON, IDC_AO_COLOR0_TEXMAP,
	PB_END,

	FRAmbientOcclusionMtl_COLOR1_TEXMAP, _T("color1Texmap"), TYPE_TEXMAP, 0, 0,
	p_subtexno, FRAmbientOcclusionMtl_TEXMAP_COLOR1,
	p_ui, TYPE_TEXMAPBUTTON, IDC_AO_COLOR1_TEXMAP,
	PB_END,

    PB_END
    );

std::map<int, std::pair<ParamID, MCHAR*>> FRMTLCLASSNAME(AmbientOcclusionMtl)::TEXMAP_MAPPING = {
	{ FRAmbientOcclusionMtl_TEXMAP_COLOR0, { FRAmbientOcclusionMtl_COLOR0_TEXMAP, _T("Color 1 map") } },
	{ FRAmbientOcclusionMtl_TEXMAP_COLOR1, { FRAmbientOcclusionMtl_COLOR1_TEXMAP, _T("Color 2 map") } }
};

FRMTLCLASSNAME(AmbientOcclusionMtl)::~FRMTLCLASSNAME(AmbientOcclusionMtl)()
{
}

// Returns a BlendValue shader with a connected Ambient Occlusion shader as the weight value
// Occluded and Unoccluded textures are connected to the BlendValue shader as the color values
frw::Value FRMTLCLASSNAME(AmbientOcclusionMtl)::getShader(const TimeValue t, MaterialParser& mtlParser)
{
	auto ms = mtlParser.materialSystem;
		
	const float radius = GetFromPb<float>(pblock, FRAmbientOcclusionMtl_RADIUS);
	const int sideId = GetFromPb<int>(pblock, FRAmbientOcclusionMtl_SIDE);
	const Color color0 = GetFromPb<Color>(pblock, FRAmbientOcclusionMtl_COLOR0);
	const Color color1 = GetFromPb<Color>(pblock, FRAmbientOcclusionMtl_COLOR1);
	Texmap* color0Texmap = GetFromPb<Texmap*>(pblock, FRAmbientOcclusionMtl_COLOR0_TEXMAP);
	Texmap* color1Texmap = GetFromPb<Texmap*>(pblock, FRAmbientOcclusionMtl_COLOR1_TEXMAP);
	float side = 1.0f;

	frw::Value color0v(color0.r, color0.g, color0.b);
	if (color0Texmap)
		color0v = mtlParser.createMap(color0Texmap, 0);

	frw::Value color1v(color1.r, color1.g, color1.b);
	if (color1Texmap)
		color1v = mtlParser.createMap(color1Texmap, 0);

	if(sideId == FRAmbientOcclusionMtl_SIDE_FRONT)
		side = 1.0f;
	else if (sideId == FRAmbientOcclusionMtl_SIDE_BACK)
		side = -1.0f;
	frw::Value weightv = mtlParser.materialSystem.ValueAmbientOcclusion(frw::Value(radius), frw::Value(side));

	// Ambient Occlusion shader returns Black when Occluded and White when Unoccluded
	// Therefore the Unoccluded color is passed as the second parameter to the blend texture
	return mtlParser.materialSystem.ValueBlend(color1v, color0v, weightv);
}

void FRMTLCLASSNAME(AmbientOcclusionMtl)::Update(TimeValue t, Interval& valid) {
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

