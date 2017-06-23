/*********************************************************************************************************************************
* Radeon ProRender for 3ds Max plugin
* Copyright (c) 2017 AMD
* All Rights Reserved
*
* Material Preview Renderer
*********************************************************************************************************************************/

#include <math.h>
#include "MPManager.h"
#include "resource.h"
#include "plugin/ParamBlock.h"
#include <wingdi.h>
#include "utils/Thread.h"
#include "AssetManagement\IAssetAccessor.h"
#include "assetmanagement\AssetType.h"
#include "Assetmanagement\iassetmanager.h"
#include <Bitmap.h>
#include "plugin/FireRenderDisplacementMtl.h"
#include "plugin/FireRenderMaterialMtl.h"
#include "plugin/ScopeManager.h"
#include <RadeonProRender.h>
#include <RprLoadStore.h>

extern HINSTANCE hInstance;

FIRERENDER_NAMESPACE_BEGIN;

#define BMUI_TIMER_PERIOD 30

static Event bmDone;

class BitmapRenderCore;

class BitmapRenderCore : public BaseThread
{
public:
	volatile unsigned int passLimit;
	volatile unsigned int passesDone;
	volatile int catastrophe;

private:
	frw::Scope scope;
	frw::FrameBuffer frameBufferMain;
	std::vector<float> fbData;
	Event eRestart;
	
	bool DumpFrameBuffer();

public:
	inline const std::vector<float>& GetFrameBuffer()
	{
		return fbData;
	}

	explicit BitmapRenderCore(frw::Scope rscope, int w, int h, int priority = THREAD_PRIORITY_NORMAL, const char* name = "BitmapRenderCore");
	~BitmapRenderCore();

	void Worker() override;

	void Restart();
};


bool BitmapRenderCore::DumpFrameBuffer()
{
	if (!frameBufferMain.Handle())
		return false;

	size_t fbSize;
	int res = rprFrameBufferGetInfo(frameBufferMain.Handle(), RPR_FRAMEBUFFER_DATA, 0, NULL, &fbSize);
	FASSERT(res == RPR_SUCCESS);

	// Get all image data from Radeon ProRender in single call, then use multiple threads to copy them over to 3ds Max
	fbData.resize(int(fbSize / sizeof(float)));
	res = rprFrameBufferGetInfo(frameBufferMain.Handle() , RPR_FRAMEBUFFER_DATA, fbData.size() * sizeof(float), fbData.data(), NULL);
	FASSERT(res == RPR_SUCCESS);

	size_t i = 0, sz = fbData.size();
	while (i < sz)
	{
		if (fbData[i + 3] > 0)
		{
			const float actualExp = 1.f / fbData[i + 3];
			fbData[i++] *= actualExp;
			fbData[i++] *= actualExp;
			fbData[i++] *= actualExp;
		}
		fbData[i++] = 1.f;
	}

	return true;
}

BitmapRenderCore::BitmapRenderCore(frw::Scope rscope, int w, int h, int priority, const char* name)
: BaseThread(name, priority), scope(rscope), eRestart(true)
{
	mSelfDelete = false;
	passLimit = 0;
	catastrophe = RPR_SUCCESS;
	
	frameBufferMain = scope.GetFrameBuffer(w, h, 0);
	
	auto res = rprContextSetAOV(scope.GetContext().Handle(), RPR_AOV_COLOR, frameBufferMain.Handle());
	FASSERT(res == RPR_SUCCESS);
	res = rprContextSetParameter1u(scope.GetContext().Handle(), "preview", 1);
	FASSERT(res == RPR_SUCCESS);
}

BitmapRenderCore::~BitmapRenderCore()
{
	scope.DestroyFrameBuffer(0);
	frameBufferMain.Reset();

	auto res = rprContextSetAOV(scope.GetContext().Handle(), RPR_AOV_COLOR, 0);
	FASSERT(res == RPR_SUCCESS);
}

