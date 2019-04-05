/*********************************************************************************************************************************
* Radeon ProRender for 3ds Max plugin
* Copyright (c) 2017 AMD
* All Rights Reserved
*
* Active Shader Renderer
*********************************************************************************************************************************/

#include "frWrap.h"

#include "FireRenderer.h"
#include "Common.h"
#include "ClassDescs.h"
#include "utils\Thread.h"
#include "utils/Utils.h"
#include "ParamDlg.h"
#include <ITabDialog.h>
#include <Notify.h>
#include <toneop.h>
#include <RadeonProRender.h>
#include <RprLoadStore.h>
#if MAX_PRODUCT_YEAR_NUMBER >= 2016
#   include <scene/IPhysicalCamera.h>
#endif

#pragma warning(push, 3)
#pragma warning(disable:4198)
#include <Math/mathutils.h>
#pragma warning(pop)
#include "ParamBlock.h"
#include "imenuman.h"
#include <memory>
#include <shlobj.h>
#include <stack>
#include <WinUser.h>
#include <resource.h>
#include <bitmap.h>
#include <notify.h>
#include <gamma.h> // gamma export for FRS files
#include <IPathConfigMgr.h>  // for IPathConfigMgr

#include "FireRenderDisplacementMtl.h"
#include "FireRenderMaterialMtl.h"
#include "FRSettingsFileHandler.h"
#include "MPManager.h"
#include "ScopeManager.h"
#include "PRManager.h"
#include "TMManager.h"
#include "CamManager.h"
#include "BgManager.h"

#include "ActiveShader.h"

FIRERENDER_NAMESPACE_BEGIN;

EventSpin ActiveShader::mGlobalLocker;


//////////////////////////////////////////////////////////////////////////////
// RENDERING
//


class ActiveShadeBitmapWriter;

class ActiveShadeRenderCore : public BaseThread
{
public:
	// termination
	Event terminationReached;
	std::atomic<DWORD> timeLimit = 100;
	std::atomic<unsigned int> passLimit = 1;
	std::atomic<unsigned int> passesDone = 0;
	std::atomic<unsigned int> restartsDone = 0;

	volatile TerminationCriteria termination = Termination_None;
	typedef enum
	{
		Result_OK,
		Result_Aborted,
		Result_Catastrophic
	} TerminationResult;
	Event mRenderThreadExit; // fired when the thread's worker is about to exit, for whichever reason
	Event mShaderCacheReady;

	volatile bool isNormals = false;
	::Bitmap *outputBitmap = 0;
	int renderDevice = 0;
	volatile float exposure = 1.f;
	volatile bool useMaxTonemapper = true;
	ActiveShadeBitmapWriter *writer = 0;

private:
	frw::Scope scope;
	frw::PostEffect normalizationFilter;
	frw::FrameBuffer frameBufferMain;
	frw::FrameBuffer frameBufferResolve;
	bool mAlphaEnabled;
	Event frameBufferAlphaEnable;
	Event frameBufferAlphaDisable;
	frw::FrameBuffer frameBufferAlpha;
	frw::FrameBuffer frameBufferAlphaResolve;
	bool mAdaptiveEnabled;
	frw::FrameBuffer frameBufferVariance;
	frw::FrameBuffer frameBufferVarianceResolve;
	CriticalSection bufSec;
	Event eRestart;

	bool DumpFrameBuffer(bool force);

	CriticalSection cameraSec;
	Event cameraChanged;
	ParsedView curView;
	CriticalSection regionSec;
	Event regionChanged;
	int xmin, ymin, xmax, ymax; // used for region rendering, define a rectangle on the framebuffer
	Box2 region;
	bool regionRender = false;
	CriticalSection ctxSec;

	CriticalSection toneMapperSec;
	Event toneMapperChanged; // fired when useMaxToneMapper is being changed
	
	ActiveShader *activeShader;

private:
	void SetupCamera(const ParsedView& view, const int imageWidth, const int imageHeight, rpr_camera outCamera);
	
public:

	inline void Lock()
	{
		ctxSec.Lock();
	}

	inline void Unlock()
	{
		ctxSec.Unlock();
	}

	inline ScopeLock GetLock()
	{
		return ScopeLock(ctxSec);
	}

	inline void StartBlit()
	{
		bufSec.Lock();
	}

	inline void EndBlit()
	{
		bufSec.Unlock();
	}

	inline void StartToneMapper()
	{
		toneMapperSec.Lock();
		useMaxTonemapper = false;
		toneMapperSec.Unlock();
		toneMapperChanged.Fire();
	}

	inline void StopToneMapper()
	{
		toneMapperSec.Lock();
		useMaxTonemapper = true;
		toneMapperSec.Unlock();
		toneMapperChanged.Fire();
	}

	inline void SetToneMappingExposure(float val)
	{
		exposure = val;
	}

	inline void EnableAlphaBuffer(bool enable)
	{
		if (enable)
			frameBufferAlphaEnable.Fire();
		else
			frameBufferAlphaDisable.Fire();
	}

	inline void SetTimeLimit(int val)
	{
		if (val <= 1)
			val = 1;
		DWORD uval = val;
		uval *= 1000; // seconds to milliseconds
		bool checkReset = (termination == Termination_Time) || (termination == Termination_PassesOrTime);
		bool reset = (checkReset && (uval > timeLimit));
		timeLimit = uval;
		if (reset)
			terminationReached.Reset();
	}

	inline void SetPassLimit(int val)
	{
		if (val <= 1)
			val = 1;
		bool checkReset = (termination == Termination_Time) || (termination == Termination_PassesOrTime);
		bool reset = (checkReset && (val > passLimit));
		passLimit = val;
		if (reset)
			terminationReached.Reset();
	}

	inline void SetLimitType(int type)
	{
		termination = (TerminationCriteria)type;
	}

	inline void ResetTermination()
	{
		terminationReached.Reset();
	}

	explicit ActiveShadeRenderCore(ActiveShader *pActiveShader, frw::Scope rscope, ActiveShadeBitmapWriter *pwriter,
		bool initialAlpha, bool initialAdaptive,
		int priority = THREAD_PRIORITY_NORMAL, const char* name = "ActiveShadeRenderCore");

	void Worker() override;

