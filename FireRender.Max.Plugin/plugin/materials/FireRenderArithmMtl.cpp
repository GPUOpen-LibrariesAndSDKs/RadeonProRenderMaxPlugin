/*********************************************************************************************************************************
* Radeon ProRender for 3ds Max plugin
* Copyright (c) 2017 AMD
* All Rights Reserved
*
* Radeon ProRender dedicated custom material plugin
*********************************************************************************************************************************/
#include "FireRenderArithmMtl.h"
#include "Resource.h"
#include "parser\MaterialParser.h"
#include "maxscript\mxsplugin\mxsPlugin.h"
#include "FireRenderDiffuseMtl.h"

FIRERENDER_NAMESPACE_BEGIN;

// All parameters of the material plugin. See FIRE_MAX_PBDESC definition for notes on backwards compatibility
static ParamBlockDesc2 pbDesc(
	0, _T("ArithmMtlPbdesc"), 0, &FireRenderArithmMtl::GetClassDesc(), P_AUTO_CONSTRUCT + P_AUTO_UI + P_VERSION, FIRERENDERMTLVER_LATEST, 0,
    //rollout
	IDD_FIRERENDER_ARITHMMTL, IDS_FR_MTL_ARITHM, 0, 0, NULL,

	FRArithmMtl_COLOR0, _T("Color1"), TYPE_RGBA, P_ANIMATABLE, 0,
	p_default, Color(0.5f, 0.5f, 0.5f), p_ui, TYPE_COLORSWATCH, IDC_ARITHM_COLOR0, PB_END,

	FRArithmMtl_COLOR1, _T("Color2"), TYPE_RGBA, P_ANIMATABLE, 0,
	p_default, Color(0.5f, 0.5f, 0.5f), p_ui, TYPE_COLORSWATCH, IDC_ARITHM_COLOR1, PB_END,

	FRArithmMtl_OP, _T("operator"), TYPE_INT, P_ANIMATABLE, 0,
	p_default, 0,
	p_ui, TYPE_INT_COMBOBOX, IDC_ARITHM_OP, 25, // <- this is count of arguments enumerated below
	IDS_ARITHM_OP_0, IDS_ARITHM_OP_1, IDS_ARITHM_OP_2, IDS_ARITHM_OP_3, IDS_ARITHM_OP_4, 
	IDS_ARITHM_OP_5, IDS_ARITHM_OP_6, IDS_ARITHM_OP_7, IDS_ARITHM_OP_8, IDS_ARITHM_OP_9, 
	IDS_ARITHM_OP_10, IDS_ARITHM_OP_11,	IDS_ARITHM_OP_MIN, IDS_ARITHM_OP_MAX, IDS_ARITHM_OP_FLOOR, 
	IDS_ARITHM_OP_MAGNITUDE, IDS_ARITHM_OP_NORMALIZE, IDS_ARITHM_OP_ABS, IDS_ARITHM_OP_MOD, IDS_ARITHM_OP_ASIN, 
	IDS_ARITHM_OP_ACOS, IDS_ARITHM_OP_ATAN, IDS_ARITHM_OP_CROSS, IDS_ARITHM_OP_CMPAVG, IDS_ARITHM_OP_AVG,
	p_vals, 
	OperatorId_Add, OperatorId_Sub, OperatorId_Mul, OperatorId_Div, OperatorId_Sin, 
	OperatorId_Cos,	OperatorId_Tan, OperatorId_SelectX, OperatorId_SelectY, OperatorId_SelectZ, 
	OperatorId_Dot, OperatorId_Pow,	OperatorId_Min, OperatorId_Max, OperatorId_Floor, 
	OperatorId_Magnitude, OperatorId_Normalize, OperatorId_Abs, OperatorId_Mod, OperatorId_ArcSin, 
	OperatorId_ArcCos, OperatorId_ArcTan, OperatorId_Cross, OperatorId_ComponentAverage, OperatorId_Average,
	PB_END,

	FRArithmMtl_COLOR0_TEXMAP, _T("color0texmap"), TYPE_TEXMAP, 0, 0,
	p_subtexno, FRDiffuseMtl_TEXMAP_COLOR0, p_ui, TYPE_TEXMAPBUTTON, IDC_ARITHM_COLOR0_TEXMAP, PB_END,

	FRArithmMtl_COLOR1_TEXMAP, _T("color1texmap"), TYPE_TEXMAP, 0, 0,
	p_subtexno, FRDiffuseMtl_TEXMAP_COLOR1, p_ui, TYPE_TEXMAPBUTTON, IDC_ARITHM_COLOR1_TEXMAP, PB_END,

    PB_END
    );

