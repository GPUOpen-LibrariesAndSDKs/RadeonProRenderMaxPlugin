/*********************************************************************************************************************************
* Radeon ProRender for 3ds Max plugin
* Copyright (c) 2017 AMD
* All Rights Reserved
*
* The principal class of the renderer plugin. Handles communication with 3ds Max and runs the rendering itself
*********************************************************************************************************************************/

#include "frWrap.h"

#include "FireRenderer.h"
#include "Common.h"
#include "ClassDescs.h"
#include "utils/Utils.h"
#include "plugin/ParamDlg.h"
#include <ITabDialog.h>
#include <RadeonProRender.h>

#include "ParamBlock.h"
#include <WinUser.h>
#include <resource.h>

#include "plugin/MPManager.h"
#include "plugin/PRManager.h"

#pragma comment(lib, "shell32.lib")


FIRERENDER_NAMESPACE_BEGIN;

void* FireRenderClassDesc::Create(BOOL loading)
{
    return new FireRenderer;
}

FireRenderer::FireRenderer()
	:isActiveShade(FALSE)
	, pblock(NULL)
	, mPaused(false)
	, hRenderProgressDlg(NULL)
	, isToneOperatorPreviewRender(FALSE)
	, mRendering(false)
	, deleteAfterRender(false)
{
	warningDlg = std::make_unique<WarningDlg>(this);
	
    //fireRenderClassDesc.MakeAutoParamBlocks(this);
	pblock = CreateParameterBlock2(&FIRE_MAX_PBDESC, this);
    FASSERT(pblock);
}

void FireRenderer::ValidateParamBlock()
{
	ScopeManagerMax::TheManager.ValidateParamBlock(pblock);
}


void FireRenderer::DeleteThis()
{
	if (!activeShader)
	{
		MPManagerMax::TheManager.Abort(); // abort eventual ongoing material preview rendering
		PRManagerMax::TheManager.Abort(this); // abort eventual ongoing production rendering
	}
	if (!mRendering)
		delete this;
	else
		deleteAfterRender = true;
}

FireRenderer::~FireRenderer() 
{
	warningDlg.reset();

    DebugPrint(_T("FireRender::~FireRender()"));
}

class EmptyParamDlg : public RendParamDlg
{
public:
	EmptyParamDlg(IRendParams* ir, BOOL readOnly, FireRenderer* renderer)
	{
	}

	virtual void AcceptParams() override
	{
	}

	virtual void RejectParams() override
	{
	}

	virtual void DeleteThis() override
	{
		delete this;
	}
};

RendParamDlg* FireRenderer::CreateParamDialog(IRendParams *ir, BOOL prog)
{
	RendParamDlg* res = 0;
	ValidateParamBlock();
    if (!prog)
	{
		res = new FireRenderParamDlg(ir, prog, this);
    }
	else
	{
		// This is a read-only progress window rollup which belongs to progress window. Find and store progress window handle.
		hRenderProgressDlg = ir->GetIRollup()->GetHwnd();
		res = new EmptyParamDlg(ir, prog, this);
	}
    return res;
}

// Add the pages you want to the dialog. Use tab as the plugin
// associated with the pages. This will allows the manager
// to remove and add the correct pages to the dialog.
void FireRenderer::AddTabToDialog(ITabbedDialog* dialog, ITabDialogPluginTab* tab)
{
	// Settings
	dialog->AddRollout(L"Settings", NULL, kSettingsTabID, tab);

	// Advanced
	dialog->AddRollout(L"Advanced", NULL, kAdvSettingsTabID, tab);

	// Scripts
	dialog->AddRollout(L"Scripts", NULL, kScriptsTabID, tab);
}

int FireRenderer::AcceptTab(ITabDialogPluginTab* tab)
{
	if (tab->GetSuperClassID() == RADIOSITY_CLASS_ID || tab->GetClassID() == Class_ID(0x4fa95e9b, 0x9a26e66) ||
	tab->GetClassID() == Class_ID(1547006576, 1564889954))
	{
		return 0; // Don't show the advanced lighting, blur, nor render elements
	}
	else
	{
		return TAB_DIALOG_ADD_TAB; // Accept all other tabs
	}
}

