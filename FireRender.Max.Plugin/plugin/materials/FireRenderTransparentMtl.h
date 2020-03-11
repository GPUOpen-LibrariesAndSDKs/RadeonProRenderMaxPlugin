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

enum FRTransparentMtl_TexmapId {
	FRTransparentMtl_TEXMAP_COLOR = 0
};

enum FRTransparentMtl_ParamID : ParamID {
	FRTransparentMtl_COLOR = 1000,
	FRTransparentMtl_COLOR_TEXMAP = 1001
};

BEGIN_DECLARE_FRMTLCLASSDESC(TransparentMtl, L"RPR Transparent Material", FIRERENDER_TRANSPARENTMTL_CID)
END_DECLARE_FRMTLCLASSDESC()

BEGIN_DECLARE_FRMTL(TransparentMtl)

virtual Color GetDiffuse(int mtlNum, BOOL backFace) override {
	return GetFromPb<Color>(pblock, FRTransparentMtl_COLOR);
}

virtual float GetXParency(int mtlNum, BOOL backFace) override {
	Color c = GetFromPb<Color>(pblock, FRTransparentMtl_COLOR);
	return (c.r + c.b + c.g) * (1.f / 3.f);
}

END_DECLARE_FRMTL(TransparentMtl)

FIRERENDER_NAMESPACE_END;