void BitmapRenderCore::Worker()
{
	unsigned int numPasses = passLimit;
	if (numPasses < 1)
		numPasses = 1;
	unsigned int numPassesMinusOne = numPasses - 1;
	passesDone = 0;

	bool clearFramebuffer = true;

	// render all passes, unless the render is cancelled (and even then render at least a single pass)
	for (; passesDone < numPasses; InterlockedIncrement(&passesDone))
	{
		if (mStop.Wait(0))
		{
			catastrophe = RPR_SUCCESS;
			passesDone = UINT_MAX;
			break;
		}
		if (eRestart.Wait(0))
		{
			clearFramebuffer = true;
			passesDone = 0;
		}
		
		if (clearFramebuffer)
		{
			frameBufferMain.Clear();
			clearFramebuffer = false;
		}

		rpr_int res = RPR_SUCCESS;
		try
		{
			res = rprContextRender(scope.GetContext().Handle());
		}
		catch (...)
		{
			debugPrint("Exception occurred in render call");
			catastrophe = RPR_ERROR_INTERNAL_ERROR;
			passesDone = UINT_MAX;
			bmDone.Fire();
			return;
		}

		if (res != RPR_SUCCESS)
		{
			TheManager->Max()->Log()->LogEntry(SYSLOG_WARN, NO_DIALOG, L"Radeon ProRender - Warning", L"rprContextRender returned '%d'\n", res);
			catastrophe = res;
			passesDone = UINT_MAX;
			bmDone.Fire();
			return;
		}

		// material preview / swatches do not update their bitmap before the render function returns
		// it seems that the bitmap passed to the render function is a provisional object, it is
		// not the actual bitmap that will be displayed.
		// therefore, we do not need to visualize incremental updates
		if (passesDone >= numPassesMinusOne)
		{
			catastrophe = RPR_SUCCESS;
			DumpFrameBuffer();
			bmDone.Fire();
			break;
		}
	}
}

void BitmapRenderCore::Restart()
{
	eRestart.Fire();
	if (!IsRunning())
		Start();
}



//////////////////////////////////////////////////////////////////////////////
//

MPManagerMax MPManagerMax::TheManager;


class MPManagerMaxClassDesc : public ClassDesc
{
public:
	int             IsPublic() { return FALSE; }
	void*           Create(BOOL loading) { return &MPManagerMax::TheManager; }
	const TCHAR*    ClassName() { return _T("RPRMPManager"); }
	SClass_ID       SuperClassID() { return GUP_CLASS_ID; }
	Class_ID        ClassID() { return MPMANAGER_MAX_CLASSID; }
	const TCHAR*    Category() { return _T(""); }
};

static MPManagerMaxClassDesc MPManagerMaxCD;

ClassDesc* MPManagerMax::GetClassDesc()
{
	return &MPManagerMaxCD;
}


MPManagerMax::MPManagerMax()
	: exitImmediately(false)
	, renderThread(0)
	, requestDestroyLoadedAssets(false)
	, matballScope(-1)
	, tempScope(-1)
	, mRenderingPreview(false)
{
}

// Activate and Stay Resident
//

DWORD MPManagerMax::Start()
{
	return GUPRESULT_KEEP;
}

void MPManagerMax::Stop()
{
}

void MPManagerMax::DeleteThis()
{
}


namespace
{
	bool importFRS(const char* filename, rpr_context& _context, rpr_scene &_scene, rpr_material_system &matsys)
	{
		rpr_int status = rprsImport(filename, _context, matsys, &_scene, false);
		if (status == RPR_SUCCESS)
		{
			return true;
		}
		else
		{
			return false;
		}
	}
};

bool MPManagerMax::Abort()
{
	bool res = true;
	if (mRenderingPreview.Wait(0))
	{
		res = false;
		exitImmediately.Fire();
		requestDestroyLoadedAssets.Fire();
	}
	return res;
}

