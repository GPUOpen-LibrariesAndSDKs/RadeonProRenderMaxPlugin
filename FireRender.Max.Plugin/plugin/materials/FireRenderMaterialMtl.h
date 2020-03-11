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


#pragma once

#include "FireRenderMtlBase.h"

FIRERENDER_NAMESPACE_BEGIN;

enum FRMaterialMtl_ParamID : ParamID {
	FRMaterialMtl_SURFACE = 1000,
	FRMaterialMtl_VOLUME = 1001,
	FRMaterialMtl_DISPLACEMENT = 1002,
	FRMaterialMtl_CAUSTICS = 1003,
	FRMaterialMtl_SHADOWCATCHER = 1004
};

BEGIN_DECLARE_FRMTLCLASSDESC(MaterialMtl, L"RPR Material", FIRERENDER_MATERIALMTL_CID)
END_DECLARE_FRMTLCLASSDESC()

#define SUB1_REF	1
#define SUB2_REF	2
#define SUB3_REF	3

BEGIN_DECLARE_FRMTL(MaterialMtl)
public:
	Mtl *sub1, *sub2;
	Texmap *sub3;
	void NotifyChanged() { NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE); }
	int NumSubMtls() { return 2; }
	Mtl* GetSubMtl(int i) { return i ? sub2 : sub1; }
	void SetSubMtl(int i, Mtl *m);
	TSTR GetSubMtlSlotName(int i) { return i ? L"Volume" : L"Surface"; }
	int NumSubs() { return 4; }
	int NumRefs() { return 4; }
	RefTargetHandle GetReference(int i);

	frw::Shader getVolumeShader(const TimeValue t, class MaterialParser& mtlParser, INode* node);

private:
	virtual void SetReference(int i, RefTargetHandle rtarg);
END_DECLARE_FRMTL(MaterialMtl)


FIRERENDER_NAMESPACE_END;
