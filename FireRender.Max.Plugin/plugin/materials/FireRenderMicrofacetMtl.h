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

enum FRMicrofacetMtl_TexmapId {
	FRMicrofacetMtl_TEXMAP_COLOR = 0,
	FRMicrofacetMtl_TEXMAP_ROUGHNESS = 1,
	FRMicrofacetMtl_TEXMAP_NORMAL = 2
};

enum FRMicrofacetMtl_ParamID : ParamID {
	FRMicrofacetMtl_COLOR = 1000,
	FRMicrofacetMtl_ROUGHNESS = 1001,
	FRMicrofacetMtl_IOR = 1002,
	FRMicrofacetMtl_COLOR_TEXMAP = 1003,
	FRMicrofacetMtl_ROUGHNESS_TEXMAP = 1004,
	FRMicrofacetMtl_NORMALMAP = 1005
};

BEGIN_DECLARE_FRMTLCLASSDESC(MicrofacetMtl, L"RPR Microfacet Material", FIRERENDER_MICROFACETMTL_CID)
END_DECLARE_FRMTLCLASSDESC()

BEGIN_DECLARE_FRMTL(MicrofacetMtl)

virtual Color GetDiffuse(int mtlNum, BOOL backFace) override {
	return GetFromPb<Color>(pblock, FRMicrofacetMtl_COLOR);
}

END_DECLARE_FRMTL(MicrofacetMtl)

FIRERENDER_NAMESPACE_END;
