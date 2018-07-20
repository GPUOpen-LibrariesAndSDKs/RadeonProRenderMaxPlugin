/*********************************************************************************************************************************
* Radeon ProRender for 3ds Max plugin
* Copyright (C) 2017 AMD
* All Rights Reserved
* 
* The principal class of the renderer plugin. Handles communication with 3ds Max and runs the rendering itself
*********************************************************************************************************************************/
#pragma once
#include "Common.h"
#include <iparamm2.h>
#include <interactiverender.h>
#include "parser/RenderParameters.h"
#include "parser/SceneCallbacks.h"
#include "utils/Thread.h"
#include "parser/SceneParser.h"
#include "frWrap.h"
#include "FrScope.h"
#include "plugin/ScopeManager.h"
#include <ISceneEventManager.h>
#include <ITabDialog.h>
#include "plugin/WarningDlg.h"
#include "parser/Synchronizer.h"
#include "ActiveShader.h"

#include<atomic>

FIRERENDER_NAMESPACE_BEGIN;

class FireRenderParamDlg;
class FireRenderer;
class FRRenderCore;
class SceneParser;


//////////////////////////////////////////////////////////////////////////////
// Active Shader class
//

class IFRRenderCore;

class IFireRender : public IInteractiveRender, ReferenceMaker, ActionCallback
{
public:
	void AbortRender() MAX2017_OVERRIDE;

private:
	IParamBlock2* pblock;

public:
	///
	IFireRender(FireRenderer* fr);
	~IFireRender();

	void BeginSession() override;
	void EndSession() override;
	void SetOwnerWnd(HWND hOwnerWnd) override;
	HWND GetOwnerWnd() const override;
	void SetIIRenderMgr(IIRenderMgr *pIIRenderMgr)  override;
	IIRenderMgr *GetIIRenderMgr(IIRenderMgr *pIIRenderMgr) const  override;
	void SetBitmap(Bitmap *pDestBitmap)  override;
	Bitmap *GetBitmap(Bitmap *pDestBitmap) const  override;
	void SetSceneINode(INode *pSceneINode)  override;
	INode *GetSceneINode() const  override;
	void SetUseViewINode(bool bUseViewINode)  override;
	bool GetUseViewINode() const  override;
	void SetViewINode(INode *pViewINode)  override;
	INode *GetViewINode() const  override;
	void SetViewExp(ViewExp *pViewExp)  override;
	ViewExp *GetViewExp() const  override;
	void SetRegion(const Box2 &region)  override;
	const Box2 &GetRegion() const  override;
	void SetDefaultLights(DefaultLight *pDefLights, int numDefLights)  override;
	const DefaultLight *GetDefaultLights(int &numDefLights) const  override;
	void SetProgressCallback(IRenderProgressCallback *pProgCB)  override;
	const IRenderProgressCallback *GetProgressCallback() const  override;
	void Render(Bitmap *pDestBitmap)  override;
	ULONG GetNodeHandle(int x, int y)  override;
	bool GetScreenBBox(Box2 &sBBox, INode *pINode)  override;
	ActionTableId GetActionTableId()  override;
	ActionCallback *GetActionCallback()  override;
	void *GetInterface() override;
	BOOL IsRendering()  override;
#if MAX_PRODUCT_YEAR_NUMBER >= 2015
	BOOL AnyUpdatesPending() override;
#endif

	void OnNotify(NotifyInfo* info);

	static void NotifyCallback(void *param, NotifyInfo *info)
	{
		auto self = static_cast<IFireRender*>(param);
		self->OnNotify(info);
	}

public:
	FireRenderer* fireRenderer = nullptr;

	IIRenderMgr *pIIRenderMgr;

	bool IsRunning() const
	{
		if (mActiveShader)
			return mActiveShader->IsRunning();
		return false;
	}

	IParamBlock2* GetParamBlock(int i) override;

	RefResult NotifyRefChanged(const Interval &, RefTargetHandle, PartID &, RefMessage, BOOL) override
	{
		return REF_DONTCARE;
	}

protected:
	ActiveShader *mActiveShader;
	Synchronizer *mSynchronizer;

	RenderParameters rparms;
};




//////////////////////////////////////////////////////////////////////////////
// The main renderer plugin class. Handles communication with 3ds Max and runs
// the rendering itself
//


class FireRenderer : public Renderer, public ITabDialogObject
{
	friend class FireRenderParamDlg;
	friend class TMtracker;

public:

#if MAX_PRODUCT_YEAR_NUMBER >= 2017
	PauseSupport IsPauseSupported() const MAX2017_OVERRIDE { return PauseSupport::None;	}
	bool HasRequirement(Requirement requirement) override { return false; }

	bool IsStopSupported() const MAX2017_OVERRIDE { return false; }
	void StopRendering() MAX2017_OVERRIDE { }
	
	void PauseRendering() MAX2017_OVERRIDE {}
	void ResumeRendering() MAX2017_OVERRIDE {}