	void Restart();

#pragma optimize("", off)
	bool SetView(ParsedView &view)
	{
		bool needResetScene;
		if (!curView.isSame(view, &needResetScene))
		{
			cameraSec.Lock();
			curView = view;
			cameraSec.Unlock();
			cameraChanged.Fire();
			return true;
		}
		return false;
	}

	bool SetViewTM(const Matrix3 &tm)
	{
		if (!(curView.tm == tm))
		{
			cameraSec.Lock();
			curView.tm = tm;
			cameraSec.Unlock();
			cameraChanged.Fire();
			return true;
		}
		return false;
	}
#pragma optimize("", on)

	void SetRegion(Box2 &region)
	{
		regionSec.Lock();
		if (region.IsEmpty())
			regionRender = false;
		else
		{
			regionRender = true;
			xmin = region.x();
			ymin = region.y();
			xmax = xmin + region.w();
			ymax = ymin + region.h();
		}
		regionSec.Unlock();
		regionChanged.Fire();
	}
};

class ActiveShadeBitmapWriter
{
	friend class ActiveShadeRenderCore;
private:
	ActiveShader *mActiveShader;
	class ActiveShadeRenderCore *thread;
	UINT_PTR UITimerId;
	Bitmap *bm;

	// eventually called by the rendering thread
	void Abort(ActiveShadeRenderCore::TerminationResult res, rpr_int err);

public:
	Event vfbUpdated;
	rpr_int errorCode = RPR_SUCCESS;
	ActiveShadeRenderCore::TerminationResult result;

public:
	ActiveShadeBitmapWriter(ActiveShader *as, Bitmap *pbm);
	~ActiveShadeBitmapWriter();

	void SetThread(ActiveShadeRenderCore *pthread);
	void TryBlitToWindow();
};

//////////////////////////////////////////////////////////////////////////////
// BITMAP WRITER IMPLEMENTATION
//

ActiveShadeBitmapWriter::ActiveShadeBitmapWriter(ActiveShader *as, Bitmap *pbm)
	: bm(pbm)
	, mActiveShader(as)
{
}

ActiveShadeBitmapWriter::~ActiveShadeBitmapWriter()
{
}

void ActiveShadeBitmapWriter::Abort(ActiveShadeRenderCore::TerminationResult res, rpr_int err)
{
	errorCode = err;
	rpr_int errorCode = RPR_SUCCESS;
	result = res;
}

void ActiveShadeBitmapWriter::SetThread(ActiveShadeRenderCore *pthread)
{
	thread = pthread;
}

void ActiveShadeBitmapWriter::TryBlitToWindow()
{
	if (vfbUpdated.Wait(0))
	{
		thread->StartBlit();

		mActiveShader->mIfr->pIIRenderMgr->UpdateDisplay(); // blit
		
		thread->EndBlit();
	}
}

//////////////////////////////////////////////////////////////////////////////
// RENDER CORE IMPLEMENTATION
//

bool ActiveShadeRenderCore::DumpFrameBuffer(bool force)
{
	if (force)
		bufSec.Lock();
	else
		if (!bufSec.TryLock())
			return false;

	if (!frameBufferResolve.Handle() || !frameBufferMain.Handle())
	{
		bufSec.Unlock();
		return false;
	}

	// Save color buffer
	std::vector<float> colorData = frameBufferResolve.GetPixelData();

	// Resolve & Save alpha buffer
	std::vector<float> alphaData;
	if (frameBufferAlpha)
	{
		frameBufferAlpha.Resolve(frameBufferAlphaResolve);
		alphaData = frameBufferAlphaResolve.GetPixelData();
	}

	if( frameBufferVariance )
	{
		frameBufferVariance.Resolve(frameBufferVarianceResolve, true);
	}

	float exposure = IsReal(this->exposure) ? this->exposure : 1.f;
	CompositeFrameBuffersToBitmap(colorData, alphaData, outputBitmap, exposure, isNormals, false);

	if (this->useMaxTonemapper)
		UpdateBitmapWithToneOperator(outputBitmap);

	bufSec.Unlock();

	return true;
}

ActiveShadeRenderCore::ActiveShadeRenderCore(ActiveShader *pActiveShader, frw::Scope rscope, ActiveShadeBitmapWriter *pwriter,
	bool initialAlpha, bool initialAdaptive,
	int priority, const char* name)
	: BaseThread(name, priority)
	, mRenderThreadExit(false)
	, activeShader(pActiveShader)
	, scope(rscope)
	, writer(pwriter)
	, eRestart(true)
	, mShaderCacheReady(false)
	, mAlphaEnabled(initialAlpha)
	, mAdaptiveEnabled(initialAdaptive)
	, passesDone(0)
	, restartsDone(0)
	, terminationReached(false)
{
	mShaderCacheReady.Fire();
}

