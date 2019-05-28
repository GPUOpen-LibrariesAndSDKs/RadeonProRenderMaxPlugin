/*********************************************************************************************************************************
* Radeon ProRender for 3ds Max plugin
* Copyright (c) 2017 AMD
* All Rights Reserved
*
* Production Renderer
*********************************************************************************************************************************/

#include "ProRenderGLTF.h" // must be the first to avoid compilation errors

#include "PRManager.h"
#include "resource.h"
#include "utils/Thread.h"
#include "AssetManagement\IAssetAccessor.h"
#include "assetmanagement\AssetType.h"
#include "Assetmanagement\iassetmanager.h"
#include "FireRenderDisplacementMtl.h"
#include "FireRenderMaterialMtl.h"
#include "CamManager.h"
#include "TMManager.h"
#include "RadeonProRender.h"
#include "RprLoadStore.h"
#include "RprSupport.h"
#include <wingdi.h>
#include <math.h>
#include <shlobj.h>
#include <gamma.h>

#include <algorithm>
#include  <cctype>

#include <mutex>
#include <ctime>
#include <iomanip>

#include "RprComposite.h"

#include "RadeonProRender_CL.h"
#include "RadeonImageFilters.h"
#include "RadeonImageFilters_cl.h"


extern HINSTANCE hInstance;

FIRERENDER_NAMESPACE_BEGIN

static Event PRManagerMaxDone(false);

inline std::wstring makeExportFileName(const std::wstring & name, const unsigned int nFrame)
{
	std::wstring wname(name);
	size_t offset = wname.find_last_of(_T("."));
	return std::wstring(wname.begin(), wname.begin() + offset) + std::wstring(_T("_")) + std::to_wstring(nFrame) + std::wstring(wname.begin() + offset, wname.end());
}

class ProductionRenderCore : public BaseThread
{
public:
	typedef enum
	{
		Result_OK,
		Result_Aborted,
		Result_Catastrophic
	} TerminationResult;

	struct FrameDataBuffer
	{
		std::vector<float> colorData;
		std::vector<float> alphaData;
		float timePassed;
		int passesDone;
	};

	int width;
	int height;

	bool isShadowCatcherEnabled;
	bool isAlphaEnabled;
	bool isDenoiserEnabled;
	bool isAdaptiveEnabled;

	// termination
	std::atomic<int> passLimit;
	std::atomic<int> passesDone;
	std::atomic<__time64_t> timeLimit;
	std::atomic<__time64_t> timePassed;
	std::atomic<TerminationCriteria> term;
	std::atomic<bool> bImmediateAbort;

	rpr_int errorCode = RPR_SUCCESS;
	TerminationResult result;

	std::unique_ptr<FrameDataBuffer> pLastFrameData;

	std::atomic<int> lastFrameDataWritedCount;

	std::atomic<bool> isNormals;
	std::atomic<bool> isToneOperatorPreviewRender;
	std::atomic<bool> doRenderStamp;
	const TCHAR* timeStampString;
	int renderDevice;
	std::atomic<float> exposure;
	std::atomic<bool> useMaxTonemapper;
	std::atomic<bool> regionMode;
	Box2 region;

	std::mutex frameDataBuffersLock;

	// To ensure that we are finished with RPRCopyFrameData
	Event rprCopyFrameDataDoneEvent;

	ImageFilter* mDenoiser = nullptr;

public:
	frw::Scope scope;
	frw::FrameBuffer frameBufferColor;
	frw::FrameBuffer frameBufferColorResolve;
	frw::FrameBuffer frameBufferAlpha;
	frw::FrameBuffer frameBufferAlphaResolve;

	// shadow catcher buffers
	frw::FrameBuffer frameBufferShadowCatcher;
	frw::FrameBuffer frameBufferBackground;
	frw::FrameBuffer frameBufferComposite;
	frw::FrameBuffer frameBufferCompositeResolve;

	// denoiser buffers (doesn't need normilezed copies because they are equal to the original ones)
	frw::FrameBuffer frameBufferShadingNormal;
	frw::FrameBuffer frameBufferDepth;
	frw::FrameBuffer frameBufferWorldCoordinate;
	frw::FrameBuffer frameBufferObjectId;
	frw::FrameBuffer frameBufferDiffuseAlbedo;
	frw::FrameBuffer frameBufferDiffuseAlbedoResolve;

	// adaptive sampling buffers with CMJ sampler
	frw::FrameBuffer frameBufferVariance;
	frw::FrameBuffer frameBufferVarianceResolve;

private:
	Event eRestart;

	void SaveFrameData(void);
	void RPRCopyFrameData(void);

	mutable struct StampCachedData
	{
		std::wstring gpuName;
		std::wstring cpuName;
		std::wstring computerName;
		std::wstring product;
		std::wstring version;
		std::wstring coreVersion;
		int lightsCount = 0;
		int shapesCount = 0;
		bool hasGpuContext = false;
		bool hasCpuContext = false;
		bool isCacheCreated = false;
	} m_stampCachedData;

public:
	bool CopyFrameDataToBitmap(::Bitmap* bitmap);
	void RenderStamp(Bitmap* DstBuffer, std::unique_ptr<ProductionRenderCore::FrameDataBuffer>& frameData) const;

	explicit ProductionRenderCore(frw::Scope rscope, int width, int height,
		bool bRenderAlpha, bool bDenoiserEnabled, bool bAdaptiveEnabled,
		int priority = THREAD_PRIORITY_NORMAL, const char* name = "ProductionRenderCore");

	void InitFramebuffers();

	void Worker() override;

	void Done(TerminationResult res, rpr_int err)
	{
		rprCopyFrameDataDoneEvent.Wait();

		errorCode = err;
		rpr_int errorCode = RPR_SUCCESS;
		result = res;
		PRManagerMaxDone.Fire();

		if(bImmediateAbort)
			scope.DestroyFrameBuffers();
	}

	void AbortImmediate() override
	{
		BaseThread::AbortImmediate();
		bImmediateAbort = true;
	}

	void Restart();

	void CompositeOutput(bool flip);

	void SetDenoiser(ImageFilter* denoiser)
	{
		mDenoiser = denoiser;
	}
};

void ProductionRenderCore::RPRCopyFrameData()
{
	// get data from RPR and put it to back buffer
	std::unique_ptr<ProductionRenderCore::FrameDataBuffer> tmpFrameData = std::make_unique<ProductionRenderCore::FrameDataBuffer>();

	if (isShadowCatcherEnabled)
	{
		tmpFrameData->colorData = frameBufferCompositeResolve.GetPixelData();
	}
	else if (mDenoiser)
	{
		tmpFrameData->colorData = mDenoiser->GetData();
	}
	else
	{
		tmpFrameData->colorData = frameBufferColorResolve.GetPixelData();
	}

	// get alpha
	if (frameBufferAlpha)
	{
		tmpFrameData->alphaData = frameBufferAlphaResolve.GetPixelData();
	}

	// Save additional frame data
	tmpFrameData->timePassed = timePassed;
	tmpFrameData->passesDone = passesDone;

	// Swap new frame with the old one in the buffer
	frameDataBuffersLock.lock();
	pLastFrameData = std::move(tmpFrameData);
	frameDataBuffersLock.unlock();

	rprCopyFrameDataDoneEvent.Fire();
}

void ProductionRenderCore::SaveFrameData()
{
	try
	{
		if( frameBufferComposite )
		{
			// calculate picture composed of rendered image and several aov's (including shadow catcher)
			CompositeOutput(false);
			frameBufferComposite.Resolve(frameBufferCompositeResolve);
		}
		else
		{
			frameBufferColor.Resolve(frameBufferColorResolve);

			if (mDenoiser)
			{
				frameBufferDiffuseAlbedo.Resolve(frameBufferDiffuseAlbedoResolve);

				mDenoiser->Run();
			}
		}

		if( frameBufferAlpha )
		{
			frameBufferAlpha.Resolve(frameBufferAlphaResolve);
		}

		if( frameBufferVariance )
		{	// Resolve the Variance AOV, used for Adaptive Sampling with the CMJ sampler
			frameBufferVariance.Resolve(frameBufferVarianceResolve, true);
		}

		rprCopyFrameDataDoneEvent.Reset();
		RPRCopyFrameData();
	}
	catch (std::exception& e)
	{
		AbortImmediate();
		rprCopyFrameDataDoneEvent.Fire();
		MessageBoxA(GetCOREInterface()->GetMAXHWnd(), e.what(), "Radeon ProRender", MB_OK | MB_ICONERROR);
		return;
	}
}

bool ProductionRenderCore::CopyFrameDataToBitmap(::Bitmap* bitmap)
{
	frameDataBuffersLock.lock();
	std::unique_ptr<ProductionRenderCore::FrameDataBuffer> pFrameData = std::move(pLastFrameData);
	frameDataBuffersLock.unlock();

	if (pFrameData.get() == nullptr)
		return false;

	float _exposure = IsReal((float)exposure) ? (float)exposure : 1.f;
	CompositeFrameBuffersToBitmap(pFrameData->colorData, pFrameData->alphaData, bitmap, _exposure, isNormals, isToneOperatorPreviewRender);

	if (useMaxTonemapper)
		UpdateBitmapWithToneOperator(bitmap);

	RenderStamp(bitmap, pFrameData);

	return true;
}

ProductionRenderCore::ProductionRenderCore(frw::Scope rscope, int width, int height,
	bool bRenderAlpha, bool bDenoiserEnabled, bool bAdaptiveEnabled,
	int priority, const char* name) :
		BaseThread(name, priority),
		scope(rscope),
		eRestart(true),
		width(width),
		height(height),
		isAlphaEnabled(bRenderAlpha),
		isDenoiserEnabled(bDenoiserEnabled),
		isAdaptiveEnabled(bAdaptiveEnabled),
		bImmediateAbort(false)
{
	// termination
	passLimit = 1;
	passesDone = 0;
	timeLimit = 10;
	term = Termination_None;
	errorCode = RPR_SUCCESS;
	lastFrameDataWritedCount = 0;

	isNormals = false;
	isToneOperatorPreviewRender = false;
	doRenderStamp = false;
	timeStampString = 0;
	renderDevice = 0;
	exposure = 1.f;
	useMaxTonemapper = true;
	regionMode = false;

	// Setup AOVs - first pass (apply AOVs for denoiser, required BEFORE translation)
	InitFramebuffers();
}

