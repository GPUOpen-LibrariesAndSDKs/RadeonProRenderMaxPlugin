#pragma once

#include "FireRenderMtlBase.h"

FIRERENDER_NAMESPACE_BEGIN;

enum FRVolumeMtl_TexmapId {
	FRVolumeMtl_TEXMAP_COLOR = 0,
	FRVolumeMtl_TEXMAP_DISTANCE = 1,
	FRVolumeMtl_TEXMAP_EMISSION = 2
};

enum FRVolumeMtl_ParamID : ParamID {
	FRVolumeMtl_Color = 1000,
	FRVolumeMtl_ColorTexmap,
	FRVolumeMtl_Distance,
	FRVolumeMtl_DistanceTexmap,
	FRVolumeMtl_EmissionMultiplier,
	FRVolumeMtl_EmissionColor,
	FRVolumeMtl_EmissionColorTexmap,
	FRVolumeMtl_ScatteringDirection,
	FRVolumeMtl_MultiScattering
};

BEGIN_DECLARE_FRMTLCLASSDESC(VolumeMtl, L"RPR Volume Material", FIRERENDER_VOLUMEMTL_CID)
END_DECLARE_FRMTLCLASSDESC()

BEGIN_DECLARE_FRMTL(VolumeMtl)
public:
	frw::Shader getVolumeShader(const TimeValue t, MaterialParser& mtlParser, INode* node);
END_DECLARE_FRMTL(VolumeMtl)

FIRERENDER_NAMESPACE_END;
