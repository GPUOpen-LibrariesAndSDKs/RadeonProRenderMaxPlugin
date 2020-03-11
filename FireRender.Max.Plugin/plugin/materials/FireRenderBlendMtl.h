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

enum FRBlendMtl_ParamID : ParamID {
	FRBlendMtl_COLOR0 = 1000,
	FRBlendMtl_COLOR1 = 1001,
	FRBlendMtl_WEIGHT = 1002,
	FRBlendMtl_WEIGHT_TEXMAP = 1003
};

BEGIN_DECLARE_FRMTLCLASSDESC(BlendMtl, L"RPR Blend Material", FIRERENDER_BLENDMTL_CID)
END_DECLARE_FRMTLCLASSDESC()

#define NSUBMTL 2
#define SUB1_REF	1
#define SUB2_REF	2
#define SUB3_REF	3

BEGIN_DECLARE_FRMTL(BlendMtl)
public:
	Mtl *sub1, *sub2;
	Texmap *sub3;
	void NotifyChanged() { NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE); }
	int NumSubMtls() { return 2; }
	Mtl* GetSubMtl(int i) { return i ? sub2 : sub1; }
	void SetSubMtl(int i, Mtl *m);
	TSTR GetSubMtlSlotName(int i) { return i ? L"Material 1" : L"Material 2"; }
	int NumSubs() { return 4; }
	int NumRefs() { return 4; }
	RefTargetHandle GetReference(int i);
private:
	virtual void SetReference(int i, RefTargetHandle rtarg);
public:
	virtual Color GetDiffuse(int mtlNum, BOOL backFace) override {
		Mtl* m0 = GetFromPb<Mtl*>(pblock, FRBlendMtl_COLOR0);
		Mtl* m1 = GetFromPb<Mtl*>(pblock, FRBlendMtl_COLOR1);
		if (m0 && m1)
		{
		Color c0 = m0->GetDiffuse(0, backFace);
		Color c1 = m1->GetDiffuse(0, backFace);
		float w = GetFromPb<float>(pblock, FRBlendMtl_WEIGHT);
		return ((c0 * w) + (c1 * (1.f - w)));
	}
		if (m0 && !m1)
			return m0->GetDiffuse(0, backFace);
		if (m1 && !m0)
			return m1->GetDiffuse(0, backFace);
		return Color(0.8, 0.8, 0.8);
	}
	virtual float GetXParency(int mtlNum, BOOL backFace) override {
		Mtl* m0 = GetFromPb<Mtl*>(pblock, FRBlendMtl_COLOR0);
		Mtl* m1 = GetFromPb<Mtl*>(pblock, FRBlendMtl_COLOR1);
		if (m0 && m1)
		{
		float t0 = m0->GetXParency(0, backFace);
		float t1 = m1->GetXParency(0, backFace);
		float w = GetFromPb<float>(pblock, FRBlendMtl_WEIGHT);
		return ((t0 * w) + (t1 * (1.f - w)));
	}
		if (m0 && !m1)
			return m0->GetXParency(0, backFace);
		if (m1 && !m0)
			return m1->GetXParency(0, backFace);
		return 0.f;
	}
END_DECLARE_FRMTL(BlendMtl)


FIRERENDER_NAMESPACE_END;
