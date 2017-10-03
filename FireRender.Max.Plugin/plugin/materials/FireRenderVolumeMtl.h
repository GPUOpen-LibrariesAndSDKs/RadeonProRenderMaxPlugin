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

class FireRenderVolumeMtlTraits
{
public:
	static const TCHAR* InternalName() { return _T("RPR Volume Material"); }
	static Class_ID ClassId() { return FIRERENDER_VOLUMEMTL_CID; }
};

class FireRenderVolumeMtl :
	public FireRenderMtl<FireRenderVolumeMtlTraits, FireRenderVolumeMtl>
{
public:
	frw::Shader getVolumeShader(const TimeValue t, MaterialParser& mtlParser, INode* node);
	frw::Shader getShader(const TimeValue t, MaterialParser& mtlParser, INode* node);
};

FIRERENDER_NAMESPACE_END;
