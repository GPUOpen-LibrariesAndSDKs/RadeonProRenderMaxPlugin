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

class FireRenderDiffuseMtlTraits
{
public:
	static const TCHAR* InternalName() { return _T("RPR Diffuse Material"); }
	static Class_ID ClassId() { return FIRERENDER_DIFFUSEMTL_CID; }
};

class FireRenderDiffuseMtl :
	public FireRenderMtl<FireRenderDiffuseMtlTraits, FireRenderDiffuseMtl>
{
public:
	frw::Shader getShader(const TimeValue t, class MaterialParser& mtlParser, INode* node);
	Color GetDiffuse(int mtlNum, BOOL backFace) override;
};

FIRERENDER_NAMESPACE_END;
