#pragma once

#include "FireRenderMtlBase.h"

FIRERENDER_NAMESPACE_BEGIN;

enum FROrenNayarMtl_TexmapId {
	FROrenNayarMtl_TEXMAP_DIFFUSE = 0,
	FROrenNayarMtl_TEXMAP_ROUGHNESS = 1,
	FROrenNayarMtl_TEXMAP_NORMAL = 2
};

enum FROrenNayarMtl_ParamID : ParamID {
	FROrenNayarMtl_COLOR = 1000,
	FROrenNayarMtl_ROUGHNESS = 1001,
	FROrenNayarMtl_COLOR_TEXMAP = 1002,
	FROrenNayarMtl_ROUGHNESS_TEXMAP = 1003,
	FROrenNayarMtl_NORMALMAP = 1004
};

BEGIN_DECLARE_FRMTLCLASSDESC(OrenNayarMtl, L"RPR Oren-Nayar Material", FIRERENDER_ORENNAYARMTL_CID)
END_DECLARE_FRMTLCLASSDESC()

class FireRenderOrenNayarMtlTraits
{
public:
	using ClassDesc = FireRenderClassDescOrenNayarMtl;
};

class FireRenderOrenNayarMtl :
	public FireRenderMtl<FireRenderOrenNayarMtlTraits>
{
public:
	Color GetDiffuse(int mtlNum, BOOL backFace) override;
	frw::Shader getShader(const TimeValue t, MaterialParser& mtlParser, INode* node);
};

FIRERENDER_NAMESPACE_END;
