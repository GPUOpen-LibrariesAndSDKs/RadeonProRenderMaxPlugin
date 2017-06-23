/*********************************************************************************************************************************
* Radeon ProRender for 3ds Max plugin
* Copyright (c) 2017 AMD
* All Rights Reserved
*
* Base class for property managers. Property managers handle property changes and propagate them to registered listeners.
*********************************************************************************************************************************/

#include "ManagerBase.h"

FIRERENDER_NAMESPACE_BEGIN;

ManagerPropertyChangeCallback::~ManagerPropertyChangeCallback()
{
	if (mManager)
		mManager->UnregisterPropertyChangeCallback(this);
}

FIRERENDER_NAMESPACE_END;
