/**********************************************************************
Copyright 2020 Advanced Micro Devices, Inc
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at
    http://www.apache.org/licenses/LICENSE-2.0
Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
********************************************************************/


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
