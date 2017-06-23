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

BEGIN_DECLARE_FRMTLCLASSDESC(AddMtl, L"RPR Add Material", FIRERENDER_ADDMTL_CID)
END_DECLARE_FRMTLCLASSDESC()

BEGIN_DECLARE_FRMTL(AddMtl)
public:
	Mtl *sub1, *sub2;
	void NotifyChanged() { NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE); }
	int NumSubMtls() { return 2; }
	Mtl* GetSubMtl(int i) { return i ? sub2 : sub1; }
	void SetSubMtl(int i, Mtl *m);
	TSTR GetSubMtlSlotName(int i) { return i ? L"Material 1" : L"Material 2"; }
	int NumSubs() { return 3; }
	int NumRefs() { return 3; }
	RefTargetHandle GetReference(int i);
private:
	virtual void SetReference(int i, RefTargetHandle rtarg);
END_DECLARE_FRMTL(AddMtl)

FIRERENDER_NAMESPACE_END;
