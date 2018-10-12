#pragma once

#include "FireRenderMtlBase.h"

FIRERENDER_NAMESPACE_BEGIN

enum FRNormalMtl_TexmapId
{
	FRNormalMtl_TEXMAP_COLOR = 0,
	FRNormalMtl_TEXMAP_ADDITIONALBUMP = 1
};

enum FRNormalMtl_ParamID : ParamID
{
	FRNormalMtl_COLOR_TEXMAP = 1000,
	FRNormalMtl_STRENGTH,
	FRNormalMtl_ISBUMP,
	FRNormalMtl_ADDITIONALBUMP,
	FRNormalMtl_SWAPRG,
	FRNormalMtl_INVERTG,
	FRNormalMtl_INVERTR,
	FRNormalMtl_BUMPSTRENGTH
};

BEGIN_DECLARE_FRTEXCLASSDESC(NormalMtl, L"RPR Normal", FIRERENDER_NORMALMTL_CID)
END_DECLARE_FRTEXCLASSDESC()

BEGIN_DECLARE_FRTEX(NormalMtl)

	static frw::Value translateGenericBump(const TimeValue t, Texmap* bump, const float& strength, MaterialParser& mtlParser);
			
END_DECLARE_FRTEX(NormalMtl)

FIRERENDER_NAMESPACE_END
