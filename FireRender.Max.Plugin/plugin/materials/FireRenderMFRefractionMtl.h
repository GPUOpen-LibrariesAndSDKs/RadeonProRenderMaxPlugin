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

enum FRMFRefractionMtl_TexmapId {
	FRMFRefractionMtl_TEXMAP_COLOR = 0,
	FRMFRefractionMtl_TEXMAP_ROUGHNESS = 1,
	FRMFRefractionMtl_TEXMAP_NORMAL = 2
};

enum FRMFRefractionMtl_ParamID : ParamID {
	FRMFRefractionMtl_COLOR = 1000,
	FRMFRefractionMtl_IOR = 1001,
	FRMFRefractionMtl_ROUGHNESS = 1002,
	FRMFRefractionMtl_COLOR_TEXMAP = 1003,
	FRMFRefractionMtl_ROUGHNESS_TEXMAP = 1004,
	FRMFRefractionMtl_NORMALMAP = 1005,
};

BEGIN_DECLARE_FRMTLCLASSDESC(MFRefractionMtl, L"RPR Microfacet Refraction Material", FIRERENDER_MFREFRACTIONMTL_CID)
END_DECLARE_FRMTLCLASSDESC()

BEGIN_DECLARE_FRMTL(MFRefractionMtl)

virtual Color GetDiffuse(int mtlNum, BOOL backFace) override {
	return GetFromPb<Color>(pblock, FRMFRefractionMtl_COLOR);
}

virtual float GetXParency(int mtlNum, BOOL backFace) override {
	return 0.5f;
}

END_DECLARE_FRMTL(MFRefractionMtl)

FIRERENDER_NAMESPACE_END;