/// This method is used to get various additional interfaces by 3ds Max
BaseInterface* FireRenderer::GetInterface(Interface_ID id)
{
    if (id == TAB_DIALOG_OBJECT_INTERFACE_ID)
		return this;
#if MAX_PRODUCT_YEAR_NUMBER < 2017	
	else if (id == IRENDERERREQUIREMENTS_INTERFACE_ID)
	{
        /// An interface that specifies some of the requirements of our plugin
        static class RequirementInterface : public IRendererRequirements {
        public:
            virtual bool HasRequirement(Requirement requirement) override {
                switch (int(requirement)) {
                case kRequirement_NoPauseSupport:
                    return false;
                case kRequirement_NoVFB:
                    // We do not have our own VFB, so we require 3ds Max to show its default one
                    return false;
                case kRequirement_DontSaveRenderOutput:
                    // We want 3ds Max to save the render output
                    return false;
                case kRequirement8_Wants32bitFPOutput:
                    // We require 3ds Max to give us 32 bit floating point frame buffer
                    return true;
                case kRequirement11_WantsObjectSelection:
                    // We can render even if there is no object selection made
                    return false;
                case 5: // = kRequirement18_NoGBufferForToneOpPreview: - 
                    // for some reason 3ds Max 2016 is already asking about this even though the value is not defined until 2017
                    return true;
                default:
                    STOP;
                }
            }
        } inter;
        return &inter;
    } 
#endif
	else
        return Renderer::GetInterface(id);
}

/// This method is used to get various additional interfaces by 3ds Max
void* FireRenderer::GetInterface(ULONG id) 
{
#if MAX_PRODUCT_YEAR_NUMBER < 2017
	if (id == IRenderElementCompatible::IID) 
	{
		/// Tells 3ds Max which render elements are compatible with our plugin
		class MyCompatibleElements : public IRenderElementCompatible {
			virtual BOOL IsCompatible(IRenderElement *pIRenderElement) override {
				bool isAOVRenderElement(IRenderElement*);
				return isAOVRenderElement(pIRenderElement);
			}
		};
		static MyCompatibleElements checker;
		return &checker;
	}
	else if (id == I_RENDER_ID)
	{
		if (!activeShader)
			activeShader = std::make_shared<IFireRender>(this);
		return activeShader.get();
    }
#endif

    return Renderer::GetInterface(id);
}

/// Used for loading/saving into a file
const int FIRE_MAX_BASECLASS_CHUNK = 0x400;

IOResult FireRenderer::Save(ISave* isave) {
    // The paramblock might have got already saved by the point this function is called 
    // -> do NOT set anything in pblock here hoping it will be saved in the file
    isave->BeginChunk(FIRE_MAX_BASECLASS_CHUNK);
    IOResult res = Renderer::Save(isave);
    isave->EndChunk();
    return res;
}

IOResult FireRenderer::Load(ILoad* iload) {
    // The paramblock is NOT loaded at this point, so do NOT attempt to read from it. We can use a post load callback for that
    IOResult res = IO_OK;
    while (IO_OK == (res = iload->OpenChunk())) {
        if (iload->CurChunkID() == FIRE_MAX_BASECLASS_CHUNK) {
            res = Renderer::Load(iload);
            iload->CloseChunk();
            break;
        }
    }
    return res;
}

RefTargetHandle FireRenderer::Clone(RemapDir &remap) {
    FireRenderer* newRend = new FireRenderer();
    BaseClone(this, newRend, remap);
    for (int i = 0; i < NumRefs(); i++) {
        if (IsRealDependency(GetReference(i))) {
            newRend->ReplaceReference(i, remap.CloneRef(GetReference(i)));
        }
    }
    return newRend;
}

void FireRenderer::ResetParams()
{
}

