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

enum FRDiffuseRefractionMtl_TexmapId {
	FRDiffuseRefractionMtl_TEXMAP_DIFFUSE = 0,
	FRDiffuseRefractionMtl_TEXMAP_ROUGHNESS = 1,
	FRDiffuseRefractionMtl_TEXMAP_NORMAL = 2
};

enum FRDiffuseRefractionMtl_ParamID : ParamID {
	FRDiffuseRefractionMtl_COLOR = 1000,
	FRDiffuseRefractionMtl_ROUGHNESS = 1001,
	FRDiffuseRefractionMtl_COLOR_TEXMAP = 1002,
	FRDiffuseRefractionMtl_ROUGHNESS_TEXMAP = 1003,
	FRDiffuseRefractionMtl_NORMALMAP = 1004
};

BEGIN_DECLARE_FRMTLCLASSDESC(DiffuseRefractionMtl, L"RPR Diffuse Refraction Material", FIRERENDER_DIFFUSEREFRACTIONMTL_CID)
END_DECLARE_FRMTLCLASSDESC()

BEGIN_DECLARE_FRMTL(DiffuseRefractionMtl)

virtual Color GetDiffuse(int mtlNum, BOOL backFace) override {
	return GetFromPb<Color>(pblock, FRDiffuseRefractionMtl_COLOR);
}

END_DECLARE_FRMTL(DiffuseRefractionMtl)

FIRERENDER_NAMESPACE_END;
