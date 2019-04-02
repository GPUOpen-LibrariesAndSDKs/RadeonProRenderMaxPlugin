/*********************************************************************************************************************************
* Radeon ProRender for 3ds Max plugin
* Copyright (c) 2017 AMD
* All Rights Reserved
*
* Synchronizer is the class that keeps track of scene changes and allows ActiveShader to react in realtime
*********************************************************************************************************************************/

#include "Synchronizer.h"
#include <notify.h>
#include "CoronaDeclarations.h"
#include "plugin/FireRenderEnvironment.h"
#include "plugin/FireRenderAnalyticalSun.h"
#include "plugin/FireRenderPortalLight.h"
#include "plugin/BgManager.h"
#include "plugin/ParamBlock.h"
#include "plugin/TmManager.h"
#include "plugin/BgManager.h"
#include "plugin/CamManager.h"
#include "plugin/ScopeManager.h"

#include "SceneCallbacks.h"

FIRERENDER_NAMESPACE_BEGIN;

#define CUI_TIMER_PERIOD 10


//////////////////////////////////////////////////////////////////////////////
//
// THIS SECTION DEALS WITH TRACKING SCENE OBJECTS AND THEIR CHANGES
// Class SynchronizerNodeEventCallback
//

#define FOR_EACH_NODE(nodes)								\
	INode* node;											\
	for( int _index=0; _index<nodes.Count(); _index++ )		\
		if( (node=NodeEventNamespace::GetNodeByKey(nodes[_index])) != NULL )

void SynchronizerNodeEventCallback::InsertRebuildCommand( NodeKeyTab& nodes )
{
	FOR_EACH_NODE(nodes)
		InsertRebuildCommand(node);
}
void SynchronizerNodeEventCallback::InsertAssignMatCommand( NodeKeyTab& nodes )
{
	FOR_EACH_NODE(nodes)
		InsertAssignMatCommand(node);
}
void SynchronizerNodeEventCallback::InsertAddCommand( NodeKeyTab& nodes )
{
	FOR_EACH_NODE(nodes)
		InsertRebuildCommand(node);
}
void SynchronizerNodeEventCallback::InsertDeleteCommand( NodeKeyTab& nodes )
{
	FOR_EACH_NODE(nodes)
		InsertDeleteCommand(node);
}
void SynchronizerNodeEventCallback::InsertShowHideCommand( NodeKeyTab& nodes )
{
	FOR_EACH_NODE(nodes)
	{
		if( node->IsHidden() )
			 InsertHideCommand(node);
		else InsertShowCommand(node);
	}
}
void SynchronizerNodeEventCallback::InsertXformCommand( NodeKeyTab& nodes )
{
	FOR_EACH_NODE(nodes)
		InsertXformCommand(node);
}
void SynchronizerNodeEventCallback::InsertMatCommand( NodeKeyTab& nodes )
{
	FOR_EACH_NODE(nodes)
		InsertMatCommand(node->GetMtl());
}


//////////////////////////////////////////////////////////////////////////////
//
// THIS SECTION DEALS WITH TRACKING SCENE OBJECTS AND THEIR CHANGES
// Class Synchronizer
//

// Helper class: ActiveShadeAttachCallback
void Synchronizer::ActiveShadeAttachCallback::PreSceneAttachLight( frw::Shape& shape, INode* node )
{
	auto ll = mSynch->mLightShapes.find(node);
	if (ll != mSynch->mLightShapes.end())
	{
		mSynch->mScope.GetScene().Detach(ll->second->Get());
		mSynch->mLightShapes.erase(ll);
	}
	mSynch->mLightShapes.insert(std::make_pair(node, new SShape(shape, node)));
}
void Synchronizer::ActiveShadeAttachCallback::PreSceneAttachLight( frw::Light& light, INode* node )
{
	auto ll = mSynch->mLights.find(node);
	if (ll != mSynch->mLights.end())
	{
		mSynch->mScope.GetScene().Detach(ll->second->Get());
		mSynch->mLights.erase(ll);
	}
	mSynch->mLights.insert(std::make_pair(node, new SLight(light, node)));
}

void Synchronizer::NotifyProc(void *param, NotifyInfo *info)
{
	Synchronizer *synch = reinterpret_cast<Synchronizer*>(param);
	if (info->intcode == NOTIFY_NODE_HIDE)
	{
		INode *node = reinterpret_cast<INode *>(info->callParam);
		synch->InsertHideCommand(node);
	}
	else if (info->intcode == NOTIFY_NODE_UNHIDE)
	{
		INode *node = reinterpret_cast<INode *>(info->callParam);
		synch->InsertShowCommand(node);
	}
	else if (info->intcode == NOTIFY_SCENE_ADDED_NODE)
	{
		INode *node = reinterpret_cast<INode *>(info->callParam);
		synch->MakeInitialReferences(node); // Called whether or not using the NodeEventSystem
		const ObjectState& state = node->EvalWorldState(synch->mBridge->t());
		if (state.obj)
		{
			const Class_ID classId = state.obj->ClassID();
			if (classId == FIRERENDER_ENVIRONMENT_CLASS_ID)
				synch->InsertStartRPREnvironmentCommand();
		}
	}
	else if (info->intcode == NOTIFY_SCENE_PRE_DELETED_NODE)
	{
		INode *node = reinterpret_cast<INode *>(info->callParam);
		
		// remove from material users right now
		synch->RemoveMaterialsFromNode(node);
				
		const ObjectState& state = node->EvalWorldState(synch->mBridge->t());
		if (state.obj)
		{
			const Class_ID classId = state.obj->ClassID();
			if (classId == FIRERENDER_ENVIRONMENT_CLASS_ID)
				synch->InsertStopRPREnvironmentCommand();
			else
				synch->InsertDeleteCommand(node);
		}
		#ifndef USE_NODEEVENTSYSTEM
		synch->RemoveReference(node);
		#endif // USE_NODEEVENTSYSTEM
	}
	else if (info->intcode == NOTIFY_FILE_PRE_SAVE_PROCESS)
	{
		synch->mBridge->LockRenderThread();
	}
	else if (info->intcode == NOTIFY_FILE_POST_SAVE_PROCESS)
	{
		synch->mBridge->UnlockRenderThread();
	}
}

