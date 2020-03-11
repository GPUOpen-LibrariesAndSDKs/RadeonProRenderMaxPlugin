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

enum FRBlendValueMtl_TexmapId {
	FRBlendValueMtl_TEXMAP_COLOR0 = 0,
	FRBlendValueMtl_TEXMAP_COLOR1 = 1,
	FRBlendValueMtl_TEXMAP_WEIGHT = 2
};

enum FRBlendValueMtl_ParamID : ParamID {
	FRBlendValueMtl_COLOR0 = 1000,
	FRBlendValueMtl_COLOR1 = 1001,
	FRBlendValueMtl_WEIGHT = 1002,
	FRBlendValueMtl_COLOR0_TEXMAP = 1003,
	FRBlendValueMtl_COLOR1_TEXMAP = 1004,
	FRBlendValueMtl_WEIGHT_TEXMAP = 1005
};

BEGIN_DECLARE_FRTEXCLASSDESC(BlendValueMtl, L"RPR Blend Value", FIRERENDER_BLENDVALUEMTL_CID)
END_DECLARE_FRTEXCLASSDESC()

BEGIN_DECLARE_FRTEX(BlendValueMtl)
virtual  AColor EvalColor(ShadeContext& sc) override
{
	Color color0 = GetFromPb<Color>(pblock, FRBlendValueMtl_COLOR0);
	Texmap* color0Texmap = GetFromPb<Texmap*>(pblock, FRBlendValueMtl_COLOR0_TEXMAP);
	if (color0Texmap)
		color0 = color0Texmap->EvalColor(sc);

	Color color1 = GetFromPb<Color>(pblock, FRBlendValueMtl_COLOR1);
	Texmap* color1Texmap = GetFromPb<Texmap*>(pblock, FRBlendValueMtl_COLOR1_TEXMAP);
	if (color1Texmap)
		color1 = color1Texmap->EvalColor(sc);

	float w = GetFromPb<float>(pblock, FRBlendValueMtl_WEIGHT);
	Texmap* wTexmap = GetFromPb<Texmap*>(pblock, FRBlendValueMtl_WEIGHT_TEXMAP);
	if (wTexmap)
		w = wTexmap->EvalColor(sc).r;

	return ((color0 * w) + (color1 * (1.f - w)));
}
END_DECLARE_FRTEX(BlendValueMtl)


FIRERENDER_NAMESPACE_END;
