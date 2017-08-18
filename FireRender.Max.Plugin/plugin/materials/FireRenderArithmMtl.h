#pragma once

#include "FireRenderMtlBase.h"

FIRERENDER_NAMESPACE_BEGIN;

enum FRArithmMtl_TexmapId {
	FRDiffuseMtl_TEXMAP_COLOR0 = 0,
	FRDiffuseMtl_TEXMAP_COLOR1 = 1
};

enum FRArithmMtl_ParamID : ParamID {
	FRArithmMtl_COLOR0 = 1000,
	FRArithmMtl_COLOR1 = 1001,
	FRArithmMtl_OP = 1002,
	FRArithmMtl_COLOR0_TEXMAP = 1003,
	FRArithmMtl_COLOR1_TEXMAP = 1004
};

enum OperatorId
{
	OperatorId_Add,
	OperatorId_Sub,
	OperatorId_Mul,
	OperatorId_Div,
	OperatorId_Sin,

	OperatorId_Cos,
	OperatorId_Tan,
	OperatorId_SelectX,
	OperatorId_SelectY,
	OperatorId_SelectZ,

	OperatorId_Dot,
	OperatorId_Pow,
	OperatorId_Min,
	OperatorId_Max,
	OperatorId_Floor,

	OperatorId_Magnitude,
	OperatorId_Normalize,
	OperatorId_Abs,
	OperatorId_Mod,
	OperatorId_ArcSin,

	OperatorId_ArcCos,
	OperatorId_ArcTan,
	OperatorId_Cross,
	OperatorId_ComponentAverage,
	OperatorId_Average,
};

BEGIN_DECLARE_FRTEXCLASSDESC(ArithmMtl, L"RPR Arithmetic", FIRERENDER_ARITHMMTL_CID)
END_DECLARE_FRTEXCLASSDESC()

#define SAFEMOD(a, b) b == 0 ? 0 : fmod(a, b)

BEGIN_DECLARE_FRTEX(ArithmMtl)
virtual  AColor EvalColor(ShadeContext& sc) override
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
END_DECLARE_FRTEX(ArithmMtl)


FIRERENDER_NAMESPACE_END;