void Synchronizer::MakeInitialReferences(ReferenceTarget* ref)
{
	if (!ref)
		return;

	if (auto node = dynamic_cast<INode*>(ref))
	{
		if (node->IsRootNode()) // Only scene root node holds XRefs
		{
			for (int i = 0; i < node->GetXRefFileCount(); i++)
				MakeInitialReferences(node->GetXRefTree(i));
		}

		for (int i = 0; i < node->NumberOfChildren(); i++)
			MakeInitialReferences(node->GetChildNode(i));

		if (node->IsNodeHidden(TRUE))
			return;
		
		if (auto objRef = node->GetObjectRef())
		{
			if (dynamic_cast<CameraObject*>(node->GetObjectRef()))
				return; // don't track cameras
			if (objRef->ClassID() == Class_ID(TARGET_CLASS_ID, 0))
				return; // don't track targets
			if (objRef->ClassID() == Class_ID(DUMMY_CLASS_ID, 0))
				return;
			if (objRef->ClassID() == Class_ID(BONE_CLASS_ID, 0))
				return;
			if (objRef->ClassID() == Class_ID(TAPEHELP_CLASS_ID, 0))
				return;
			if (objRef->ClassID() == Class_ID(GRIDHELP_CLASS_ID, 0))
				return;
			if (objRef->ClassID() == Class_ID(POINTHELP_CLASS_ID, 0))
				return;
			if (objRef->ClassID() == Class_ID(PROTHELP_CLASS_ID, 0))
				return;
			
			//auto id = Animatable::GetHandleByAnim(node);
			if (objRef->ClassID() == Corona::SCATTER_CID)
			{
// this is intentional. This code needs to be ported. Do not remove.
#ifdef CODE_TO_PORT 
				if (ScopeManagerMax::CoronaOK)
				{
				// If this is a scatter node, we will use its function publishing interface to get list of scattered nodes and report them 
				// all with their transformation matrices
				Corona::ScatterFpOps* ops = GetScatterOpsInterface(objRef->GetParamBlockByID(0)->GetDesc()->cd);
				const Point3 cameraPos = parameters.renderGlobalContext.worldToCam.GetTrans() * GetUnitScale();
				ops->update(objRef, cameraPos, parameters.t); // necessary so we use the correct time/config for the scatter

				const int numInstances = ops->getNumInstances(objRef);

				for (int i = 0; i < numInstances; i++)
				{
					auto node = ops->getInstanceObject(objRef, i);
					auto tm = ops->getInstanceTm(objRef, i);
					tm.SetTrans(tm.GetTrans() * masterScale);
					output.push_back(ParsedNode((id << 16) + i, node, tm));
				}
			}
#endif
			}
			else
			{
				// The commented code below needs to be ported.Do not remove.
#ifdef CODE_TO_PORT
				auto tm = input->GetObjTMAfterWSM(parameters.t);
				tm.SetTrans(tm.GetTrans() * masterScale);
				output.push_back(ParsedNode(id, input, tm));

				Matrix3 ntmMotion = input->GetObjTMAfterWSM(parameters.t + 1);
				ntmMotion.SetTrans(ntmMotion.GetTrans() * masterScale);
				output.back().tmMotion = ntmMotion;
#endif
				#ifndef USE_NODEEVENTSYSTEM
				AddReference(node);
				#endif // USE_NODEEVENTSYSTEM
				InsertRebuildCommand(node);

				MakeInitialReferences(node->GetMtl());
			}
		}
	}
	else if (auto r = dynamic_cast<Object*>(ref))
	{
		#ifndef USE_NODEEVENTSYSTEM
		AddReference(r);
		#endif // USE_NODEEVENTSYSTEM
	}
	else if (auto r = dynamic_cast<MtlBase*>(ref))
	{
		// TODO: Improve caching and reduce overhead for materials used multiple times in scene.
		// Debugging suggests performance problems with redundant material processing.
		#ifndef USE_NODEEVENTSYSTEM
		AddReference(r);
		#endif // USE_NODEEVENTSYSTEM
		if (r->IsMultiMtl())
		{
			Mtl* mtl = reinterpret_cast<Mtl*>(r);
			// Remove from list of multi-materials.  Otherwise entries are redundantly populated,
			// and synchronizer believes multi-materials have more submaterials than in reality
			auto iter = mMultiMats.find(mtl);
			if (iter != mMultiMats.end())
				mMultiMats.erase(mtl);

			for (int i = 0; i < mtl->NumSubMtls(); i++)
			{
				auto sub = mtl->GetSubMtl(i);
				mMultiMats[mtl].push_back(sub);
				MakeInitialReferences(sub);
			}
		}
	}
}

Synchronizer::Synchronizer(frw::Scope scope, INode *pSceneINode, SynchronizerBridge *pBridge)
	: mUITimerId(0)
	, mScope(scope)
	, mSceneINode(pSceneINode)
	, mBridge(pBridge)
	, mtlParser(scope)
	, mNotifyLastPartID(0)
	, mNotifyLastNode(0)
	, mMAXEnvironment(0)
	, mEnvironmentColor(-1.f, -1.f, -1.f)
	, mMAXEnvironmentUse(false)
	, mToneMapperCallback(this)
	, mBackgroundCallback(this)
	, mParametersTracker(this)
	, mNodeEventCallbackKey(-1)
{
	mtlParser.SetTimeValue(pBridge->t());
	mtlParser.SetParamBlock(pBridge->GetPBlock());

	MakeInitialReferences(pSceneINode); // Called whether or not using the NodeEventSystem

	#ifdef USE_NODEEVENTSYSTEM
	mNodeEventCallbackKey = GetISceneEventManager()->RegisterCallback( this );
	#endif //USE_NODEEVENTSYSTEM

	int res;
	res = RegisterNotification(NotifyProc, this, NOTIFY_NODE_HIDE);
	res = RegisterNotification(NotifyProc, this, NOTIFY_NODE_UNHIDE);
	res = RegisterNotification(NotifyProc, this, NOTIFY_SCENE_ADDED_NODE);
	res = RegisterNotification(NotifyProc, this, NOTIFY_SCENE_PRE_DELETED_NODE);
	res = RegisterNotification(NotifyProc, this, NOTIFY_FILE_PRE_SAVE_PROCESS);
	res = RegisterNotification(NotifyProc, this, NOTIFY_FILE_POST_SAVE_PROCESS);

	TmManagerMax::TheManager.RegisterPropertyChangeCallback(&mToneMapperCallback);
	BgManagerMax::TheManager.RegisterPropertyChangeCallback(&mBackgroundCallback);

	auto rprEnv = BgManagerMax::TheManager.GetEnvironmentNode();
	if (rprEnv)
		InsertStartRPREnvironmentCommand();
	mMAXEnvironmentForceRebuild = true;

	BOOL on = FALSE;
	TmManagerMax::TheManager.GetProperty(PARAM_TM_OVERRIDE_MAX_TONEMAPPERS, on);
	if (on)
	{
		InsertStartRPRToneMapper();
		InsertModifyRPRToneMapper();
	}

	FASSERT(mParametersTracker.AddToTracker(PARAM_MAX_RAY_DEPTH));
	FASSERT(mParametersTracker.AddToTracker(PARAM_RENDER_MODE));
	FASSERT(mParametersTracker.AddToTracker(PARAM_IMAGE_FILTER));
	FASSERT(mParametersTracker.AddToTracker(PARAM_USE_TEXTURE_COMPRESSION));
	FASSERT(mParametersTracker.AddToTracker(PARAM_IMAGE_FILTER_WIDTH));
	FASSERT(mParametersTracker.AddToTracker(PARAM_USE_IRRADIANCE_CLAMP));
	FASSERT(mParametersTracker.AddToTracker(PARAM_IRRADIANCE_CLAMP));
	FASSERT(mParametersTracker.AddToTracker(PARAM_CONTEXT_ITERATIONS));
	FASSERT(mParametersTracker.AddToTracker(PARAM_TIME_LIMIT));
	FASSERT(mParametersTracker.AddToTracker(PARAM_PASS_LIMIT));
	FASSERT(mParametersTracker.AddToTracker(PARAM_RENDER_LIMIT));
	FASSERT(mParametersTracker.AddToTracker(PARAM_QUALITY_RAYCAST_EPSILON));
	FASSERT(mParametersTracker.AddToTracker(PARAM_ADAPTIVE_NOISE_THRESHOLD));
	FASSERT(mParametersTracker.AddToTracker(PARAM_ADAPTIVE_TILESIZE));
	FASSERT(mParametersTracker.AddToTracker(PARAM_SAMPLES_MAX));
}

