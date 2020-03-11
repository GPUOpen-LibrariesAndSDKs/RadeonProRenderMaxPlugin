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

#include "FireRenderColourCorrectionMtl.h"
#include "Resource.h"
#include "parser\MaterialParser.h"
#include "maxscript\mxsplugin\mxsPlugin.h"
#include "CoronaDeclarations.h"
#include "FireRenderMaterialMtl.h"
#include "FireRenderUberMtl.h"
#include "FireRenderUberMtlv2.h"
#include "plugin/ParamBlock.h"
#include "utils/Utils.h"


FIRERENDER_NAMESPACE_BEGIN

IMPLEMENT_FRMTLCLASSDESC(ColourCorMtl)

FRMTLCLASSDESCNAME(ColourCorMtl) FRMTLCLASSNAME(ColourCorMtl)::ClassDescInstance;

// All parameters of the material plugin. See FIRE_MAX_PBDESC definition for notes on backwards compatibility
static ParamBlockDesc2 pbDesc(
	0, _T("ColourCorMtlPbdesc"), 0, &FRMTLCLASSNAME(ColourCorMtl)::ClassDescInstance, P_AUTO_CONSTRUCT + P_AUTO_UI + P_VERSION, FIRERENDERMTLVER_LATEST, 0,
	//rollout
	IDD_FIRERENDER_COLOURCORRMTL, IDS_FR_MTL_COLOURCORR, 0, 0, NULL,

	FRColourCorMtl_ISENABLED, _T("UseGammaCorrection"), TYPE_BOOL, P_ANIMATABLE, 0,
	p_default, FALSE, p_ui, TYPE_SINGLECHEKBOX, IDC_COLOURCORR_ENABLE, PB_END,

	FRColourCorMtl_COLOR_TEXMAP, _T("Map"), TYPE_TEXMAP, 0, 0,
	p_subtexno, FRColourCorMtl_TEXMAP_COLOR, p_ui, TYPE_TEXMAPBUTTON, IDC_COLOURCOR_TEXMAP, PB_END,

	FRColourCorMtl_BOUNDARY, _T("Boundary"), TYPE_INT, P_ANIMATABLE, 0,
	p_default, RPR_GAMMA_RAW, p_ui, TYPE_INT_COMBOBOX, IDC_COLOURCOR_GAMMA_BOUNDARY, 2, IDS_STRING400, IDS_STRING401, 
	p_vals, RPR_GAMMA_RAW, RPR_GAMMA_SRGB, PB_END,

	FRColourCorMtl_USECUSTOM, _T("UseCustomGamma"), TYPE_BOOL, P_ANIMATABLE, 0,
	p_default, FALSE, p_ui, TYPE_SINGLECHEKBOX, IDC_COLOURCORR_ENABLE2, PB_END,

	FRColourCorMtl_CUSTOMGAMMA, _T("Min Height"), TYPE_FLOAT, P_ANIMATABLE, 0,
	p_default, 1.0f, p_range, 0.5f, 20.0f, p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_COLOURCORR_CUSTGAMMA, IDC_COLOURCORR_CUSTGAMMA_S, SPIN_AUTOSCALE, PB_END,

	PB_END
);

std::map<int, std::pair<ParamID, MCHAR*>> FRMTLCLASSNAME(ColourCorMtl)::TEXMAP_MAPPING = {
	{ FRColourCorMtl_TEXMAP_COLOR,{ FRColourCorMtl_COLOR_TEXMAP, _T("Map") } }
};

FRMTLCLASSNAME(ColourCorMtl)::~FRMTLCLASSNAME(ColourCorMtl)()
{
}


frw::Value FRMTLCLASSNAME(ColourCorMtl)::getShader(const TimeValue t, MaterialParser& mtlParser)
{
	frw::Value color(0.f, 0.f, 0.f);

	Texmap* texmap = GetFromPb<Texmap*>(pblock, FRColourCorMtl_COLOR_TEXMAP);
	if (texmap)
		color = mtlParser.createMap(texmap, MAP_FLAG_NOGAMMA);

	return color;
}

void FRMTLCLASSNAME(ColourCorMtl)::Update(TimeValue t, Interval& valid) 
{
	for (int i = 0; i < NumSubTexmaps(); ++i) 
	{
		// we are required to recursively call Update on all our submaps
		Texmap* map = GetSubTexmap(i);
		if (map != NULL) 
		{
			map->Update(t, valid);
		}
	}

	this->pblock->GetValidity(t, valid);
}

FIRERENDER_NAMESPACE_END;
