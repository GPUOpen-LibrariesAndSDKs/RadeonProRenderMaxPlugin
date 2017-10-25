/*********************************************************************************************************************************
* Radeon ProRender for 3ds Max plugin
* Copyright (c) 2017 AMD
* All Rights Reserved
*
* Production Renderer
*********************************************************************************************************************************/

#pragma once

#include <max.h>
#include <guplib.h>
#include <notify.h>
#include <iparamb2.h>
#include "Common.h"
#include "plugin/ManagerBase.h"
#include "parser/RenderParameters.h"
#include "parser/SceneCallbacks.h"
#include "plugin/FireRenderer.h"
#include <thread>

FIRERENDER_NAMESPACE_BEGIN;

#define MPMANAGER_VERSION 1


class FireRenderer;
class ProductionRenderCore;

typedef enum
{
	Termination_None,
	Termination_Passes,
	Termination_Time
} TerminationCriteria;

// render elements
typedef enum
{
	FramebufferTypeId_Color,
	FramebufferTypeId_ColorResolve,
	FramebufferTypeId_Alpha,
	FramebufferTypeId_AlphaResolve,
	FramebufferTypeId_WorldCoordinate,
	FramebufferTypeId_WorldCoordinateResolve,
	FramebufferTypeId_UV,
	FramebufferTypeId_UVResolve,
	FramebufferTypeId_MaterialIdx,
	FramebufferTypeId_MaterialIdxResolve,
	FramebufferTypeId_GeometricNormal,
	FramebufferTypeId_GeometricNormalResolve,
	FramebufferTypeId_ShadingNormal,
	FramebufferTypeId_ShadingNormalResolve,
	FramebufferTypeId_Depth,
	FramebufferTypeId_DepthResolve,
	FramebufferTypeId_ObjectId,
	FramebufferTypeId_ObjectIdResolve,
	FrameBufferTypeId_GroupId,
	FrameBufferTypeId_GroupIdResolve,
	FrameBufferTypeId_ShadowCatcher,
	FrameBufferTypeId_Background,
	FrameBufferTypeId_Composite,
} FramebufferTypeId;


class PRManagerMax : public GUP, public ReferenceMaker
{
public:
	static PRManagerMax TheManager;
	static Event bmDone;

public:
	PRManagerMax();
	~PRManagerMax();

	// GUP Methods
	DWORD Start() override;
	void Stop() override;

	void DeleteThis() override;

	static ClassDesc* GetClassDesc();

public:

	int Open(FireRenderer *renderer, HWND hwnd, RendProgressCallback* prog);

	int Render(FireRenderer *pRenderer, TimeValue t, ::Bitmap* frontBuffer, FrameRendParams &frp, HWND hwnd, RendProgressCallback* prog, ViewParams* viewPar);

	void Close(FireRenderer *pRenderer, HWND hwnd, RendProgressCallback* prog);

	void Abort(FireRenderer *pRenderer);

	static const MCHAR* GetStampHelp();

private:
	class Data
	{
	public:
		ScopeID scopeId;

		TerminationCriteria termCriteria;
		unsigned int passLimit;
		__time64_t timeLimit;
		bool isNormals;
		bool shouldToneMap;
		float toneMappingExposure;
		bool isToneOperatorPreviewRender;
		bool isAlphaEnabled;

		class ProductionRenderCore* renderThread;
		std::thread* helperThread;
		std::atomic<bool> bRenderCancelled;
		bool bRenderThreadDone;
		std::atomic<bool> bQuitHelperThread;
		std::atomic<bool> bitmapUpdated;

		Data()
			: scopeId(-1)
			, toneMappingExposure(1.0f)
			, renderThread(nullptr)
			, helperThread(nullptr)
			, isToneOperatorPreviewRender(false)
			, bRenderCancelled(false)
			, isNormals(false)
			, shouldToneMap(false)
			, bitmapUpdated(false)
		{}
	};

	std::map<FireRenderer *, Data *> mInstances;
	
	void CleanUpRender(FireRenderer *pRenderer);

	static void NotifyProc(void *param, NotifyInfo *info);

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
	void MakeReference(RefTargetHandle rtarg)
	{
		ReplaceReference(0, rtarg);
	}
	RefResult NotifyRefChanged(NOTIFY_REF_CHANGED_PARAMETERS) override;

	void SetupCamera(const ParsedView& view, const int imageWidth, const int imageHeight, rpr_camera outCamera);

	bool IsWndMessageToSkip(const MSG &msg);

	FramebufferTypeId GetFramebufferTypeIdForAOV(rpr_aov aov);
	FramebufferTypeId GetFramebufferTypeIdForAOVResolve(rpr_aov aov);
};

FIRERENDER_NAMESPACE_END;