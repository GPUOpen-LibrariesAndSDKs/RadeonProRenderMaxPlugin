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
