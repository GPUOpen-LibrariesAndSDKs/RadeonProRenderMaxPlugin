/*********************************************************************************************************************************
* Radeon ProRender for 3ds Max plugin
* Copyright (C) 2017 AMD
* All Rights Reserved
* 
* An aggregate of all render settings objects we need
*********************************************************************************************************************************/
#pragma once
#include "Common.h"
#include "utils/Stack.h"
#include "utils/Utils.h"
FIRERENDER_NAMESPACE_BEGIN;

typedef struct _FireRenderView : public View
{
	virtual Point2 ViewToScreen(Point3 p)
	{
		STOP;
	}
} FireRenderView;

// This class aggregates all render configuration classes we encounter during rendering
struct RenderParameters
{

    // Root node of the scene that is being parsed/rendered
    INode* sceneNode;

    // Node holding the camera we are currently parsing/rendering. Can be NULL for free views
    INode* viewNode;

    // Any default lights that should be included in the render
    Stack<DefaultLight> defaultLights;

    // Param block from the RPR plugin. It holds the render settings that should be used for the actual frame
    IParamBlock2* pblock;

    // 3ds Max interface for reporting rendering progress and signalling that the render was cancelled
    RendProgressCallback* progress;

    // an interface we are required to provide to GetRenderMesh(), but it does not appear to be used anywhere
	FireRenderView view;

    // Various 3ds Max render parameters classes
    ViewParams viewParams;
    RendParams rendParams;
    FrameRendParams frameRendParams;
    RenderGlobalContext renderGlobalContext;
    TimeValue t;
    HWND hWnd;
};


FIRERENDER_NAMESPACE_END;