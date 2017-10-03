#pragma once

#include "FireRenderMtlBase.h"

FIRERENDER_NAMESPACE_BEGIN;

enum FRBlendMtl_ParamID : ParamID {
	FRBlendMtl_COLOR0 = 1000,
	FRBlendMtl_COLOR1 = 1001,
	FRBlendMtl_WEIGHT = 1002,
	FRBlendMtl_WEIGHT_TEXMAP = 1003
};

#define NSUBMTL 2
#define SUB1_REF	1
#define SUB2_REF	2
#define SUB3_REF	3

class FireRenderBlendMtlTraits
{
public:
	static const TCHAR* InternalName() { return _T("RPR Blend Material"); }
	static Class_ID ClassId() { return FIRERENDER_BLENDMTL_CID; }
};

class FireRenderBlendMtl :
	public FireRenderMtl<FireRenderBlendMtlTraits, FireRenderBlendMtl>
{
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
	Color GetDiffuse(int mtlNum, BOOL backFace) override;
	float GetXParency(int mtlNum, BOOL backFace) override;
	frw::Shader getShader(const TimeValue t, class MaterialParser& mtlParser, INode* node);

private:
	virtual void SetReference(int i, RefTargetHandle rtarg);
};

FIRERENDER_NAMESPACE_END;
