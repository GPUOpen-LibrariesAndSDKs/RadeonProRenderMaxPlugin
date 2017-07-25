/*********************************************************************************************************************************
* Radeon ProRender for 3ds Max plugin
* Copyright (c) 2017 AMD
* All Rights Reserved
*
* Production Renderer
*********************************************************************************************************************************/

#include <math.h>
#include "PRManager.h"
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
#include "plugin/CamManager.h"
#include "plugin/TMManager.h"
#include <RadeonProRender.h>
#include <RprLoadStore.h>
#include <RprSupport.h>
#include <shlobj.h>
#include <gamma.h> // gamma export for FRS files
#include <mutex>
#include <future>
extern HINSTANCE hInstance;

FIRERENDER_NAMESPACE_BEGIN;

#define BMUI_TIMER_PERIOD 33

Event PRManagerMax::bmDone;

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

	std::future<void> copyResult; // for async: If the std::future obtained from std::async is not moved from or bound to a reference, 
								  // the destructor of the std::future will block at the end of the full expression until the asynchronous operation completes,
								  // essentially making code synchronous

	std::mutex frameDataBuffersLock;

	std::mutex rprBuffersLock;

private:
	frw::Scope scope;
	frw::FrameBuffer frameBufferColor;
	frw::FrameBuffer frameBufferColorResolve;
	frw::FrameBuffer frameBufferAlpha;
	frw::FrameBuffer frameBufferAlphaResolve;
	frw::FrameBuffer frameBufferShadowCatcher;
	frw::FrameBuffer frameBufferShadowCatcherResolve;
	Event eRestart;

	void SaveFrameData (void);
	rpr_int GetDataFromBuffer (std::vector<float> &data, const fr_framebuffer& frameBuffer);
	void RPRCopyFrameData (void);
	

public:
	bool CopyFrameDataToBitmap(::Bitmap* bitmap);
	void RenderStamp(Bitmap* DstBuffer, std::unique_ptr<ProductionRenderCore::FrameDataBuffer>& frameData) const;

	explicit ProductionRenderCore(frw::Scope rscope, bool bRenderAlpha, int width, int height, int priority = THREAD_PRIORITY_NORMAL, const char* name = "ProductionRenderCore");
	
	void Worker() override;

	void Done(TerminationResult res, rpr_int err)
	{
		errorCode = err;
		rpr_int errorCode = RPR_SUCCESS;
		result = res;
		PRManagerMax::bmDone.Fire();

		if(bImmediateAbort)
			scope.DestroyFrameBuffers();
	}

	void Abort() override
	{
		BaseThread::Abort();
	}

	void AbortImmediate() override
	{
		BaseThread::AbortImmediate();
		bImmediateAbort = true;
	}

	void Restart();
};

rpr_int ProductionRenderCore::GetDataFromBuffer(std::vector<float> &data, const fr_framebuffer& frameBuffer)
{
	size_t dataSize;
	rpr_int res = rprFrameBufferGetInfo(frameBuffer
		, RPR_FRAMEBUFFER_DATA
		, 0
		, NULL
		, &dataSize
	);
	FASSERT(res == RPR_SUCCESS);

	data.resize(dataSize / sizeof(float));
	res = rprFrameBufferGetInfo(frameBuffer
		, RPR_FRAMEBUFFER_DATA
		, data.size() * sizeof(float)
		, data.data()
		, NULL
	);
	FASSERT(res == RPR_SUCCESS);

	return res;
}

void ProductionRenderCore::RPRCopyFrameData()
{
	rprBuffersLock.lock();

	// get data from RPR and put it to back buffer
	std::unique_ptr<ProductionRenderCore::FrameDataBuffer> tmpFrameData = std::make_unique<ProductionRenderCore::FrameDataBuffer>();

	// get rgb
	rpr_int res;
	res = GetDataFromBuffer(tmpFrameData->colorData, frameBufferColorResolve.Handle());
	FASSERT(res == RPR_SUCCESS);
	frameBufferColorResolve.Clear();

	// get alpha
	if (frameBufferAlpha)
	{
		res = GetDataFromBuffer(tmpFrameData->alphaData, frameBufferAlphaResolve.Handle());
		FASSERT(res == RPR_SUCCESS);
		frameBufferAlphaResolve.Clear();
	}

	// - Save additional frame data
	tmpFrameData->timePassed = timePassed;
	tmpFrameData->passesDone = passesDone;

	rprBuffersLock.unlock();

	// - Swap new frame with the old one in the buffer
	frameDataBuffersLock.lock();
	pLastFrameData = std::move(tmpFrameData);
	frameDataBuffersLock.unlock();
}