std::map<int, std::pair<ParamID, MCHAR*>> FireRenderArithmMtl::TEXMAP_MAPPING = {
	{ FRDiffuseMtl_TEXMAP_COLOR0, { FRArithmMtl_COLOR0_TEXMAP, _T("Color 1 map") } },
	{ FRDiffuseMtl_TEXMAP_COLOR1, { FRArithmMtl_COLOR1_TEXMAP, _T("Color 2 map") } }
};

frw::Value FireRenderArithmMtl::GetShader(const TimeValue t, MaterialParser& mtlParser)
{
	auto ms = mtlParser.materialSystem;
		
	const Color color0 = GetFromPb<Color>(pblock, FRArithmMtl_COLOR0);
	const Color color1 = GetFromPb<Color>(pblock, FRArithmMtl_COLOR1);
	Texmap* color0Texmap = GetFromPb<Texmap*>(pblock, FRArithmMtl_COLOR0_TEXMAP);
	Texmap* color1Texmap = GetFromPb<Texmap*>(pblock, FRArithmMtl_COLOR1_TEXMAP);
	int op = GetFromPb<int>(pblock, FRArithmMtl_OP);

	frw::Value color0v(color0.r, color0.g, color0.b);
	if (color0Texmap)
		color0v = mtlParser.createMap(color0Texmap, 0);

	frw::Value color1v(color1.r, color1.g, color1.b);
	if (color1Texmap)
		color1v = mtlParser.createMap(color1Texmap, 0);

	switch (op)
	{
		case OperatorId_Add: // add
			return mtlParser.materialSystem.ValueAdd(color0v, color1v);
		case OperatorId_Sub: // sub
			return mtlParser.materialSystem.ValueSub(color0v, color1v);
		case OperatorId_Mul: // mul
			return mtlParser.materialSystem.ValueMul(color0v, color1v);
		case OperatorId_Div: // div
			return mtlParser.materialSystem.ValueDiv(color0v, color1v);
		case OperatorId_Sin: // sin
			return mtlParser.materialSystem.ValueSin(color0v);
		case OperatorId_Cos: // cos
			return mtlParser.materialSystem.ValueCos(color0v);
		case OperatorId_Tan: // tan
			return mtlParser.materialSystem.ValueTan(color0v);
		case OperatorId_SelectX: // select x
			return mtlParser.materialSystem.ValueSelectX(color0v);
		case OperatorId_SelectY: // select y
			return mtlParser.materialSystem.ValueSelectY(color0v);
		case OperatorId_SelectZ: // select z
			return mtlParser.materialSystem.ValueSelectZ(color0v);
		case OperatorId_Dot: // dot3
			return mtlParser.materialSystem.ValueDot(color0v, color1v);
		case OperatorId_Pow: // pow
			return mtlParser.materialSystem.ValuePow(color0v, color1v);

		case OperatorId_Min:
			return mtlParser.materialSystem.ValueMin(color0v, color1v);
		case OperatorId_Max:
			return mtlParser.materialSystem.ValueMax(color0v, color1v);
		case OperatorId_Floor:
			return mtlParser.materialSystem.ValueFloor(color0v);
		case OperatorId_Magnitude:
			return mtlParser.materialSystem.ValueMagnitude(color0v);
		case OperatorId_Normalize:
			return mtlParser.materialSystem.ValueNormalize(color0v);
		case OperatorId_Abs:
			return mtlParser.materialSystem.ValueAbs(color0v);

		case OperatorId_Mod:
			return mtlParser.materialSystem.ValueMod(color0v, color1v);
		case OperatorId_ArcSin:
			return mtlParser.materialSystem.ValueArcSin(color0v);
		case OperatorId_ArcCos:
			return mtlParser.materialSystem.ValueArcCos(color0v);
		case OperatorId_ArcTan:
			return mtlParser.materialSystem.ValueArcTan(color0v, color1v);
		case OperatorId_Cross:
			return mtlParser.materialSystem.ValueCross(color0v, color1v);
		case OperatorId_ComponentAverage:
			return mtlParser.materialSystem.ValueComponentAverage(color0v);
		case OperatorId_Average:
			return mtlParser.materialSystem.ValueAverage(color0v, color1v);
	}

	return mtlParser.materialSystem.ValueAdd(color0v, color1v); // dummy
}

