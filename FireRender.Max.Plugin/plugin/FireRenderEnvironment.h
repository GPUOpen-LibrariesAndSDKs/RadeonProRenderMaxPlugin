/*********************************************************************************************************************************
* Radeon ProRender for 3ds Max plugin
* Copyright (c) 2017 AMD
* All Rights Reserved
*
* Custom environment 3ds MAX scene node (compass). The environment node allows to manually rotate the environment (or define the
* North direction for the sky system). If the environment node is deleted, the Radeon ProRender custom environment is
* deactivated, and the renderer will use the current 3ds max environment settings instead.
*********************************************************************************************************************************/

FIRERENDER_NAMESPACE_BEGIN
ClassDesc2* GetFireRenderEnvironmentDesc();
const Class_ID FIRERENDER_ENVIRONMENT_CLASS_ID(0x2e1b2bdd, 0x35c7052e);

FIRERENDER_NAMESPACE_END