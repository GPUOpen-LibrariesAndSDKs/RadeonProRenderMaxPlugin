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

enum FRDiffuseMtl_TexmapId {
	FRDiffuseMtl_TEXMAP_DIFFUSE = 0,
	FRDiffuseMtl_TEXMAP_NORMAL = 1
};

enum FRDiffuseMtl_ParamID : ParamID {
	FRDiffuseMtl_COLOR = 1000,
	FRDiffuseMtl_COLOR_TEXMAP = 1002,
	FRDiffuseMtl_NORMALMAP = 1003
};

BEGIN_DECLARE_FRMTLCLASSDESC(DiffuseMtl, L"RPR Diffuse Material", FIRERENDER_DIFFUSEMTL_CID)
END_DECLARE_FRMTLCLASSDESC()

BEGIN_DECLARE_FRMTL(DiffuseMtl)
public:
virtual Color GetDiffuse(int mtlNum, BOOL backFace) override {
	return GetFromPb<Color>(pblock, FRDiffuseMtl_COLOR);
}
END_DECLARE_FRMTL(DiffuseMtl)

FIRERENDER_NAMESPACE_END;