int FireRenderer::Open(INode* scene, INode* vnode, ViewParams* viewPar, RendParams& rpar, HWND hwnd, DefaultLight* defaultLights, int numDefLights, RendProgressCallback* prog) 
{
	mRendering = true;

	ValidateParamBlock();

	parameters.rendParams = rpar;
	
    DebugPrint(_T("FireRender::Open"));
	
    // Initialize our aggregate parameters class
    parameters.pblock = pblock;
    parameters.sceneNode = scene;
    parameters.viewNode = vnode;
	if (viewPar)
	parameters.viewParams = *viewPar;
    
    parameters.defaultLights.clear();
    for (int i = 0; i < numDefLights; ++i) {
        parameters.defaultLights.push(defaultLights[i]);
    }
    parameters.renderGlobalContext.renderer = this;

	if (!rpar.inMtlEdit)
		return PRManagerMax::TheManager.Open(this, hwnd, prog);
	
	FASSERT(viewPar);

    return 1;
}

#if MAX_PRODUCT_YEAR_NUMBER >= 2017

IInteractiveRender* FireRenderer::GetIInteractiveRender()
{
	if (!activeShader)
		activeShader = std::make_shared<IFireRender>(this);
	return activeShader.get();
}

void FireRenderer::GetVendorInformation(MSTR& info) const
{
	info = L"Radeon ProRender";
}

void FireRenderer::GetPlatformInformation(MSTR& info) const
{
	info = L"x64 GPU ray tracer";
}

#endif

int FireRenderer::Render(TimeValue t, ::Bitmap* tobm, FrameRendParams &frp, HWND hwnd, RendProgressCallback* prog, ViewParams* viewPar)
{
	class ScopeSetWindow
	{
	private:
		HWND &mTarget;
	public:
		ScopeSetWindow(HWND &target, const HWND &value)
			: mTarget(target)
		{
			mTarget = value;
		}
		~ScopeSetWindow()
		{
			mTarget = 0;
		}
	};

	ScopeSetWindow SetWindow(mRenderWindow, hwnd);

	if (InMaterialEditor())
		return  MPManagerMax::TheManager.Render(this, t, tobm, frp, hwnd, prog, viewPar, parameters.sceneNode, parameters.viewNode, parameters.rendParams);
	return PRManagerMax::TheManager.Render(this, t, tobm, frp, hwnd, prog, viewPar);
}

void FireRenderer::Close(HWND hwnd, RendProgressCallback* prog)
{
	class ScopedFinalize
	{
	private:
		FireRenderer *mFr;
	public:
		ScopedFinalize(FireRenderer *pFr)
		: mFr(pFr)
		{
		}

		~ScopedFinalize()
		{
			if (mFr->deleteAfterRender)
				delete mFr;
			mFr->mRendering = false;
		}
	};

	ScopedFinalize Finalize(this);

	if (!InMaterialEditor())
		return PRManagerMax::TheManager.Close(this, hwnd, prog);
	
    DebugPrint(_T("FireRender::Close"));
}

RefResult FireRenderer::NotifyRefChanged(NOTIFY_REF_CHANGED_PARAMETERS)
{
	return REF_SUCCEED;
}

//////////////////////////////////////////////////////////////////////////////
// Active Shader
//

void IFireRender::OnNotify(NotifyInfo* info)
{
	switch(info->intcode)
	{
		case NOTIFY_SYSTEM_PRE_RESET:
		case NOTIFY_SYSTEM_PRE_NEW:
		case NOTIFY_PLUGINS_PRE_SHUTDOWN:
		{
			EndSession();	// calling this as it seems to be bypassed otherwise
			auto mgr = GetIIRenderMgr(0);
			if (mgr)
				mgr->Close();
		}
		break;

		case NOTIFY_POST_RENDERER_CHANGE:
		{
			int id = reinterpret_cast<int>(info->callParam);
			if (id == 0)
			{
				auto renderer = GetCOREInterface()->GetRenderer((RenderSettingID)id, false);
				if (renderer)
				{
					auto cid = renderer->ClassID();
					if (cid != FIRE_MAX_CID)
					{
						EndSession();
						auto mgr = GetIIRenderMgr(0);
						if (mgr)
							mgr->Close();
					}
				}
			}
		}
		break;

		case NOTIFY_PRE_RENDER:
		{
			// don't want activeshader on while rendering production
			auto mgr = GetIIRenderMgr(0);
			if (mgr)
				mgr->Close();
		}
		break;
	
	default:
		DebugPrint(L"Unhandled notification: 0x%08X\n", info->intcode);
		break;
	}
}

