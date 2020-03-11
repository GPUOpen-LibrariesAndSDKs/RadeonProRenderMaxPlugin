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

#include "Common.h"

FIRERENDER_NAMESPACE_BEGIN

enum IESLightColorMode
{
	IES_LIGHT_COLOR_MODE_COLOR,
	IES_LIGHT_COLOR_MODE_TEMPERATURE
};

enum IESLightParameter : ParamID
{
	IES_PARAM_P0 = 101,
	IES_PARAM_P1,
	IES_PARAM_ENABLED,
	IES_PARAM_PROFILE,
	IES_PARAM_AREA_WIDTH,
	IES_PARAM_LIGHT_ROTATION_X,
	IES_PARAM_LIGHT_ROTATION_Y,
	IES_PARAM_LIGHT_ROTATION_Z,
	IES_PARAM_TARGETED,
	IES_PARAM_INTENSITY,
	IES_PARAM_COLOR_MODE,
	IES_PARAM_COLOR,
	IES_PARAM_TEMPERATURE,
	IES_PARAM_SHADOWS_ENABLED,
	IES_PARAM_SHADOWS_SOFTNESS,
	IES_PARAM_SHADOWS_TRANSPARENCY,
	IES_PARAM_VOLUME_SCALE
};

FIRERENDER_NAMESPACE_END
