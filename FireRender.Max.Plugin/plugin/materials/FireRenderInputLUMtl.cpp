/*********************************************************************************************************************************
* Radeon ProRender for 3ds Max plugin
* Copyright (c) 2017 AMD
* All Rights Reserved
*
* Radeon ProRender dedicated custom material plugin
*********************************************************************************************************************************/
#include "FireRenderInputLUMtl.h"
#include "Resource.h"
#include "parser\MaterialParser.h"
#include "maxscript\mxsplugin\mxsPlugin.h"
#include "FireRenderDiffuseMtl.h"

FIRERENDER_NAMESPACE_BEGIN;

IMPLEMENT_FRMTLCLASSDESC(InputLUMtl)

FRMTLCLASSDESCNAME(InputLUMtl) FRMTLCLASSNAME(InputLUMtl)::ClassDescInstance;

// All parameters of the material plugin. See FIRE_MAX_PBDESC definition for notes on backwards compatibility
static ParamBlockDesc2 pbDesc(
	0, _T("InputLUMtlPbdesc"), 0, &FRMTLCLASSNAME(InputLUMtl)::ClassDescInstance, P_AUTO_CONSTRUCT + P_AUTO_UI + P_VERSION, FIRERENDERMTLVER_LATEST, 0,
    //rollout
	IDD_FIRERENDER_INPUTLUMTL, IDS_FR_MTL_INPUTLU, 0, 0, NULL,

	InputLUMtl_VALUE, _T("value"), TYPE_INT, P_ANIMATABLE, 0,
	p_default, 0,
	p_ui, TYPE_INT_COMBOBOX, IDC_INPUTLU_VALUE, 4,
	IDS_INPUTLU_VALUE_0, IDS_INPUTLU_VALUE_1, IDS_INPUTLU_VALUE_2, IDS_INPUTLU_VALUE_3,
	p_vals, 0, 1, 2, 3, PB_END,

    PB_END
    );

std::map<int, std::pair<ParamID, MCHAR*>> FRMTLCLASSNAME(InputLUMtl)::TEXMAP_MAPPING;

FRMTLCLASSNAME(InputLUMtl)::~FRMTLCLASSNAME(InputLUMtl)()
{
}


frw::Value FRMTLCLASSNAME(InputLUMtl)::getShader(const TimeValue t, MaterialParser& mtlParser)
{
	auto ms = mtlParser.materialSystem;
		
	int op = GetFromPb<int>(pblock, InputLUMtl_VALUE);

	switch (op)
	{
		case 0: // UV
			return mtlParser.materialSystem.ValueLookupUV(0);
		case 1: // N
			return mtlParser.materialSystem.ValueLookupN();
		case 2: // P
			return mtlParser.materialSystem.ValueLookupP();
		case 3: // INVEC
			return mtlParser.materialSystem.ValueLookupINVEC();
		case 4: // OUTVEC
			return mtlParser.materialSystem.ValueLookupOUTVEC();
	}

	return mtlParser.materialSystem.ValueLookupUV(0); // dummy
}

void FRMTLCLASSNAME(InputLUMtl)::Update(TimeValue t, Interval& valid) {
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
