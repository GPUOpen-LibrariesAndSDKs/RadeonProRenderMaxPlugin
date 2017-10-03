#pragma once

#include "FireRenderMtlBase.h"

FIRERENDER_NAMESPACE_BEGIN;

enum FRTransparentMtl_TexmapId {
	FRTransparentMtl_TEXMAP_COLOR = 0
};

enum FRTransparentMtl_ParamID : ParamID {
	FRTransparentMtl_COLOR = 1000,
	FRTransparentMtl_COLOR_TEXMAP = 1001
};

BEGIN_DECLARE_FRMTLCLASSDESC(TransparentMtl, L"RPR Transparent Material", FIRERENDER_TRANSPARENTMTL_CID)
END_DECLARE_FRMTLCLASSDESC()

class FireRenderTransparentMtlTraits
{
public:
	using ClassDesc = FireRenderClassDescTransparentMtl;
};

class FireRenderTransparentMtl :
	public FireRenderMtl<FireRenderTransparentMtlTraits>
{
public:
	Color GetDiffuse(int mtlNum, BOOL backFace) override;
	float GetXParency(int mtlNum, BOOL backFace) override;
	frw::Shader getShader(const TimeValue t, MaterialParser& mtlParser, INode* node);
};

FIRERENDER_NAMESPACE_END;