IFireRender::IFireRender(FireRenderer* fr) 
{
	pblock = CreateParameterBlock2(&FIRE_MAX_PBDESC, this);
	fireRenderer = fr;
	fr->isActiveShade = true;

	pIIRenderMgr = 0;

	mSynchronizer = nullptr;

	RegisterNotification(NotifyCallback, this, NOTIFY_PRE_RENDER);
	RegisterNotification(NotifyCallback, this, NOTIFY_PLUGINS_PRE_SHUTDOWN);
	RegisterNotification(NotifyCallback, this, NOTIFY_POST_RENDERER_CHANGE);
	RegisterNotification(NotifyCallback, this, NOTIFY_SYSTEM_PRE_RESET);
	RegisterNotification(NotifyCallback, this, NOTIFY_SYSTEM_PRE_NEW);

	mActiveShader = new ActiveShader(this);
}

IFireRender::~IFireRender() 
{
	DebugPrint(L"IFireRender::~IFireRender\n");

	delete mActiveShader;
	
	UnRegisterNotification(NotifyCallback, this, NOTIFY_PLUGINS_PRE_SHUTDOWN);
	UnRegisterNotification(NotifyCallback, this, NOTIFY_POST_RENDERER_CHANGE);
	UnRegisterNotification(NotifyCallback, this, NOTIFY_SYSTEM_PRE_RESET);
	UnRegisterNotification(NotifyCallback, this, NOTIFY_SYSTEM_PRE_NEW);
	UnRegisterNotification(NotifyCallback, this, NOTIFY_PRE_RENDER);
}


extern ParamBlockDesc2 FIRE_MAX_PBDESC;


IParamBlock2* IFireRender::GetParamBlock(int i)
{
	return pblock;
}

// called when activesdhade starts rendering. at this time all the other parameters
// have been properly set by 3dsmax
//

void IFireRender::BeginSession() 
{ 
	mActiveShader->Begin();
}

// called when activesdhade needs to stop rendering and close
//
void IFireRender::EndSession() 
{ 
	mActiveShader->End();
}

// the window that hosts activeshade
//
void IFireRender::SetOwnerWnd(HWND hOwnerWnd)
{ 
	mActiveShader->SetOwnerWnd(hOwnerWnd);
}

HWND IFireRender::GetOwnerWnd() const
{ 
	return mActiveShader->GetOwnerWnd();
}

// the render manager
//
void IFireRender::SetIIRenderMgr(IIRenderMgr *pIIRenderMgr)
{ 
	this->pIIRenderMgr = pIIRenderMgr;
}

IIRenderMgr *IFireRender::GetIIRenderMgr(IIRenderMgr *pIIRenderMgr) const
{ 
	return this->pIIRenderMgr;
}

// the bitmap we are interactively rendering to
//
void IFireRender::SetBitmap(Bitmap *pDestBitmap)
{ 
	DebugPrint(L"IFireRender::SetBitmap(%X)\n", pDestBitmap);
	mActiveShader->SetOutputBitmap(pDestBitmap);
}

Bitmap *IFireRender::GetBitmap(Bitmap *pDestBitmap) const 
{ 
	return mActiveShader->GetOutputBitmap();
}


// the root scene node
//
void IFireRender::SetSceneINode(INode *pSceneINode)  
{
	mActiveShader->SetSceneRootNode(pSceneINode);
}

INode *IFireRender::GetSceneINode() const
{ 
	return mActiveShader->GetSceneRootNode();
}

// This method sets whether to use the ViewINode. When a separate camera node
// is needed instead of ViewExp, the interactive rendering manager will set
// the viewINode to the interactive renderer and set UseViewINode to TRUE.
//
void IFireRender::SetUseViewINode(bool bUseViewINode)
{ 
	mActiveShader->SetUseViewINode(bUseViewINode);
}

bool IFireRender::GetUseViewINode() const
{ 
	return mActiveShader->GetUseViewINode();
}

// the camera node if any, otherwise NULL
//
void IFireRender::SetViewINode(INode *pViewINode)
{ 
	mActiveShader->SetViewINode(pViewINode);
}