Synchronizer::~Synchronizer()
{
	BgManagerMax::TheManager.UnregisterPropertyChangeCallback(&mBackgroundCallback);
	TmManagerMax::TheManager.UnregisterPropertyChangeCallback(&mToneMapperCallback);
	if (mUITimerId)
	{
		KillTimer(GetCOREInterface()->GetMAXHWnd(), mUITimerId);
		mUITimerId = 0;
	}
	UnRegisterNotification(NotifyProc, this, NOTIFY_NODE_HIDE);
	UnRegisterNotification(NotifyProc, this, NOTIFY_NODE_UNHIDE);
	UnRegisterNotification(NotifyProc, this, NOTIFY_SCENE_ADDED_NODE);
	UnRegisterNotification(NotifyProc, this, NOTIFY_SCENE_PRE_DELETED_NODE);
	UnRegisterNotification(NotifyProc, this, NOTIFY_FILE_PRE_SAVE_PROCESS);
	UnRegisterNotification(NotifyProc, this, NOTIFY_FILE_POST_SAVE_PROCESS);

	if( mNodeEventCallbackKey != -1 )
		GetISceneEventManager()->UnRegisterCallback( mNodeEventCallbackKey );
	mNodeEventCallbackKey = -1;
}

// utility: gets the node that references an object. If more nodes are referencing this object (instances)
// the first instance is returned.
//
INode* Synchronizer::GetNodeFromObject(BaseObject* obj)
{
	ULONG handle = 0;
	NotifyDependents(FOREVER, (PartID)&handle, REFMSG_GET_NODE_HANDLE);
	INode *node = GetCOREInterface()->GetINodeByHandle(handle);
	return node;
}

RefResult Synchronizer::NotifyRefChanged(const Interval& iv, RefTargetHandle hTarg, PartID& partID, RefMessage msg, BOOL propagate)
{
	auto node = dynamic_cast<INode*>(hTarg);
	auto mtlbase = dynamic_cast<MtlBase*>(hTarg);
	auto object = dynamic_cast<Object*>(hTarg);
	CameraObject* camera = 0;
	LightObject* light = 0;
	if (node)
	{
		camera = dynamic_cast<CameraObject*>(node->GetObjectRef());
		light = dynamic_cast<LightObject*>(node->GetObjectRef());
	}

	if (object)
	{
		switch (msg)
		{
		case REFMSG_CHANGE:
		{
			if (partID == PART_OBJECT_TYPE)
			{
				// (structural change), usually sent
				// when a modifier is added (ie UVW add)
				// action: rebuild object
				InsertRebuildCommand(GetNodeFromObject(object));
			}
			else if (partID == PART_GEOM)
			{
				// mesh edit (ie vertices moved)
				// action: rebuild object
				InsertRebuildCommand(GetNodeFromObject(object));
			}
			else
			{
				if (partID == 0x800037ff)
				{
					// property changes or other modifications
					// sent to parametric geometries, cameras, lights
					// action: rebuild object (or rebuild light, or update camera)
					InsertRebuildCommand(GetNodeFromObject(object));
				}
				else if (partID & PART_TEXMAP)
				{
					// texture mapping channel modified, or sub-material assigned to
					// geometry elements (ie faces)
					// action: rebuild object
					InsertRebuildCommand(GetNodeFromObject(object));
				}
			}
		}
		break;
		}
	}
	else if (node)
	{
		switch (msg)
		{
		case REFMSG_CHANGE:
		{
			if (partID == PART_TM)
			{
				// transform matrix was changed (translation, scale, rotation)
				// action: invalidate xform
				InsertXformCommand(node);
			}
			else if (partID == PART_OBJECT_TYPE)
			{
				// (structural change), usually sent
				// when a modifier is added (ie UVW add)
				// action: rebuild object
				InsertRebuildCommand(node);
			}
			else if (partID == PART_GEOM)
			{
				// mesh edit (ie vertices moved)
				// action: rebuild object
				InsertRebuildCommand(node);
			}
			else
			{
				if (partID == 0x800037ff)
				{
					// property changes or other modifications
					// sent to parametric geometries, cameras, lights
					// action: rebuild object (or rebuild light, or update camera)
					if (mNotifyLastPartID != PART_TM || mNotifyLastNode != node)
						InsertRebuildCommand(node);
				}
				else if (partID & PART_TEXMAP)
				{
					// texture mapping channel modified, or sub-material assigned to
					// geometry elements (ie faces)
					// action: rebuild object
					InsertRebuildCommand(node);
				}
			}
		}
		break;
		case REFMSG_NODE_NAMECHANGE:
		{
		}
		break;
		case REFMSG_NODE_MATERIAL_CHANGED:
		{
			MakeInitialReferences(node->GetMtl());
			InsertAssignMatCommand(node);
		}
		break;
		}
	}
	else if (mtlbase)
	{
		// a material's property was changed
		// action: rebuild material and re-assign to its users
		if (auto mat = dynamic_cast<Mtl*>(mtlbase))
			InsertMatCommand(mat);
	}

	mNotifyLastPartID = partID;
	mNotifyLastNode = node;
	
	return REF_SUCCEED;
}

//
// tonemapper's property changes trap
//

void TMPropertyCallback::propChangedCallback(class ManagerMaxBase *owner, int paramId, int sender)
{
	switch (paramId)
	{
		case PARAM_TM_OVERRIDE_MAX_TONEMAPPERS:
		{
			BOOL on = FALSE;
			TmManagerMax::TheManager.GetProperty(PARAM_TM_OVERRIDE_MAX_TONEMAPPERS, on);
			if (on && !mSynch->mToneMapperRunning)
			{
				mSynch->InsertStartRPRToneMapper();
			}
			else if (!on && mSynch->mToneMapperRunning)
			{
				mSynch->InsertStopRPRToneMapper();
			}
		}
		break;
		
		case PARAM_TM_OPERATOR:
		case PARAM_TM_REINHARD_PRESCALE:
		case PARAM_TM_REINHARD_POSTSCALE:
		case PARAM_TM_REINHARD_BURN:
		case PARAM_TM_PHOTOLINEAR_ISO:
		case PARAM_TM_PHOTOLINEAR_FSTOP:
		case PARAM_TM_PHOTOLINEAR_SHUTTERSPEED:
		case PARAM_TM_SIMPLIFIED_EXPOSURE:
		case PARAM_TM_SIMPLIFIED_CONTRAST:
		case PARAM_TM_SIMPLIFIED_WHITEBALANCE:
		case PARAM_TM_SIMPLIFIED_TINTCOLOR:
			if (mSynch->mToneMapperRunning)
				mSynch->InsertModifyRPRToneMapper();
		break;
	}
}

