/*********************************************************************************************************************************
* Radeon ProRender for 3ds Max plugin
* Copyright (c) 2017 AMD
* All Rights Reserved
*
* Additional plug-in to export (individually or  in bulk) materials to Radeon ProRender XML files
*********************************************************************************************************************************/

#pragma once

#include "Common.h"
#include "ClassDescs.h"

FIRERENDER_NAMESPACE_BEGIN;

class IFRMaterialExporter
{
public:
	static FireRenderClassDesc_MaterialExport *GetClassDesc();
};

FIRERENDER_NAMESPACE_END;
