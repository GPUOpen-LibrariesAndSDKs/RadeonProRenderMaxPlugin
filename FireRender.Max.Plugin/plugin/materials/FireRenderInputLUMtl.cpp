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
	0, _T("InputLUMtlPbdesc"), 0, &FRMTLCLASSNAME(InputLUMtl)::ClassDescInstance, P_AUTO_CONSTRUCT + P_AUTO_UI + P_VERSION,
	FIRERENDERMTLVER_LATEST, 0,
	//rollout
	IDD_FIRERENDER_INPUTLUMTL, IDS_FR_MTL_INPUTLU, 0, 0, NULL,

	InputLUMtl_VALUE, _T("input"), TYPE_INT, P_ANIMATABLE, 0,
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
