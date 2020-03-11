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

enum FRAvgMtl_TexmapId {
	FRAvgMtlMtl_TEXMAP_COLOR = 0
};

enum FRAvgMtl_ParamID : ParamID {
	FRAvgMtl_COLOR = 1000,
	FRAvgMtl_COLOR_TEXMAP = 1001
};

BEGIN_DECLARE_FRTEXCLASSDESC(AvgMtl, L"RPR Average", FIRERENDER_AVERAGEMTL_CID)
END_DECLARE_FRTEXCLASSDESC()

BEGIN_DECLARE_FRTEX(AvgMtl)
	virtual  AColor EvalColor(ShadeContext& sc) override
	{
		Color color = GetFromPb<Color>(pblock, FRAvgMtl_COLOR);
		Texmap* colorTexmap = GetFromPb<Texmap*>(pblock, FRAvgMtl_COLOR_TEXMAP);
		if (colorTexmap)
			color = colorTexmap->EvalColor(sc);
		double avg = (color.r + color.g + color.b) * 0.33333333333333333333333333333333;
		return AColor(avg, avg, avg, 1.0);
	}
END_DECLARE_FRTEX(AvgMtl)


FIRERENDER_NAMESPACE_END;
