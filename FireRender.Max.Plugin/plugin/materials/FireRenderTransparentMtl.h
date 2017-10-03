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

class FireRenderTransparentMtlTraits
{
public:
	static const TCHAR* InternalName() { return _T("RPR Transparent Material"); }
	static Class_ID ClassId() { return FIRERENDER_TRANSPARENTMTL_CID; }
};

class FireRenderTransparentMtl :
	public FireRenderMtl<FireRenderTransparentMtlTraits, FireRenderTransparentMtl>
{
public:
	Color GetDiffuse(int mtlNum, BOOL backFace) override;
	float GetXParency(int mtlNum, BOOL backFace) override;
	frw::Shader getShader(const TimeValue t, MaterialParser& mtlParser, INode* node);
};

FIRERENDER_NAMESPACE_END;