int MPManagerMax::Render(FireRenderer *renderer, TimeValue t, ::Bitmap* tobm, FrameRendParams &frp, HWND hwnd, RendProgressCallback* prog, ViewParams* viewPar, INode *sceneNode, INode *viewNode, RendParams &rendParams)
{
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

	ScopeResetEventOnExit rendering(mRenderingPreview);

	CActiveShadeLocker aslocker; // block activeshader's rendering until finished

	exitImmediately.Reset();
	requestDestroyLoadedAssets.Reset();

	auto pblock = renderer->GetParamBlock(0);

	pblock->GetValue(PARAM_OVERRIDE_MAX_PREVIEW_SHAPES, 0, overrideMaxShapes, FOREVER);

	curObject = getObjectToRender(sceneNode);
	
	parameters.frameRendParams = frp;
	parameters.t = t;
	parameters.hWnd = hwnd;
	parameters.progress = prog;
	parameters.pblock = pblock;
	parameters.sceneNode = sceneNode;
	parameters.viewNode = viewNode;
	parameters.rendParams = rendParams;
	if (viewPar)
		parameters.viewParams = *viewPar;

	if (prog)
		prog->SetTitle(_T("Material Preview: Creating Radeon ProRender context..."));

	if (matballScope == -1)
		InitializeScene(pblock);
	else
		setVisible(curObject);
	
	frw::Scope scope = ScopeManagerMax::TheManager.GetScope(matballScope);
	
	tempScope = ScopeManagerMax::TheManager.CreateLocalScope(matballScope);

	renderThread = new BitmapRenderCore(scope, tobm->Width(), tobm->Height());
	renderThread->passLimit = GetFromPb<int>(pblock, PARAM_MTL_PREVIEW_PASSES);
	
	UpdateMaterial();
	
	renderThread->Start();

	bmDone.Reset();

	bool loop = true;
	while (loop)
	{
		if (!exitImmediately.Wait(0) && bmDone.Wait(0)) // rendering is complete
		{
			loop = false;
			if (renderThread->catastrophe == RPR_SUCCESS)
			{
				auto fb = renderThread->GetFrameBuffer();
				int height = tobm->Height();
				int width = tobm->Width();
#pragma omp parallel
				for (int y = 0; y < height; ++y)
				{
					tobm->PutPixels(0, y, width, ((BMM_Color_fl*)fb.data()) + y * width);
				}
				tobm->RefreshWindow();
			}
		}
		else
		{
			if (exitImmediately.Wait(0))
				loop = false;
			else
			{
				MSG msg;
				if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
				{
					TranslateMessage(&msg);
					DispatchMessage(&msg);

				}
				if (exitImmediately.Wait(0))
					loop = false;
			}
		}
		if (!loop)
		{
			ReplaceReference(0, 0);
			if (renderThread)
			{
				renderThread->Abort();
				delete renderThread;
				renderThread = 0;
			}

			ResetMaterial();
			ScopeManagerMax::TheManager.DestroyScope(tempScope);
			tempScope = -1;

			if (requestDestroyLoadedAssets.Wait(0))
			{
				DestroyLoadedAssets();
				ScopeManagerMax::TheManager.DestroyScope(matballScope);
				matballScope = -1;
				requestDestroyLoadedAssets.Reset();
			}
		}
		Sleep(0);
	}

	if (exitImmediately.Wait(0))
		return 0;

	return 1;
}

void MPManagerMax::UpdateMaterial()
{
	SceneCallbacks callbacks;

	callbacks.beforeParsing(parameters.t);

	frw::Scope temp = ScopeManagerMax::TheManager.GetScope(tempScope);
	
	SceneParser parser(parameters, callbacks, temp);

	// BALL
	bool castShadows = true;
	frw::Shader volumeShader;
	frw::Shader surfaceShader;
	Mtl* material = parser.parseMaterialPreview(surfaceShader, volumeShader, castShadows);

	if (material)
		ReplaceReference(0, material);

	float minHeight, maxHeight, subdivision, creaseWeight;
	int boundary;
	bool notAccurate;

	frw::Value displImageNode = FRMTLCLASSNAME(DisplacementMtl)::translateDisplacement(parameters.t, parser.mtlParser, material,
		minHeight, maxHeight, subdivision, creaseWeight, boundary, notAccurate);

	SetMaterial(parser, volumeShader, surfaceShader, castShadows, parameters.rendParams.envMap != nullptr);

	SetDisplacement(parser, displImageNode, minHeight * 0.5f, maxHeight * 0.5f, subdivision, creaseWeight, boundary);

	callbacks.afterParsing();
}

