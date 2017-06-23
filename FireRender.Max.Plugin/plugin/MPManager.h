/*********************************************************************************************************************************
* Radeon ProRender for 3ds Max plugin
* Copyright (c) 2017 AMD
* All Rights Reserved
*
* Material Preview Renderer
*********************************************************************************************************************************/

#pragma once

#include <max.h>
#include <guplib.h>
#include <notify.h>
#include <iparamb2.h>
#include "Common.h"
#include "frWrap.h"
#include "frScope.h"
#include <string>
#include <map>
#include <set>
#include <algorithm>
#include <string> 
#include "plugin/ManagerBase.h"
#include "parser/RenderParameters.h"
#include "parser/SceneCallbacks.h"
#include "plugin/FireRenderer.h"


FIRERENDER_NAMESPACE_BEGIN;

#define MPMANAGER_VERSION 1


class FireRenderer;
class BitmapRenderCore;

class MPManagerMax : public GUP, public ReferenceMaker
{
public:
	static MPManagerMax TheManager;
	Event mRenderingPreview;

public:
	MPManagerMax();
	~MPManagerMax() {}

	// GUP Methods
	DWORD Start() override;
	void Stop() override;

	void DeleteThis() override;

	static ClassDesc* GetClassDesc();

public:
	int Render(class FireRenderer *renderer, TimeValue t, ::Bitmap* tobm, FrameRendParams &frp, HWND hwnd, RendProgressCallback* prog, ViewParams* viewPar,
		INode *sceneNode, INode *viewNode, RendParams &rendParams);

	bool Abort();
	
private:
	enum ShapeKeys
	{
		BallShape1,
		BallShape2,
		BallShape3,
		BackgroundShape,
		SphereShape,
		CylinderShape,
		CubeShape,
		Light = BackgroundShape + 100
	};

	enum
	{
		ObjCylinder,
		ObjCube,
		ObjMatball,
		ObjSphere
	};

	BOOL overrideMaxShapes = TRUE;
	int curObject = ObjMatball;

	int getObjectToRender(INode *sceneNode);
	void setVisible(int whichObject);
	
	enum ShaderKeys
	{
		DiffuseMaterial,
		EmissiveMaterial,
		TexturedBgMaterial,
	};

	Event exitImmediately;

	ScopeID matballScope; // holds the basic matball scene, we won't delete this until we have to
	ScopeID tempScope; // holds temporary translated materials and operations within one rendering cycle
	
	
	// these hold pointers to the stuff loaded from the matball scene
	std::vector<rpr_image> imageList;
	std::vector<rpr_material_node> materialList;
	std::vector<rpr_shape> shapeList;
	std::vector<rpr_light> lightList;
	std::vector<rpr_camera> cameraList;
	
	RenderParameters parameters;
		
	class BitmapRenderCore *renderThread;

	void InitializeScene(IParamBlock2 *pblock);

	void UpdateMaterial();
	void SetMaterial(SceneParser &parser, frw::Shader volumeShader, frw::Shader surfaceShader, bool castShadows, bool env);
	void SetDisplacement(SceneParser &parser, frw::Value img, const float &minHeight, const float &maxHeight,
		const float &subdivision, const float &creaseWeight, int boundary);
	void ResetMaterial();

	Event requestDestroyLoadedAssets;
	void DestroyLoadedAssets(); // destroy the loaded matball scene

	// reference maker
	RefTargetHandle refTarget;
	int NumRefs() override
	{
		return 1;
	}
	RefTargetHandle GetReference(int i) override
	{
		return refTarget;
	}
	void SetReference(int i, RefTargetHandle rtarg) override
	{
		refTarget = rtarg;
	}
	RefResult NotifyRefChanged(NOTIFY_REF_CHANGED_PARAMETERS) override;
};

FIRERENDER_NAMESPACE_END;
