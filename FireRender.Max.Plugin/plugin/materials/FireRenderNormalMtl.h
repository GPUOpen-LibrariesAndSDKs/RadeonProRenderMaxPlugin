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

FIRERENDER_NAMESPACE_BEGIN

enum FRNormalMtl_TexmapId
{
	FRNormalMtl_TEXMAP_COLOR = 0,
	FRNormalMtl_TEXMAP_ADDITIONALBUMP = 1
};

enum FRNormalMtl_ParamID : ParamID
{
	FRNormalMtl_COLOR_TEXMAP = 1000,
	FRNormalMtl_STRENGTH,
	FRNormalMtl_ISBUMP,
	FRNormalMtl_ADDITIONALBUMP,
	FRNormalMtl_SWAPRG,
	FRNormalMtl_INVERTG,
	FRNormalMtl_INVERTR,
	FRNormalMtl_BUMPSTRENGTH
};

BEGIN_DECLARE_FRTEXCLASSDESC(NormalMtl, L"RPR Normal", FIRERENDER_NORMALMTL_CID)
END_DECLARE_FRTEXCLASSDESC()

BEGIN_DECLARE_FRTEX(NormalMtl)

	static frw::Value translateGenericBump(const TimeValue t, Texmap* bump, const float& strength, MaterialParser& mtlParser);
			
END_DECLARE_FRTEX(NormalMtl)

FIRERENDER_NAMESPACE_END