// Creates framebuffer and applies mandatory AOVs - called in two passes
// First pass to apply AOVs for denoiser, required BEFORE translation
// Second pass to apply AOVs for Shadow Catcher, fails unless AFTER translation
void ProductionRenderCore::InitFramebuffers()
{
	frw::Context ctx = scope.GetContext();

	// Set color buffer
	frameBufferColor = scope.GetFrameBuffer(width, height, FramebufferTypeId_Color);
	frameBufferColorResolve = scope.GetFrameBuffer(width, height, FramebufferTypeId_ColorResolve);
	ctx.SetAOV(frameBufferColor, RPR_AOV_COLOR);

	// TODO: Remove this?  Maybe unnecessary if safe to apply AOV framebuffers twice
	bool hadFrameBufferShadowCatcher = (bool)frameBufferShadowCatcher;
	bool hadFrameBufferBackground = (bool)frameBufferBackground;
	bool hadFrameBufferDepth =(bool) frameBufferDepth;
	bool hadFrameBufferComposite = (bool)frameBufferComposite;
	bool hadFrameBufferCompositeResolve = (bool)frameBufferCompositeResolve;
	bool hadFrameBufferAlpha = (bool)frameBufferAlpha;
	bool hadFrameBufferAlphaResolve = (bool)frameBufferAlphaResolve;
	bool hadFrameBufferShadingNormal = (bool)frameBufferShadingNormal;
	bool hadFrameBufferWorldCoordinate = (bool)frameBufferWorldCoordinate;
	bool hadFrameBufferObjectId = (bool)frameBufferObjectId;
	bool hadFrameBufferDiffuseAlbedo = (bool)frameBufferDiffuseAlbedo;
	bool hadFrameBufferDiffuseAlbedoResolve = (bool)frameBufferDiffuseAlbedoResolve;
	bool hadFrameBufferVariance = (bool)frameBufferVariance;
	bool hadFrameBufferVarianceResolve = (bool)frameBufferVarianceResolve;

	//Find shadow catcher shader
	//Fails on first pass before translation, works on second pass after translation
	isShadowCatcherEnabled = scope.GetShadowCatcherShader().IsValid();

	if (isShadowCatcherEnabled) 
	{
		// Set background buffer
		if( !frameBufferShadowCatcher )
			frameBufferShadowCatcher = scope.GetFrameBuffer(width, height, FrameBufferTypeId_ShadowCatcher);
		if( !frameBufferBackground )
			frameBufferBackground = scope.GetFrameBuffer(width, height, FrameBufferTypeId_Background);

		// Set depth buffer
		if( !frameBufferDepth )
			frameBufferDepth = scope.GetFrameBuffer(width, height, FramebufferTypeId_Depth);

		// buffers for results
		if( !frameBufferComposite )
			frameBufferComposite = scope.GetFrameBuffer(width, height, FrameBufferTypeId_Composite);
		if( !frameBufferCompositeResolve )
			frameBufferCompositeResolve = scope.GetFrameBuffer(width, height, FrameBufferTypeId_CompositeResolve);

		isAlphaEnabled = true;
		useMaxTonemapper = false;
	}

	// Set alpha buffer
	if (isAlphaEnabled)
	{
		if( !frameBufferAlpha )
			frameBufferAlpha = scope.GetFrameBuffer(width, height, FramebufferTypeId_Alpha);
		if( !frameBufferAlphaResolve )
			frameBufferAlphaResolve = scope.GetFrameBuffer(width, height, FramebufferTypeId_AlphaResolve);
	}

	// additional frame buffers for denoising
	if (isDenoiserEnabled)
	{
		if( !frameBufferShadingNormal )
			frameBufferShadingNormal = scope.GetFrameBuffer(width, height, FramebufferTypeId_ShadingNormal);
		if( !frameBufferDepth )
			frameBufferDepth = scope.GetFrameBuffer(width, height, FramebufferTypeId_Depth);
		if( !frameBufferWorldCoordinate )
			frameBufferWorldCoordinate = scope.GetFrameBuffer(width, height, FramebufferTypeId_WorldCoordinate);
		if( !frameBufferObjectId )
			frameBufferObjectId = scope.GetFrameBuffer(width, height, FramebufferTypeId_ObjectId);
		if( !frameBufferDiffuseAlbedo )
			frameBufferDiffuseAlbedo = scope.GetFrameBuffer(width, height, FramebufferTypeId_DiffuseAlbedo);
		if( !frameBufferDiffuseAlbedoResolve )
			frameBufferDiffuseAlbedoResolve = scope.GetFrameBuffer(width, height, FramebufferTypeId_DiffuseAlbedoResolve);
	}

	// Set the Variance AOV, used for Adaptive Sampling with the CMJ sampler
	if (isAdaptiveEnabled)
	{
		// Set shadow buffer
		if( !frameBufferVariance )
			frameBufferVariance = scope.GetFrameBuffer(width, height, FrameBufferTypeId_Variance);
		if( !frameBufferVarianceResolve )
			frameBufferVarianceResolve = scope.GetFrameBuffer(width, height, FrameBufferTypeId_VarianceResolve);
	}

	// TODO: Safe to apply AOV framebuffers twice, for the two passes?
	if( frameBufferShadowCatcher && !hadFrameBufferShadowCatcher )
		ctx.SetAOV(frameBufferShadowCatcher, RPR_AOV_SHADOW_CATCHER);
	if( frameBufferBackground && !hadFrameBufferBackground )
		ctx.SetAOV(frameBufferBackground, RPR_AOV_BACKGROUND);
	if( frameBufferAlpha && !hadFrameBufferAlpha )
		ctx.SetAOV(frameBufferAlpha, RPR_AOV_OPACITY);
	if( frameBufferShadingNormal && !hadFrameBufferShadingNormal )
		ctx.SetAOV(frameBufferShadingNormal, RPR_AOV_SHADING_NORMAL);
	if( frameBufferDepth && !hadFrameBufferDepth )
		ctx.SetAOV(frameBufferDepth, RPR_AOV_DEPTH);
	if( frameBufferWorldCoordinate && !hadFrameBufferWorldCoordinate )
		ctx.SetAOV(frameBufferWorldCoordinate, RPR_AOV_WORLD_COORDINATE);
	if( frameBufferObjectId && !hadFrameBufferObjectId )
		ctx.SetAOV(frameBufferObjectId, RPR_AOV_OBJECT_ID);
	if( frameBufferDiffuseAlbedo && !hadFrameBufferDiffuseAlbedo )
		ctx.SetAOV(frameBufferDiffuseAlbedo, RPR_AOV_DIFFUSE_ALBEDO);
	if( frameBufferVariance && !hadFrameBufferVariance )
		ctx.SetAOV(frameBufferVariance, RPR_AOV_VARIANCE);
}

// Shadow catcher Impl
void ProductionRenderCore::CompositeOutput(bool flip)
{
	unsigned int width = scope.Width();
	unsigned int height = scope.Height();

	frw::Context context = scope.GetContext();

	// Step 1.
	// Combine normalized color, background and opacity AOVs using lerp
	RprComposite compositeBg(context.Handle(), RPR_COMPOSITE_FRAMEBUFFER);
	compositeBg.SetInputFb("framebuffer.input", frameBufferBackground.Handle());

	RprComposite compositeColor(context.Handle(), RPR_COMPOSITE_FRAMEBUFFER);
	compositeColor.SetInputFb("framebuffer.input", frameBufferColor.Handle());

	RprComposite compositeOpacity(context.Handle(), RPR_COMPOSITE_FRAMEBUFFER);
	compositeOpacity.SetInputFb("framebuffer.input", frameBufferAlpha.Handle());

	RprComposite compositeBgNorm(context.Handle(), RPR_COMPOSITE_NORMALIZE);
	compositeBgNorm.SetInputC("normalize.color", compositeBg);

	RprComposite compositeColorNorm(context.Handle(), RPR_COMPOSITE_NORMALIZE);
	compositeColorNorm.SetInputC("normalize.color", compositeColor);

	RprComposite compositeOpacityNorm(context.Handle(), RPR_COMPOSITE_NORMALIZE);
	compositeOpacityNorm.SetInputC("normalize.color", compositeOpacity);

	RprComposite compositeLerp1(context.Handle(), RPR_COMPOSITE_LERP_VALUE);
	compositeLerp1.SetInputC("lerp.color0", compositeBgNorm);
	compositeLerp1.SetInputC("lerp.color1", compositeColorNorm);
	compositeLerp1.SetInputC("lerp.weight", compositeOpacityNorm);

	// Step 2.
	// Combine result from step 1, black color and normalized shadow catcher AOV
	RprComposite compositeShadowCatcher(context.Handle(), RPR_COMPOSITE_FRAMEBUFFER);
	compositeShadowCatcher.SetInputFb("framebuffer.input", frameBufferShadowCatcher.Handle());

	//Find first shadow catcher shader
	frw::Shader shadowCatcherShader = scope.GetShadowCatcherShader();
	assert(shadowCatcherShader);
	
	float r = 0.0f;
	float g = 0.0f;
	float b = 0.0f;
	float a = 0.0f;
	
	shadowCatcherShader.GetShadowColor(&r, &g, &b, &a);

	float weight = shadowCatcherShader.GetShadowWeight();

	RprComposite compositeShadowColor(context.Handle(), RPR_COMPOSITE_CONSTANT);

	// We have "Shadow Transparency" parameter in Shadow Catcher, but not "Shadow opacity"
	// It means we should invert this parameter
	compositeShadowColor.SetInput4f("constant.input", r, g, b, 1.0f - a);

	RprComposite compositeShadowWeight(context.Handle(), RPR_COMPOSITE_CONSTANT);
	compositeShadowWeight.SetInput4f("constant.input", weight, weight, weight, weight);

	RprComposite compositeShadowCatcherNorm(context.Handle(), RPR_COMPOSITE_NORMALIZE);
	compositeShadowCatcherNorm.SetInputC("normalize.color", compositeShadowCatcher);
	rprCompositeSetInput1u(compositeShadowCatcherNorm, "normalize.aovtype", RPR_AOV_SHADOW_CATCHER);

	RprComposite compositeSCWeight(context.Handle(), RPR_COMPOSITE_ARITHMETIC);
	compositeSCWeight.SetInputC("arithmetic.color0", compositeShadowCatcherNorm);
	compositeSCWeight.SetInputC("arithmetic.color1", compositeShadowWeight);
	compositeSCWeight.SetInputOp("arithmetic.op", RPR_MATERIAL_NODE_OP_MUL);

	RprComposite compositeLerp2(context.Handle(), RPR_COMPOSITE_LERP_VALUE);

	//Setting BgIsEnv to false means that we should use shadow catcher color instead of environment background
	if (shadowCatcherShader.BgIsEnv())
		compositeLerp2.SetInputC("lerp.color0", compositeLerp1);
	else
		compositeLerp2.SetInputC("lerp.color0", compositeColorNorm);

	compositeLerp2.SetInputC("lerp.color1", compositeShadowColor);
	compositeLerp2.SetInputC("lerp.weight", compositeSCWeight);

	// Step 3.
	// Compute results from step 2 into separate framebuffer
	rpr_int status = rprCompositeCompute(compositeLerp2, frameBufferComposite.Handle());
	FASSERT(RPR_SUCCESS == status);
}

