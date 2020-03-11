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

#include <max.h>
#include <guplib.h>
#include <notify.h>

#include "parser/RenderParameters.h"
#include "plugin/FireRenderer.h"

#include <atomic> 

/*
	This class is responsible for drawing thumbnails in Material Editor.
	When MPManagerMax::Render is invoked a Window message loop is started inside this function to provide
	a reaction for UI (the call itself is synchronous for 3ds Max). A separate rendering thread is
	doing rendering itself, implementing several calls to the RPR Core. Scene recalculaion and an acccess to
	the scene from Core must be synchronized (made in one thread in this implementation).
*/

FIRERENDER_NAMESPACE_BEGIN

class FireRenderer;

class MPManagerMax : public GUP, public ReferenceMaker
{
public:
	static MPManagerMax TheManager;

public:
	MPManagerMax();
	~MPManagerMax();

	// GUP Methods
	DWORD Start() override;
	void Stop() override;

	virtual void DeleteThis() override;

	static ClassDesc* GetClassDesc();

public:
	int Render(class FireRenderer *renderer, TimeValue t, ::Bitmap* tobm, FrameRendParams &frp, HWND hwnd,
		RendProgressCallback* prog, ViewParams* viewPar,
		INode* sceneNode, INode* viewNode, RendParams& rendParams);

	bool Abort();

	void UpdateMaterial();

private:
	enum FramebufferType
	{
		FramebufferColor = 0x100,
		FramebufferColorNormalized = 0x101,
	};

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

	int getObjectToRender(INode* sceneNode);
	void setVisible(int whichObject);
	
	enum ShaderKeys
	{
		DiffuseMaterial,
		EmissiveMaterial,
		TexturedBgMaterial,
	};

	ScopeID matballScopeID = -1; // holds the basic matball scene, we won't delete this until we have to
	ScopeID tempScopeID = -1; // holds temporary translated materials and operations within one rendering cycle
	
	// these hold pointers to the stuff loaded from the matball scene
	std::vector<rpr_image> imageList;
	std::vector<rpr_material_node> materialList;
	std::vector<rpr_shape> shapeList;
	std::vector<rpr_light> lightList;
	std::vector<rpr_camera> cameraList;
	
	RenderParameters parameters;
		
	void InitializeScene(IParamBlock2 *pblock);

	void SetMaterial(SceneParser &parser, frw::Shader volumeShader, frw::Shader surfaceShader, bool castShadows, bool env);
	void SetDisplacement(SceneParser &parser, frw::Value img, const float &minHeight, const float &maxHeight,
		const float &subdivision, const float &creaseWeight, int boundary);
	void ResetMaterial();

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

	void renderingThreadProc(int width, int height, std::vector<float>& fbData);

	std::atomic<bool> mRenderingPreview = false;
	std::atomic<bool> mExitImmediately = false;
	std::atomic<bool> mRequestDestroyLoadedAssets = false;
	std::atomic<bool> mPreviewReady = false;
	std::atomic<bool> mRestart = false;
};

class CActiveShadeLocker
{
public:
	CActiveShadeLocker()
	{
		ActiveShader::mGlobalLocker.Fire();
	}

	~CActiveShadeLocker()
	{
		ActiveShader::mGlobalLocker.Reset();
	}
};

FIRERENDER_NAMESPACE_END
