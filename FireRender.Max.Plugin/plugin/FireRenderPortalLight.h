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

FIRERENDER_NAMESPACE_BEGIN

enum PortalLightParameter : ParamID
{
	PORTAL_PARAM_P0 = 101,
	PORTAL_PARAM_P1 = 102
};

ClassDesc2* GetFireRenderPortalLightDesc();
const Class_ID FIRERENDER_PORTALLIGHT_CLASS_ID(0x7ab5467f, 0x1c96049e);

class IFireRenderPortalLight
{
public:
	virtual void GetCorners(Point3& p1, Point3& p2, Point3& p3, Point3& p4) = 0;
};

FIRERENDER_NAMESPACE_END