void ProductionRenderCore::Worker()
{
	unsigned int numPasses = passLimit;

	if (numPasses < 1)
		numPasses = 1;
	
	passesDone = 0;
	__time64_t startedAt = time(0);

	// Setup AOVs - second pass (apply AOVs for Shadow Catcher, fails unless AFTER translation)
	InitFramebuffers();
	bool clearFramebuffer = true;

	int xmin = 0, ymin = 0, xmax = 0, ymax = 0;

	if (regionMode)
	{
		if (region.IsEmpty())
		{
			regionMode = false;
		}
		else
		{
			xmin = region.x();
			ymin = region.y();
			xmax = xmin + region.w();
			ymax = ymin + region.h();
		}
	}

	rprCopyFrameDataDoneEvent.Fire();

	// render all passes, unless the render is cancelled (and even then render at least a single pass)
	while (1)
	{
		// Stop thread
		if (mStop.Wait(0) || bImmediateAbort)
		{
			Done(Result_Aborted, RPR_SUCCESS);
			return;
		}
		
		// Restart (reset) thread
		if (eRestart.Wait(0))
		{
			clearFramebuffer = true;
			passesDone = 0;
		}
		
		// Clear the frame buffers we render into
		// Only do this for the first render pass
		if (clearFramebuffer)
		{
			frameBufferColor.Clear();

			if( frameBufferAlpha )
			{
				frameBufferAlpha.Clear();
			}

			if( frameBufferShadowCatcher )
			{
				frameBufferShadowCatcher.Clear();
			}

			if( frameBufferBackground )
			{
				frameBufferBackground.Clear();
			}

			if( frameBufferVariance )
			{
				frameBufferVariance.Clear();
			}

			clearFramebuffer = false;
		}

		// Render
		rpr_int status = RPR_SUCCESS;

		try
		{
			frw::Context& ctx = scope.GetContext();
			status = regionMode ? ctx.RenderTile(xmin, xmax, ymin, ymax) : ctx.Render();
		}
		catch (...) // Exception
		{
			debugPrint("Exception occurred in render call");
			Done(Result_Catastrophic, RPR_ERROR_INTERNAL_ERROR);
			return;
		}

		// Render failed
		if (status != RPR_SUCCESS)
		{
			Done(Result_Catastrophic, status);
			return;
		}

		// Stop thread
		if (bImmediateAbort)
		{
			Done(Result_Aborted, RPR_SUCCESS);
			return;
		}

		// One more pass got rendered
		++passesDone;

		// Save rendered frame buffer + elapsed time etc (frame data for blitting), for main thread for further processing & blitting
		SaveFrameData();
		
		// Terminate thread when we reach a specific (amount of time) or (pass count) set by the artist
		__time64_t current = time(0);
		timePassed = current - startedAt;

		if (term != Termination_None)
		{
			if( ((term == Termination_Passes) || (term == Termination_PassesOrTime)) && (passesDone >= numPasses) ) // Passes done, terminate
			{
				Done(Result_OK, RPR_SUCCESS);
				return;
			}
			else if( ((term == Termination_Time) || (term == Termination_PassesOrTime)) && (timePassed >= timeLimit)) // Time limit reached, terminate
			{
				Done(Result_OK, RPR_SUCCESS);
				return;
			}
		}
	}
}

void ProductionRenderCore::Restart()
{
	eRestart.Fire();
	if (!IsRunning())
		Start();
}

void ProductionRenderCore::RenderStamp(Bitmap* DstBuffer, std::unique_ptr<ProductionRenderCore::FrameDataBuffer>& frameData) const
{
	if (!doRenderStamp)
		return;
	
	if (!timeStampString || !timeStampString[0])
		return; // empty string

	const TCHAR* sourceStr = timeStampString;

	auto scene = scope.GetScene();

	if (!m_stampCachedData.isCacheCreated)
	{
		// cache not created yet => write values to cache
		m_stampCachedData.gpuName = GetFriendlyUsedGPUName();
		m_stampCachedData.cpuName = GetCPUName();
		m_stampCachedData.computerName = ComputerName();
		m_stampCachedData.lightsCount = scene.LightObjectCount();
		m_stampCachedData.shapesCount = scene.ShapeObjectCount();
		GetProductAndVersion(m_stampCachedData.product, m_stampCachedData.version, m_stampCachedData.coreVersion);

		m_stampCachedData.hasCpuContext = ScopeManagerMax::TheManager.cpuInfo.isUsed;
		m_stampCachedData.hasGpuContext = ScopeManagerMax::TheManager.getGpuUsedCount() > 0;

		m_stampCachedData.isCacheCreated = true;
	}

	// parse string
	std::wstringstream outStream;

	while (MCHAR c = *sourceStr++)
	{
		if (c != '%')
		{
			outStream << c;
			continue;
		}

		// here we have escape sequence
		c = *sourceStr++;

		if (!c)
		{
			outStream << L'%'; // this was a last character in string
			break;
		}

		switch (c)
		{
		case '%': // %% - add single % character
			outStream << c;
			break;

		case 'p': // performance
		{
			c = *sourceStr++;

			switch (c)
			{
			case 't': // %pt - total elapsed time
			{
				unsigned int secs = (int) frameData->timePassed;
				unsigned int hrs = secs / (60 * 60);
				secs = secs % (60 * 60);
				unsigned int mins = secs / 60;
				secs = secs % 60;

				outStream << std::setw(2) << std::setfill(L'0') << std::to_wstring(hrs) << L":"
					<< std::setw(2) << std::setfill(L'0') << std::to_wstring(mins) << L":"
					<< std::setw(2) << std::setfill(L'0') << std::to_wstring(secs);
			}
			break;

			case 'p': // %pp - passes
				outStream << std::to_wstring(frameData->passesDone);
				break;
			}
		}
		break;

		case 's': // scene information
		{
			c = *sourceStr++;

			switch (c)
			{
			case 'l': // %sl - number of light primitives
				outStream << std::to_wstring(m_stampCachedData.lightsCount);
				break;

			case 'o': // %so - number of objects
				outStream << std::to_wstring(m_stampCachedData.shapesCount);
				break;
			}
		}
		break;

		case 'c': // CPU name
			outStream << m_stampCachedData.cpuName;
			break;

		case 'g': // GPU name
			outStream << m_stampCachedData.gpuName;
			break;

		case 'r': // rendering mode
		{
			if (m_stampCachedData.hasCpuContext && m_stampCachedData.hasGpuContext)
			{
				outStream << L"CPU/GPU";
			}
			else if (m_stampCachedData.hasCpuContext)
			{
				outStream << L"CPU";
			}
			else if (m_stampCachedData.hasGpuContext)
			{
				outStream << L"GPU";
			}
		}
		break;

		case 'h': // used hardware
		{
			if (m_stampCachedData.hasCpuContext && m_stampCachedData.hasGpuContext)
			{
				outStream << m_stampCachedData.cpuName + L" / " + m_stampCachedData.gpuName;
			}
			else if (m_stampCachedData.hasCpuContext)
			{
				outStream << m_stampCachedData.cpuName;
			}
			else if (m_stampCachedData.hasGpuContext)
			{
				outStream << m_stampCachedData.gpuName;
			}
		}
		break;

		case 'i': // computer name
			outStream << m_stampCachedData.computerName;
			break;

		case 'd': // current date
		{
			auto now = std::chrono::system_clock::now();
			auto in_time_t = std::chrono::system_clock::to_time_t(now);
			std::tm tmBuffer;

			localtime_s(&tmBuffer, &in_time_t);
			outStream << std::put_time(&tmBuffer, L"%Y %b %d, %X"); // e.g. 2018 Jul 11, 11:42:30
		}
		break;

		case 'b': // build number
		{
			outStream << m_stampCachedData.version << L" (core " << m_stampCachedData.coreVersion << L")";
		}
		break;

		default:
			// wrong escape sequence, add character
			if (c)
			{
				outStream << L'%' << c;
			}
		}

		if (!c)
		{
			break; // could happen when string ends with multi-character escape sequence, like immediately after "%p" etc
		}
	}

	Bitmap* bmp = RenderTextToBitmap(outStream.str().c_str());
	
	// copy 'bmp' to 'DstBuffer' at bottom-right corner
	int x = DstBuffer->Width() > bmp->Width() ? DstBuffer->Width() - bmp->Width() : 0;
	int y = DstBuffer->Height() - bmp->Height() ? DstBuffer->Height() - bmp->Height() : 0;
	
	BlitBitmap(DstBuffer, bmp, x, y, 0.5f);

	bmp->DeleteThis();
}