void ProductionRenderCore::SaveFrameData()
{
	rprBuffersLock.lock();

	// - Resolve & save Color buffer
	frameBufferColor.Resolve(frameBufferColorResolve);

	// - Resolve & save Alpha buffer
	if (frameBufferAlpha)
	{
		frameBufferAlpha.Resolve(frameBufferAlphaResolve);
	}

	rprBuffersLock.unlock();

	copyResult = std::async(std::launch::async, &ProductionRenderCore::RPRCopyFrameData, this);
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

ProductionRenderCore::ProductionRenderCore(frw::Scope rscope, bool bRenderAlpha, int width, int height, int priority, const char* name)
: BaseThread(name, priority), scope(rscope), eRestart(true), bImmediateAbort(false)
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

	mSelfDelete = false;

	frw::Context ctx = scope.GetContext();

	// Set color buffer
	frameBufferColor = scope.GetFrameBuffer(width, height, FramebufferTypeId_Color);
	frameBufferColorResolve = scope.GetFrameBuffer(width, height, FramebufferTypeId_ColorResolve);
	ctx.SetAOV(frameBufferColor, RPR_AOV_COLOR);

	// Set shadow buffer
	frameBufferShadowCatcher = scope.GetFrameBuffer(width, height, FrameBufferTypeId_ShadowCatcher);
	frameBufferShadowCatcherResolve = scope.GetFrameBuffer(width, height, FrameBufferTypeId_ShadowCatcherResolve);
	ctx.SetAOV(frameBufferShadowCatcher, RPR_AOV_SHADOW_CATCHER);

	// Set alpha buffer
	if (bRenderAlpha)
	{
		frameBufferAlpha = scope.GetFrameBuffer(width, height, FramebufferTypeId_Alpha);
		frameBufferAlphaResolve = scope.GetFrameBuffer(width, height, FramebufferTypeId_AlphaResolve);
		ctx.SetAOV(frameBufferAlpha, RPR_AOV_OPACITY);
	}
}

