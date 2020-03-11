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

enum FRFresnelSchlickMtl_TexmapId {
	FRFresnelSchlickMtl_TEXMAP_INVEC = 0,
	FRFresnelSchlickMtl_TEXMAP_N = 1,
	FRFresnelSchlickMtl_TEXMAP_REFLECTANCE = 2,
};

enum FRFresnelSchlickMtl_ParamID : ParamID {
	FRFresnelSchlickMtl_REFLECTANCE = 1001,
	FRFresnelSchlickMtl_INVEC_TEXMAP = 1003,
	FRFresnelSchlickMtl_N_TEXMAP = 1004,
	FRFresnelSchlickMtl_REFLECTANCE_TEXMAP = 1005
};

BEGIN_DECLARE_FRTEXCLASSDESC(FresnelSchlickMtl, L"RPR Fresnel-Schlick", FIRERENDER_FRESNELSCHLICKMTL_CID)
END_DECLARE_FRTEXCLASSDESC()

BEGIN_DECLARE_FRTEX(FresnelSchlickMtl)
END_DECLARE_FRTEX(FresnelSchlickMtl)


FIRERENDER_NAMESPACE_END;