//////////////////////////////////////////////////////////////////////////////
//

PRManagerMax PRManagerMax::TheManager;

class PRManagerMaxClassDesc : public ClassDesc
{
public:
	int             IsPublic() { return FALSE; }
	void*           Create(BOOL loading) { return &PRManagerMax::TheManager; }
	const TCHAR*    ClassName() { return _T("RPRPRManager"); }
	SClass_ID       SuperClassID() { return GUP_CLASS_ID; }
	Class_ID        ClassID() { return PRMANAGER_MAX_CLASSID; }
	const TCHAR*    Category() { return _T(""); }
};

static PRManagerMaxClassDesc PRManagerMaxCD;

ClassDesc* PRManagerMax::GetClassDesc()
{
	return &PRManagerMaxCD;
}


PRManagerMax::PRManagerMax()
{
	refTarget = 0;

	// initialize models path string for ML denoiser
	std::string modulePath = ws2s(GetModuleFolder());

	size_t pos = modulePath.rfind("plug-ins");
	
	if (pos != std::string::npos)
	{
		mMlModelPath.assign(modulePath, 0, pos);
		mMlModelPath.append("data\\models");
	}
}

PRManagerMax::~PRManagerMax()
{
	for (auto renderer : mInstances)
		CleanUpRender(renderer.first);
}

void PRManagerMax::NotifyProc(void *param, NotifyInfo *info)
{
	PRManagerMax *mp = reinterpret_cast<PRManagerMax*>(param);

	if (info->intcode == NOTIFY_SYSTEM_PRE_RESET || info->intcode == NOTIFY_SYSTEM_PRE_NEW)
	{
		for (auto renderer : mp->mInstances)
			if (renderer.second->renderThread)
				renderer.second->renderThread->Abort();
			else
			{
				ScopeManagerMax::TheManager.DestroyScope(renderer.second->scopeId);
				renderer.second->scopeId = -1;
			}
	}
	else if (info->intcode == NOTIFY_PRE_RENDERER_CHANGE)
	{
		for (auto renderer : mp->mInstances)
			if (renderer.second->renderThread)
				renderer.second->renderThread->Abort();
	}
	else if (info->intcode == NOTIFY_POST_RENDERER_CHANGE)
	{
		size_t id = reinterpret_cast<size_t>(info->callParam);
		if (id == 0)
		{
			bool keep = false;
			auto renderer = GetCOREInterface()->GetRenderer((RenderSettingID)id, false);
			
			if (renderer)
			{
				auto cid = renderer->ClassID();
				if (cid == FIRE_MAX_CID)
					keep = true;
			}

			if (!keep)
			{
				auto dd = mp->mInstances.find(static_cast<FireRenderer*>(renderer));
				if (dd != mp->mInstances.end())
				{
					if (dd->second->renderThread)
						dd->second->renderThread->Abort();
					else
					{
						ScopeManagerMax::TheManager.DestroyScope(dd->second->scopeId);
						dd->second->scopeId = -1;
					}
				}
			}
		}
	}
}

void PRManagerMax::CleanUpRender(FireRenderer* pRenderer)
{
	auto dd = mInstances.find(static_cast<FireRenderer*>(pRenderer));

	if (dd != mInstances.end())
	{
		auto data = dd->second;

		if (data->helperThread)
		{
			data->helperThread->join();

			delete data->helperThread;
			data->helperThread = nullptr;
		}

		if (data->renderThread)
		{
			data->renderThread->Abort();

			if (!data->bRenderThreadDone)
			{
				PRManagerMaxDone.Wait();
			}

			delete data->renderThread;
			data->renderThread = nullptr;
		}
	}
}

// Activate and Stay Resident
DWORD PRManagerMax::Start()
{
	int res;
	res = RegisterNotification(NotifyProc, this, NOTIFY_SYSTEM_PRE_RESET);
	res = RegisterNotification(NotifyProc, this, NOTIFY_SYSTEM_PRE_NEW);
	res = RegisterNotification(NotifyProc, this, NOTIFY_PRE_RENDERER_CHANGE);
	res = RegisterNotification(NotifyProc, this, NOTIFY_POST_RENDERER_CHANGE);
	return GUPRESULT_KEEP;
}

void PRManagerMax::Stop()
{
	UnRegisterNotification(NotifyProc, this, NOTIFY_SYSTEM_PRE_RESET);
	UnRegisterNotification(NotifyProc, this, NOTIFY_SYSTEM_PRE_NEW);
	UnRegisterNotification(NotifyProc, this, NOTIFY_PRE_RENDERER_CHANGE);
	UnRegisterNotification(NotifyProc, this, NOTIFY_POST_RENDERER_CHANGE);
}

void PRManagerMax::DeleteThis()
{
	for (auto renderer : mInstances)
		if (renderer.second->renderThread)
			renderer.second->renderThread->Abort();
}

int PRManagerMax::Open(FireRenderer *pRenderer, HWND hWnd, RendProgressCallback* pProg)
{
	auto dd = mInstances.find(static_cast<FireRenderer*>(pRenderer));
	FASSERT(dd == mInstances.end());

	Data *data = new Data();
	mInstances.insert(std::make_pair(pRenderer, data));

	auto &parameters = pRenderer->parameters;

	if (parameters.rendParams.IsToneOperatorPreviewRender())
	{
		data->isToneOperatorPreviewRender = true;
		data->termCriteria = Termination_Passes;
		data->passLimit = 10;
	}
	else
	{
		int renderLimitType = GetFromPb<int>(parameters.pblock, PARAM_RENDER_LIMIT);
		switch (renderLimitType)
		{
		case TerminationCriteria::Termination_Passes:
			data->termCriteria = Termination_Passes;
			data->passLimit = GetFromPb<int>(parameters.pblock, PARAM_PASS_LIMIT);
			break;
		case TerminationCriteria::Termination_Time:
			data->termCriteria = Termination_Time;
			data->timeLimit = GetFromPb<int>(parameters.pblock, PARAM_TIME_LIMIT);
			break;
		case TerminationCriteria::Termination_None:
			data->termCriteria = Termination_None;
			break;
		case TerminationCriteria::Termination_PassesOrTime:
			data->termCriteria = Termination_PassesOrTime;
			data->passLimit = GetFromPb<int>(parameters.pblock, PARAM_PASS_LIMIT);
			data->timeLimit = GetFromPb<int>(parameters.pblock, PARAM_TIME_LIMIT);
			break;
		}
	}

	data->scopeId = ScopeManagerMax::TheManager.CreateScope(parameters.pblock);
	
	auto scope = ScopeManagerMax::TheManager.GetScope(data->scopeId);
	auto context = scope.GetContext();

	const float filterRadius = GetFromPb<float>(parameters.pblock, PARAM_IMAGE_FILTER_WIDTH);
	context.SetParameter("texturecompression", GetFromPb<int>(parameters.pblock, PARAM_USE_TEXTURE_COMPRESSION));
	context.SetParameter("rendermode", GetFromPb<int>(parameters.pblock, PARAM_RENDER_MODE));
	context.SetParameter("maxRecursion", GetFromPb<int>(parameters.pblock, PARAM_MAX_RAY_DEPTH));
	context.SetParameter("imagefilter.type", GetFromPb<int>(parameters.pblock, PARAM_IMAGE_FILTER));
	context.SetParameter("imagefilter.box.radius", filterRadius);
	context.SetParameter("imagefilter.gaussian.radius", filterRadius);
	context.SetParameter("imagefilter.triangle.radius", filterRadius);
	context.SetParameter("imagefilter.mitchell.radius", filterRadius);
	context.SetParameter("imagefilter.lanczos.radius", filterRadius);
	context.SetParameter("imagefilter.blackmanharris.radius", filterRadius);
	context.SetParameter("iterations", GetFromPb<int>(parameters.pblock, PARAM_CONTEXT_ITERATIONS));
	context.SetParameter("pdfthreshold", 0.f);
	context.SetParameter("preview", 0);
	data->adaptiveThreshold = GetFromPb<float>(parameters.pblock, PARAM_ADAPTIVE_NOISE_THRESHOLD);
	context.SetParameter("as.threshold", data->adaptiveThreshold);
	context.SetParameter("as.tilesize", GetFromPb<int>(parameters.pblock, PARAM_ADAPTIVE_TILESIZE));
	context.SetParameter("as.minspp", GetFromPb<int>(parameters.pblock, PARAM_SAMPLES_MIN));


	float raycastEpsilon = GetFromPb<float>(parameters.pblock, PARAM_QUALITY_RAYCAST_EPSILON);
	context.SetParameter("raycastepsilon", raycastEpsilon);

	BOOL useIrradianceClamp = FALSE;
	float irradianceClamp = FLT_MAX;
	parameters.pblock->GetValue(PARAM_USE_IRRADIANCE_CLAMP, 0, useIrradianceClamp, Interval());
	if (useIrradianceClamp)
		parameters.pblock->GetValue(PARAM_IRRADIANCE_CLAMP, 0, irradianceClamp, Interval());
	context.SetParameter("radianceclamp", irradianceClamp);

	BroadcastNotification(NOTIFY_PRE_RENDER, &parameters.rendParams);
	
	return 1;
}

void PRManagerMax::Close(FireRenderer *pRenderer, HWND hwnd, RendProgressCallback* prog)
{
	auto dd = mInstances.find(static_cast<FireRenderer*>(pRenderer));
	FASSERT(dd != mInstances.end());

	const Data* data = dd->second;
	auto& parameters = pRenderer->parameters;

	ScopeManagerMax::TheManager.DestroyScope(data->scopeId);

	delete data;
	mInstances.erase(dd);

	BroadcastNotification(NOTIFY_POST_RENDER);
}