void ActiveShadeRenderCore::SetupCamera(const ParsedView& view, const int imageWidth, const int imageHeight, rpr_camera outCamera)
{
	rpr_int res;
	Point3 camOriginOffset = Point3(0.f, 0.f, 0.f); // used to shift ortho camera origin so it can behave as "in infinity"

	res = rprObjectSetName(outCamera, view.cameraNodeName.c_str());
	FCHECK(res);

	switch (view.projection)
	{
		case P_PERSPECTIVE:
		{
			res = rprCameraSetMode(outCamera, RPR_CAMERA_MODE_PERSPECTIVE);
			FCHECK(res);

			res = rprCameraSetFocalLength(outCamera, view.focalLength);
			FCHECK(res);

			res = rprCameraSetSensorSize(outCamera, view.sensorWidth, view.sensorWidth / float(imageWidth) * float(imageHeight));
			FCHECK(res);

			if (view.useDof) {
				res = rprCameraSetFStop(outCamera, view.fSTop);
				FCHECK(res);

				res = rprCameraSetApertureBlades(outCamera, view.bokehBlades);
				FCHECK(res);

				res = rprCameraSetFocusDistance(outCamera, view.focusDistance);
				FCHECK(res);
			}
			else {
				res = rprCameraSetFStop(outCamera, std::numeric_limits<float>::infinity());
				FCHECK(res);
			}

			break;
		}
		case P_ORTHO:
			res = rprCameraSetMode(outCamera, RPR_CAMERA_MODE_ORTHOGRAPHIC);
			FCHECK(res);
			res = rprCameraSetOrthoWidth(outCamera, view.orthoSize);
			FCHECK(res);
			res = rprCameraSetOrthoHeight(outCamera, view.orthoSize / float(imageWidth) * float(imageHeight));
			FCHECK(res);
			camOriginOffset.z += view.orthoSize*1.f;
			break;
		default:
			STOP;
	}

	Matrix3 tm = FlipAxes(view.tm);

	// Create the 3 principal points from the transformation matrix
	Point3 ORIGIN = tm.PointTransform(camOriginOffset);
	Point3 TARGET = tm.PointTransform(camOriginOffset + Point3(0.f, 0.f, -1.f));
	Point3 ROLL = tm.VectorTransform(Point3(0.f, 1.f, 0.f));
	res = rprCameraLookAt(outCamera, ORIGIN.x, ORIGIN.y, ORIGIN.z, TARGET.x, TARGET.y, TARGET.z, ROLL.x, ROLL.y, ROLL.z);
	FCHECK(res);

	res = rprCameraSetTiltCorrection(outCamera, -view.tilt.x, -view.tilt.y);
	FCHECK(res);

	switch (FRCameraType(view.projectionOverride))
	{
		case FRCameraType_Default:break;
		case FRCameraType_LatitudeLongitude_360:
			res = rprCameraSetMode(outCamera, RPR_CAMERA_MODE_LATITUDE_LONGITUDE_360);
			FCHECK(res);
			break;
		case FRCameraType_LatitudeLongitude_Stereo:
			res = rprCameraSetMode(outCamera, RPR_CAMERA_MODE_LATITUDE_LONGITUDE_STEREO);
			FCHECK(res);
			break;
		case FRCameraType_Cubemap:
			res = rprCameraSetMode(outCamera, RPR_CAMERA_MODE_CUBEMAP);
			FCHECK(res);
			break;
		case FRCameraType_Cubemap_Stereo:
			res = rprCameraSetMode(outCamera, RPR_CAMERA_MODE_CUBEMAP_STEREO);
			FCHECK(res);
			break;
		default:
			FASSERT(!"camera type not supported yet");
	};

	if (view.useMotionBlur)
	{
		res = rprCameraSetExposure(outCamera, view.cameraExposure);
		FCHECK(res);
	}
}

