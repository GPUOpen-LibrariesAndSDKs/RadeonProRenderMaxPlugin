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

#define SAFEMOD(a, b) b == 0 ? 0 : fmod(a, b)

class FireRenderArithmMtlTraits
{
public:
	static const TCHAR* InternalName() { return _T("RPR Arithmetic"); }
	static Class_ID ClassId() { return FIRERENDER_ARITHMMTL_CID; }
};

class FireRenderArithmMtl :
	public FireRenderTex<FireRenderArithmMtlTraits, FireRenderArithmMtl>
{
public:
	AColor EvalColor(ShadeContext& sc) override;
	void Update(TimeValue t, Interval& valid) override;
	frw::Value GetShader(const TimeValue t, class MaterialParser& mtlParser) override;
};

FIRERENDER_NAMESPACE_END;