int PRManagerMax::Render(FireRenderer* pRenderer, TimeValue t, ::Bitmap* frontBuffer, FrameRendParams &frp, HWND hwnd, RendProgressCallback* prog, ViewParams* viewPar)
{
	SuspendAll(TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE);

	int returnValue = 1;

	auto dd = mInstances.find(static_cast<FireRenderer*>(pRenderer));
	FASSERT(dd != mInstances.end());

	PRManagerMax::Data* data = dd->second;

	RenderParameters& parameters = pRenderer->parameters;

	// It can happen that the user immediately cancelled the PREVIOUS render, and here we do a lazy cleanup
	CleanUpRender(pRenderer);

	int renderWidth = frontBuffer->Width();
	int renderHeight = frontBuffer->Height();

	BroadcastNotification(NOTIFY_PRE_RENDERFRAME, &parameters.renderGlobalContext);

	parameters.frameRendParams = frp;
	parameters.t = t;
	parameters.hWnd = hwnd;
	parameters.progress = prog;
	if (viewPar)
		parameters.viewParams = *viewPar;
	
	frw::Scope scope = ScopeManagerMax::TheManager.GetScope(data->scopeId);
	auto context = scope.GetContext();

	// Parse scene info
	SceneCallbacks callbacks;
	auto parser = std::make_unique<SceneParser>(parameters, callbacks, scope);

	if (parameters.progress)
		parameters.progress->SetTitle(_T("Synchronizing scene..."));

	auto scene = scope.GetScene(true);

	// Do we need alpha channel rendering?
	BOOL bRenderAlpha;
	parameters.pblock->GetValue(TRPARAM_BG_ENABLEALPHA, 0, bRenderAlpha, Interval());

	// setup Denoiser
	bool bDenoiserEnabled = false;
	DenoiserType denoiserType = DenoiserNone;

	std::tie(bDenoiserEnabled, denoiserType) = IsDenoiserEnabled(parameters.pblock);

	// Disable Adaptive Sampling with the CMJ sampler, if noise threshold is zero
	bool bAdaptiveEnabled = (data->adaptiveThreshold > 0);

	// create frame buffer
	data->renderThread = new ProductionRenderCore(scope, renderWidth, renderHeight,
		bool_cast(bRenderAlpha), bDenoiserEnabled, bAdaptiveEnabled);

	// Render Elements
	auto renderElementMgr = parameters.rendParams.GetRenderElementMgr();
	
	if (renderElementMgr)
	{
		int renderElementCount = renderElementMgr->NumRenderElements();

		if (renderElementCount)
		{
			for (int i = 0; i<renderElementCount; ++i)
			{
				IRenderElement* renderElement = renderElementMgr->GetRenderElement(i);
				rpr_aov getAOVRenderElementFRId(IRenderElement* renderElement);  // external, defined in AOVs.cpp
				rpr_aov aov = getAOVRenderElementFRId(renderElement);
				if (RPR_AOV_MAX != aov)
				{
					auto fb = scope.GetFrameBuffer(renderWidth, renderHeight, GetFramebufferTypeIdForAOV(aov));
					context.SetAOV(fb, aov);
					fb.Clear();

					if (aov == RPR_AOV_OPACITY)
						bRenderAlpha = true;
				}
			}
		}
	}
	
	// Post process {tonemapping, etc}
	BOOL overrideTonemappers = FALSE;
	const int renderMode = GetFromPb<int>(parameters.pblock, PARAM_RENDER_MODE);
	bool isNormals = (renderMode == RPR_RENDER_MODE_NORMAL);
	data->toneMappingExposure = parser->view.cameraExposure;
	BOOL res = TmManagerMax::TheManager.GetProperty(PARAM_TM_OVERRIDE_MAX_TONEMAPPERS, overrideTonemappers); FASSERT(res);

	frw::PostEffect normalization;
	frw::PostEffect simple_tonemap;
	frw::PostEffect white_balance;
	frw::PostEffect gamma_correction;
	frw::PostEffect tonemap;

	normalization = frw::PostEffect(context, frw::PostEffectTypeNormalization);
	context.Attach(normalization);

	if (overrideTonemappers)
	{
		int toneMappingType = frw::ToneMappingTypeNone;
		res = TmManagerMax::TheManager.GetProperty(PARAM_TM_OPERATOR, toneMappingType); FASSERT(res);
		
		const int renderMode = GetFromPb<int>(parameters.pblock, PARAM_RENDER_MODE);

		// reset the exposure to 1 if we are doing render passes
		if (renderMode == RPR_RENDER_MODE_WIREFRAME || renderMode == RPR_RENDER_MODE_MATERIAL_INDEX ||
			renderMode == RPR_RENDER_MODE_POSITION || renderMode == RPR_RENDER_MODE_NORMAL || renderMode == RPR_RENDER_MODE_TEXCOORD || renderMode == RPR_RENDER_MODE_AMBIENT_OCCLUSION) {
			data->toneMappingExposure = 1.f;
		}

		// Enable the tone mapping only if we are in suitable render mode
		data->shouldToneMap = (toneMappingType != frw::ToneMappingTypeNone) &&
			(renderMode == RPR_RENDER_MODE_GLOBAL_ILLUMINATION || renderMode == RPR_RENDER_MODE_DIRECT_ILLUMINATION || renderMode == RPR_RENDER_MODE_DIRECT_ILLUMINATION_NO_SHADOW);

		if (data->shouldToneMap)
		{
			switch (toneMappingType)
			{
				case frw::ToneMappingTypeSimplified:
				{
					context.SetParameter("tonemapping.type", RPR_TONEMAPPING_OPERATOR_NONE);

					float exposure, contrast;
					int whitebalance;
					Color tintcolor;
					res = TmManagerMax::TheManager.GetProperty(PARAM_TM_SIMPLIFIED_EXPOSURE, exposure); FASSERT(res);
					res = TmManagerMax::TheManager.GetProperty(PARAM_TM_SIMPLIFIED_CONTRAST, contrast); FASSERT(res);
					res = TmManagerMax::TheManager.GetProperty(PARAM_TM_SIMPLIFIED_WHITEBALANCE, whitebalance); FASSERT(res);
					res = TmManagerMax::TheManager.GetProperty(PARAM_TM_SIMPLIFIED_TINTCOLOR, tintcolor); FASSERT(res);

					simple_tonemap = frw::PostEffect(context, frw::PostEffectTypeSimpleTonemap);
					simple_tonemap.SetParameter("exposure", exposure);
					simple_tonemap.SetParameter("contrast", contrast);
                    context.Attach(simple_tonemap);

					white_balance = frw::PostEffect(context, frw::PostEffectTypeWhiteBalance);
					white_balance.SetParameter("colorspace", frw::ColorSpaceTypeAdobeSRGB);
					white_balance.SetParameter("colortemp", (float)whitebalance );
					context.Attach(white_balance);

					gamma_correction = frw::PostEffect(context, frw::PostEffectTypeGammaCorrection);
					context.Attach(gamma_correction);
				}
				break;
				case frw::ToneMappingTypeLinear:
				{
					tonemap = frw::PostEffect(context, frw::PostEffectTypeToneMap);
					context.Attach(tonemap);

					gamma_correction = frw::PostEffect(context, frw::PostEffectTypeGammaCorrection);
					context.Attach(gamma_correction);

					float iso, fstop, shutterspeed;
					res = TmManagerMax::TheManager.GetProperty(PARAM_TM_PHOTOLINEAR_ISO, iso); FASSERT(res);
					res = TmManagerMax::TheManager.GetProperty(PARAM_TM_PHOTOLINEAR_FSTOP, fstop); FASSERT(res);
					res = TmManagerMax::TheManager.GetProperty(PARAM_TM_PHOTOLINEAR_SHUTTERSPEED, shutterspeed); FASSERT(res);
					context.SetParameter("tonemapping.type", RPR_TONEMAPPING_OPERATOR_PHOTOLINEAR);
					context.SetParameter("tonemapping.photolinear.sensitivity", iso * 0.01f);
					context.SetParameter("tonemapping.photolinear.fstop", fstop);
					context.SetParameter("tonemapping.photolinear.exposure", 1.0f / shutterspeed);
				}
				break;
				case frw::ToneMappingTypeNonLinear:
				{
					tonemap = frw::PostEffect(context, frw::PostEffectTypeToneMap);
					context.Attach(tonemap);

					gamma_correction = frw::PostEffect(context, frw::PostEffectTypeGammaCorrection);
					context.Attach(gamma_correction);

					float reinhard02Prescale, reinhard02Postscale, reinhard02Burn;
					res = TmManagerMax::TheManager.GetProperty(PARAM_TM_REINHARD_PRESCALE, reinhard02Prescale); FASSERT(res);
					res = TmManagerMax::TheManager.GetProperty(PARAM_TM_REINHARD_POSTSCALE, reinhard02Postscale); FASSERT(res);
					res = TmManagerMax::TheManager.GetProperty(PARAM_TM_REINHARD_BURN, reinhard02Burn); FASSERT(res);
					context.SetParameter("tonemapping.type", RPR_TONEMAPPING_OPERATOR_REINHARD02);
					context.SetParameter("tonemapping.reinhard02.prescale", reinhard02Prescale);
					context.SetParameter("tonemapping.reinhard02.postscale", reinhard02Postscale);
					context.SetParameter("tonemapping.reinhard02.burn", reinhard02Burn);
				}
				break;
			}
		}
	}	

	int lightCount = scene.LightCount();
	int objectCount = 0;
	const auto& objects = scene.GetShapes();
	int faceCount = 0;
	for (auto it : objects)
	{
		if (auto mtl = it.GetShader())
		{
			if (mtl.GetShaderType() == frw::ShaderTypeEmissive)
				lightCount++;
		}

		objectCount++;

		faceCount += it.GetFaceCount();
	}

	if (parameters.progress)
		parameters.progress->SetSceneStats(lightCount, lightCount, 0, objectCount, faceCount);

	// warn if no lights or non-photometric lights
	if (!GetCOREInterface()->GetQuietMode())
	{
		if (!data->isToneOperatorPreviewRender && (parser->flags.usesNonPMLights && lightCount > 0 && parser.get()->parsed.defaultLightCount == 0))
		{
			pRenderer->warningDlg->show(_T("This light type is not supported in RPR, please use Photometric lights."), "LightTypeNotSupportedNotification");
			EnableWindow(pRenderer->warningDlg->warningWindow, TRUE);
		}
		if (!data->isToneOperatorPreviewRender && parser->hasDirectDisplacements)
		{
			pRenderer->warningDlg->show(_T("Using RPR Displacement to input a displacement map will provide you with better control."), "DisplacementNotification");
			EnableWindow(pRenderer->warningDlg->warningWindow, TRUE);
		}
	}

#ifdef SWITCH_AXES
	res = context.SetParameter("xflip", 1);
	FCHECK(res);
	res = context.SetParameter("yflip", 1);
	FCHECK(res);
#else
	context.SetParameter("xflip", 0);
	context.SetParameter("yflip", 1);
#endif

	auto camera = context.CreateCamera(); // Create a camera - we can do it from scratch for every frame as it does nto leak memory
	scene.SetCamera(camera);

	parser->SynchronizeView(true);

	SetupCamera(parser->view, renderWidth, renderHeight, camera.Handle());

	// synchronize scene
	parser->Synchronize(true);


	// during exporting proccess - "fake" rendering is called to set up the rpr context
	// ExportState::IsEnable is set up to ture if scene is exported and skip actual rendring or false if it is usual rendering 
	if (exportState.IsEnable)
	{

		int extra_3dsmax_gammaenabled = 0;

		const char* extra_int_names[] =
		{
			"3dsmax.gammaenabled",
			"3dsmax.tonemapping.isnormals",
			"3dsmax.tonemapping.shouldtonemap",
		};

		const int extra_int_values[] =
		{
			gammaMgr.enable ? 1 : 0,
			data->isNormals ? 1 : 0,
			data->shouldToneMap ? 1 : 0
		};

		const char* extra_float_names[] =
		{
			"3dsmax.displaygamma",
			"3dsmax.fileingamma",
			"3dsmax.fileoutgamma",
			"3dsmax.tonemapping.exposure",
		};

		const float extra_float_values[] =
		{
			gammaMgr.dispGamma,
			gammaMgr.fileInGamma,
			gammaMgr.fileOutGamma,
			data->toneMappingExposure
		};

		// remain file name if current frame is exported 
		// add num frame ending to file if multiple frames are exported
		std::string exportFilename = ws2s((exportState.IsExportCurrentFrame) ? exportState.FileName : makeExportFileName(exportState.FileName, t / GetTicksPerFrame()));

		frw::Scope scope = ScopeManagerMax::TheManager.GetScope(data->scopeId);
		rpr_context context = scope.GetContext().Handle();
		rprx_context contextEx = scope.GetContextEx();
		rpr_material_system matSystem = scope.GetMaterialSystem().Handle();
		rpr_scene scene = scope.GetScene().Handle();
		std::vector<rpr_scene> scenes{ scene };

		std::string ext;
		int extStart = (int)exportFilename.rfind(L'.');

		if (extStart != std::string::npos)
		{
			ext = exportFilename.substr(extStart + 1);
		}
		else
		{
			return FALSE;
		}

		std::transform(ext.begin(), ext.end(), ext.begin(), std::tolower);
		
		
		
		bool exportOk = true;

		if ("gltf" == ext)
		{
#if (RPR_API_VERSION >= 0x010032500)
			int statusExport = rprExportToGLTF(exportFilename.c_str(), context, matSystem, contextEx, &scenes[0], scenes.size(), 0);
#else
			int statusExport = rprExportToGLTF(exportFilename.c_str(), context, matSystem, contextEx, &scenes[0], scenes.size());
#endif
			exportOk = statusExport == GLTF_SUCCESS;
		}
		else if ("rpr" == ext)
		{
#if (RPR_API_VERSION >= 0x010032400)

			unsigned int exportFlags = (exportState.IsUseExternalFiles) ? RPRLOADSTORE_EXPORTFLAG_EXTERNALFILES : 0;
			rpr_int statusExport = rprsExport(exportFilename.c_str(), context, scene, 0, 0, 0, 0, 0, 0, exportFlags);
#else
			rpr_int statusExport = rprsExport(exportFilename.c_str(), context, scene, 0, 0, 0, 0, 0, 0);
#endif
			exportOk = statusExport == RPR_SUCCESS;
		}

		if (!exportOk)
			MessageBox(GetCOREInterface()->GetMAXHWnd(), _T("Failed to export scene"), _T("Radeon ProRender warning"), MB_OK);

		data->bRenderThreadDone = true;
		return returnValue;
	}

	bool isShadowCatcherEnabled = scope.GetShadowCatcherShader().IsValid();
	if (isShadowCatcherEnabled)
		bRenderAlpha = true;

	// setup ProductionRenderCore data
	data->renderThread->term = data->termCriteria;
	data->renderThread->passLimit = data->passLimit;
	data->renderThread->timeLimit = data->timeLimit;
	data->renderThread->isNormals = data->isNormals;
	data->renderThread->isToneOperatorPreviewRender = data->isToneOperatorPreviewRender;
	data->renderThread->doRenderStamp = GetFromPb<bool>(parameters.pblock, PARAM_STAMP_ENABLED);
	parameters.pblock->GetValue(PARAM_STAMP_TEXT, 0, data->renderThread->timeStampString, FOREVER);
	data->renderThread->renderDevice = GetFromPb<int>(parameters.pblock, PARAM_RENDER_DEVICE);
	data->renderThread->exposure = data->toneMappingExposure;
	data->renderThread->useMaxTonemapper = isShadowCatcherEnabled ? false : !overrideTonemappers;
	data->renderThread->regionMode = (parameters.rendParams.rendType == RENDTYPE_REGION);
	data->isAlphaEnabled = bool_cast(bRenderAlpha);
	data->isDenoiserEnabled = bDenoiserEnabled;
	data->isAdaptiveEnabled = bAdaptiveEnabled;

	if (data->renderThread->regionMode)
	{
		int xmin = parameters.frameRendParams.regxmin;
		int xmax = parameters.frameRendParams.regxmax;
		int ymin = parameters.frameRendParams.regymin;
		int ymax = parameters.frameRendParams.regymax;
		if ((xmax - xmin > 1) && (ymax - ymin > 1))
		{
			Box2 region;
			region.SetXY(xmin, ymin);
			region.SetWH(xmax - xmin, ymax - ymin);
			data->renderThread->region = region;
		}
	}

	// denoiser setup procedure uses frame buffers created in ProductionRenderCore
	if (bDenoiserEnabled)
	{
		SetupDenoiser(context, data, renderWidth, renderHeight, denoiserType, parameters.pblock);
		data->renderThread->SetDenoiser(data->mDenoiser.get());
	}

	// bmDone event will be fired when renderThread finished his job
	PRManagerMaxDone.Reset();

	// Start rendering
	data->renderThread->Start();

	data->bRenderCancelled = false;
	data->bRenderThreadDone = false;
	data->bQuitHelperThread = false;

	data->helperThread = new std::thread([data, frontBuffer]()
	{
		while (!data->bQuitHelperThread)
		{
			if (!data->bitmapUpdated) // bitmap is not refreshed yet => skip (to prevent tearing)
			{
				// - do the copy
				bool isBitmapUpdated = data->renderThread->CopyFrameDataToBitmap(frontBuffer);

				// - Blit frontBuffer to render window
				data->bitmapUpdated = isBitmapUpdated; // <= have to do the bitmap refresh in main Max thread; otherwise Max sdk can crash
			}

			Sleep(30);
		}
	});

	std::wstring prevProgressString;
	HWND shaderCacheDlg = NULL;

	while (true)
	{
		MSG msg;
		
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		// Check for render cancel from user, update progress message
		if ( GetCOREInterface()->CheckForRenderAbort() )
		{
			if (shaderCacheDlg)
			{
				DestroyWindow(shaderCacheDlg);
				shaderCacheDlg = NULL;
			}

			data->renderThread->AbortImmediate();
			data->bQuitHelperThread = true;
			data->bRenderCancelled = true;
			
			break;
		}
		else
		{
			if (parameters.progress)
			{
				bool doCancel = false;
				if( (data->termCriteria == Termination_Passes) || (data->termCriteria == Termination_PassesOrTime) )
					doCancel = (parameters.progress->Progress(data->renderThread->passesDone, data->passLimit) != RENDPROG_CONTINUE);
				else if( data->termCriteria == Termination_Time )
					doCancel = (parameters.progress->Progress(data->renderThread->timePassed * 1000, data->timeLimit * 1000) != RENDPROG_CONTINUE);
				else
					doCancel = (parameters.progress->Progress(0, 100) != RENDPROG_CONTINUE);

				// Update progress message
				std::wstringstream ss;

				if (data->renderThread->passesDone == 0 && ScopeManagerMax::TheManager.GetCompiledShadersCount() < 12)
				{
					if (!shaderCacheDlg)
					{
						if (parameters.progress)
							parameters.progress->SetTitle(_T("Rebuilding shader cache..."));
						
						// Create dialog
						shaderCacheDlg = CreateDialog(FireRender::fireRenderHInstance, MAKEINTRESOURCE(IDD_SHADER_CACHE), GetCOREInterface()->GetMAXHWnd(), NULL);
						ShowWindow(shaderCacheDlg, SW_SHOW);

						// Center it's position
						RECT maxRect;
						GetWindowRect(GetCOREInterface()->GetMAXHWnd(), &maxRect);
						
						RECT dialogRect;
						GetWindowRect(shaderCacheDlg, &dialogRect);

						int dlgWidth = (dialogRect.right - dialogRect.left);
						int dlgHeight = (dialogRect.bottom - dialogRect.top);
						LONG left = (maxRect.left + maxRect.right) * 0.5f - dlgWidth * 0.5;
						LONG top = (maxRect.bottom + maxRect.top) * 0.5f - dlgHeight * 0.5;

						SetWindowPos(shaderCacheDlg, NULL, left, top, dlgWidth, dlgHeight, 0);
					}
				}
				else
				{
					if (shaderCacheDlg)
					{
						DestroyWindow(shaderCacheDlg);
						shaderCacheDlg = NULL;
					}

					ss << _T("Rendering (pass ") << data->renderThread->passesDone;
					if( (data->termCriteria == Termination_Passes) || (data->termCriteria == Termination_PassesOrTime) )
						ss << _T(" of ") << data->passLimit;
					ss << _T(", elapsed: ") << (int)data->renderThread->timePassed << _T("s)...");
				}


				// avoid blitting the same thing all the time
				std::wstring  curProgressString = ss.str();
				if (curProgressString != prevProgressString)
				{
					prevProgressString = curProgressString;
					parameters.progress->SetTitle(curProgressString.c_str());
				}

				if (doCancel)
					data->renderThread->Abort();
			}
		}

		// Render thread finished
		data->bRenderThreadDone |= PRManagerMaxDone.Wait(0);

		// Render thread finished, but wait till all frameData got blitted
		if (data->bRenderThreadDone && !data->bitmapUpdated)
		{
			data->bQuitHelperThread = true;
			break;
		}

		// - Blit frontBuffer to render window
		if (data->bitmapUpdated) // if bitmap is not copied yet => skip refresh (don't want to pause interface)
		{
			frontBuffer->RefreshWindow(); // <= have to do the bitmap refresh in main Max thread; otherwise Max sdk can crash
			data->bitmapUpdated = false;
		}

		//Don't hog too much with CPU
		Sleep(30);
	}

	// handle result of rendering operation (ok, aborted, fatal error)
	if (data->renderThread->result != ProductionRenderCore::Result_OK)
		returnValue = 0;

	if (data->renderThread->errorCode != RPR_SUCCESS)
	{
		std::wstring errorMsg = GetRPRErrorString(data->renderThread->errorCode);
		
		MessageBox(GetCOREInterface()->GetMAXHWnd(), errorMsg.c_str(), L"Radeon ProRender", MB_OK | MB_ICONERROR);
	}

	if (!data->bRenderCancelled)
	{
		// commit render elements
		if ((data->renderThread->errorCode == RPR_SUCCESS) && (data->renderThread->result != ProductionRenderCore::Result_Catastrophic))
		{
			PostProcessAOVs(parameters, scope);
		}

		scope.DestroyFrameBuffers();
	}

	CleanUpRender(pRenderer);

	BroadcastNotification(NOTIFY_POST_RENDERFRAME, &parameters.renderGlobalContext);
	
	return returnValue;
}