void ActiveShadeRenderCore::Worker()
{
	ScopeFireEventOnExit exitEvent(mRenderThreadExit);

	AccumulationTimer timer;

	passesDone = 0;
	restartsDone = 0;

	bool clearFramebuffer = true;

	bool useRegion = false;
	int rxmin = 0, rxmax = 0, rymin = 0, rymax = 0;

	int prevbmw = 0;
	int prevbmh = 0;

	bool cameraInit = false;
	
	normalizationFilter = frw::PostEffect(scope.GetContext(), frw::PostEffectTypeNormalization);
	scope.GetContext().Attach(normalizationFilter);

	rpr_int res = rprContextSetParameter1u(scope.GetContext().Handle(), "preview", 1);
	FCHECK_CONTEXT(res, scope.GetContext().Handle(), "rprContextSetParameter1u");

	// render all passes, unless the render is cancelled (and even then render at least a single pass)
	while (1)
	{
		rpr_int res = RPR_SUCCESS;

		// ABORT?
		if (mStop.Wait(0))
		{
			writer->Abort(Result_Aborted, RPR_SUCCESS);
			break;
		}

		if (ActiveShader::mGlobalLocker.Wait(0))
			continue;

		if (!mShaderCacheReady.Wait(0))
			continue;

		outputBitmap = activeShader->GetOutputBitmap().load();
		if (outputBitmap)
		{
			if (toneMapperChanged.Wait(0))
			{
			}

			if (frameBufferAlphaEnable.Wait(0) && !mAlphaEnabled)
			{
				mAlphaEnabled = true;
				prevbmw = -1;
				prevbmh = -1;
			}
			if (frameBufferAlphaDisable.Wait(0) && mAlphaEnabled)
			{
				mAlphaEnabled = false;
				prevbmw = -1;
				prevbmh = -1;
			}
			
			int w = outputBitmap->Width();
			int h = outputBitmap->Height();
			if (prevbmw != w || prevbmh != h)
			{
				auto lock = GetLock();
				if (frameBufferMain)
				{
					scope.DestroyFrameBuffer(FramebufferTypeId_Color);
					frameBufferMain.Reset();
				}
				if (frameBufferResolve)
				{
					scope.DestroyFrameBuffer(FramebufferTypeId_ColorResolve);
					frameBufferResolve.Reset();
				}
				if (frameBufferAlpha)
				{
					scope.DestroyFrameBuffer(FramebufferTypeId_Alpha);
					frameBufferAlpha.Reset();
				}
				if (frameBufferAlphaResolve)
				{
					scope.DestroyFrameBuffer(FramebufferTypeId_AlphaResolve);
					frameBufferAlphaResolve.Reset();
				}
				if (frameBufferVariance)
				{
					scope.DestroyFrameBuffer(FrameBufferTypeId_Variance);
					frameBufferVariance.Reset();
				}
				if (frameBufferVarianceResolve)
				{
					scope.DestroyFrameBuffer(FrameBufferTypeId_VarianceResolve);
					frameBufferVarianceResolve.Reset();
				}

				// Set color buffer
				frameBufferMain = scope.GetFrameBuffer(w, h, FramebufferTypeId_Color);
				frameBufferResolve = scope.GetFrameBuffer(w, h, FramebufferTypeId_ColorResolve);
				res = scope.GetContext().SetAOV(frameBufferMain, RPR_AOV_COLOR);
				if (res != RPR_SUCCESS)
				{
					TheManager->Max()->Log()->LogEntry(SYSLOG_WARN, NO_DIALOG, L"Radeon ProRender - Warning", L"rprContextSetAOV(COLOR) returned '%d'\n", res);
					writer->Abort(Result_Catastrophic, res);
					break;
				}

				// Set alpha buffer
				if (mAlphaEnabled)
				{
					frameBufferAlpha = scope.GetFrameBuffer(w, h, FramebufferTypeId_Alpha);
					frameBufferAlphaResolve = scope.GetFrameBuffer(w, h, FramebufferTypeId_AlphaResolve);
					res = scope.GetContext().SetAOV(frameBufferAlpha, RPR_AOV_OPACITY);
				}

				// Set variance buffer
				if (mAdaptiveEnabled)
				{
					frameBufferVariance = scope.GetFrameBuffer(w, h, FrameBufferTypeId_Variance);
					frameBufferVarianceResolve = scope.GetFrameBuffer(w, h, FrameBufferTypeId_VarianceResolve);
					res = scope.GetContext().SetAOV(frameBufferVariance, RPR_AOV_VARIANCE);
				}

				if (res != RPR_SUCCESS)
				{
					TheManager->Max()->Log()->LogEntry(SYSLOG_WARN, NO_DIALOG, L"Radeon ProRender - Warning", L"rprContextSetAOV(ALPHA) returned '%d'\n", res);
					writer->Abort(Result_Catastrophic, res);
					break;
				}
				prevbmw = w;
				prevbmh = h;
				eRestart.Fire();
			}
		}
		else
			continue; // activeshade doesn't render offline

		if (cameraChanged.Wait(0))
		{
			if (outputBitmap)
			{
				cameraSec.Lock();
				SetupCamera(curView, outputBitmap->Width(), outputBitmap->Height(), activeShader->camera[0].Handle());
				cameraSec.Unlock();
				cameraInit = true;
				eRestart.Fire();
			}
			else
				cameraChanged.Fire();
		}

		if (regionChanged.Wait(0))
		{
			regionSec.Lock();
			useRegion = regionRender;
			rxmin = xmin;
			rxmax = xmax;
			rymin = ymin;
			rymax = ymax;
			regionSec.Unlock();
		}

		// RESTART?
		if (eRestart.Wait(0))
		{
			clearFramebuffer = true;
			if( passesDone>0 ) // only count as a restart if some rendering was completed
				restartsDone++;
			passesDone = 0;
			timer.Reset();
			terminationReached.Reset();
		}

		if (terminationReached.Wait(0))
			continue;

		if (clearFramebuffer)
		{
			frameBufferMain.Clear();
			frameBufferResolve.Clear();

			if (frameBufferAlpha)
			{
				frameBufferAlpha.Clear();
				frameBufferAlphaResolve.Clear();
			}

			if( frameBufferVariance )
			{
				frameBufferVariance.Clear();
				frameBufferVarianceResolve.Clear();
			}

			clearFramebuffer = false;
		}

		bool rendered = false;

		// RENDER
		if (cameraInit)
		{
			Lock();
			try
			{
				timer.Start();
				if (useRegion)
				{
					res = rprContextRenderTile(scope.GetContext().Handle(), rxmin, rxmax, rymin, rymax);
					FCHECK_CONTEXT(res, scope.GetContext().Handle(), "rprContextRenderTile");
				}
				else
				{
					res = rprContextRender(scope.GetContext().Handle());
					FCHECK_CONTEXT(res, scope.GetContext().Handle(), "rprContextRender");
				}
				if (res == RPR_SUCCESS)
				{
					frameBufferMain.Resolve(frameBufferResolve);
					rendered = true;
				}
				timer.Stop();
			}
			catch (...)
			{// CATASTROPHE?
				debugPrint("Exception occurred in render call");
				writer->Abort(Result_Catastrophic, RPR_ERROR_INTERNAL_ERROR);
				Unlock();
				break;
			}
			Unlock();
		}

		// CATASTROPHE?
		if (res != RPR_SUCCESS)
		{
			TheManager->Max()->Log()->LogEntry(SYSLOG_WARN, NO_DIALOG, L"Radeon ProRender - Warning", L"rprContextRender returned '%d'\n", res);
			writer->Abort(Result_Catastrophic, res);
			return;
		}

		// BLIT
		if (rendered)
		{
			bool fbDumped = DumpFrameBuffer(false);
			if (fbDumped)
			{
				writer->bm = outputBitmap;
				writer->vfbUpdated.Fire();
			}

			// TERMINATION
			passesDone++;

			if (passesDone == 1)
				mShaderCacheReady.Reset();

			if (termination != TerminationCriteria::Termination_None)
			{
				if( (termination == TerminationCriteria::Termination_Passes) ||
					(termination == TerminationCriteria::Termination_PassesOrTime) )
				{
					if (passesDone == passLimit)
						terminationReached.Fire();
				}
				else if( (termination == TerminationCriteria::Termination_Time) ||
					     (termination == TerminationCriteria::Termination_PassesOrTime) )
				{
					if (timer.GetElapsed() >= timeLimit)
						terminationReached.Fire();
				}
			}
		}
	
	}
	if (frameBufferMain)
	{
		scope.DestroyFrameBuffer(FramebufferTypeId_Color);
		frameBufferMain.Reset();
	}
	if (frameBufferResolve)
	{
		scope.DestroyFrameBuffer(FramebufferTypeId_ColorResolve);
		frameBufferResolve.Reset();
	}

	if (frameBufferAlpha)
	{
		scope.DestroyFrameBuffer(FramebufferTypeId_Alpha);
		frameBufferAlpha.Reset();
	}
	if (frameBufferAlphaResolve)
	{
		scope.DestroyFrameBuffer(FramebufferTypeId_AlphaResolve);
		frameBufferAlphaResolve.Reset();
	}

	if (frameBufferVariance)
	{
		scope.DestroyFrameBuffer(FrameBufferTypeId_Variance);
		frameBufferVariance.Reset();
	}
	if (frameBufferVarianceResolve)
	{
		scope.DestroyFrameBuffer(FrameBufferTypeId_VarianceResolve);
		frameBufferVarianceResolve.Reset();
	}
}

void ActiveShadeRenderCore::Restart()
{
	eRestart.Fire();
	if (!IsRunning())
		Start();
}

//////////////////////////////////////////////////////////////////////////////
// BRIDGE INTERFACE
//

ActiveShader::ActiveShader(class IFireRender *ifr) :
	mIfr(ifr),
	mScopeId(-1),
	mOutputBitmap(0),
	mSceneINode(0),
	mShaderCacheDlg(nullptr),
	mRenderThread(nullptr)
{
	mBridge = new ActiveShadeSynchronizerBridge(this);
}

ActiveShader::~ActiveShader()
{
	delete mBridge;
}