void BgPropertyCallback::propChangedCallback(class ManagerMaxBase *owner, int paramId, int sender)
{
	switch (paramId)
	{
		case PARAM_BG_USE:
			// intentionally blank. we infer the use of rpr background from the presence of
			// (or lack hereof) a rpr environment node in the scene
			break;

		case PARAM_BG_ENABLEALPHA:
		{
			BOOL enabled;
			BgManagerMax::TheManager.GetProperty(PARAM_BG_ENABLEALPHA, enabled);
			if (enabled)
				mSynch->InsertEnableAlphaChannelCommand();
			else
				mSynch->InsertDisableAlphaChannelCommand();
		}
		break;

		case PARAM_BG_TYPE:
		case PARAM_BG_IBL_SOLIDCOLOR:
		case PARAM_BG_IBL_INTENSITY:
		case PARAM_BG_IBL_MAP:
		case PARAM_BG_IBL_BACKPLATE:
		case PARAM_BG_IBL_REFLECTIONMAP:
		case PARAM_BG_IBL_REFRACTIONMAP:
		case PARAM_BG_IBL_MAP_USE:
		case PARAM_BG_IBL_BACKPLATE_USE:
		case PARAM_BG_IBL_REFLECTIONMAP_USE:
		case PARAM_BG_IBL_REFRACTIONMAP_USE:

		case PARAM_BG_SKY_TYPE:
		case PARAM_BG_SKY_AZIMUTH:
		case PARAM_BG_SKY_ALTITUDE:
		case PARAM_BG_SKY_ALBEDO:
		case PARAM_BG_SKY_TURBIDITY:
		case PARAM_BG_SKY_HOURS:
		case PARAM_BG_SKY_MINUTES:
		case PARAM_BG_SKY_SECONDS:
		case PARAM_BG_SKY_DAY:
		case PARAM_BG_SKY_MONTH:
		case PARAM_BG_SKY_YEAR:
		case PARAM_BG_SKY_TIMEZONE:
		case PARAM_BG_SKY_LATITUDE:
		case PARAM_BG_SKY_LONGITUDE:
		case PARAM_BG_SKY_DAYLIGHTSAVING:
		case PARAM_BG_SKY_HAZE:
		case PARAM_BG_SKY_GROUND_COLOR:
		case PARAM_BG_SKY_GROUND_ALBEDO:
		case PARAM_BG_SKY_DISCSIZE:
		case PARAM_BG_SKY_INTENSITY:
		case PARAM_BG_SKY_BACKPLATE:
		case PARAM_BG_SKY_REFLECTIONMAP:
		case PARAM_BG_SKY_REFRACTIONMAP:
		case PARAM_BG_SKY_BACKPLATE_USE:
		case PARAM_BG_SKY_REFLECTIONMAP_USE:
		case PARAM_BG_SKY_REFRACTIONMAP_USE:
		case PARAM_BG_SKY_FILTER_COLOR:
		case PARAM_BG_SKY_SHADOW_SOFTNESS:
		case PARAM_BG_SKY_SATURATION:

		// GROUND
		case PARAM_GROUND_ACTIVE:
		case PARAM_GROUND_RADIUS:
		case PARAM_GROUND_GROUND_HEIGHT:
		case PARAM_GROUND_SHADOWS:
		case PARAM_GROUND_REFLECTIONS_STRENGTH:
		case PARAM_GROUND_REFLECTIONS:
		case PARAM_GROUND_REFLECTIONS_COLOR:
		case PARAM_GROUND_REFLECTIONS_ROUGHNESS:
			mSynch->InsertModifyRPREnvironmentCommand();
		break;
	}
}

int Synchronizer::NumRefs()
{
	return int_cast(mRefsMapI.size());
}

RefTargetHandle Synchronizer::GetReference(int i)
{
	return mRefsMapI[i];
}

void Synchronizer::SetReference(int i, RefTargetHandle rtarg)
{
	mRefsMapI[i] = rtarg;
	mRefsMap[rtarg] = i;
}

int Synchronizer::FindReference(RefTargetHandle hTarg)
{
	const std::map<RefTargetHandle, int>::iterator ii = mRefsMap.find(hTarg);
	if (ii != mRefsMap.end())
		return ii->second;
	return -1;
}

void Synchronizer::AddReference(RefTargetHandle hTarg)
{
	if (FindReference(hTarg) == -1)
		ReplaceReference(NumRefs(), hTarg);
}

void Synchronizer::RemoveReference(RefTargetHandle hTarg)
{
	int i = FindReference(hTarg);
	if (i != -1)
	{
		mRefsMap.erase(hTarg);
		mRefsMapI.erase(i);
	}
}


//////////////////////////////////////////////////////////////////////////////
//
// THIS SECTION DEALS WITH COMMAND QUEUEING (LINING UP CHANGES FOR EXECUTION)
//
// Commands are stored in separate queues (actually hashed structures) each
// belonging to one of the main categories (rebuild object, assign material,
// show/hide, modify trasform matrix, delete).
// These commands are accumulate over a brief period of time to avoid clogging
// the system with continuous expensive updates, and finally executed.
//
// the order of execution is the following:
// rebuild objects
// assign materials
// assign xform
// show/hide
// delete objects

void Synchronizer::InsertRebuildCommand(INode *pNode)
{
	if (pNode)
	{
		// no need to assign materials: when the node is rebuilt, its current materials
		// will be used anywway
		mQueue.Remove(SQUEUE_OBJ_ASSIGNMAT, pNode);
		
		// no need to modify transforms either: when the node is rebuilt, its xform is reset
		mQueue.Remove(SQUEUE_OBJ_XFORM, pNode);

		mQueue.Insert(SQUEUE_OBJ_REBUILD, pNode);

		// rebuild is an expensive operation. when we receive a reequest, we reset the
		// main thread timer, so to avoid multiple rebuilds to occur in case an object
		// is being modified continuosly during an edit operation
		ResetTimerProc();
	}
}

void Synchronizer::InsertAssignMatCommand(INode *pNode)
{
	if (pNode)
	{
		mQueue.Insert(SQUEUE_OBJ_ASSIGNMAT, pNode);
	}
}