void MPManagerMax::SetMaterial(SceneParser &parser, frw::Shader volumeShader, frw::Shader surfaceShader, bool castShadows, bool env)
{
	frw::Scope scope = ScopeManagerMax::TheManager.GetScope(matballScope);
	
	switch (curObject)
	{
		case ObjCylinder:
			scope.GetShape(CylinderShape).SetShader(surfaceShader);
			scope.GetShape(CylinderShape).SetVolumeShader(volumeShader);
			break;
		case ObjCube:
			scope.GetShape(CubeShape).SetShader(surfaceShader);
			scope.GetShape(CubeShape).SetVolumeShader(volumeShader);
			break;
		case ObjMatball:
			for (auto shapeId : { BallShape1, BallShape2, BallShape3 })
			{
				scope.GetShape(shapeId).SetShader(surfaceShader);
				scope.GetShape(shapeId).SetVolumeShader(volumeShader);
			}
			break;
		case ObjSphere:
			scope.GetShape(SphereShape).SetShader(surfaceShader);
			scope.GetShape(SphereShape).SetVolumeShader(volumeShader);
			break;
	}
	
	switch (curObject)
	{
	case ObjCylinder:
		scope.GetShape(CylinderShape).SetShadowFlag(castShadows);
		break;
	case ObjCube:
		scope.GetShape(CubeShape).SetShadowFlag(castShadows);
		break;
	case ObjMatball:
		for (auto shapeId : { BallShape1, BallShape2, BallShape3 })
	{
			scope.GetShape(shapeId).SetShadowFlag(castShadows);
		}
		break;
	case ObjSphere:
		scope.GetShape(SphereShape).SetShadowFlag(castShadows);
		break;
	}

	// BACKGROUND
	if (env)
	{
		auto bgtextured = scope.GetShader(TexturedBgMaterial);
		scope.GetShape(BackgroundShape).SetShader(bgtextured);
	}
	else
		{
		auto bgdiffuse = scope.GetShader(DiffuseMaterial);
		scope.GetShape(BackgroundShape).SetShader(bgdiffuse);
	}
		}

void MPManagerMax::SetDisplacement(SceneParser &parser, frw::Value img, const float &minHeight, const float &maxHeight,
	const float &subdivision, const float &creaseWeight, int boundary)
		{
	frw::Scope scope = ScopeManagerMax::TheManager.GetScope(matballScope);

	if (img)
	{
			switch (curObject)
			{
			case ObjCylinder:
				scope.GetShape(CylinderShape).SetDisplacement(img, minHeight, maxHeight);
				scope.GetShape(CylinderShape).SetSubdivisionFactor(subdivision);
				scope.GetShape(CylinderShape).SetSubdivisionCreaseWeight(creaseWeight);
				scope.GetShape(CylinderShape).SetSubdivisionBoundaryInterop(boundary);
				break;
			case ObjCube:
				scope.GetShape(CubeShape).SetDisplacement(img, minHeight, maxHeight);
				scope.GetShape(CubeShape).SetSubdivisionFactor(subdivision);
				scope.GetShape(CubeShape).SetSubdivisionCreaseWeight(creaseWeight);
				scope.GetShape(CubeShape).SetSubdivisionBoundaryInterop(boundary);
				break;
			case ObjMatball:
				for (auto shapeId : { BallShape1, BallShape2, BallShape3 })
				{
					scope.GetShape(shapeId).SetDisplacement(img, minHeight, maxHeight);
					scope.GetShape(shapeId).SetSubdivisionFactor(subdivision);
					scope.GetShape(shapeId).SetSubdivisionCreaseWeight(creaseWeight);
					scope.GetShape(shapeId).SetSubdivisionBoundaryInterop(boundary);
				}
				break;
			case ObjSphere:
				scope.GetShape(SphereShape).SetDisplacement(img, minHeight, maxHeight);
				scope.GetShape(SphereShape).SetSubdivisionFactor(subdivision);
				scope.GetShape(SphereShape).SetSubdivisionCreaseWeight(creaseWeight);
				scope.GetShape(SphereShape).SetSubdivisionBoundaryInterop(boundary);
				break;
			}
		}
		else
	{
		switch (curObject)
		{
		case ObjCylinder:
			scope.GetShape(CylinderShape).RemoveDisplacement();
			break;
		case ObjCube:
			scope.GetShape(CubeShape).RemoveDisplacement();
			break;
		case ObjMatball:
			for (auto shapeId : { BallShape1, BallShape2, BallShape3 })
			{
				scope.GetShape(shapeId).RemoveDisplacement();
			}
			break;
		case ObjSphere:
			scope.GetShape(SphereShape).RemoveDisplacement();
			break;
		}
	}
}

