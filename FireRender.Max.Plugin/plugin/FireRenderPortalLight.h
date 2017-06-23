/*********************************************************************************************************************************
* Radeon ProRender for 3ds Max plugin
* Copyright (c) 2017 AMD
* All Rights Reserved
*
* Custom portal light 3ds MAX scene node. This custom node allows the definition of portal light, using rectangular shapes
* that can be positioned and oriented in the MAX scene.
*********************************************************************************************************************************/

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