void PRManagerMax::PostProcessAOVs(const RenderParameters& parameters, frw::Scope scope) const
{
	IRenderElementMgr* renderElementMgr = parameters.rendParams.GetRenderElementMgr();

	int renderWidth = parameters.rendParams.width;
	int renderHeight = parameters.rendParams.height;

	if (renderElementMgr)
	{
		int renderElementCount = renderElementMgr->NumRenderElements();

		for (int i = 0; i < renderElementCount; ++i)
		{
			IRenderElement* renderElement = renderElementMgr->GetRenderElement(i);
			rpr_aov getAOVRenderElementFRId(IRenderElement* renderElement);
			rpr_aov aov = getAOVRenderElementFRId(renderElement);

			if (RPR_AOV_MAX != aov)
			{
				frw::FrameBuffer fb = scope.GetFrameBuffer(GetFramebufferTypeIdForAOV(aov));

				if (fb.Handle())
				{
					frw::FrameBuffer fbResolve = scope.GetFrameBuffer(renderWidth, renderHeight, GetFramebufferTypeIdForAOVResolve(aov));

					if( GetFramebufferTypeIdForAOVResolve(aov) == FrameBufferTypeId_VarianceResolve )
						fb.Resolve(fbResolve, true); // Special case: Variance needs a "normalize only" resolve
					else // Normal case
						fb.Resolve(fbResolve);

					std::vector<float> colorData = fbResolve.GetPixelData();

					if (RPR_AOV_DEPTH == aov)
					{
						PostProcessDepth(colorData, renderWidth, renderHeight);
					}

					MSTR name = renderElement->GetName();

					PBBitmap* b = nullptr;

					renderElement->GetPBBitmap(b);

					if (b && b->bm)
					{
						CopyDataToBitmap(colorData, std::vector<float>(), b->bm, 1.0f, false);
						b->bm->RefreshWindow();
					}
				}
			}
		}
	}
}

