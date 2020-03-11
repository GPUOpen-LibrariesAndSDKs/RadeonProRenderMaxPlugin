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

enum FROrenNayarMtl_TexmapId {
	FROrenNayarMtl_TEXMAP_DIFFUSE = 0,
	FROrenNayarMtl_TEXMAP_ROUGHNESS = 1,
	FROrenNayarMtl_TEXMAP_NORMAL = 2
};

enum FROrenNayarMtl_ParamID : ParamID {
	FROrenNayarMtl_COLOR = 1000,
	FROrenNayarMtl_ROUGHNESS = 1001,
	FROrenNayarMtl_COLOR_TEXMAP = 1002,
	FROrenNayarMtl_ROUGHNESS_TEXMAP = 1003,
	FROrenNayarMtl_NORMALMAP = 1004
};

BEGIN_DECLARE_FRMTLCLASSDESC(OrenNayarMtl, L"RPR Oren-Nayar Material", FIRERENDER_ORENNAYARMTL_CID)
END_DECLARE_FRMTLCLASSDESC()

BEGIN_DECLARE_FRMTL(OrenNayarMtl)

virtual Color GetDiffuse(int mtlNum, BOOL backFace) override {
	return GetFromPb<Color>(pblock, FROrenNayarMtl_COLOR);
}

END_DECLARE_FRMTL(OrenNayarMtl)

FIRERENDER_NAMESPACE_END;
