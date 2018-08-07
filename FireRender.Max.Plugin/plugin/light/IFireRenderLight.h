/*********************************************************************************************************************************
* Radeon ProRender for 3ds Max plugin
* Copyright (c) 2017 AMD
* All Rights Reserved
*
* Interface class for RPR Max Plugin lights
*********************************************************************************************************************************/
#pragma once

#include "Common.h"
#include "parser/SceneParser.h"
#include "parser/RenderParameters.h"

class IFireRenderLight
{
public:
	virtual void CreateSceneLight(
		TimeValue t, const FireRender::ParsedNode& node, frw::Scope scope,
		FireRender::SceneAttachCallback* sceneAttachCallback=NULL ) = 0;
	virtual bool DisplayLight(TimeValue t, INode* inode, ViewExp *vpt, int flags) = 0;
	virtual bool CalculateBBox(void) = 0;
};