void ActiveShader::Begin()
{
	if (mRunning)
		return;

	mRunning = true;

	auto bm = mOutputBitmap.load();

	auto rend = GetCOREInterface()->GetRenderer(RS_IReshade);
	auto pblock = rend->GetParamBlock(0);

	mScopeId = ScopeManagerMax::TheManager.CreateScope(pblock);
	frw::Scope scope = ScopeManagerMax::TheManager.GetScope(mScopeId);
	frw::Context context = scope.GetContext();
	frw::MaterialSystem materialSystem = scope.GetMaterialSystem();
	frw::Scene scene = scope.GetScene(true);
	camera.push_back(context.CreateCamera());
	scene.SetCamera(camera[0]);

	// Set up some basic rendering parameters in Radeon ProRender
	context.SetParameter("texturecompression", GetFromPb<int>(pblock, PARAM_USE_TEXTURE_COMPRESSION));
	context.SetParameter("rendermode", GetFromPb<int>(pblock, PARAM_RENDER_MODE));
	context.SetParameter("maxRecursion", GetFromPb<int>(pblock, PARAM_MAX_RAY_DEPTH));
	context.SetParameter("imagefilter.type", GetFromPb<int>(pblock, PARAM_IMAGE_FILTER));
	const float filterRadius = GetFromPb<float>(pblock, PARAM_IMAGE_FILTER_WIDTH);
	context.SetParameter("imagefilter.box.radius", filterRadius);
	context.SetParameter("imagefilter.gaussian.radius", filterRadius);
	context.SetParameter("imagefilter.triangle.radius", filterRadius);
	context.SetParameter("imagefilter.mitchell.radius", filterRadius);
	context.SetParameter("imagefilter.lanczos.radius", filterRadius);
	context.SetParameter("imagefilter.blackmanharris.radius", filterRadius);
	context.SetParameter("iterations", GetFromPb<int>(pblock, PARAM_CONTEXT_ITERATIONS));
	context.SetParameter("pdfthreshold", 0.f);
	context.SetParameter("raycastepsilon", GetFromPb<float>(pblock, PARAM_QUALITY_RAYCAST_EPSILON));
	const float adaptiveThreshold = GetFromPb<float>(pblock, PARAM_ADAPTIVE_NOISE_THRESHOLD);
	context.SetParameter("as.threshold", adaptiveThreshold);
	context.SetParameter("as.tilesize", GetFromPb<int>(pblock, PARAM_ADAPTIVE_TILESIZE));
	context.SetParameter("as.minspp", GetFromPb<int>(pblock, PARAM_SAMPLES_MIN));

	BOOL useIrradianceClamp = FALSE;
	pblock->GetValue(PARAM_USE_IRRADIANCE_CLAMP, 0, useIrradianceClamp, Interval());
	if (useIrradianceClamp)
	{
		float irradianceClamp = FLT_MAX;
		pblock->GetValue(PARAM_IRRADIANCE_CLAMP, 0, irradianceClamp, Interval());
		context.SetParameter("radianceclamp", irradianceClamp);
	}
	else
		context.SetParameter("radianceclamp", FLT_MAX);

	pViewExp = &GetCOREInterface()->GetActiveViewExp();
	mViewID = pViewExp->GetViewID();

	INode* outCam = 0;
	viewParams = ViewExp2viewParams(*pViewExp, outCam);
	mIfr->fireRenderer->isToneOperatorPreviewRender = false;

	context.SetParameter("xflip", 0);
	context.SetParameter("yflip", 1);
	
	int renderLimitType;
	pblock->GetValue(PARAM_RENDER_LIMIT, 0, renderLimitType, Interval());
	int timeLimit, passLimit;
	pblock->GetValue(PARAM_TIME_LIMIT, 0, timeLimit, Interval());
	pblock->GetValue(PARAM_PASS_LIMIT, 0, passLimit, Interval());

	mSynchronizer = new ActiveShadeSynchronizer(this, scope, mSceneINode, mBridge);

	ActiveShadeBitmapWriter *writer = new ActiveShadeBitmapWriter(this, bm);

	BOOL alphaEnabled;
	BgManagerMax::TheManager.GetProperty(PARAM_BG_ENABLEALPHA, alphaEnabled);

	// Disable Adaptive Sampling with the CMJ sampler, if noise threshold is zero
	bool isAdaptiveEnabled = (adaptiveThreshold > 0);

	// TODO: support region mode and denoiser

	mRenderThread = new ActiveShadeRenderCore(this, scope, writer, bool_cast(alphaEnabled), isAdaptiveEnabled );
	mRenderThread->SetLimitType(renderLimitType);
	mRenderThread->SetTimeLimit(timeLimit);
	mRenderThread->SetPassLimit(passLimit);

	writer->SetThread(mRenderThread);
		
	mRenderThread->Start();
	mSynchronizer->Start();
}

void ActiveShader::End()
{
	if (!mRunning)
		return;

	mRunning = false;

	if (mShaderCacheDlg)
	{
		DestroyWindow(mShaderCacheDlg);
		mShaderCacheDlg = NULL;
	}

	mSynchronizer->Stop();
	delete mSynchronizer;
	mSynchronizer = 0;
	
	if (mRenderThread)
	{
		mRenderThread->Abort();
		delete mRenderThread;
		mRenderThread = nullptr;
	}

	camera.clear();

	hOwnerWnd = 0;
	mSceneINode = 0;
	bUseViewINode = false;
	pViewINode = 0;
	pViewExp = 0;
	pProgCB = 0;

	try
	{
		ScopeManagerMax::TheManager.DestroyScope(mScopeId);
	}
	catch (...)
	{

	}
}

void ActiveShader::SetOutputBitmap(Bitmap *pDestBitmap)
{
	mOutputBitmap = pDestBitmap;
}

std::atomic<Bitmap*> &ActiveShader::GetOutputBitmap()
{
	return mOutputBitmap;
}

void ActiveShader::SetSceneRootNode(INode *root)
{
	mSceneINode = root;
}

INode* ActiveShader::GetSceneRootNode() const
{
	return mSceneINode;
}

void ActiveShader::SetOwnerWnd(HWND hwnd)
{
	// we are only interested if the activeshader is docked
	// in which case the behavior of viewport rendering would change
	// if the activeshader is docked, it will render whichever active
	// view is selected and worked on. If the activeshader is not
	// docked, it will track the viewport that was active at the time
	// activeshader was fired up.
	if (GetWindowStyle(hwnd) & WS_CHILDWINDOW)
		hOwnerWnd = hwnd;
}

