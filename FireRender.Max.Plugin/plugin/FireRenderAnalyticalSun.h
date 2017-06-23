/*********************************************************************************************************************************
* Radeon ProRender for 3ds Max plugin
* Copyright (c) 2017 AMD
* All Rights Reserved
*
* Custom analytical sun 3ds MAX scene node. The Analytical sun allows to manually control azimuth and elevation of the sun,
* when the sky system is set to analytical mode.
*********************************************************************************************************************************/

#pragma once

FIRERENDER_NAMESPACE_BEGIN

ClassDesc2* GetFireRenderAnalyticalSunDesc();
const Class_ID FIRERENDER_ANALYTICALSUN_CLASS_ID(0x21e80a8c, 0x58495b83);

FIRERENDER_NAMESPACE_END