void Synchronizer::InsertDeleteCommand(INode *pNode)
{
	if (pNode)
	{
		mQueue.Remove(SQUEUE_OBJ_REBUILD, pNode);
		mQueue.Remove(SQUEUE_OBJ_ASSIGNMAT, pNode);
		mQueue.Remove(SQUEUE_OBJ_SHOW, pNode);
		mQueue.Remove(SQUEUE_OBJ_HIDE, pNode);
		mQueue.Remove(SQUEUE_OBJ_XFORM, pNode);
		mQueue.Insert(SQUEUE_OBJ_DELETE, pNode);
	}
}

void Synchronizer::InsertShowCommand(INode *pNode)
{
	if (pNode)
	{
		mQueue.Remove(SQUEUE_OBJ_HIDE, pNode);
		mQueue.Insert(SQUEUE_OBJ_SHOW, pNode);
	}
}

void Synchronizer::InsertHideCommand(INode *pNode)
{
	if (pNode)
	{
		mQueue.Remove(SQUEUE_OBJ_SHOW, pNode);
		mQueue.Insert(SQUEUE_OBJ_HIDE, pNode);
	}
}

void Synchronizer::InsertXformCommand(INode *pNode)
{
	if (pNode)
	{
		mQueue.Insert(SQUEUE_OBJ_XFORM, pNode);
	}
}

void Synchronizer::InsertMatCommand(Mtl *pMat)
{
	if (pMat)
	{
		mQueue.Insert(SQUEUE_MAT_REBUILD, pMat);
	}
}

void Synchronizer::InsertStartRPREnvironmentCommand()
{
	mQueue.Insert(SQUEUE_BG_RPR_START);
}

void Synchronizer::InsertModifyRPREnvironmentCommand()
{
	mQueue.Insert(SQUEUE_BG_RPR_MODIFY);
}

void Synchronizer::InsertStopRPREnvironmentCommand()
{
	mQueue.Remove(SQUEUE_BG_RPR_START);
	mQueue.Insert(SQUEUE_BG_RPR_STOP);
}

void Synchronizer::InsertStartRPRToneMapper()
{
	mQueue.Insert(SQUEUE_RPRTONEMAP_START);
}

void Synchronizer::InsertModifyRPRToneMapper()
{
	mQueue.Insert(SQUEUE_RPRTONEMAP_MODIFY);
}

void Synchronizer::InsertStopRPRToneMapper()
{
	mQueue.Remove(SQUEUE_RPRTONEMAP_START);
	mQueue.Remove(SQUEUE_RPRTONEMAP_MODIFY);
	mQueue.Insert(SQUEUE_RPRTONEMAP_STOP);
}

void Synchronizer::InsertEnableAlphaChannelCommand()
{
	mQueue.Remove(SQUEUE_ALPHA_DISABLE);
	mQueue.Insert(SQUEUE_ALPHA_ENABLE);
}

void Synchronizer::InsertDisableAlphaChannelCommand()
{
	mQueue.Remove(SQUEUE_ALPHA_ENABLE);
	mQueue.Insert(SQUEUE_ALPHA_DISABLE);
}

//////////////////////////////////////////////////////////////////////////////
//
// THIS SECTION CONTAINS THE CODE THAT EXECUTES CHANGES
//

void Synchronizer::Start()
{
	FASSERT(mRunning == false);
	mRunning = true;
	mFirstRun = true;

	// Prepares the object for adding items. It has to be called before any call to addItem()
	// Is called only once (during first run)
	callbacks.beforeParsing(mBridge->t());

	mUITimerId = SetTimer(GetCOREInterface()->GetMAXHWnd(), UINT_PTR(this), CUI_TIMER_PERIOD, UITimerProc);
}

void Synchronizer::Stop()
{
	if (mRunning)
	{
		if (mUITimerId)
		{
			KillTimer(GetCOREInterface()->GetMAXHWnd(), mUITimerId);
			mUITimerId = 0;
		}
		mRunning = false;
	}

	callbacks.afterParsing(); // this should be done only on the end of rendering! (according to Max SDK)
}

namespace
{
	void traverseMaterialCallback(SceneCallbacks &callbacks, Mtl* material)
	{
		if (!material)
			return;
		callbacks.addItem(material);
		for (int i = 0; i < material->NumSubMtls(); ++i)
			traverseMaterialCallback(callbacks, material->GetSubMtl(i));
	}
}

