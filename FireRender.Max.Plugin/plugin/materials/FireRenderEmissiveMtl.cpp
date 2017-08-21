/*********************************************************************************************************************************
* Radeon ProRender for 3ds Max plugin
* Copyright (c) 2017 AMD
* All Rights Reserved
*
* Radeon ProRender dedicated custom material plugin
*********************************************************************************************************************************/
#include "FireRenderEmissiveMtl.h"
#include "Resource.h"
#include "parser\MaterialParser.h"
#include "maxscript\mxsplugin\mxsPlugin.h"
#include "utils/KelvinToColor.h"

FIRERENDER_NAMESPACE_BEGIN;

IMPLEMENT_FRMTLCLASSDESC(EmissiveMtl)

FRMTLCLASSDESCNAME(EmissiveMtl) FRMTLCLASSNAME(EmissiveMtl)::ClassDescInstance;

namespace
{
	class KelvinSwatchAccessor : public PBAccessor
	{
		// read-only
		void Set(PB2Value& v, ReferenceMaker* owner, ParamID id, int tabIndex, TimeValue t)
		{
		}

		virtual void Get(PB2Value& v, ReferenceMaker* owner, ParamID id, int tabIndex, TimeValue t, Interval &valid)
		{
			IParamBlock2* pb = owner->GetParamBlock(0);
			float kelvin = GetFromPb<float>(pb, FREmissiveMtl_KELVIN);
			Color kcolor = KelvinToColor(kelvin);
			v.p->x = kcolor.r;
			v.p->y = kcolor.g;
			v.p->z = kcolor.b;
		}
	};
	static KelvinSwatchAccessor FollowKelvinSwatch;
}

// All parameters of the material plugin. See FIRE_MAX_PBDESC definition for notes on backwards compatibility
static ParamBlockDesc2 pbDesc(
	0, _T("EmissiveMtlPbdesc"), 0, &FRMTLCLASSNAME(EmissiveMtl)::ClassDescInstance, P_AUTO_CONSTRUCT + P_AUTO_UI + P_VERSION, FIRERENDERMTLVER_LATEST, 0,
    //rollout
	IDD_FIRERENDER_EMISSIVEMTL, IDS_FR_MTL_EMISSIVE, 0, 0, NULL,

	FREmissiveMtl_INTENSITY_MODE, _T("IntMode"), TYPE_INT, 0, 0,
	p_default, FREmissiveMtl_IntMode_Watts,
	p_ui, TYPE_RADIO, 2, IDC_RD_WATTS, IDC_RD_LUMENS,
	p_vals, FREmissiveMtl_IntMode_Watts, FREmissiveMtl_IntMode_Lumens,
	p_end,

	FREmissiveMtl_COLOR_MODE, _T("ColorMode"), TYPE_INT, 0, 0,
	p_default, FREmissiveMtl_ColorMode_RGB,
	p_ui, TYPE_RADIO, 2, IDC_RD_RGB, IDC_RD_KELVIN,
	p_vals, FREmissiveMtl_ColorMode_RGB, FREmissiveMtl_ColorMode_Kelvin,
	p_end,

	FREmissiveMtl_WATTS, _T("Multiplier"), TYPE_FLOAT, P_ANIMATABLE, 0,
	p_default, 1.0f, p_range, 0.f, 9999999.f, p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_EMISSIVE_WATTS, IDC_EMISSIVE_WATTS_S, SPIN_AUTOSCALE, PB_END,

	FREmissiveMtl_LUMENS, _T("Lumens"), TYPE_FLOAT, P_ANIMATABLE, 0,
	p_default, 683.f, p_range, 0.f, FLT_MAX, p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_EMISSIVE_LUMENS, IDC_EMISSIVE_LUMENS_S, SPIN_AUTOSCALE, PB_END,
	
	FREmissiveMtl_COLOR, _T("Color"), TYPE_RGBA, P_ANIMATABLE, 0,
    p_default, Color(1.0f, 1.0f, 1.0f), p_ui, TYPE_COLORSWATCH, IDC_EMISSIVE_COLOR, PB_END,

	FREmissiveMtl_COLOR_TEXMAP, _T("colorTexmap"), TYPE_TEXMAP, 0, 0,
	p_subtexno, FREmissiveMtl_TEXMAP_COLOR, p_ui, TYPE_TEXMAPBUTTON, IDC_EMISSIVE_COLOR_TEXMAP, PB_END,

	FREmissiveMtl_KELVIN, _T("Kelvin"), TYPE_FLOAT, P_ANIMATABLE, 0,
	p_default, DefaultKelvin, p_range, MinKelvin, MaxKelvin, p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_EMISSIVE_KELVIN, IDC_EMISSIVE_KELVIN_S, SPIN_AUTOSCALE, PB_END,

	FREmissiveMtl_KELVINSWATCH, _T("Color"), TYPE_RGBA, 0, 0,
	p_default, Color(1.0f, 1.0f, 1.0f), p_ui, TYPE_COLORSWATCH, IDC_EMISSIVE_KELVIN_SWATCH, 
	p_accessor, &FollowKelvinSwatch, PB_END,

    PB_END
    );

std::map<int, std::pair<ParamID, MCHAR*>> FRMTLCLASSNAME(EmissiveMtl)::TEXMAP_MAPPING = {
	{ FREmissiveMtl_TEXMAP_COLOR, { FREmissiveMtl_COLOR_TEXMAP, _T("Color Map") } },
};

FRMTLCLASSNAME(EmissiveMtl)::~FRMTLCLASSNAME(EmissiveMtl)()
{
}


frw::Shader FRMTLCLASSNAME(EmissiveMtl)::getShader(const TimeValue t, MaterialParser& mtlParser, INode* node)
{
	auto ms = mtlParser.materialSystem;

	mtlParser.shaderData.mNumEmissive++;

	frw::EmissiveShader material(ms);
		
	int intMode = GetFromPb<int>(pblock, FREmissiveMtl_INTENSITY_MODE);
	int colMode = GetFromPb<int>(pblock, FREmissiveMtl_COLOR_MODE);

	float mult = 1.f;

	switch (intMode)
	{
		case FREmissiveMtl_IntMode_Watts:
			mult = GetFromPb<float>(pblock, FREmissiveMtl_WATTS);
			break;
		case FREmissiveMtl_IntMode_Lumens:
			mult = GetFromPb<float>(pblock, FREmissiveMtl_LUMENS) * (1.f / 683.f);
			break;
	}

	frw::Value color;

	switch (colMode)
	{
		case FREmissiveMtl_ColorMode_RGB:
		{
	Texmap* colorTexmap = GetFromPb<Texmap*>(pblock, FREmissiveMtl_COLOR_TEXMAP);
	if (colorTexmap)
		color = mtlParser.createMap(colorTexmap, 0);
			else
			{
				const Color filter = GetFromPb<Color>(pblock, FREmissiveMtl_COLOR);
				color = frw::Value(filter.r, filter.g, filter.b);
			}
		}
		break;

		case FREmissiveMtl_ColorMode_Kelvin:
		{
			float kelvin = GetFromPb<float>(pblock, FREmissiveMtl_KELVIN);
			Color kcolor = KelvinToColor(kelvin);
			color = frw::Value(kcolor.r, kcolor.g, kcolor.b);
		}
		break;
	}

	material.SetColor(mtlParser.materialSystem.ValueMul(color, mult));
		
    return material;
}

FIRERENDER_NAMESPACE_END;