	bool CompatibleWithAnyRenderElement() const MAX2017_OVERRIDE { return true; }
	bool CompatibleWithRenderElement(IRenderElement& pIRenderElement) const MAX2017_OVERRIDE { return true; }
	IInteractiveRender* GetIInteractiveRender() MAX2017_OVERRIDE;
	void GetVendorInformation(MSTR& info) const MAX2017_OVERRIDE;
	void GetPlatformInformation(MSTR& info) const MAX2017_OVERRIDE;
#endif

	// ITabDialogObject
	// Add your tab(s) to the dialog. This will only be called if
	// both this object and the dialog agree that the tab should
	// be added. Dialog is the address of the dialog.
	void AddTabToDialog(ITabbedDialog* dialog, ITabDialogPluginTab* tab) override;

	// Return true if the tabs added by the ITabDialogPluginTab tab
	// are acceptable for this dialog. Dialog is the dialog being
	// filtered.
	int AcceptTab(ITabDialogPluginTab* tab) override;
	
private:
	std::wstring exportFRSceneFileName;
	
public:
	std::unique_ptr<WarningDlg> warningDlg;
	HWND mRenderWindow; // valid during rendering, handle of the VFB window
	HWND hRenderProgressDlg; // valid during rendering, handle of the progress window

	// A single object that aggregates all the various render settings structs 3ds Max gives us
	RenderParameters parameters;

protected:
    // Our reference #0. Aparamblock where we store all render settings
    IParamBlock2* pblock;
    
	std::shared_ptr<IFireRender> activeShader;

	// if true, we are anywhere in-between Open, Render and Close calls
	bool mRendering; 

	// if true, "delete this" must be performed upon returning from the render call
	bool deleteAfterRender;
	
public:

	bool InMaterialEditor() const
	{
		return bool_cast(parameters.rendParams.inMtlEdit);
	}
	bool InExposureControlPreview()
	{
		return parameters.rendParams.IsToneOperatorPreviewRender();
	}
	bool InActiveShade()
	{
		return bool_cast( isActiveShade );
	}

	void SetFireRenderExportSceneFilename(std::wstring fileName)
	{
		exportFRSceneFileName = fileName;
	}

	inline const std::wstring& GetFireRenderExportSceneFilename() const
	{
		return exportFRSceneFileName;
	}

	bool mPaused;

	BOOL isToneOperatorPreviewRender;
	BOOL isActiveShade;
public:

    FireRenderer();

    virtual ~FireRenderer();

	//make sure all params are adequate, like GPU count
	void ValidateParamBlock();

    // Misc 3ds Max methods

    virtual void GetClassName(TSTR& s) override
	{
        s = FIRE_MAX_NAME;
    }

    virtual Class_ID ClassID() override
	{
        return FIRE_MAX_CID;
    }

	virtual void DeleteThis() override;

    virtual BaseInterface* GetInterface(Interface_ID  id) override;
    
    virtual void* GetInterface(ULONG id) override;
    
    virtual int NumParamBlocks() override
	{
        return 1;
    }
    
    virtual IParamBlock2* GetParamBlock(int i) override
	{
        FASSERT(i == 0); 
        return this->pblock;
    }
    
    virtual IParamBlock2* GetParamBlockByID(BlockID id) override
	{
        FASSERT(id == 0);
        return GetParamBlock(id);
    }
    
    virtual RefTargetHandle GetReference(int  i) override
	{
        FASSERT(i == 0); 
        return this->pblock;
    }
    
    virtual void SetReference(int i, RefTargetHandle rtarg) override
	{
        FASSERT(i == 0); 
        this->pblock = dynamic_cast<IParamBlock2*>(rtarg);
    }
    
    virtual int NumRefs() override
	{
        return 1;
    }
    
    virtual int NumSubs() override
	{
        return NumRefs();
    }
    
    virtual Animatable* SubAnim(int i) override
	{
        return GetReference(i);
    }
    
    virtual TSTR SubAnimName(int i) override
	{
        FASSERT(i == 0); 
        return _T("Pblock");
    }
    
    virtual int SubNumToRefNum(int subNum) override
	{
        return subNum;
    }
    
	virtual RefResult NotifyRefChanged(NOTIFY_REF_CHANGED_PARAMETERS) override;

    virtual IOResult Save(ISave *isave) override;
    
    virtual IOResult Load(ILoad *iload) override;
    
    virtual RefTargetHandle Clone(RemapDir &remap) override;

    // 3dsmax Renderer class methods

    /// Starts a rendering session. Called once
    virtual int Open(INode* scene, INode* vnode, ViewParams* viewPar, RendParams& rpar, HWND hwnd, DefaultLight* defaultLights,
                     int numDefLights, RendProgressCallback* prog) override;
    
    /// Renders a single frame in given time t. Called once per each rendered frame of animation
    virtual int Render(TimeValue t, ::Bitmap* tobm, FrameRendParams &frp, HWND hwnd, RendProgressCallback* prog,
                       ViewParams* viewPar) override;

    /// Ends a rendering session. Called once
    virtual void Close(HWND hwnd, RendProgressCallback* prog) override;

    /// Creates the render settings dialog UI
    virtual RendParamDlg* CreateParamDialog(IRendParams *ir, BOOL prog) override;
	
    /// Called by Max when Max is reset
    virtual void ResetParams() override;
};


FIRERENDER_NAMESPACE_END;