HWND ActiveShader::GetOwnerWnd() const
{
	return hOwnerWnd;
}

void ActiveShader::SetUseViewINode(bool use)
{
	bUseViewINode = use;
}

bool ActiveShader::GetUseViewINode() const
{
	return bUseViewINode;
}

void ActiveShader::SetViewINode(INode *node)
{
	pViewINode = node;
}

INode *ActiveShader::GetViewINode() const
{
	return pViewINode;
}

void ActiveShader::SetViewExp(ViewExp *pViewExp)
{
}

ViewExp *ActiveShader::GetViewExp() const
{
	return pViewExp;
}

void ActiveShader::SetRegion(const Box2 &box)
{
	region = box;
	if (mRenderThread)
		mRenderThread->SetRegion(region);
}

const Box2 &ActiveShader::GetRegion() const
{
	return region;
}

void ActiveShader::SetProgressCallback(IRenderProgressCallback *cb)
{
	pProgCB = cb;
}

const IRenderProgressCallback *ActiveShader::GetProgressCallback() const
{
	return pProgCB;
}

void ActiveShader::SetDefaultLights(DefaultLight *pDefLights, int numDefLights)
{
	mDefLights = pDefLights;
	mNumDefLight = numDefLights;
}

const DefaultLight *ActiveShader::GetDefaultLights(int &numDefLights) const
{
	numDefLights = mNumDefLight;
	return mDefLights;
}

BOOL ActiveShader::AnyUpdatesPending()
{
	// the proper way would be to execute the commented code below
	// however, if we do, material previews will not be allowed
	// to render while an activeshader instance is running.
	// the commented code below is intentionally left for
	// eventual future use
	//return !mSynchronizer->IsQueueEmpty();
	return FALSE;
}

BOOL ActiveShader::IsRendering()
{
	// the proper way would be to execute the commented code below
	// however, if we do, material previews will not be allowed
	// to render while an activeshader instance is running.
	// the commented code below is intentionally left for
	// eventual future use
	//if (mRenderThread)
	//	if (!mRenderThread->mRenderThreadExit.Wait(0))
	//		return TRUE;
	return FALSE;
}

void ActiveShader::AbortRender()
{
	if (mRenderThread)
	{
		mRenderThread->Abort();
		delete mRenderThread;
		mRenderThread = nullptr;
	}
}

//////////////////////////////////////////////////////////////////////////////
// ActiveShadeSynchronizer
//

