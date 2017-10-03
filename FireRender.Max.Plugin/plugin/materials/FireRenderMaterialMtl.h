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

class FireRenderMaterialMtlTraits
{
public:
	using ClassDesc = FireRenderClassDescMaterialMtl;
};

class FireRenderMaterialMtl :
	public FireRenderMtl<FireRenderMaterialMtlTraits>
{
public:
	Mtl *sub1, *sub2;
	Texmap *sub3;

	int NumSubMtls() override;
	Mtl* GetSubMtl(int i) override;
	void SetSubMtl(int i, Mtl *m) override;
	TSTR GetSubMtlSlotName(int i) override;
	int NumSubs() override;
	int NumRefs() override;
	RefTargetHandle GetReference(int i) override;

	void NotifyChanged();
	frw::Shader getVolumeShader(const TimeValue t, class MaterialParser& mtlParser, INode* node);
	frw::Shader getShader(const TimeValue t, class MaterialParser& mtlParser, INode* node);

private:
	void SetReference(int i, RefTargetHandle rtarg) override;
};


FIRERENDER_NAMESPACE_END;
