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

enum FRColourCorMtl_TexmapId {
	FRColourCorMtl_TEXMAP_COLOR = 0
};

enum FRColourCorMtl_ParamID : ParamID {
	FRColourCorMtl_COLOR_TEXMAP = 1000,
	FRColourCorMtl_BOUNDARY,
	FRColourCorMtl_ISENABLED,
	FRColourCorMtl_USECUSTOM,
	FRColourCorMtl_CUSTOMGAMMA,
};

enum RPR_GAMMA_TYPE
{
	RPR_GAMMA_RAW,
	RPR_GAMMA_SRGB,
};

BEGIN_DECLARE_FRTEXCLASSDESC(ColourCorMtl, L"Texture Gamma Correction", FIRERENDER_COLOURCORMTL_CID)
END_DECLARE_FRTEXCLASSDESC()

BEGIN_DECLARE_FRTEX(ColourCorMtl)
virtual AColor EvalColor(ShadeContext& sc) override
{
	Color color(0.f, 0.f, 0.f);

	Texmap* colorTexmap = GetFromPb<Texmap*>(pblock, FRColourCorMtl_COLOR_TEXMAP);
	if (colorTexmap)
		color = colorTexmap->EvalColor(sc);

	return AColor(color.r, color.g, color.b);
}


END_DECLARE_FRTEX(ColourCorMtl)


FIRERENDER_NAMESPACE_END;
