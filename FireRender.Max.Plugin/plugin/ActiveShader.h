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

#include "parser/Synchronizer.h"
#include "ParamBlock.h"
#include <interactiverender.h>
#include <atomic>

FIRERENDER_NAMESPACE_BEGIN;

class IFireRender;
class ActiveShadeRenderCore;
class ActiveShader;
class ActiveShadeBitmapWriter;

class ActiveShadeSynchronizerBridge : public SynchronizerBridge
{
private:
	class ActiveShader *mActiveShader;

public:
	ActiveShadeSynchronizerBridge(class ActiveShader *as)
		: mActiveShader(as)
	{
	}

	inline const TimeValue t() override
	{
		return GetCOREInterface()->GetTime();
	}

	void LockRenderThread() override;
	void UnlockRenderThread() override;
	void StartToneMapper() override;
	void StopToneMapper() override;
	void SetToneMappingExposure(float exposure) override;
	void ClearFB() override;
	IRenderProgressCallback *GetProgressCB() override;
	IParamBlock2 *GetPBlock() override;
	bool RenderThreadAlive() override;
	const DefaultLight *GetDefaultLights(int &numDefLights) const override;
	void EnableAlphaBuffer();
	void DisableAlphaBuffer();
	void SetTimeLimit(int val);
	void SetPassLimit(int val);
	void SetLimitType(int type);
	void ResetInteractiveTermination();
};

class ActiveShadeSynchronizer : public Synchronizer
{
public:
	ActiveShadeSynchronizer(class ActiveShader *as, frw::Scope scope, INode *pSceneINode, ActiveShadeSynchronizerBridge *pBridge)
	: Synchronizer(scope, pSceneINode, pBridge)
	, mActiveShader(as)
	{
	}

protected:
	ParsedView ParseView();

	void CustomCPUSideSynch() override;
	class ActiveShader *mActiveShader;
};

class ActiveShader
{
friend class ActiveShadeSynchronizer;
friend class ActiveShadeSynchronizerBridge;
friend class ActiveShadeBitmapWriter;
private:
	class ActiveShadeRenderCore *mRenderThread;
	ScopeID mScopeId;
	
	ActiveShadeSynchronizer *mSynchronizer;

	INode *mSceneINode; // the scene root node
	std::atomic<Bitmap*> mOutputBitmap; // the bitmap we blit to
	int mViewID; // the viewport id, at the time activeshade is launched
	Box2 region; // the rendering region
	bool bUseViewINode = false;
	INode *pViewINode = 0;
	ViewExp *pViewExp = 0;
	HWND hOwnerWnd = 0;
	ViewParams viewParams;
	IRenderProgressCallback *pProgCB = 0;
	DefaultLight *mDefLights = 0;
	int mNumDefLight = 0;
	HWND mShaderCacheDlg; // shader cache rebuilding dialog box

	bool mRunning = false; // true after Begin is called, false after End is called

	ActiveShadeSynchronizerBridge *mBridge = 0;

public:
	class IFireRender *mIfr;
	std::vector<frw::Camera> camera;

	static EventSpin mGlobalLocker; // while signaled, activeshader will not render

public:
	ActiveShader(class IFireRender *ifr);
	~ActiveShader();

	inline bool IsRunning() const
	{
		return mRunning;
	}

	void Begin();
	void End();

	void AbortRender();

	void SetOutputBitmap(Bitmap *pDestBitmap);
	std::atomic<Bitmap*> &GetOutputBitmap();

	void SetSceneRootNode(INode *root);
	INode* GetSceneRootNode() const;

	void SetOwnerWnd(HWND hwnd);
	HWND GetOwnerWnd() const;

	void SetUseViewINode(bool use);
	bool GetUseViewINode() const;

	void SetViewINode(INode *node);
	INode *GetViewINode() const;

	void SetViewExp(ViewExp *pViewExp);
	ViewExp *GetViewExp() const;

	void SetRegion(const Box2 &region);
	const Box2 &GetRegion() const;

	void SetProgressCallback(IRenderProgressCallback *cb);
	const IRenderProgressCallback *GetProgressCallback() const;

	void SetDefaultLights(DefaultLight *pDefLights, int numDefLights);
	const DefaultLight *GetDefaultLights(int &numDefLights) const;

	BOOL AnyUpdatesPending();

	BOOL IsRendering();
};

FIRERENDER_NAMESPACE_END;