// This function is executed in main thread, and it's called with a frequency of CUI_TIMER_PERIOD milliseconds.
// It checks for the presence of stuff in the change queues, and synchronizes RPR accordingly. It also checks
// for other scene changes (ie background and settings)
//
VOID CALLBACK Synchronizer::UITimerProc(_In_ HWND hwnd, _In_ UINT uMsg, _In_ UINT_PTR idEvent, _In_ DWORD dwTime)
{
	class AutoTimerReset
	{
	private:
		Synchronizer *mSynch;
	public:
		AutoTimerReset(Synchronizer *pSynch)
			: mSynch(pSynch)
		{
			if (mSynch->mUITimerId)
			{
				KillTimer(GetCOREInterface()->GetMAXHWnd(), mSynch->mUITimerId);
				mSynch->mUITimerId = 0;
			}
		}
		~AutoTimerReset()
		{
			mSynch->mUITimerId = SetTimer(GetCOREInterface()->GetMAXHWnd(), UINT_PTR(mSynch), CUI_TIMER_PERIOD, Synchronizer::UITimerProc);
		}
	};

	class ScopeLockRenderThread
	{
	private:
		Synchronizer *mSynch;
		bool mLocked;
	public:
		ScopeLockRenderThread(Synchronizer *pSynch)
			: mSynch(pSynch), mLocked(true)
		{
			mSynch->mBridge->LockRenderThread();
		}

		~ScopeLockRenderThread()
		{
			if (mLocked)
				mSynch->mBridge->UnlockRenderThread();
		}
		
		void Unlock()
		{
			if (mLocked)
			{
				mSynch->mBridge->UnlockRenderThread();
				mLocked = false;
			}
		}
	};


	Synchronizer *synch = reinterpret_cast<Synchronizer*>(idEvent);

	if (!synch->mBridge->RenderThreadAlive())
		return; // stop

	if( synch->mNodeEventCallbackKey != -1 )
		GetISceneEventManager()->TriggerMessages(synch->mNodeEventCallbackKey);

	AutoTimerReset TimerReset(synch);
				
	synch->mMasterScale = float(GetMasterScale(UNITS_METERS));

	bool ClearEvent = synch->mQueue.ClearEvent();
	bool TonemapEvent = synch->mQueue.TonemapEvent();

	std::vector<int> renderChanges;
	synch->mParametersTracker.CollectChanges(renderChanges);
	if (!renderChanges.empty())
		ClearEvent = true;

	
	ScopeLockRenderThread RenderThreadLock(synch);

	// check whether MAX environment needs update
	bool MaxEnvironmentNeedsUpdate = false;
	if (!BgManagerMax::TheManager.GetEnvironmentNode())
		MaxEnvironmentNeedsUpdate = synch->MAXEnvironmentNeedsUpdate();
	if (MaxEnvironmentNeedsUpdate)
		ClearEvent = true;

	// check wether we need default light(s)
	bool hasLightSources =
		(!synch->mBgLight.empty()) || (!synch->mEmissives.empty()) ||
		(!synch->mLights.empty()) || (!synch->mLightShapes.empty());
	bool removeDefaultLights = false;
	bool addDefaultLights = false;
	if (synch->mUsingDefaultLights && hasLightSources)
	{
		ClearEvent = true;
		removeDefaultLights = true;
	}
	if (!synch->mUsingDefaultLights && !hasLightSources)
	{
		ClearEvent = true;
		addDefaultLights = true;
	}
	
	if (ClearEvent || TonemapEvent)
	{
		std::map<AnimHandle, std::list<INode *>> instances;
		int totalGeoms = 0;
		std::vector<INode*> nodesToRebuild;

		if (synch->mBridge->GetProgressCB())
			synch->mBridge->GetProgressCB()->SetTitle(_T("Synchronizing..."));

		if (!renderChanges.empty())
			synch->UpdateRenderSettings(renderChanges);

		// process object rebuild commands first
		for (auto ii = synch->mQueue.begin(); ii != synch->mQueue.end(); ii++)
		{
			switch (ii->Code())
			{
			case SQUEUE_OBJ_REBUILD:
			{
				auto node = ii->Node();
				try
				{
					ObjectState state;
					try
					{
						state = node->EvalWorldState(synch->mBridge->t());
					}
					catch (...)
					{
						continue;
					}

					if (!state.obj || !node->Renderable())
						continue;

					const SClass_ID sClassId = state.obj->SuperClassID();
					const Class_ID classId = state.obj->ClassID();

					// We specifically ignore some objects that either render garbage, or crash when we try to render them:
					// CAT parent: Class ID 0x56ae72e5, 0x389b6659
					// Delegate helper: Class ID 0x40c07baa, 0x245c7fe6
					if (classId == Class_ID(0x56ae72e5, 0x389b6659) || classId == Class_ID(0x40c07baa, 0x245c7fe6))
						continue;

					if (classId == Corona::LIGHT_CID || sClassId == GEOMOBJECT_CLASS_ID || state.obj->CanConvertToType(triObjectClassID) != FALSE)
					{
						// make sure object is rendereable and ignore zero-scale/degenerated
						// TM objects (which will have determinant 0)
						auto tm = node->GetObjTMAfterWSM(synch->mBridge->t());
						if (state.obj->IsRenderable() && GetDeterminant(tm) != 0.f)
						{
							// We sort all the geometry objects according to their Animatable handle, not by raw pointers. This is because while object 
							// addresses may get reused (the result of EvalWorldState may be a temporary object that is deleted as soon as another 
							// object in the scene is evaluated), AnimHandle is always unique. This way we dont get any false "instancing" of two 
							// different objects caused by random evaluated object address being aliased with previous address of already deleted object
							auto handle = Animatable::GetHandleByAnim(state.obj);
							FASSERT(handle != 0);

							totalGeoms++;

							instances[handle].push_back(node);
						}
					}
					else if ((classId == FireRenderIESLight::GetClassId()) || 
							(classId == FIRERENDER_PHYSICALLIGHT_CLASS_ID))
					{
						FireRenderLight* light = dynamic_cast<FireRenderLight*>(state.obj);
						if (light && light->GetUseLight())
						{
							ActiveShadeAttachCallback callback(synch);
							TimeValue t = synch->mBridge->t();
							auto tm = node->GetObjTMAfterWSM(t);
							tm.SetTrans(tm.GetTrans() * synch->mMasterScale);
							size_t id = 0;
							ParsedNode parsedNode(id,node,tm);
							light->CreateSceneLight( t, parsedNode, synch->mScope, &callback );
						}
					}
					else if ((sClassId == LIGHT_CLASS_ID) && (classId != Corona::LIGHT_CID) && (classId != FIRERENDER_ENVIRONMENT_CLASS_ID)
						&& (classId != FIRERENDER_ANALYTICALSUN_CLASS_ID) && (classId != FIRERENDER_PORTALLIGHT_CLASS_ID))
					{
						if (synch->mBridge->GetProgressCB())
							synch->mBridge->GetProgressCB()->SetTitle(_T("Synchronizing: Rebuilding Light"));
						// A general light object, but not Corona Light (which we parse as a geometry)
						synch->RebuildMaxLight(node, state.obj);
					}
					else if (classId == Corona::SUN_OBJECT_CID)
					{
						if (ScopeManagerMax::CoronaOK)
						synch->RebuildCoronaSun(node, state.obj);
					}
					else if (classId == FIRERENDER_PORTALLIGHT_CLASS_ID)
					{
						synch->RebuildFRPortal(node, state.obj);
					}
				}
				catch (...)
				{
					// EvalWorldState might fail: drop the operation
				}
			}
			break;
			}
		}

		// object rebuilding: progress report
		{
			int numInstances = int_cast(instances.size());
			wchar_t tempStr[1024];
			int i = 1;
			for (auto ii : instances)
			{
				for (auto jj : ii.second)
				{
					synch->callbacks.addItem(jj); // traverses the node tree, adds nodes to callbacks and calls update on materials
					if (jj->GetMtl())
						traverseMaterialCallback(synch->callbacks, jj->GetMtl()); // traverses the tree a little bit differently (but calls addItem() still)
				}
				wsprintf(tempStr, L"Synchronizing: Rebuilding Object %d of %d (%s)", i++, numInstances, (*ii.second.begin())->GetName());
				if (synch->mBridge->GetProgressCB())
					synch->mBridge->GetProgressCB()->SetTitle(tempStr);
				synch->RebuildGeometry(ii.second);
			}
		}

		// process remaining commands
		for (auto ii = synch->mQueue.begin(); ii != synch->mQueue.end(); ii++)
		{
			switch (ii->Code())
			{
			case SQUEUE_OBJ_DELETE:
			{
				if (synch->mBridge->GetProgressCB())
					synch->mBridge->GetProgressCB()->SetTitle(_T("Synchronizing: Deleting Object"));

				auto ss = synch->mShapes.find(ii->Node());
				if (ss != synch->mShapes.end())
				{
					synch->DeleteGeometry(ii->Node());
				}
				else
				{
					if (ii->Node() == BgManagerMax::TheManager.GetEnvironmentNode())
						synch->RemoveEnvironment();
					else
					{
						auto ss = synch->mLights.find(ii->Node());
						if (ss != synch->mLights.end())
						{
							synch->mScope.GetScene().Detach(ss->second->Get());
							synch->mLights.erase(ss);
						}
						else
						{
							auto ss = synch->mLightShapes.find(ii->Node());
							if (ss != synch->mLightShapes.end())
							{
								synch->mScope.GetScene().Detach(ss->second->Get());
								synch->mLightShapes.erase(ss);
							}

						}
					}
					// check for portals
				}
			}
			break;

			case SQUEUE_OBJ_ASSIGNMAT:
			{
				if (!synch->mFirstRun)
				{
					if (synch->mBridge->GetProgressCB())
						synch->mBridge->GetProgressCB()->SetTitle(_T("Synchronizing: Assigning Material"));
					if (!synch->ResetMaterial(ii->Node()))
						nodesToRebuild.push_back(ii->Node());
				}
			}
			break;

			case SQUEUE_OBJ_SHOW:
			{
				if (synch->mBridge->GetProgressCB())
					synch->mBridge->GetProgressCB()->SetTitle(_T("Synchronizing: Updating Visibility"));
				if (!synch->Show(ii->Node()))
					nodesToRebuild.push_back(ii->Node());
			}
			break;

			case SQUEUE_OBJ_HIDE:
			{
				if (synch->mBridge->GetProgressCB())
					synch->mBridge->GetProgressCB()->SetTitle(_T("Synchronizing: Updating Visibility"));
				synch->Hide(ii->Node());
			}
			break;

			case SQUEUE_OBJ_XFORM:
			{
				if (synch->mBridge->GetProgressCB())
					synch->mBridge->GetProgressCB()->SetTitle(_T("Synchronizing: Updating XForm"));

				auto node = ii->Node();

				auto ss = synch->mShapes.find(node);
				if (ss != synch->mShapes.end())
				{
					auto tm = node->GetObjTMAfterWSM(synch->mBridge->t());
					tm.SetTrans(tm.GetTrans() * synch->mMasterScale);
					for (auto si : ss->second)
						si->Get().SetTransform(tm);
				}
				else
				{
					auto sl = synch->mLightShapes.find(node);
					if (sl != synch->mLightShapes.end())
					{
						auto tm = node->GetObjTMAfterWSM(synch->mBridge->t());
						tm.SetTrans(tm.GetTrans() * synch->mMasterScale);
						sl->second->Get().SetTransform(tm);
					}
					else
					{
						auto ll = synch->mLights.find(node);
						if (ll != synch->mLights.end())
						{
							auto tm = node->GetObjTMAfterWSM(synch->mBridge->t());
							tm.SetTrans(tm.GetTrans() * synch->mMasterScale);
							ll->second->Get().SetTransform(tm);
						}
						else
						{
							auto pp = synch->mPortals.find(node);
							if (pp != synch->mPortals.end())
							{
								auto tm = node->GetObjTMAfterWSM(synch->mBridge->t());
								tm.SetTrans(tm.GetTrans() * synch->mMasterScale);
								pp->second.SetTransform(tm);
							}
							else
							{
								// This code needs to be checked. Do not remove.
								//if (ii == synch->mRPREnvironment)
								//synch->UpdateRPREnvironment();
								//synch->UpdateEnvXForm(ii);
							}
						}
					}
				}
			}
			break;

			case SQUEUE_MAT_REBUILD:
			{
				if (!synch->mFirstRun)
				{
					auto mat = ii->Mat();
					if (synch->mBridge->GetProgressCB())
						synch->mBridge->GetProgressCB()->SetTitle(_T("Synchronizing: Updating Material"));
					synch->UpdateMaterial(mat, nodesToRebuild);
				}
			}
			break;

			case SQUEUE_RPRTONEMAP_START:
			{
				synch->mBridge->StartToneMapper();
				synch->mToneMapperRunning = true;
			}
			break;

			case SQUEUE_RPRTONEMAP_MODIFY:
			{
				synch->UpdateToneMapper();
			}
			break;

			case SQUEUE_RPRTONEMAP_STOP:
			{
				synch->mBridge->StopToneMapper();
				synch->mToneMapperRunning = false;
			}
			break;

			case SQUEUE_BG_RPR_START:
			{
				synch->AddRPREnvironment();
			}
			break;

			case SQUEUE_BG_RPR_MODIFY:
			{
				synch->UpdateRPREnvironment();
			}
			break;

			case SQUEUE_BG_RPR_STOP:
			{
				synch->RemoveRPREnvironment();
			}
			break;

			case SQUEUE_ALPHA_ENABLE:
			{
				synch->mBridge->EnableAlphaBuffer();
			}
			break;

			case SQUEUE_ALPHA_DISABLE:
			{
				synch->mBridge->DisableAlphaBuffer();
			}
			break;

			}
		}

		synch->mQueue.clear();

		synch->mFirstRun = false;
		
		// we may have collected a few nodes that need rebuilding
		if (!nodesToRebuild.empty())
		{
			for (auto ii : nodesToRebuild)
				synch->InsertRebuildCommand(ii);
		}
				
		// check wether we need to use max environment
		if (MaxEnvironmentNeedsUpdate)
			synch->UpdateMAXEnvironment();

		// check wether we need to use or stop using default lights
		if (addDefaultLights)
		{
			synch->mUsingDefaultLights = true;
			synch->AddDefaultLights();
		}
		else if (removeDefaultLights)
		{
			synch->mUsingDefaultLights = false;
			synch->RemoveDefaultLights();
		}
		
		if (ClearEvent)
			synch->mBridge->ClearFB();
		else
			synch->mBridge->ResetInteractiveTermination();

		if (synch->mBridge->GetProgressCB())
			synch->mBridge->GetProgressCB()->SetTitle(_T(""));
	}

	RenderThreadLock.Unlock();

	synch->CustomCPUSideSynch();

	synch->mBridge->GetProgressCB()->SetTitle(_T(""));
}