ParsedView ActiveShadeSynchronizer::ParseView()
{
	ParsedView output = {};
	output.cameraExposure = 1.f;

	auto t = mBridge->t();
	float masterScale = GetMasterScale(UNITS_METERS);

	BOOL res;
	BOOL overWriteDOFSettings;
	BOOL useRPRMotionBlur;
	res = CamManagerMax::TheManager.GetProperty(PARAM_CAM_OVERWRITE_DOF_SETTINGS, overWriteDOFSettings);
	FASSERT(res);
	res = CamManagerMax::TheManager.GetProperty(PARAM_CAM_USE_MOTION_BLUR, useRPRMotionBlur);
	FASSERT(res);

	if (!overWriteDOFSettings)
	{
		//set default values:
		BOOL useDof;
		res = CamManagerMax::TheManager.GetProperty(PARAM_CAM_USE_DOF, useDof);
		FASSERT(res);
		output.useDof = useDof != FALSE;

		res = CamManagerMax::TheManager.GetProperty(PARAM_CAM_FOCUS_DIST, output.focusDistance);
		FASSERT(res);
		res = CamManagerMax::TheManager.GetProperty(PARAM_CAM_SENSOR_WIDTH, output.sensorWidth);
		FASSERT(res);
		res = CamManagerMax::TheManager.GetProperty(PARAM_CAM_F_STOP, output.fSTop);
		FASSERT(res);
		res = CamManagerMax::TheManager.GetProperty(PARAM_CAM_BLADES, output.bokehBlades);
		FASSERT(res);
		res = CamManagerMax::TheManager.GetProperty(PARAM_CAM_FOCAL_LENGTH, output.focalLength);
		FASSERT(res);
		res = CamManagerMax::TheManager.GetProperty(PARAM_CAM_FOV, output.perspectiveFov);
		FASSERT(res);
	}

	output.tilt.Set(0, 0);

	if (mActiveShader->pViewINode)
	{	// we are currently rendering a camera - eval the view node, cast to camera, parse its data

		std::wstring name = mActiveShader->pViewINode->GetName();
		output.cameraNodeName = std::string(name.begin(), name.end());

		const ObjectState& os = mActiveShader->pViewINode->EvalWorldState(t);
		CameraObject* cam = dynamic_cast<CameraObject*>(os.obj);
		cam->UpdateTargDistance(t, mActiveShader->pViewINode); // necessary, otherwise we sometimes get incorrect target distance

		Matrix3 camTransform = mActiveShader->pViewINode->GetObjTMAfterWSM(t);
		camTransform.SetTrans(camTransform.GetTrans() * masterScale);
		output.tm = camTransform;

		Matrix3 camTransformMotion = mActiveShader->pViewINode->GetObjTMAfterWSM(t + 1);
		camTransformMotion.SetTrans(camTransformMotion.GetTrans() * masterScale);
		output.tmMotion = camTransformMotion;
		
		output.perspectiveFov = cam->GetFOV(t);
		// Do not allow zero fov value
		if (output.perspectiveFov < 0.001f)
			output.perspectiveFov = 0.001f;

		output.projection = cam->IsOrtho() ? P_ORTHO : P_PERSPECTIVE;

		bool physicalCameraUsed = false;
#if MAX_PRODUCT_YEAR_NUMBER >= 2016
		MaxSDK::IPhysicalCamera* physicalCamera = dynamic_cast<MaxSDK::IPhysicalCamera*>(cam);
		if (physicalCamera) {
			physicalCameraUsed = true;
			// Physical camera, if present, overrides the DOF parameters
			output.useDof = physicalCamera->GetDOFEnabled(t, Interval());

			output.sensorWidth = physicalCamera->GetFilmWidth(t, Interval()) * getSystemUnitsToMillimeters();

			if (output.useDof) {
				output.focusDistance = physicalCamera->GetFocusDistance(t, Interval()) * getSystemUnitsToMillimeters() * 0.001f;
				output.fSTop = physicalCamera->GetLensApertureFNumber(t, Interval());
				output.bokehBlades = physicalCamera->GetBokehNumberOfBlades(t, Interval());
			}

			output.tilt = physicalCamera->GetTiltCorrection(t, Interval());

			if (!useRPRMotionBlur)
			{
				// Motion blur
				output.useMotionBlur = physicalCamera->GetMotionBlurEnabled(t, Interval());

				float shutterOpenDuration = physicalCamera->GetShutterDurationInFrames(t, Interval());
				output.motionBlurScale = shutterOpenDuration;
				output.cameraExposure = shutterOpenDuration;
			}
		}
#endif
		if (!physicalCameraUsed) {
			output.useDof = bool_cast( cam->GetMultiPassEffectEnabled(t, Interval()) );
			//sensor width is 36mm on default
			output.sensorWidth = 36;
			if (output.useDof)
			{
				//use dof possible
				IMultiPassCameraEffect *mpCE = cam->GetIMultiPassCameraEffect();

				for (int i = 0; i < mpCE->NumSubs(); i++)
				{
					Animatable *animatableMP = mpCE->SubAnim(i);
					MSTR animatableNameMP = mpCE->SubAnimName(i);

					MaxSDK::Util::MaxString dofpString;
					dofpString.SetUTF8("Depth of Field Parameters");

					if (animatableNameMP.ToMaxString().Compare(dofpString) == 0) {
						IParamBlock2 *pb = dynamic_cast<IParamBlock2*>(animatableMP);
						ParamBlockDesc2 *pbDesc = pb->GetDesc();
						int utdIdx = pbDesc->NameToIndex(L"useTargetDistance");
						if (utdIdx != -1) {
							BOOL useTargetDistance = true;
							float focalDepth = cam->GetTDist(t) * getSystemUnitsToMillimeters() * 0.001f;

#if MAX_PRODUCT_YEAR_NUMBER >= 2016
							pb->GetValueByName<BOOL>(L"useTargetDistance", t, useTargetDistance, Interval());
							pb->GetValueByName<float>(L"focalDepth", t, focalDepth, Interval());
#else
							ParamID pUseTD = GetParamIDFromName(L"useTargetDistance", pb);
							if (pUseTD != -1)
								pb->GetValue(pUseTD, t, useTargetDistance, Interval());

							ParamID pFocalDepth = GetParamIDFromName(L"focalDepth", pb);
							if (pFocalDepth != -1)
								pb->GetValue(pFocalDepth, t, focalDepth, Interval());
#endif
							if (useTargetDistance)
								output.focusDistance = cam->GetTDist(t) * getSystemUnitsToMillimeters() * 0.001f;
							else
								output.focusDistance = focalDepth * getSystemUnitsToMillimeters() * 0.001f;
						}
					}
				}
			}
		}

		output.orthoSize = output.focusDistance*std::tan(output.perspectiveFov * 0.5f) * 2.f;

		float focalLength = output.sensorWidth / (2.f * std::tan(output.perspectiveFov * 0.5f));
		FASSERT(!isnan(focalLength) && focalLength > 0 && focalLength < std::numeric_limits<float>::infinity());
		output.focalLength = focalLength;
	}
	else
	{
		// No camera present. Use ViewParams class
		output.perspectiveFov = mActiveShader->viewParams.fov;
		output.orthoSize = mActiveShader->viewParams.zoom * 400.f * masterScale;    //400 = magic constant necessary, from 3dsmax api doc
																			//sensor width is 36mm on default
		output.sensorWidth = 36;
		output.projection = (mActiveShader->viewParams.projType == PROJ_PERSPECTIVE) ? P_PERSPECTIVE : P_ORTHO;

		if (output.projection == P_PERSPECTIVE)
		{
			float focalLength = output.sensorWidth / (2.f * std::tan(output.perspectiveFov / 2.f));
			FASSERT(!isnan(focalLength) && focalLength > 0 && focalLength < std::numeric_limits<float>::infinity());
			output.focalLength = focalLength;
		}

		output.tm = mActiveShader->viewParams.affineTM;
		output.tm.SetTrans(output.tm.GetTrans() * masterScale);
		output.tm.Invert();

		output.tmMotion = output.tm;
	}

	if (overWriteDOFSettings)
	{
		//overwrite active, ignore previously set values

		BOOL useDof;
		res = CamManagerMax::TheManager.GetProperty(PARAM_CAM_USE_DOF, useDof);
		FASSERT(res);
		output.useDof = useDof != FALSE;

		if (useDof) {
			res = CamManagerMax::TheManager.GetProperty(PARAM_CAM_FOCUS_DIST, output.focusDistance);
			FASSERT(res);
			res = CamManagerMax::TheManager.GetProperty(PARAM_CAM_SENSOR_WIDTH, output.sensorWidth);
			FASSERT(res);
			res = CamManagerMax::TheManager.GetProperty(PARAM_CAM_F_STOP, output.fSTop);
			FASSERT(res);
			res = CamManagerMax::TheManager.GetProperty(PARAM_CAM_BLADES, output.bokehBlades);
			FASSERT(res);
			res = CamManagerMax::TheManager.GetProperty(PARAM_CAM_FOCAL_LENGTH, output.focalLength);
			FASSERT(res);
			res = CamManagerMax::TheManager.GetProperty(PARAM_CAM_FOV, output.perspectiveFov);
			FASSERT(res);

			if (mActiveShader->pViewINode)
				output.orthoSize = output.focusDistance*std::tan(output.perspectiveFov / 2.f) * 2.f;
			else
				output.orthoSize = mActiveShader->viewParams.zoom * 400.f * masterScale;
		}
	}

	res = CamManagerMax::TheManager.GetProperty(PARAM_CAM_TYPE, output.projectionOverride);
	FASSERT(res);

	if (useRPRMotionBlur)
	{
		output.useMotionBlur = true;

		res = CamManagerMax::TheManager.GetProperty(PARAM_CAM_MOTION_BLUR_SCALE, output.motionBlurScale);
		FASSERT(res);

		res = CamManagerMax::TheManager.GetProperty(PARAM_CAM_EXPOSURE, output.cameraExposure);
		FASSERT(res);
	}

	return output;
}