void MPManagerMax::ResetMaterial()
{
	frw::Scope scope = ScopeManagerMax::TheManager.GetScope(matballScope);

	for (auto shapeId : { BallShape1, BallShape2, BallShape3, SphereShape, CylinderShape, CubeShape })
	{
		scope.GetShape(shapeId).SetShader(scope.GetShader(DiffuseMaterial));
		scope.GetShape(shapeId).SetVolumeShader(frw::Shader());
		scope.GetShape(shapeId).RemoveDisplacement();
	}
}

void MPManagerMax::setVisible(int whichObject)
{
	// hide all but cur object
	frw::Scope scope = ScopeManagerMax::TheManager.GetScope(matballScope);
	for (auto shapeId : { BallShape1, BallShape2, BallShape3, SphereShape, CylinderShape, CubeShape })
	{
		scope.GetShape(shapeId).SetVisibility(false);
	}
	switch (curObject)
	{
	case ObjCylinder:
		scope.GetShape(CylinderShape).SetVisibility(true);
		break;
	case ObjCube:
		scope.GetShape(CubeShape).SetVisibility(true);
		break;
	case ObjMatball:
		for (auto shapeId : { BallShape1, BallShape2, BallShape3 })
		{
			scope.GetShape(shapeId).SetVisibility(true);
		}
		break;
	case ObjSphere:
		scope.GetShape(SphereShape).SetVisibility(true);
		break;
	}
}

void MPManagerMax::InitializeScene(IParamBlock2 *pblock)
{
	matballScope = ScopeManagerMax::TheManager.CreateScope(pblock);
	frw::Scope scope = ScopeManagerMax::TheManager.GetScope(matballScope);
		
	auto _ms = scope.GetMaterialSystem().Handle();
	auto _context = scope.GetContext().Handle();
	rpr_scene _scene = nullptr;

	const std::string thumbnail = ToAscii(GetModuleFolder()) + "matball.frs";
	bool ires = importFRS(thumbnail.c_str(), _context, _scene, _ms);
	
	if (!ires)
	{
		MessageBoxA(GetCOREInterface()->GetMAXHWnd(), (std::string("Load failed: ") + thumbnail).c_str(), "error", MB_OK | MB_ICONERROR);
		_scene = scope.GetScene().Handle();
	}
	else
	{
		int nbImages = 0;
		int nbMaterials = 0;
		int nbShapes = 0;
		int nbLights = 0;
		int nbCameras = 0;

		rpr_int status = rprsListImportedImages(NULL, 0, &nbImages); FASSERT(status == RPR_SUCCESS);
		imageList.resize(nbImages);
		status = rprsListImportedImages(imageList.data(), sizeof(fr_image)*nbImages, NULL); FASSERT(status == RPR_SUCCESS);
		
		status = rprsListImportedShapes(NULL, 0, &nbShapes); FASSERT(status == RPR_SUCCESS);
		shapeList.resize(nbShapes);
		status = rprsListImportedShapes(shapeList.data(), sizeof(rpr_shape)*nbShapes, NULL); FASSERT(status == RPR_SUCCESS);

		status = rprsListImportedMaterialNodes(NULL, 0, &nbMaterials); FASSERT(status == RPR_SUCCESS);
		materialList.resize(nbMaterials);
		status = rprsListImportedMaterialNodes(materialList.data(), sizeof(rpr_material_node)*nbMaterials, NULL); FASSERT(status == RPR_SUCCESS);

		status = rprsListImportedLights(NULL, 0, &nbLights); FASSERT(status == RPR_SUCCESS);
		lightList.resize(nbLights);
		status = rprsListImportedLights(lightList.data(), sizeof(rpr_light)*nbLights, NULL); FASSERT(status == RPR_SUCCESS);

		status = rprsListImportedCameras(NULL, 0, &nbCameras); FASSERT(status == RPR_SUCCESS);
		cameraList.resize(nbCameras);
		status = rprsListImportedCameras(cameraList.data(), sizeof(rpr_camera)*nbCameras, NULL); FASSERT(status == RPR_SUCCESS);
				
		scope.SetScene(frw::Scene(_scene, scope.GetContext()));
	}

	for (auto shape : shapeList)
	{
		rpr_shape_type type;
		rpr_int res = rprShapeGetInfo(shape, RPR_SHAPE_TYPE, sizeof(rpr_shape_type), &type, NULL); FASSERT(res == RPR_SUCCESS);
		if (type == RPR_SHAPE_TYPE_MESH)
		{
			frw::Shape shape(shape, scope.GetContext(), false);
			uint64_t  vertex_count = 0;
			res = rprMeshGetInfo(shape.Handle(), RPR_MESH_VERTEX_COUNT, sizeof(vertex_count), &vertex_count, NULL); FASSERT(res == RPR_SUCCESS);
			// FR does not support names or anything else that can identify an object
			// so we need to identify objects by other hardcoded features such as number of vertices (sigh)
			if (vertex_count == 7188)
				scope.SetShape(BallShape1, shape);
			else if (vertex_count == 2306)
				scope.SetShape(BallShape2, shape);
			else if (vertex_count == 4672)
				scope.SetShape(BallShape3, shape);
			else if (vertex_count == 20)
				scope.SetShape(BackgroundShape, shape);
			else if (vertex_count == 8)
				scope.SetShape(CubeShape, shape);
			else if (vertex_count == 98)
				scope.SetShape(CylinderShape, shape);
			else if (vertex_count == 1106)
				scope.SetShape(SphereShape, shape);
		}
	}

	if (!scope.GetShader(DiffuseMaterial))
	{
		frw::DiffuseShader shader(scope.GetMaterialSystem());
		shader.SetColor(0.9);
		scope.SetShader(DiffuseMaterial, shader);
	}

	for (auto shapeId : { BallShape1, BallShape2, BallShape3, SphereShape, CylinderShape, CubeShape })
		scope.GetShape(shapeId).SetShader(scope.GetShader(DiffuseMaterial));

	if (!scope.GetShader(TexturedBgMaterial))
	{
		rpr_material_node textured_bg = nullptr;
		auto res = rprShapeGetInfo(scope.GetShape(BackgroundShape).Handle(), RPR_SHAPE_MATERIAL, sizeof(textured_bg), &textured_bg, nullptr);
		FASSERT(RPR_SUCCESS == res);
		scope.GetShape(BackgroundShape).SetShader(scope.GetShader(DiffuseMaterial));
		frw::Shader shader(textured_bg, scope.GetMaterialSystem(), false);
		scope.SetShader(TexturedBgMaterial, shader);
	}

	setVisible(curObject);
}

