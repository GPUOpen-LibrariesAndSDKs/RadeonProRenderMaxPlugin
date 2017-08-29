#pragma once

#include "FireRenderMtlBase.h"

FIRERENDER_NAMESPACE_BEGIN;

enum FRPbrMtl_TexmapId
{
	FRPBRMTL_MAP_DIFFUSE_COLOR     = 0,
};

enum FRPbrMtl_ParamID : ParamID
{
	FRPBRMTL_DIFFUSE_COLOR_MUL = 1000,
	FRPBRMTL_DIFFUSE_COLOR     = 1001,
	FRPBRMTL_DIFFUSE_COLOR_MAP = 1002
};

BEGIN_DECLARE_FRMTLCLASSDESC(PbrMtl, L"RPR PBR Material", FIRERENDER_PBRMTL_CID)
END_DECLARE_FRMTLCLASSDESC()

BEGIN_DECLARE_FRMTL(PbrMtl)

public:
	virtual Color GetDiffuse(int mtlNum, BOOL backFace) override
	{
		return GetFromPb<Color>(pblock, FRPBRMTL_DIFFUSE_COLOR);
	}

	frw::Shader getVolumeShader(const TimeValue t, MaterialParser& mtlParser, INode* node);

END_DECLARE_FRMTL(PbrMtl)

FIRERENDER_NAMESPACE_END;