std::tuple<float, float> PRManagerMax::SearchMinMax(const std::vector<float>& data, int width, int height) const
{
	float min = std::numeric_limits<float>::max();
	float max = std::numeric_limits<float>::lowest();

	// Search Min and Max values automatically
	for (int y = 0; y < height; y++)
	{
		for (int x = 0; x < width; x++)
		{
			int index = (y * width + x) * FbComponentsNumber;
			float val = data[index];
			
			if (!isinf<float>(val))
			{
				if (min > val)
				{
					min = val;
				}

				if (max < val)
				{
					max = val;
				}
			}
		}
	}

	return std::make_tuple(min, max);
}

void PRManagerMax::PostProcessDepth(std::vector<float>& data, int width, int height) const
{
	float min = 0.0f;
	float max = 1.0f;

	std::tie(min, max) = SearchMinMax(data, width, height);

	for (int y = 0; y < height; y++)
	{
		for (int x = 0; x < width; x++)
		{
			int index = (y * width + x) * FbComponentsNumber;
			float val = data[index];

			if (!isinf<float>(val))
			{
				if (val >= max)
				{
					val = 1.0f;
				}
				else if (val <= min)
				{
					val = 0.0f;
				}
				else
				{
					val = (val - min) / (max - min);
				}
			}
			else
			{
				val = 1.0f;
			}

			data[index] = data[index+1] = data[index+2] = val;
		}
	}
}

std::tuple<bool, DenoiserType> PRManagerMax::IsDenoiserEnabled(IParamBlock2* pb) const
{
	BOOL isDenoiserEnabled = false;
	int denoiserType = DenoiserNone;

	BOOL res = pb->GetValue(PARAM_DENOISER_ENABLED, 0, isDenoiserEnabled, Interval());
	FASSERT(res);

	if (isDenoiserEnabled)
	{
		res = pb->GetValue(PARAM_DENOISER_TYPE, 0, denoiserType, Interval());
		FASSERT(res);

		isDenoiserEnabled = denoiserType != DenoiserNone;
	}

	return std::make_tuple(bool_cast(isDenoiserEnabled), static_cast<DenoiserType>(denoiserType));
}


void PRManagerMax::SetupDenoiser(frw::Context context, PRManagerMax::Data* data, int imageWidth, int imageHeight,
	DenoiserType type, IParamBlock2* pb)
{
	const rpr_framebuffer fbColor = data->renderThread->frameBufferColorResolve.Handle();
	const rpr_framebuffer fbShadingNormal = data->renderThread->frameBufferShadingNormal.Handle();
	const rpr_framebuffer fbDepth = data->renderThread->frameBufferDepth.Handle();
	const rpr_framebuffer fbWorldCoord = data->renderThread->frameBufferWorldCoordinate.Handle();
	const rpr_framebuffer fbObjectId = data->renderThread->frameBufferObjectId.Handle();
	const rpr_framebuffer fbDiffuseAlbedo = data->renderThread->frameBufferDiffuseAlbedoResolve.Handle();
	const rpr_framebuffer fbTrans = fbObjectId;

	data->mDenoiser.reset( new ImageFilter(context.Handle(), imageWidth, imageHeight, mMlModelPath.c_str()));
	ImageFilter& filter = *data->mDenoiser;

	if (DenoiserBilateral == type)
	{
		filter.CreateFilter(RifFilterType::BilateralDenoise);
		filter.AddInput(RifColor, fbColor, 0.3f);
		filter.AddInput(RifNormal, fbShadingNormal, 0.01f);
		filter.AddInput(RifWorldCoordinate, fbWorldCoord, 0.01f);

		int bilateralRadius = 0;
		BOOL res = pb->GetValue(PARAM_DENOISER_BILATERAL_RADIUS, 0, bilateralRadius, FOREVER);
		FASSERT(res);

		RifParam p = { RifParamType::RifInt, bilateralRadius };
		filter.AddParam("radius", p);
	}
	else if (DenoiserLwr == type)
	{
		filter.CreateFilter(RifFilterType::LwrDenoise);
		filter.AddInput(RifColor, fbColor, 0.1f);
		filter.AddInput(RifNormal, fbShadingNormal, 0.1f);
		filter.AddInput(RifDepth, fbDepth, 0.1f);
		filter.AddInput(RifWorldCoordinate, fbWorldCoord, 0.1f);
		filter.AddInput(RifObjectId, fbObjectId, 0.1f);
		filter.AddInput(RifTrans, fbTrans, 0.1f);

		RifParam p;

		int lwrSamples = 0;
		BOOL res = pb->GetValue(PARAM_DENOISER_LWR_SAMPLES, 0, lwrSamples, FOREVER);
		FASSERT(res);

		p = { RifParamType::RifInt, lwrSamples };
		filter.AddParam("samples", p);

		int lwrRadius = 0;
		res = pb->GetValue(PARAM_DENOISER_LWR_RADIUS, 0, lwrRadius, FOREVER);
		FASSERT(res);

		p = { RifParamType::RifInt, lwrRadius };
		filter.AddParam("halfWindow", p);

		float lwrBandwidth = 0;
		res = pb->GetValue(PARAM_DENOISER_LWR_BANDWIDTH, 0, lwrBandwidth, FOREVER);
		FASSERT(res);

		p.mType = RifParamType::RifFloat;
		p.mData.f = lwrBandwidth;
		filter.AddParam("bandwidth", p);
	}
	else if (DenoiserEaw == type)
	{
		filter.CreateFilter(RifFilterType::EawDenoise);

		float sigma = 0.0f;
		BOOL res = pb->GetValue(PARAM_DENOISER_EAW_COLOR, 0, sigma, FOREVER);
		FASSERT(res);
		filter.AddInput(RifColor, fbColor, sigma);
	
		res = pb->GetValue(PARAM_DENOISER_EAW_NORMAL, 0, sigma, FOREVER);
		FASSERT(res);
		filter.AddInput(RifNormal, fbShadingNormal, sigma);

		res = pb->GetValue(PARAM_DENOISER_EAW_DEPTH, 0, sigma, FOREVER);
		FASSERT(res);
		filter.AddInput(RifDepth, fbDepth, sigma);

		res = pb->GetValue(PARAM_DENOISER_EAW_OBJECTID, 0, sigma, FOREVER);
		FASSERT(res);
		filter.AddInput(RifTrans, fbTrans, sigma);

		filter.AddInput(RifWorldCoordinate, fbWorldCoord, 0.1f);
		filter.AddInput(RifObjectId, fbObjectId, 0.1f);
	}
	else if (DenoiserMl == type)
	{
		filter.CreateFilter(RifFilterType::MlDenoise);
		filter.AddInput(RifColor, fbColor, 0.0f);
		filter.AddInput(RifNormal, fbShadingNormal, 0.0f);
		filter.AddInput(RifDepth, fbDepth, 0.0f);
		filter.AddInput(RifAlbedo, fbDiffuseAlbedo, 0.0f);
	}


	filter.AttachFilter();
}

