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

enum FRReflectionMtl_TexmapId {
	FRReflectionMtl_TEXMAP_COLOR = 0,
	FRReflectionMtl_TEXMAP_NORMAL = 1
};

enum FRReflectionMtl_ParamID : ParamID {
	FRReflectionMtl_COLOR = 1000,
	FRReflectionMtl_COLOR_TEXMAP = 1002,
	FRReflectionMtl_NORMALMAP = 1003
};

BEGIN_DECLARE_FRMTLCLASSDESC(ReflectionMtl, L"RPR Reflection Material", FIRERENDER_REFLECTIONMTL_CID)
END_DECLARE_FRMTLCLASSDESC()

BEGIN_DECLARE_FRMTL(ReflectionMtl)

virtual Color GetDiffuse(int mtlNum, BOOL backFace) override {
	return GetFromPb<Color>(pblock, FRReflectionMtl_COLOR);
}

END_DECLARE_FRMTL(ReflectionMtl)

FIRERENDER_NAMESPACE_END;