AColor FireRenderArithmMtl::EvalColor(ShadeContext& sc)
{
	Color color0, color1;
	Texmap* colorTexmap0 = GetFromPb<Texmap*>(pblock, FRArithmMtl_COLOR0_TEXMAP);
	if (colorTexmap0)
		color0 = colorTexmap0->EvalColor(sc);
	else
		color0 = GetFromPb<Color>(pblock, FRArithmMtl_COLOR0);
	Texmap* colorTexmap1 = GetFromPb<Texmap*>(pblock, FRArithmMtl_COLOR1_TEXMAP);
	if (colorTexmap1)
		color1 = colorTexmap1->EvalColor(sc);
	else
		color1 = GetFromPb<Color>(pblock, FRArithmMtl_COLOR1);
	AColor retval;
	int op = GetFromPb<int>(pblock, FRArithmMtl_OP);
	switch (op)
	{
	case OperatorId_Add: // add
		retval = AColor(color0 + color1);
		break;
	case OperatorId_Sub: // sub
		retval = AColor(color0 - color1);
		break;
	case OperatorId_Mul: // mul
		retval = AColor(color0 * color1);
		break;
	case OperatorId_Div: // div
		retval = AColor(color0 / color1);
		break;
	case OperatorId_Sin: // sin
		retval = AColor(sin(color0.r), sin(color0.g), sin(color0.b));
		break;
	case OperatorId_Cos: // cos
		retval = AColor(cos(color0.r), cos(color0.g), cos(color0.b));
		break;
	case OperatorId_Tan: // tan
		retval = AColor(tan(color0.r), tan(color0.g), tan(color0.b));
		break;
	case OperatorId_SelectX: // select x
		retval = AColor(color0.r, color0.r, color0.r);
		break;
	case OperatorId_SelectY: // select y
		retval = AColor(color0.g, color0.g, color0.g);
		break;
	case OperatorId_SelectZ: // select z
		retval = AColor(color0.b, color0.b, color0.b);
		break;
	case OperatorId_Dot: // dot3
	{
		float dot = color0.r * color1.r + color0.g * color1.g + color0.b * color1.b;
		retval = AColor(dot, dot, dot);
	}
	break;
	case OperatorId_Pow: // pow
		retval = AColor(pow(color0.r, color1.r), pow(color0.g, color1.g), pow(color0.b, color1.b));
		break;
	case OperatorId_Min:
		retval = AColor(std::min(color0.r, color1.r), std::min(color0.g, color1.g), std::min(color0.b, color1.b));
		break;
	case OperatorId_Max:
		retval = AColor(std::max(color0.r, color1.r), std::max(color0.g, color1.g), std::max(color0.b, color1.b));
		break;
	case OperatorId_Floor:
		retval = AColor(floor(color0.r), floor(color0.g), floor(color0.b));
		break;
	case OperatorId_Magnitude:
	{
		float mag = sqrt(color0.r*color0.r + color0.g*color0.g + color0.b*color0.b);
		retval = AColor(mag, mag, mag);
	}
	break;
	case OperatorId_Normalize:
	{
		auto m = sqrt(color0.r*color0.r + color0.g*color0.g + color0.b*color0.b);
		if (m > 0)
			m = 1.0f / m;
		retval = AColor(color0.r * m, color0.g * m, color0.b * m);
	}
	break;
	case OperatorId_Abs:
		retval = AColor(abs(color0.r), abs(color0.g), abs(color0.b));
		break;
	case OperatorId_Mod:
		retval = AColor(SAFEMOD(color0.r, color1.r), SAFEMOD(color0.g, color1.g), SAFEMOD(color0.b, color1.b));
		break;
	case OperatorId_ArcSin:
		retval = AColor(asin(color0.r), asin(color0.g), asin(color0.b));
		break;
	case OperatorId_ArcCos:
		retval = AColor(acos(color0.r), acos(color0.g), acos(color0.b));
		break;
	case OperatorId_ArcTan:
		retval = AColor(atan2(color0.r, color1.r), atan2(color0.g, color1.g), atan2(color0.b, color1.b));
		break;
	case OperatorId_Cross:
		retval = AColor(color0.g * color1.b - color0.b * color1.g,
			color0.b * color1.r - color0.r * color1.b,
			color0.r * color1.g - color0.g * color1.r);
		break;
	case OperatorId_ComponentAverage:
	{
		float a = (color0.r + color0.g + color0.b) *  1.f / 3.f;
		retval = AColor(a, a, a);
	}
	break;
	case OperatorId_Average:
		retval = AColor((color0.r + color1.r) * 0.5,
			(color0.g + color1.g) * 0.5,
			(color0.b + color1.b) * 0.5);
		break;
	}
	return retval;
}

void FireRenderArithmMtl::Update(TimeValue t, Interval& valid)
{
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
