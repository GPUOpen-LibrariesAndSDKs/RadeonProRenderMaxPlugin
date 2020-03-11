/**********************************************************************
Copyright 2020 Advanced Micro Devices, Inc
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at
    http://www.apache.org/licenses/LICENSE-2.0
Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
********************************************************************/

#include "FireRenderArithmMtl.h"
#include "Resource.h"
#include "parser\MaterialParser.h"
#include "maxscript\mxsplugin\mxsPlugin.h"
#include "FireRenderDiffuseMtl.h"

FIRERENDER_NAMESPACE_BEGIN;

IMPLEMENT_FRMTLCLASSDESC(ArithmMtl)

FRMTLCLASSDESCNAME(ArithmMtl) FRMTLCLASSNAME(ArithmMtl)::ClassDescInstance;

// All parameters of the material plugin. See FIRE_MAX_PBDESC definition for notes on backwards compatibility
static ParamBlockDesc2 pbDesc(
	0, _T("ArithmMtlPbdesc"), 0, &FRMTLCLASSNAME(ArithmMtl)::ClassDescInstance, P_AUTO_CONSTRUCT + P_AUTO_UI + P_VERSION, FIRERENDERMTLVER_LATEST, 0,
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

std::map<int, std::pair<ParamID, MCHAR*>> FRMTLCLASSNAME(ArithmMtl)::TEXMAP_MAPPING = {
	{ FRDiffuseMtl_TEXMAP_COLOR0, { FRArithmMtl_COLOR0_TEXMAP, _T("Color 1 map") } },
	{ FRDiffuseMtl_TEXMAP_COLOR1, { FRArithmMtl_COLOR1_TEXMAP, _T("Color 2 map") } }
};

FRMTLCLASSNAME(ArithmMtl)::~FRMTLCLASSNAME(ArithmMtl)()
{
}


frw::Value FRMTLCLASSNAME(ArithmMtl)::getShader(const TimeValue t, MaterialParser& mtlParser)
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

void FRMTLCLASSNAME(ArithmMtl)::Update(TimeValue t, Interval& valid) {
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