void PRManagerMax::Abort(FireRenderer *pRenderer)
{
	auto dd = mInstances.find(static_cast<FireRenderer*>(pRenderer));
	if (dd != mInstances.end())
	{
		auto data = dd->second;

		if (data->renderThread)
		{
			data->bRenderCancelled = true;
			data->renderThread->Abort();
		}
	}	
}

void PRManagerMax::EnableExportState(bool state)
{
	exportState.IsEnable = state;
}

void PRManagerMax::EnableUseExternalFiles(bool state)
{
	exportState.IsUseExternalFiles = state;
}

bool PRManagerMax::GetIsExportCurrentFrame()
{
	return exportState.IsExportCurrentFrame;
}

void PRManagerMax::SetIsExportCurrentFrame(bool state)
{
	exportState.IsExportCurrentFrame = state;
}

void PRManagerMax::SetExportName(const std::wstring & fileName)
{
	exportState.FileName = fileName;
}

RefResult PRManagerMax::NotifyRefChanged(NOTIFY_REF_CHANGED_PARAMETERS)
{
	return REF_DONTCARE;
}

void PRManagerMax::SetupCamera(const ParsedView& view, const int imageWidth, const int imageHeight, rpr_camera outCamera)
{
	int res;
	Point3 camOriginOffset = Point3(0.f, 0.f, 0.f); // used to shift ortho camera origin so it can behave as "in infinity"

	res = rprObjectSetName(outCamera, view.cameraNodeName.c_str());
	FCHECK(res);

	switch (view.projection) {
	case P_PERSPECTIVE:
	{
		res = rprCameraSetMode(outCamera, RPR_CAMERA_MODE_PERSPECTIVE);
		FCHECK(res);

		res = rprCameraSetFocalLength(outCamera, view.focalLength);
		FCHECK(res);

		res = rprCameraSetSensorSize(outCamera, view.sensorWidth, view.sensorWidth / float(imageWidth) * float(imageHeight));
		FCHECK(res);

		res = rprCameraSetTiltCorrection(outCamera, -view.tilt.x, -view.tilt.y);
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

	// Create the 3 principal points from the transformation matrix
	Matrix3 tm = FlipAxes(view.tm);
	Point3 ORIGIN = tm.PointTransform(camOriginOffset);
	Point3 TARGET = tm.PointTransform(camOriginOffset + Point3(0.f, 0.f, -1.f));
	Point3 ROLL = tm.VectorTransform(Point3(0.f, 1.f, 0.f));

	res = rprCameraLookAt(outCamera, ORIGIN.x, ORIGIN.y, ORIGIN.z, TARGET.x, TARGET.y, TARGET.z, ROLL.x, ROLL.y, ROLL.z);
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

FramebufferTypeId PRManagerMax::GetFramebufferTypeIdForAOV(rpr_aov aov) const
{
	static const std::unordered_map<rpr_aov, FramebufferTypeId> fbToAov =
	{
		{ RPR_AOV_OPACITY,                   FramebufferTypeId_Alpha },
		{ RPR_AOV_WORLD_COORDINATE,          FramebufferTypeId_WorldCoordinate },
		{ RPR_AOV_UV,                        FramebufferTypeId_UV },
		{ RPR_AOV_MATERIAL_IDX,              FramebufferTypeId_MaterialIdx },
		{ RPR_AOV_GEOMETRIC_NORMAL,          FramebufferTypeId_GeometricNormal },
		{ RPR_AOV_SHADING_NORMAL,            FramebufferTypeId_ShadingNormal },
		{ RPR_AOV_DEPTH,                     FramebufferTypeId_Depth },
		{ RPR_AOV_OBJECT_ID,                 FramebufferTypeId_ObjectId },
		{ RPR_AOV_OBJECT_GROUP_ID,           FrameBufferTypeId_GroupId },
		{ RPR_AOV_BACKGROUND,                FrameBufferTypeId_Background },
		{ RPR_AOV_EMISSION,                  FrameBufferTypeId_Emission },
		{ RPR_AOV_VELOCITY,                  FrameBufferTypeId_Velocity },
		{ RPR_AOV_DIRECT_ILLUMINATION,       FrameBufferTypeId_DirectIllumination },
		{ RPR_AOV_INDIRECT_ILLUMINATION,     FrameBufferTypeId_IndirectIllumination },
		{ RPR_AOV_AO,                        FrameBufferTypeId_AO },
		{ RPR_AOV_DIRECT_DIFFUSE,            FrameBufferTypeId_DirectDiffuse },
		{ RPR_AOV_DIRECT_REFLECT,            FrameBufferTypeId_DirectReflect },
		{ RPR_AOV_INDIRECT_DIFFUSE,          FrameBufferTypeId_IndirectDiffuse },
		{ RPR_AOV_INDIRECT_REFLECT,          FrameBufferTypeId_IndirectReflect },
		{ RPR_AOV_REFRACT,                   FrameBufferTypeId_DirectRefract },
		{ RPR_AOV_VOLUME,                    FrameBufferTypeId_Volume },
		{ RPR_AOV_VARIANCE,                  FrameBufferTypeId_Variance }
	};

	auto fbToAovIt = fbToAov.find(aov);

	FASSERT( fbToAovIt != fbToAov.end() );

	if ( fbToAovIt != fbToAov.end() )
		return fbToAovIt->second;

	return FramebufferTypeId_Color;
}

FramebufferTypeId PRManagerMax::GetFramebufferTypeIdForAOVResolve(rpr_aov aov) const
{
	static const std::unordered_map<rpr_aov, FramebufferTypeId> fbToAov =
	{
		{ RPR_AOV_OPACITY,                   FramebufferTypeId_AlphaResolve },
		{ RPR_AOV_WORLD_COORDINATE,          FramebufferTypeId_WorldCoordinateResolve },
		{ RPR_AOV_UV,                        FramebufferTypeId_UVResolve },
		{ RPR_AOV_MATERIAL_IDX,              FramebufferTypeId_MaterialIdxResolve },
		{ RPR_AOV_GEOMETRIC_NORMAL,          FramebufferTypeId_GeometricNormalResolve },
		{ RPR_AOV_SHADING_NORMAL,            FramebufferTypeId_ShadingNormalResolve },
		{ RPR_AOV_DEPTH,                     FramebufferTypeId_DepthResolve },
		{ RPR_AOV_OBJECT_ID,                 FramebufferTypeId_ObjectIdResolve },
		{ RPR_AOV_OBJECT_GROUP_ID,           FrameBufferTypeId_GroupIdResolve },
		{ RPR_AOV_BACKGROUND,                FrameBufferTypeId_BackgroundResolve },
		{ RPR_AOV_EMISSION,                  FrameBufferTypeId_EmissionResolve },
		{ RPR_AOV_VELOCITY,                  FrameBufferTypeId_VelocityResolve },
		{ RPR_AOV_DIRECT_ILLUMINATION,       FrameBufferTypeId_DirectIlluminationResolve },
		{ RPR_AOV_INDIRECT_ILLUMINATION,     FrameBufferTypeId_IndirectIlluminationResolve },
		{ RPR_AOV_AO,                        FrameBufferTypeId_AOResolve },
		{ RPR_AOV_DIRECT_DIFFUSE,            FrameBufferTypeId_DirectDiffuseResolve },
		{ RPR_AOV_DIRECT_REFLECT,            FrameBufferTypeId_DirectReflectResolve },
		{ RPR_AOV_INDIRECT_DIFFUSE,          FrameBufferTypeId_IndirectDiffuseResolve },
		{ RPR_AOV_INDIRECT_REFLECT,          FrameBufferTypeId_IndirectReflectResolve },
		{ RPR_AOV_REFRACT,                   FrameBufferTypeId_DirectRefractResolve },
		{ RPR_AOV_VOLUME,                    FrameBufferTypeId_VolumeResolve },
		{ RPR_AOV_VARIANCE,                  FrameBufferTypeId_VarianceResolve }
	};

	auto fbToAovIt = fbToAov.find(aov);

	FASSERT(fbToAovIt != fbToAov.end());

	if (fbToAovIt != fbToAov.end())
		return fbToAovIt->second;

	return FramebufferTypeId_Color;
}

#define HELP(str1, str2) _T(str1) _T("\t") _T(str2) _T("\n")

const MCHAR* PRManagerMax::GetStampHelp()
{
	return _T("Special symbols:\n\n")
		HELP("%pt", "total elapsed time")
		HELP("%pp", "passes")
		HELP("%sl", "number of lights")
		HELP("%so", "number of objects")
		HELP("%i",  "computer name")
		HELP("%c", "CPU name")
		HELP("%g", "used GPU name")
		HELP("%h", "used hardware (mix of %c and %g)")
		HELP("%r", "render device type (CPU, GPU, both)")
		HELP("%d", "current date and time")
		HELP("%b", "Radeon ProRenderer version number");
}

FIRERENDER_NAMESPACE_END