void Synchronizer::ResetTimerProc()
{
	if (mRunning && mUITimerId)
	{
		KillTimer(GetCOREInterface()->GetMAXHWnd(), mUITimerId);
		mUITimerId = SetTimer(GetCOREInterface()->GetMAXHWnd(), UINT_PTR(this), CUI_TIMER_PERIOD, UITimerProc);
	}
}

//////////////////////////////////////////////////////////////////////////////
// Generic Parameters Tracker

bool ParamsTracker::AddToTracker(int param)
{
	auto pblock = mSynchronizer->mBridge->GetPBlock();
	auto t = mSynchronizer->mBridge->t();
	auto def = pblock->GetParamDef(param);

	Value v;
	v.mType = def.type;

	bool res = true;

	switch (def.type)
	{
	case TYPE_ANGLE:
	case TYPE_FLOAT:
	case TYPE_WORLD:
	{
		float p;
		pblock->GetValue(param, t, p, FOREVER);
		v.mSimpleValue.float_val = p;
	}
	break;

	case TYPE_INT:
	{
		int p;
		pblock->GetValue(param, t, p, FOREVER);
		v.mSimpleValue.int_val = p;
	}
	break;

	case TYPE_BOOL:
	{
		BOOL p;
		pblock->GetValue(param, t, p, FOREVER);
		v.mSimpleValue.int_val = p;
	}
	break;

	case TYPE_COLOR:
	{
		Color p;
		pblock->GetValue(param, t, p, FOREVER);
		v.mColor.r = p.r;
		v.mColor.g = p.g;
		v.mColor.b = p.b;
	}
	break;

	case TYPE_RGBA:
	{
		AColor p;
		pblock->GetValue(param, t, p, FOREVER);
		v.mColor.a = p.a;
		v.mColor.r = p.r;
		v.mColor.g = p.g;
		v.mColor.b = p.b;
	}
	break;
	
	case TYPE_TEXMAP:
	{
		Texmap *p;
		pblock->GetValue(param, t, p, FOREVER);
		v.mSimpleValue.tex_val = p;
	}
	break;

	case TYPE_BITMAP:
	{
		PBBitmap *p;
		pblock->GetValue(param, t, p, FOREVER);
		v.mSimpleValue.bitmap_val = p;
	}
	break;

	case TYPE_MTL:
	{
		Mtl *p;
		pblock->GetValue(param, t, p, FOREVER);
		v.mSimpleValue.mtl_val = p;
	}
	break;

	case TYPE_INODE:
	{
		INode *p;
		pblock->GetValue(param, t, p, FOREVER);
		v.mSimpleValue.node_val = p;
	}
	break;

	case TYPE_MATRIX3:
	{
		Matrix3 p;
		pblock->GetValue(param, t, p, FOREVER);
		v.mMatrix = p;
	}
	break;
	
	case TYPE_POINT3:
	{
		Point3 p;
		pblock->GetValue(param, t, p, FOREVER);
		v.mPoint.x = p.x;
		v.mPoint.y = p.y;
		v.mPoint.z = p.z;
	}
	break;

	case TYPE_POINT4:
	{
		Point4 p;
		pblock->GetValue(param, t, p, FOREVER);
		v.mPoint.x = p.x;
		v.mPoint.y = p.y;
		v.mPoint.z = p.z;
		v.mPoint.w = p.w;
	}
	break;

	default:
		res = false;
		break;
	}

	if (res)
		mTracker.insert(std::make_pair(param, v));

	return res;
}

