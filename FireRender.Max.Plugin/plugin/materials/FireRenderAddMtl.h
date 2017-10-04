#pragma once

#include "FireRenderMtlBase.h"

FIRERENDER_NAMESPACE_BEGIN;

enum FRAddMtl_ParamID : ParamID {
	FRAddMtl_COLOR0 = 1000,
	FRAddMtl_COLOR1 = 1001
};

#define NSUBMTL 2
#define SUB1_REF	1
#define SUB2_REF	2

class FireRenderAddMtlTraits
{
public:
	static const TCHAR* InternalName() { return _T("RPR Add Material"); }
	static Class_ID ClassId() { return FIRERENDER_ADDMTL_CID; }
};

class FireRenderAddMtl :
	public FireRenderMtl<FireRenderAddMtlTraits, FireRenderAddMtl>
{
public:
	Mtl *sub1, *sub2;
	void NotifyChanged() { NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE); }
	int NumSubMtls() override { return 2; }
	Mtl* GetSubMtl(int i) override { return i ? sub2 : sub1; }
	void SetSubMtl(int i, Mtl *m) override;
	TSTR GetSubMtlSlotName(int i) override { return i ? L"Material 1" : L"Material 2"; }
	int NumSubs() override { return 3; }
	int NumRefs() override { return 3; }
	RefTargetHandle GetReference(int i) override;

	frw::Shader GetShader(const TimeValue t, class MaterialParser& mtlParser, INode* node) override;

private:
	virtual void SetReference(int i, RefTargetHandle rtarg);
};

FIRERENDER_NAMESPACE_END;