void ActiveShadeSynchronizer::CustomCPUSideSynch()
{
	// update camera / view
	if (!mActiveShader->hOwnerWnd)
		mActiveShader->pViewExp = &GetCOREInterface14()->GetViewExpByID(mActiveShader->mViewID);
	if (!mActiveShader->pViewExp || mActiveShader->hOwnerWnd)
		mActiveShader->pViewExp = &GetCOREInterface()->GetActiveViewExp();

	INode* outCam;
	mActiveShader->viewParams = ViewExp2viewParams(*mActiveShader->pViewExp, outCam);
	if (!mActiveShader->bUseViewINode)
		mActiveShader->pViewINode = outCam;

	auto view = ParseView();

	if (mActiveShader->mRenderThread)
	{
		mActiveShader->mRenderThread->SetView(view);
		if (mActiveShader->mRenderThread->writer)
			mActiveShader->mRenderThread->writer->TryBlitToWindow();
	}

	unsigned int passesDone = 0, restartsDone = 0;
	if (mActiveShader->mRenderThread!=nullptr )
	{
		passesDone = mActiveShader->mRenderThread->passesDone;
		restartsDone = mActiveShader->mRenderThread->restartsDone;
	}

	// Handle shader cache dialog box
	// If either passesDone (iteration count) or restartsDone is greater than zero,
	// then at least one iteration was completed in the past, dialog may be closed.
	// If not closed, the dialog prevents further iterations from proceeding.
	if ( (passesDone==0) && (restartsDone==0) &&
		 (ScopeManagerMax::TheManager.GetCompiledShadersCount() < 17) )
	{
		if (mActiveShader->mShaderCacheDlg == NULL)
		{
			// Create dialog
			mActiveShader->mShaderCacheDlg = CreateDialog(FireRender::fireRenderHInstance, MAKEINTRESOURCE(IDD_SHADER_CACHE), GetCOREInterface()->GetMAXHWnd(), NULL);
			ShowWindow(mActiveShader->mShaderCacheDlg, SW_SHOW);

			// Center it's position
			RECT maxRect;
			GetWindowRect(GetCOREInterface()->GetMAXHWnd(), &maxRect);

			RECT dialogRect;
			GetWindowRect(mActiveShader->mShaderCacheDlg, &dialogRect);

			int dlgWidth = (dialogRect.right - dialogRect.left);
			int dlgHeight = (dialogRect.bottom - dialogRect.top);
			LONG left = (maxRect.left + maxRect.right) * 0.5f - dlgWidth * 0.5;
			LONG top = (maxRect.bottom + maxRect.top) * 0.5f - dlgHeight * 0.5;

			SetWindowPos(mActiveShader->mShaderCacheDlg, NULL, left, top, dlgWidth, dlgHeight, 0);
		}
	}
	else
	{
		if (mActiveShader->mShaderCacheDlg)
		{
			DestroyWindow(mActiveShader->mShaderCacheDlg);
			mActiveShader->mShaderCacheDlg = NULL;
		}
		if (mActiveShader->mRenderThread)
			mActiveShader->mRenderThread->mShaderCacheReady.Fire();
	}
}

//////////////////////////////////////////////////////////////////////////////
// ActiveShadeSynchronizerBridge
//

void ActiveShadeSynchronizerBridge::LockRenderThread()
{
	if (mActiveShader->mRenderThread)
		mActiveShader->mRenderThread->Lock();
}

void ActiveShadeSynchronizerBridge::UnlockRenderThread()
{
	if (mActiveShader->mRenderThread)
		mActiveShader->mRenderThread->Unlock();
}

void ActiveShadeSynchronizerBridge::StartToneMapper()
{
	if (mActiveShader->mRenderThread)
		mActiveShader->mRenderThread->StartToneMapper();
}

void ActiveShadeSynchronizerBridge::StopToneMapper()
{
	if (mActiveShader->mRenderThread)
		mActiveShader->mRenderThread->StopToneMapper();
}

void ActiveShadeSynchronizerBridge::SetToneMappingExposure(float val)
{
	if (mActiveShader->mRenderThread)
		mActiveShader->mRenderThread->SetToneMappingExposure(val);
}

void ActiveShadeSynchronizerBridge::ClearFB()
{
	if (mActiveShader->mRenderThread)
		mActiveShader->mRenderThread->Restart();
}

IRenderProgressCallback *ActiveShadeSynchronizerBridge::GetProgressCB()
{
	return mActiveShader->pProgCB;
}

const DefaultLight *ActiveShadeSynchronizerBridge::GetDefaultLights(int &numDefLights) const
{
	numDefLights = mActiveShader->mNumDefLight;
	return mActiveShader->mDefLights;
}

IParamBlock2 *ActiveShadeSynchronizerBridge::GetPBlock()
{
	auto rend = GetCOREInterface()->GetRenderer(RS_IReshade);
	return rend->GetParamBlock(0);
}

bool ActiveShadeSynchronizerBridge::RenderThreadAlive()
{
	if (mActiveShader->mRenderThread)
		if (!mActiveShader->mRenderThread->mRenderThreadExit.Wait(0))
			return true;
	return false;
}

void ActiveShadeSynchronizerBridge::EnableAlphaBuffer()
{
	if (mActiveShader->mRenderThread)
		mActiveShader->mRenderThread->EnableAlphaBuffer(true);
}

void ActiveShadeSynchronizerBridge::DisableAlphaBuffer()
{
	if (mActiveShader->mRenderThread)
		mActiveShader->mRenderThread->EnableAlphaBuffer(false);
}

void ActiveShadeSynchronizerBridge::SetTimeLimit(int val)
{
	if (mActiveShader->mRenderThread)
		mActiveShader->mRenderThread->SetTimeLimit(val);
}

void ActiveShadeSynchronizerBridge::SetPassLimit(int val)
{
	if (mActiveShader->mRenderThread)
		mActiveShader->mRenderThread->SetPassLimit(val);
}

void ActiveShadeSynchronizerBridge::SetLimitType(int type)
{
	if (mActiveShader->mRenderThread)
		mActiveShader->mRenderThread->SetLimitType(type);
}

void ActiveShadeSynchronizerBridge::ResetInteractiveTermination()
{
	if (mActiveShader->mRenderThread)
		mActiveShader->mRenderThread->ResetTermination();
}

FIRERENDER_NAMESPACE_END;