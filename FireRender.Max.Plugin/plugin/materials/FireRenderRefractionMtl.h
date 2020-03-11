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

enum FRRefractionMtl_TexmapId {
	FRRefractionMtl_TEXMAP_COLOR = 0,
	FRRefractionMtl_TEXMAP_NORMAL = 1
};

enum FRRefractionMtl_ParamID : ParamID {
	FRRefractionMtl_COLOR = 1000,
	FRRefractionMtl_IOR = 1001,
	FRRefractionMtl_COLOR_TEXMAP = 1002,
	FRRefractionMtl_NORMALMAP = 1003
};

BEGIN_DECLARE_FRMTLCLASSDESC(RefractionMtl, L"RPR Refraction Material", FIRERENDER_REFRACTIONMTL_CID)
END_DECLARE_FRMTLCLASSDESC()

BEGIN_DECLARE_FRMTL(RefractionMtl)

virtual Color GetDiffuse(int mtlNum, BOOL backFace) override {
	return GetFromPb<Color>(pblock, FRRefractionMtl_COLOR);
}

virtual float GetXParency(int mtlNum, BOOL backFace) override {
	return 0.5f;
}

END_DECLARE_FRMTL(RefractionMtl)

FIRERENDER_NAMESPACE_END;