INode *IFireRender::GetViewINode() const
{ 
	return mActiveShader->GetViewINode();
}

// The ViewExp is the view specification for docked windows. The interactive
// renderer gets the view params out of the ViewExp
//
void IFireRender::SetViewExp(ViewExp *pViewExp)
{ 
	mActiveShader->SetViewExp(pViewExp);
}

ViewExp *IFireRender::GetViewExp() const
{ 
	return mActiveShader->GetViewExp();
}

// There are two standard interactive modes that should be supported in all
// interactive renderers : region rendering and selected object rendering,
// and these modes should ideally work in consort if at all possible : scenes
// are often very complex and the plugin renderer must be able to limit
// complexity to increase interactivity.Note that if Box2::IsEmpty() returns
// TRUE, it indicates to render entire bitmap
//
void IFireRender::SetRegion(const Box2 &region)
{ 
	mActiveShader->SetRegion(region);
}

const Box2 &IFireRender::GetRegion() const
{ 
	return mActiveShader->GetRegion();
}

// These lights will be used when no user specified lights are in the scene.
// This should be noted when the scene is traversed in begin session, and of
// course altered if new user lights are created
//
void IFireRender::SetDefaultLights(DefaultLight *pDefLights, int numDefLights)
{ 
	mActiveShader->SetDefaultLights(pDefLights, numDefLights);
}

const DefaultLight *IFireRender::GetDefaultLights(int &numDefLights) const
{ 
	return mActiveShader->GetDefaultLights(numDefLights);
}

// The Progress/Abort Callback should be called by the renderer ideally about
// every 100 milliseconds, but the actual range varies widely. The callback
// allows the manager to display rendering progress and/or abort a rendering
//
void IFireRender::SetProgressCallback(IRenderProgressCallback *pProgCB)
{ 
	mActiveShader->SetProgressCallback(pProgCB);
}

const IRenderProgressCallback *IFireRender::GetProgressCallback() const
{ 
	return mActiveShader->GetProgressCallback();
}

// "Render" is called when the activeshade is supposed to provide a
// static, non-interactive rendering. It's a command that can be issued to
// the activeshade (the only must-have command). All other eventual commands
// would be custom and defined through GetActionTableId and GetActionCallback.
//
void IFireRender::Render(Bitmap *pDestBitmap)
{ 
}

// This method returns the closest node handle for a given bitmap pixel location.
// This can be implemented with an item buffer, by using ray casting, or some
// other method and allows the interactive rendering manager to implement object
// selection - UNSUPPORTED IN FR AS OF 02/02/2016
//
ULONG IFireRender::GetNodeHandle(int x, int y)
{ 
	return 0;
}

// This method returns the screen bounding box of the corresponding INode, so
// the selection box corners can be drawn
//
bool IFireRender::GetScreenBBox(Box2 &sBBox, INode *pINode)
{ 
	return false;
}

// This method returns the ActionTableId for any action items the renderer may
// implement. This method will return 0 if none are available. Action tables
// are used as context sensitive command system to generate quad menus and the
// like from the various objects in the scene
//
ActionTableId IFireRender::GetActionTableId()
{ 
	//return 0x476259ed;
	return 0;
}

// This method returns a pointer to an ActionCallback for any action items the
// renderer may implement. This method will return NULL if none are available
//
ActionCallback *IFireRender::GetActionCallback()
{ 
	//return this;
	return NULL;
}

// General extension mechanism, access to additional method interfaces
//
void *IFireRender::GetInterface()
{ 
	return 0;
}

// this function is used by 3dsmax to make sure that there are no race conditions
// when shutting down the rendering manager and activeshade is still running.
//
BOOL IFireRender::IsRendering()
{ 
	return mActiveShader->IsRendering();
}

// this function is 2015 sdk onward only
//
#if MAX_PRODUCT_YEAR_NUMBER >= 2015
BOOL IFireRender::AnyUpdatesPending()
{ 
	return mActiveShader->AnyUpdatesPending();
}
#endif

void IFireRender::AbortRender()
{
	mActiveShader->AbortRender();
}

FIRERENDER_NAMESPACE_END;