void ParamsTracker::CollectChanges(std::vector<int>& res)
{
	auto pblock = mSynchronizer->mBridge->GetPBlock();
	
	if (!pblock)
		return;

	auto t = mSynchronizer->mBridge->t();

	for (auto ii = mTracker.begin(); ii != mTracker.end(); ii++)
	{
		switch (ii->second.mType)
		{
		case TYPE_ANGLE:
		case TYPE_FLOAT:
		case TYPE_WORLD:
		{
			float p;
			pblock->GetValue(ii->first, t, p, FOREVER);
			if (ii->second.mSimpleValue.float_val != p)
			{
				ii->second.mSimpleValue.float_val = p;
				res.push_back(ii->first);
			}
		}
		break;

		case TYPE_INT:
		{
			int p;
			pblock->GetValue(ii->first, t, p, FOREVER);
			if (ii->second.mSimpleValue.int_val != p)
			{
				ii->second.mSimpleValue.int_val = p;
				res.push_back(ii->first);
			}
		}
		break;

		case TYPE_BOOL:
		{
			BOOL p;
			pblock->GetValue(ii->first, t, p, FOREVER);
			if (ii->second.mSimpleValue.int_val != p)
			{
				ii->second.mSimpleValue.int_val = p;
				res.push_back(ii->first);
			}
		}
		break;

		case TYPE_COLOR:
		{
			Color p;
			pblock->GetValue(ii->first, t, p, FOREVER);
			if (ii->second.mColor.r != p.r || ii->second.mColor.g != p.g || ii->second.mColor.b != p.b)
			{
				ii->second.mColor.r = p.r;
				ii->second.mColor.g = p.g;
				ii->second.mColor.g = p.b;
				res.push_back(ii->first);
			}
		}
		break;

		case TYPE_RGBA:
		{
			AColor p;
			pblock->GetValue(ii->first, t, p, FOREVER);
			if (ii->second.mColor.r != p.r || ii->second.mColor.g != p.g || ii->second.mColor.b != p.b || ii->second.mColor.a != p.a)
			{
				ii->second.mColor.r = p.r;
				ii->second.mColor.g = p.g;
				ii->second.mColor.g = p.b;
				ii->second.mColor.a = p.a;
				res.push_back(ii->first);
			}
		}
		break;

		case TYPE_TEXMAP:
		{
			Texmap *p;
			pblock->GetValue(ii->first, t, p, FOREVER);
			if (ii->second.mSimpleValue.tex_val != p)
			{
				ii->second.mSimpleValue.tex_val = p;
				res.push_back(ii->first);
			}
		}
		break;

		case TYPE_BITMAP:
		{
			PBBitmap *p;
			pblock->GetValue(ii->first, t, p, FOREVER);
			if (ii->second.mSimpleValue.bitmap_val != p)
			{
				ii->second.mSimpleValue.bitmap_val = p;
				res.push_back(ii->first);
			}
		}
		break;

		case TYPE_MTL:
		{
			Mtl *p;
			pblock->GetValue(ii->first, t, p, FOREVER);
			if (ii->second.mSimpleValue.mtl_val != p)
			{
				ii->second.mSimpleValue.mtl_val = p;
				res.push_back(ii->first);
			}
		}
		break;

		case TYPE_INODE:
		{
			INode *p;
			pblock->GetValue(ii->first, t, p, FOREVER);
			if (ii->second.mSimpleValue.node_val != p)
			{
				ii->second.mSimpleValue.node_val = p;
				res.push_back(ii->first);
			}
		}
		break;

		case TYPE_MATRIX3:
		{
			Matrix3 p;
			pblock->GetValue(ii->first, t, p, FOREVER);
			if (ii->second.mMatrix != p)
			{
				ii->second.mMatrix = p;
				res.push_back(ii->first);
			}
			
		}
		break;

		case TYPE_POINT3:
		{
			Point3 p;
			pblock->GetValue(ii->first, t, p, FOREVER);
			if (ii->second.mPoint.x != p.x || ii->second.mPoint.y != p.y || ii->second.mPoint.z != p.z)
			{
				ii->second.mPoint.x = p.x;
				ii->second.mPoint.y = p.y;
				ii->second.mPoint.z = p.z;
				res.push_back(ii->first);
			}
		}
		break;

		case TYPE_POINT4:
		{
			Point4 p;
			pblock->GetValue(ii->first, t, p, FOREVER);
			if (ii->second.mPoint.x != p.x || ii->second.mPoint.y != p.y || ii->second.mPoint.z != p.z || ii->second.mPoint.w != p.w)
			{
				ii->second.mPoint.x = p.x;
				ii->second.mPoint.y = p.y;
				ii->second.mPoint.z = p.z;
				ii->second.mPoint.w = p.w;
				res.push_back(ii->first);
			}
		}
		break;
		}
	}
}

FIRERENDER_NAMESPACE_END;