void ProductionRenderCore::Worker()
{
	unsigned int numPasses = passLimit;
	if (numPasses < 1)
		numPasses = 1;
	
	passesDone = 0;
	__time64_t startedAt = time(0);

	bool clearFramebuffer = true;

	int xmin = 0, ymin = 0, xmax = 0, ymax = 0;
	if (regionMode)
	{
		if (region.IsEmpty())
			regionMode = false;
		else
		{
			xmin = region.x();
			ymin = region.y();
			xmax = xmin + region.w();
			ymax = ymin + region.h();
		}
	}

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
		if (clearFramebuffer)
		{
			frameBufferColor.Clear();

			if (frameBufferAlpha)
			{
				frameBufferAlpha.Clear();
			}

			frameBufferShadowCatcher.Clear();
			frameBufferShadowCatcherResolve.Clear();

			clearFramebuffer = false;
		}

		// Render
		rpr_int res = RPR_SUCCESS;
		try
		{
			if (regionMode)
				res = rprContextRenderTile(scope.GetContext().Handle(), xmin, xmax, ymin, ymax);
			else
				res = rprContextRender(scope.GetContext().Handle());
		}
		catch (...) // Exception
		{
			debugPrint("Exception occurred in render call");
			Done(Result_Catastrophic, RPR_ERROR_INTERNAL_ERROR);
			return;
		}

		// Render failed
		if (res != RPR_SUCCESS)
		{
			TheManager->Max()->Log()->LogEntry(SYSLOG_WARN, NO_DIALOG, L"Radeon ProRender - Warning", L"rprContextRender returned '%d'\n", res);
			Done(Result_Catastrophic, res);
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
			if (term == Termination_Passes && passesDone == numPasses) // Passes done, terminate
			{
				Done(Result_OK, RPR_SUCCESS);
				return;
			}
			else if (term == Termination_Time && timePassed >= timeLimit) // Time limit reached, terminate
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
	const TCHAR *str2 = timeStampString;

	auto scene = scope.GetScene();

	// parse string
	std::wstring str;
	str.reserve(256);
	while (MCHAR c = *str2++)
	{
		if (c != '%')
		{
			str += c;
			continue;
		}
		// here we have escape sequence
		c = *str2++;
		if (!c)
		{
			str += L'%'; // this was a last character in string
			break;
		}

		static const int uninitNumericValue = 0xDDAABBAA;
		int numericValue = uninitNumericValue;
		switch (c)
		{
		case '%': // %% - add single % character
			str += c;
			break;
		case 'p': // performance
		{
			c = *str2++;
			switch (c)
			{
			case 't': // %pt - total elapsed time
			{
				wchar_t buffer[32];
				unsigned int secs = (int)frameData->timePassed;
				int hrs = secs / (60 * 60);
				secs = secs % (60 * 60);
				int mins = secs / 60;
				secs = secs % 60;
				wsprintf(buffer, _T("%d:%02d:%02d"), hrs, mins, secs);
				str += buffer;
			}
			break;
			case 'p': // %pp - passes
				numericValue = frameData->passesDone;
				break;
			}
		}
		break;
		case 's': // scene information
		{
			c = *str2++;
			switch (c)
			{
			case 'l': // %sl - number of light primitives
				numericValue = scene.LightCount();
				break;
			case 'o': // %so - number of objects
				numericValue = scene.ShapeCount();
				break;
			}
		}
		break;
		case 'c': // CPU name
			str += GetCPUName();
			break;
		case 'g': // GPU name
			str += GetFriendlyUsedGPUName();
			break;
		case 'r': // rendering mode
		{
			if (renderDevice == RPR_RENDERDEVICE_CPUONLY)
				str += _T("CPU");
			else if (renderDevice == RPR_RENDERDEVICE_GPUONLY)
				str += _T("GPU");
			else
				str += _T("CPU/GPU");
		}
		break;
		case 'h': // used hardware
		{
			if (renderDevice == RPR_RENDERDEVICE_CPUONLY)
				str += GetCPUName();
			else if (renderDevice == RPR_RENDERDEVICE_GPUONLY)
				str += GetFriendlyUsedGPUName();
			else
				str += GetCPUName() + _T(" / ") + GetFriendlyUsedGPUName();
		}
		break;
		case 'i': // computer name
		{
			wchar_t buffer[256];
			DWORD size = 256;
			GetComputerName(buffer, &size);
			str += buffer;
		}
		break;
		case 'd': // current date
		{
			char buffer[256];
			time_t itime;
			time(&itime);
			ctime_s(buffer, 256, &itime);
			str += ToUnicode(buffer);
		}
		break;
		case 'b': // build number
		{
			std::wstring name, version;
			GetProductAndVersion(name, version);
			str += version;
		}
		break;
		default:
			// wrong escape sequence, add character
			if (c)
			{
				str += L'%';
				str += c;
			}
		}

		if (!c) break; // could happen when string ends with multi-character escape sequence, like immediately after "%p" etc

		if (numericValue != uninitNumericValue)
		{
			// the value was represented as simple number, add it here
			wchar_t buffer[32];
			wsprintf(buffer, _T("%d"), numericValue);
			str += buffer;
		}
	}

	Bitmap* bmp = RenderTextToBitmap(str.c_str());
	// copy 'bmp' to 'DstBuffer' at bottom-right corner
	int x = DstBuffer->Width() - bmp->Width();
	int y = DstBuffer->Height() - bmp->Height();
	if (x < 0) x = 0;
	if (y < 0) y = 0;
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

void PRManagerMax::CleanUpRender(FireRenderer *pRenderer)
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
				bmDone.Wait();

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
		case RPR_RENDER_LIMIT_PASS:
			data->termCriteria = Termination_Passes;
			data->passLimit = GetFromPb<int>(parameters.pblock, PARAM_PASS_LIMIT);
			break;
		case RPR_RENDER_LIMIT_TIME:
			data->termCriteria = Termination_Time;
			data->timeLimit = GetFromPb<int>(parameters.pblock, PARAM_TIME_LIMIT);
			break;
		case RPR_RENDER_LIMIT_UNLIMITED:
			data->termCriteria = Termination_None;
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
	context.SetParameter("aacellsize", GetFromPb<int>(parameters.pblock, PARAM_AA_GRID_SIZE));
	context.SetParameter("aasamples", GetFromPb<int>(parameters.pblock, PARAM_AA_SAMPLE_COUNT));
	context.SetParameter("pdfthreshold", 0.f);

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

	auto data = dd->second;


	auto &parameters = pRenderer->parameters;

	// Export the model if requested
	if (parameters.pblock && GetFromPb<bool>(parameters.pblock, PARAM_EXPORTMODEL_CHECK))
	{
		auto filename = pRenderer->GetFireRenderExportSceneFilename();
		if (filename.length() == 0)
		{
			TCHAR my_documents[MAX_PATH];
			HRESULT result = SHGetFolderPath(NULL, CSIDL_PERSONAL, NULL, SHGFP_TYPE_CURRENT, my_documents);
			if (result == S_OK)
				filename = std::wstring(my_documents) + L"\\3dsMax\\export\\";
			filename += L"exportscene.rprs";
		}

		int extra_3dsmax_gammaenabled = 0;

		const char* extra_int_names[] = {
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


		const char* extra_float_names[] = {
			"3dsmax.displaygamma",
			"3dsmax.fileingamma",
			"3dsmax.fileoutgamma",
			"3dsmax.tonemapping.exposure",
		};
		const float extra_float_values[] = {
			gammaMgr.dispGamma,
			gammaMgr.fileInGamma,
			gammaMgr.fileOutGamma,
			data->toneMappingExposure
		};

		//check that the path only contains ASCII characters.
		//FYI, frsExport manages only char* because this library is also used in Linux, and it seems that wchar_t is not Linux friendly.
		bool goodAscii = true;
		for (int i = 0; i<filename.size(); i++)
		{
			if (filename[i] > 0x0ff)
			{
				goodAscii = false;
				break;
			}
		}

		if (!goodAscii)
		{
			MessageBoxA(GetCOREInterface()->GetMAXHWnd(), "No Export. The path to FRS export must have ascii characters only.", "Radeon ProRender warning", MB_OK);
		}
		else
		{
			auto scope = ScopeManagerMax::TheManager.GetScope(data->scopeId);
			rpr_int statusExport = rprsExport(ToAscii(filename).c_str(), scope.GetContext().Handle(), scope.GetScene().Handle(),
				sizeof(extra_int_values) / sizeof(extra_int_values[0]),
				extra_int_names,
				extra_int_values,
				sizeof(extra_float_values) / sizeof(extra_float_values[0]),
				extra_float_names,
				extra_float_values
			);

			if (statusExport == RPR_SUCCESS) {
				wchar_t successMessage[2048];
				swprintf_s(successMessage, L"Model successfully exported to : %s", filename.c_str());
				GetCOREInterface()->DisplayTempPrompt(successMessage, 10000);
			}
		}
	}

	ScopeManagerMax::TheManager.DestroyScope(data->scopeId);

	delete data;
	mInstances.erase(dd);

	BroadcastNotification(NOTIFY_POST_RENDER);
}


int PRManagerMax::Render(FireRenderer* pRenderer, TimeValue t, ::Bitmap* frontBuffer, FrameRendParams &frp, HWND hwnd, RendProgressCallback* prog, ViewParams* viewPar)
{
	int returnValue = 1;

	auto dd = mInstances.find(static_cast<FireRenderer*>(pRenderer));
	FASSERT(dd != mInstances.end());

	auto data = dd->second;

	auto &parameters = pRenderer->parameters;

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

	parser->Synchronize(true);
	
	// Do we need alpha channel rendering?
	BOOL bRenderAlpha;
	parameters.pblock->GetValue(TRPARAM_BG_ENABLEALPHA, 0, bRenderAlpha, Interval());

	//Render Elements
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

	auto scene = scope.GetScene(true);
	auto camera = context.CreateCamera(); // Create a camera - we can do it from scratch for every frame as it does nto leak memory
	scene.SetCamera(camera);	

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
	FASSERT(res == RPR_SUCCESS);
	res = context.SetParameter("yflip", 1);
	FASSERT(res == RPR_SUCCESS);
#else
	context.SetParameter("xflip", 0);
	context.SetParameter("yflip", 1);
#endif

	SetupCamera(parser->view, renderWidth, renderHeight, camera.Handle());
	
	data->renderThread = new ProductionRenderCore(scope, bRenderAlpha, renderWidth, renderHeight);
	data->renderThread->term = data->termCriteria;
	data->renderThread->passLimit = data->passLimit;
	data->renderThread->timeLimit = data->timeLimit;
	data->renderThread->isNormals = data->isNormals;
	data->renderThread->isToneOperatorPreviewRender = data->isToneOperatorPreviewRender;
	data->renderThread->doRenderStamp = GetFromPb<bool>(parameters.pblock, PARAM_STAMP_ENABLED);
	parameters.pblock->GetValue(PARAM_STAMP_TEXT, 0, data->renderThread->timeStampString, FOREVER);
	data->renderThread->renderDevice = GetFromPb<int>(parameters.pblock, PARAM_RENDER_DEVICE);
	data->renderThread->exposure = data->toneMappingExposure;
	data->renderThread->useMaxTonemapper = !overrideTonemappers;
	data->renderThread->regionMode = (parameters.rendParams.rendType == RENDTYPE_REGION);
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

	// bmDone event will be fired when renderThread finished his job
	bmDone.Reset();

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
			bool process = IsWndMessageToSkip(msg);
			// Skip all mouse and keyboard messages for any window except render progress and warningDlg
			if ((msg.hwnd == pRenderer->mRenderWindow) || IsChild(pRenderer->mRenderWindow, msg.hwnd))
				process = true;
			if (!process)
				if ((msg.hwnd == pRenderer->hRenderProgressDlg) || IsChild(pRenderer->hRenderProgressDlg, msg.hwnd))
					process = true;
			if (!process)
				if (pRenderer->warningDlg.get() && ((msg.hwnd == pRenderer->warningDlg->warningWindow) || IsChild(pRenderer->warningDlg->warningWindow, msg.hwnd)))
					process = true;
			if (process)
				DispatchMessage(&msg);
		}

		// Check for render cancel from user, update progress message
		if (GetCOREInterface()->CheckForRenderAbort())
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
				if (data->termCriteria == Termination_Passes)
					doCancel = (parameters.progress->Progress(data->renderThread->passesDone, data->passLimit) != RENDPROG_CONTINUE);
				else if (data->termCriteria == Termination_Time)
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
					if (data->termCriteria == Termination_Passes)
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
		data->bRenderThreadDone |= bmDone.Wait(0);

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
		std::wstring errorMsg;
		switch (data->renderThread->errorCode)
		{
			case RPR_ERROR_OUT_OF_VIDEO_MEMORY: errorMsg = L"Out of video memory error.\nThis scene and its textures requires more memory than is available.\nTo render you will need to upgrade your GPU"; break;
			case RPR_ERROR_OUT_OF_SYSTEM_MEMORY: errorMsg = L"Out of system memory error.\nThis scene and its textures requires more memory than is available.\nTo render you will need to upgrade your hardware"; break;
			case RPR_ERROR_MATERIAL_STACK_OVERFLOW: errorMsg = L"Material Stack Overflow"; break;
			case RPR_ERROR_COMPUTE_API_NOT_SUPPORTED: errorMsg = L"Compute API Not Supported"; break;
			case RPR_ERROR_INVALID_LIGHTPATH_EXPR: errorMsg = L"Invalid LightPath Expression"; break;
			case RPR_ERROR_INVALID_IMAGE: errorMsg = L"Invalid Image"; break;
			case RPR_ERROR_INVALID_AA_METHOD: errorMsg = L"Invalid AA Method"; break;
			case RPR_ERROR_UNSUPPORTED_IMAGE_FORMAT: errorMsg = L"Unsupported Image Format"; break;
			case RPR_ERROR_INVALID_GL_TEXTURE: errorMsg = L"Invalid GL Texture"; break;
			case RPR_ERROR_INVALID_CL_IMAGE: errorMsg = L"Invalid CL Image"; break;
			case RPR_ERROR_INVALID_OBJECT: errorMsg = L"Invalid Object"; break;
			case RPR_ERROR_INVALID_PARAMETER: errorMsg = L"Invalid Parameter"; break;
			case RPR_ERROR_INVALID_TAG: errorMsg = L"Invalid Tag"; break;
			case RPR_ERROR_INVALID_LIGHT: errorMsg = L"Invalid Light"; break;
			case RPR_ERROR_INVALID_CONTEXT: errorMsg = L"Invalid Context"; break;
			case RPR_ERROR_UNIMPLEMENTED: errorMsg = L"Unimplemented"; break;
			case RPR_ERROR_INVALID_API_VERSION: errorMsg = L"Invalid API Version"; break;
			case RPR_ERROR_INTERNAL_ERROR: errorMsg = L"Internal Error"; break;
			case RPR_ERROR_IO_ERROR: errorMsg = L"IO Error"; break;
			case RPR_ERROR_UNSUPPORTED_SHADER_PARAMETER_TYPE: errorMsg = L"Unsupported Shader Parameter Type"; break;
			case RPR_ERROR_INVALID_PARAMETER_TYPE: errorMsg = L"Invalid Parameter Type"; break;
			default:
				errorMsg = L"Unspecified internal error";
				break;
		}
		MessageBox(GetCOREInterface()->GetMAXHWnd(), errorMsg.c_str(), L"Radeon ProRender", MB_OK | MB_ICONERROR);
	}

	if (!data->bRenderCancelled)
	{
		// commit render elements
		if ((data->renderThread->errorCode == RPR_SUCCESS) && (data->renderThread->result != ProductionRenderCore::Result_Catastrophic))
		{
			auto renderElementMgr = parameters.rendParams.GetRenderElementMgr();
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
						auto fb = scope.GetFrameBuffer(GetFramebufferTypeIdForAOV(aov));
						if (fb.Handle())
						{
							auto fbResolve = scope.GetFrameBuffer(renderWidth, renderHeight, GetFramebufferTypeIdForAOVResolve(aov));
							fb.Resolve(fbResolve);

							std::vector<float> colorData = fbResolve.GetPixelData();

							MSTR name = renderElement->GetName();

							PBBitmap *b = nullptr;
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

		scope.DestroyFrameBuffers();
	}

	CleanUpRender(pRenderer);

	BroadcastNotification(NOTIFY_POST_RENDERFRAME, &parameters.renderGlobalContext);
	
	return returnValue;
}

void PRManagerMax::Abort(FireRenderer *pRenderer)
{
	auto dd = mInstances.find(static_cast<FireRenderer*>(pRenderer));
	if (dd != mInstances.end())
	{
		auto data = dd->second;
		if (data->renderThread)
			data->renderThread->Abort();
	}	
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
	FASSERT(res == RPR_SUCCESS);

	switch (view.projection) {
	case P_PERSPECTIVE:
	{
		res = rprCameraSetMode(outCamera, RPR_CAMERA_MODE_PERSPECTIVE);
		FASSERT(res == RPR_SUCCESS);

		res = rprCameraSetFocalLength(outCamera, view.focalLength);
		FASSERT(res == RPR_SUCCESS);

		res = rprCameraSetSensorSize(outCamera, view.sensorWidth, view.sensorWidth / float(imageWidth) * float(imageHeight));
		FASSERT(res == RPR_SUCCESS);

		if (view.useDof) {
			res = rprCameraSetFStop(outCamera, view.fSTop);
			FASSERT(res == RPR_SUCCESS);

			res = rprCameraSetApertureBlades(outCamera, view.bokehBlades);
			FASSERT(res == RPR_SUCCESS);

			res = rprCameraSetFocusDistance(outCamera, view.focusDistance);
			FASSERT(res == RPR_SUCCESS);
		}
		else {
			res = rprCameraSetFStop(outCamera, std::numeric_limits<float>::infinity());
			FASSERT(res == RPR_SUCCESS);
		}

		break;
	}
	case P_ORTHO:
		res = rprCameraSetMode(outCamera, RPR_CAMERA_MODE_ORTHOGRAPHIC);
		FASSERT(res == RPR_SUCCESS);
		res = rprCameraSetOrthoWidth(outCamera, view.orthoSize);
		FASSERT(res == RPR_SUCCESS);
		res = rprCameraSetOrthoHeight(outCamera, view.orthoSize / float(imageWidth) * float(imageHeight));
		FASSERT(res == RPR_SUCCESS);
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
	FASSERT(res == RPR_SUCCESS);

	switch (FRCameraType(view.projectionOverride))
	{
		case FRCameraType_Default:break;
		case FRCameraType_LatitudeLongitude_360:
			rprCameraSetMode(outCamera, RPR_CAMERA_MODE_LATITUDE_LONGITUDE_360);
			break;
		case FRCameraType_LatitudeLongitude_Stereo:
			rprCameraSetMode(outCamera, RPR_CAMERA_MODE_LATITUDE_LONGITUDE_STEREO);
			break;
		case FRCameraType_Cubemap:
			rprCameraSetMode(outCamera, RPR_CAMERA_MODE_CUBEMAP);
			break;
		case FRCameraType_Cubemap_Stereo:
			rprCameraSetMode(outCamera, RPR_CAMERA_MODE_CUBEMAP_STEREO);
			break;
		default:
			FASSERT(!"camera type not supported yet");
	};

	if (view.useMotionBlur)
		rprCameraSetExposure(outCamera, view.cameraExposure);
}

bool PRManagerMax::IsWndMessageToSkip(const MSG &msg)
{
	// for Max menu lock, skip only these messages
	return  msg.message == WM_KEYDOWN || msg.message == WM_KEYUP ||
		msg.message == WM_RBUTTONDOWN || msg.message == WM_RBUTTONUP || msg.message == WM_RBUTTONDBLCLK ||
		msg.message == WM_LBUTTONDOWN || msg.message == WM_LBUTTONUP || msg.message == WM_LBUTTONDBLCLK ||
		msg.message == WM_MBUTTONDOWN || msg.message == WM_MBUTTONUP || msg.message == WM_MBUTTONDBLCLK;
}

FramebufferTypeId PRManagerMax::GetFramebufferTypeIdForAOV(rpr_aov aov)
{
	switch (aov)
	{
		//case RPR_AOV_COLOR: !! not using main color in render elements
		case RPR_AOV_OPACITY: return FramebufferTypeId_Alpha;
		case RPR_AOV_WORLD_COORDINATE: return FramebufferTypeId_WorldCoordinate;
		case RPR_AOV_UV: return FramebufferTypeId_UV;
		case RPR_AOV_MATERIAL_IDX: return FramebufferTypeId_MaterialIdx;
		case RPR_AOV_GEOMETRIC_NORMAL: return FramebufferTypeId_GeometricNormal;
		case RPR_AOV_SHADING_NORMAL: return FramebufferTypeId_ShadingNormal;
		case RPR_AOV_DEPTH: return FramebufferTypeId_Depth;
		case RPR_AOV_OBJECT_ID: return FramebufferTypeId_ObjectId;
		case RPR_AOV_OBJECT_GROUP_ID: return FrameBufferTypeId_GroupId;
		case RPR_AOV_SHADOW_CATCHER: return FrameBufferTypeId_ShadowCatcher;
	};
	FASSERT(false);
	return FramebufferTypeId_Color;
}

FramebufferTypeId PRManagerMax::GetFramebufferTypeIdForAOVResolve(rpr_aov aov)
{
	switch (aov)
	{
		//case RPR_AOV_COLOR: !! not using main color in render elements
	case RPR_AOV_OPACITY: return FramebufferTypeId_AlphaResolve;
	case RPR_AOV_WORLD_COORDINATE: return FramebufferTypeId_WorldCoordinateResolve;
	case RPR_AOV_UV: return FramebufferTypeId_UVResolve;
	case RPR_AOV_MATERIAL_IDX: return FramebufferTypeId_MaterialIdxResolve;
	case RPR_AOV_GEOMETRIC_NORMAL: return FramebufferTypeId_GeometricNormalResolve;
	case RPR_AOV_SHADING_NORMAL: return FramebufferTypeId_ShadingNormalResolve;
	case RPR_AOV_DEPTH: return FramebufferTypeId_DepthResolve;
	case RPR_AOV_OBJECT_ID: return FramebufferTypeId_ObjectIdResolve;
	case RPR_AOV_OBJECT_GROUP_ID: return FrameBufferTypeId_GroupIdResolve;
	case RPR_AOV_SHADOW_CATCHER: return FrameBufferTypeId_ShadowCatcherResolve;
	};
	FASSERT(false);
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

#undef HELP

FIRERENDER_NAMESPACE_END;