RefResult MPManagerMax::NotifyRefChanged(NOTIFY_REF_CHANGED_PARAMETERS)
{
	PartID pId = partId;

	auto mtlbase = dynamic_cast<MtlBase*>(hTarget);

	if (mtlbase)
	{
		switch (msg)
		{
		case REFMSG_DISPLAY_MATERIAL_CHANGE:
		case REFMSG_CHANGE:
		case REFMSG_TARGET_DELETED:
		case REFMSG_REF_DELETED:
		case REFMSG_REF_ADDED:
			if (renderThread)
			{
				UpdateMaterial();
				renderThread->Restart();
			}
			break;
		}
	}

	return REF_SUCCEED;
}

void MPManagerMax::DestroyLoadedAssets()
{
	rpr_int res;
	frw::Scope scope = ScopeManagerMax::TheManager.GetScope(matballScope);

	for (auto i : materialList)
	{
		res = rprObjectDelete(i); FASSERT(res == RPR_SUCCESS);
	}
	materialList.clear();

	for (auto i : imageList)
	{
		res = rprObjectDelete(i); FASSERT(res == RPR_SUCCESS);
	}
	imageList.clear();

	shapeList.clear();

	for (auto i : lightList)
	{
		res = rprSceneDetachLight(scope.GetScene().Handle(), i); FASSERT(res == RPR_SUCCESS);
		res = rprObjectDelete(i); FASSERT(res == RPR_SUCCESS);
	}
	lightList.clear();

	for (auto i : cameraList)
	{
		res = rprObjectDelete(i); FASSERT(res == RPR_SUCCESS);
	}
	cameraList.clear();
}

int MPManagerMax::getObjectToRender(INode *sceneNode)
{
	if (overrideMaxShapes)
		return ObjMatball;
	auto name = sceneNode->GetName();
	Object* objRef = sceneNode->GetObjectRef();
	if (auto geomObject = dynamic_cast<GeomObject*>(objRef))
	{
		int pts = geomObject->NumPoints();
		if (pts == 422)
			return ObjSphere;
		else if (pts == 122)
			return ObjCylinder;
		else if (pts == 8)
			return ObjCube;
	}
	return ObjSphere;
}

FIRERENDER_NAMESPACE_END;
