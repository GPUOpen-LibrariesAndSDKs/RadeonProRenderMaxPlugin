#pragma once

#include "FireRenderMtlBase.h"

FIRERENDER_NAMESPACE_BEGIN;

enum FRDiffuseMtl_TexmapId {
	FRDiffuseMtl_TEXMAP_DIFFUSE = 0,
	FRDiffuseMtl_TEXMAP_NORMAL = 1
};

enum FRDiffuseMtl_ParamID : ParamID {
	FRDiffuseMtl_COLOR = 1000,
	FRDiffuseMtl_COLOR_TEXMAP = 1002,
	FRDiffuseMtl_NORMALMAP = 1003
};

BEGIN_DECLARE_FRMTLCLASSDESC(DiffuseMtl, L"RPR Diffuse Material", FIRERENDER_DIFFUSEMTL_CID)
END_DECLARE_FRMTLCLASSDESC()

class FireRenderDiffuseMtlTraits
{
public:
	using ClassDesc = FireRenderClassDescDiffuseMtl;
};

class FireRenderDiffuseMtl :
	public FireRenderMtl<FireRenderDiffuseMtlTraits>
{
public:
	frw::Shader getShader(const TimeValue t, class MaterialParser& mtlParser, INode* node);
	Color GetDiffuse(int mtlNum, BOOL backFace) override;
};

FIRERENDER_NAMESPACE_END;
