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


#include "FireRenderIES_Panel.h"

FIRERENDER_NAMESPACE_BEGIN

const float MaxSpinner::DefaultFloatSettings::Min = 0.0f;
const float MaxSpinner::DefaultFloatSettings::Max = FLT_MAX;
const float MaxSpinner::DefaultFloatSettings::Default = 1.0f;
const float MaxSpinner::DefaultFloatSettings::Delta = 0.1f;

const float MaxSpinner::DefaultRotationSettings::Min = -180.0f;
const float MaxSpinner::DefaultRotationSettings::Max = 180.0f;
const float MaxSpinner::DefaultRotationSettings::Default = 0.0f;
const float MaxSpinner::DefaultRotationSettings::Delta = 1.0f;

FIRERENDER_NAMESPACE_END
