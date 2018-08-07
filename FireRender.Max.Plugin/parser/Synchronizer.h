/*********************************************************************************************************************************
* Radeon ProRender for 3ds Max plugin
* Copyright (c) 2017 AMD
* All Rights Reserved
*
* Synchronizer is the class that keeps track of scene changes and allows ActiveShader to react in realtime
*********************************************************************************************************************************/

#pragma once

#include "frWrap.h"
#include "Common.h"
#include <interactiverender.h>
#include <RadeonProRender.h>
#include "MaterialParser.h"
#include "../utils/Thread.h"
#include "plugin/ManagerBase.h"
#include <vector>
#include <string>
#include <map>
#include <queue>
#include <memory>
#include <FrScope.h>
#include "SceneCallbacks.h"
#include "plugin/light/FireRenderIESLight.h"
#include "plugin/light/physical/FireRenderPhysicalLight.h"

FIRERENDER_NAMESPACE_BEGIN;

class SynchronizerBridge
{
public:
	virtual const TimeValue t() = 0;
	virtual void LockRenderThread() = 0;
	virtual void UnlockRenderThread() = 0;
	virtual void StartToneMapper() = 0;
	virtual void StopToneMapper() = 0;
	virtual void ClearFB() = 0; // resets frame buffer when something changes
	virtual IRenderProgressCallback *GetProgressCB() = 0; // gets the progress callback
	virtual IParamBlock2 *GetPBlock() = 0; // gets the renderer's parameters block
	virtual bool RenderThreadAlive() = 0;
	virtual const DefaultLight *GetDefaultLights(int &numDefLights) const = 0;
	virtual void EnableAlphaBuffer() = 0; // enables the apha AOV framebuffer
	virtual void DisableAlphaBuffer() = 0; // disables the apha AOV framebuffer
	virtual void SetTimeLimit(int val) = 0; // sets the termination's time limit
	virtual void SetPassLimit(int val) = 0; // sets the termination's passes limit
	virtual void SetLimitType(int type) = 0; // sets the termination type
	virtual void ResetInteractiveTermination() = 0; // resets the termination if interactive renderer is ongoing
};

class SBase
{
public:
	Mtl *mMtl;
	INode *mParent;

	SBase()
	{
		mMtl = 0;
		mParent = 0;
	}
	
	inline bool IsParent(const INode *p) const
	{
		return (p == mParent);
	}

	inline bool IsMaterial(const Mtl *m) const
	{
		return (m == mMtl);
	}
};

class SShape : public SBase
{
public:
	frw::Shape mShape;

	SShape(frw::Shape s)
		: mShape(s)
	{
		mParent = 0;
		mMtl = 0;
	}

	SShape(frw::Shape s, INode *p)
		: mShape(s)
	{
		mParent = p;
		mMtl = 0;
	}

	SShape(frw::Shape s, INode *p, Mtl *m)
		: mShape(s)
	{
		mParent = p;
		mMtl = m;
	}

	inline SShape & operator = (const SShape & left)
	{
		mShape = left.mShape;
		mParent = left.mParent;
		mMtl = left.mMtl;
		return *this;
	}

	inline bool operator == (const SShape & left) const
	{
		return mShape.Handle() == left.mShape.Handle();
	}

	inline frw::Shape & Get()
	{
		return mShape;
	}
};

inline bool operator ==  (const SShape & right, const SShape & left)
{
	return right.mShape.Handle() == left.mShape.Handle();
}

class SLight : public SBase
{
public:
	frw::Light mLight;

	SLight(frw::Light s)
		: mLight(s)
	{
		mMtl = 0;
		mParent = 0;
	}

	SLight(frw::Light s, INode *p)
		: mLight(s)
	{
		mMtl = 0;
		mParent = p;
	}

	SLight(frw::Light s, INode *p, Mtl *m)
		: mLight(s)
	{
		mMtl = m;
		mParent = p;
	}

	inline SLight & operator = (const SLight & left)
	{
		mLight = left.mLight;
		mParent = left.mParent;
		mMtl = left.mMtl;
		return *this;
	}

	inline bool operator == (const SLight & left) const
	{
		return mLight.Handle() == left.mLight.Handle();
	}

	inline frw::Light & Get()
	{
		return mLight;
	}
};

inline bool operator ==  (const SLight & right, const SLight & left)
{
	return right.mLight.Handle() == left.mLight.Handle();
}

typedef std::shared_ptr<SLight> SLightPtr;
typedef std::shared_ptr<SShape> SShapePtr;

class TMPropertyCallback : public ManagerPropertyChangeCallback
{
private:
	class Synchronizer *mSynch;

public:
	TMPropertyCallback(class Synchronizer *synch)
	: mSynch(synch)
	{
	}

	void propChangedCallback(class ManagerMaxBase *owner, int paramId, int sender) override;
};

class BgPropertyCallback : public ManagerPropertyChangeCallback
{
private:
	class Synchronizer *mSynch;

public:
	BgPropertyCallback(class Synchronizer *synch)
		: mSynch(synch)
	{
	}

	void propChangedCallback(class ManagerMaxBase *owner, int paramId, int sender) override;
};

//////////////////////////////////////////////////////////////////////////////
// The command queue accumulates scene changes which are committed
// to the rendering engine via a timed function on the main thread (MAX is
// not thread safe)
//

enum
{
	SQUEUE_OBJ_REBUILD,
	SQUEUE_OBJ_DELETE,
	SQUEUE_OBJ_ASSIGNMAT,
	SQUEUE_OBJ_SHOW,
	SQUEUE_OBJ_HIDE,
	SQUEUE_OBJ_XFORM,
	SQUEUE_MAT_REBUILD,
	SQUEUE_RPRTONEMAP_START,
	SQUEUE_RPRTONEMAP_MODIFY,
	SQUEUE_RPRTONEMAP_STOP,
	SQUEUE_BG_RPR_START,
	SQUEUE_BG_RPR_MODIFY,
	SQUEUE_BG_RPR_STOP,
	SQUEUE_ALPHA_ENABLE,
	SQUEUE_ALPHA_DISABLE
};

class SQueueItem
{
private:
	int mCode;
	INode *mNode;
	Mtl *mMat;

public:
	SQueueItem(int code)
		: mCode(code)
		, mNode(0)
		, mMat(0)
	{
	}

	SQueueItem(int code, INode *node)
		: mCode(code)
		, mNode(node)
		, mMat(0)
	{
	}

	SQueueItem(int code, Mtl *mat)
		: mCode(code)
		, mMat(mat)
		, mNode(0)
	{
	}

	inline const SQueueItem & operator = (const SQueueItem &other)
	{
		mCode = other.mCode;
		mNode = other.mNode;
		mMat = other.mMat;
		return *this;
	}

	inline int Code() const
	{
		return mCode;
	}

	inline INode *Node() const
	{
		return mNode;
	}

	inline Mtl *Mat() const
	{
		return mMat;
	}
};

typedef std::list<SQueueItem> SQueueItemList;
typedef std::list<SQueueItem>::iterator SQueueItemIt;
typedef std::list<SQueueItem>::const_iterator Const_SQueueItemIt;

class SQueue
{
private:
	SQueueItemList mList;
	std::map<INode*, std::map<int, SQueueItemIt>> mNodeItems;
	std::map<Mtl*, std::map<int, SQueueItemIt>> mMtlItems;
	std::map<int, SQueueItemIt> mActionItems;

	int mClearEvent;
	int mTonemapEvent;

public:
	SQueue()
	: mClearEvent(0)
	, mTonemapEvent(0)
	{
	}

	~SQueue()
	{
	}

	inline void Remove(int code)
	{
		auto ii = mActionItems.find(code);
		if (ii != mActionItems.end())
		{
			mList.erase(ii->second);
			mActionItems.erase(ii);
			if (code == SQUEUE_RPRTONEMAP_MODIFY || code == SQUEUE_RPRTONEMAP_START || code == SQUEUE_RPRTONEMAP_STOP)
				mTonemapEvent--;
			else
				mClearEvent--;
		}
	}

	inline void Remove(int code, INode *node)
	{
		auto ii = mNodeItems.find(node);
		if (ii != mNodeItems.end())
		{
			auto cc = ii->second.find(code);
			if (cc != ii->second.end())
			{
				mList.erase(cc->second);
				ii->second.erase(cc);
				if (ii->second.empty())
					mNodeItems.erase(ii);
				mClearEvent--;
			}
		}
	}

	inline void Remove(int code, Mtl *mtl)
	{
		auto ii = mMtlItems.find(mtl);
		if (ii != mMtlItems.end())
		{
			auto cc = ii->second.find(code);
			if (cc != ii->second.end())
			{
				mList.erase(cc->second);
				ii->second.erase(cc);
				if (ii->second.empty())
					mMtlItems.erase(ii);
				mClearEvent--;
			}
		}
	}

	inline void Insert(int code)
	{
		Remove(code);
		SQueueItem item(code);
		mList.push_front(item);
		
		SQueueItemIt it= mList.begin();
		mActionItems.insert(std::make_pair(code, it));

		if (code == SQUEUE_RPRTONEMAP_MODIFY || code == SQUEUE_RPRTONEMAP_START || code == SQUEUE_RPRTONEMAP_STOP)
			mTonemapEvent++;
		else
			mClearEvent++;
	}

	inline void Insert(int code, INode *node)
	{
		Remove(code, node);
		SQueueItem item(code, node);
		mList.push_front(item);

		SQueueItemIt it = mList.begin();
		mNodeItems[node].insert(std::make_pair(code, it));
		
		mClearEvent++;
	}

	inline void Insert(int code, Mtl *mtl)
	{
		Remove(code, mtl);
		SQueueItem item(code, mtl);
		mList.push_front(item);

		SQueueItemIt it = mList.begin();
		mMtlItems[mtl].insert(std::make_pair(code, it));
		
		mClearEvent++;
	}
	
	Const_SQueueItemIt begin() const
	{
		return mList.begin();
	}

	Const_SQueueItemIt end() const
	{
		return mList.end();
	}

	void clear()
	{
		mList.clear();
		mActionItems.clear();
		mNodeItems.clear();
		mMtlItems.clear();
		mClearEvent = 0;
		mTonemapEvent = 0;
	}

	inline bool ClearEvent() const
	{
		return mClearEvent > 0;
	}

	inline bool TonemapEvent() const
	{
		return mTonemapEvent > 0;
	}
};

//////////////////////////////////////////////////////////////////////////////
// The ParamsTracker class tracks generic/advanced rendering parameter changes
//

class ParamsTracker
{
private:
	class Value
	{
	public:
		int mType;

		union
		{
			float float_val;
			int int_val;
			Mtl *mtl_val;
			Texmap *tex_val;
			PBBitmap *bitmap_val;
			INode *node_val;
		} mSimpleValue;

		Point4 mPoint;
		Matrix3 mMatrix;
		std::wstring mString;
		AColor mColor;

		Value()
		: mType(-1)
		{
		}
	};

	std::map<int, Value> mTracker;
	class Synchronizer *mSynchronizer;

public:

	ParamsTracker(class Synchronizer *Synchronizer)
	:mSynchronizer(Synchronizer)
	{
	}

	// adds a parameter to the tracker. the parameter must be defined
	// in the associated parameter block
	bool AddToTracker(int param);

	// goes through the parameters being tracked and determines which
	// ones have been modified. the resulting vector will contain a
	// list of IDs of the parameters whose values have effectively changed.
	void CollectChanges(std::vector<int>& res);

	const Value &GetParam(int p)
	{
		static const Value EmptyValue;
		auto pp = mTracker.find(p);
		if (pp != mTracker.end())
			return pp->second;
		return EmptyValue;
	}
};

//////////////////////////////////////////////////////////////////////////////
// Synchronizer is the main synchronization class. It tracks changes in the
// scene and manages their effects on a running instance of RPR context.
// It works in tandem with a rendering thread.
//

class Synchronizer : public ReferenceMaker
{
friend class TMPropertyCallback;
friend class BgPropertyCallback;
friend class ParamsTracker;
private:
	class ActiveShadeAttachCallback : public SceneAttachCallback
	{
	public:
		Synchronizer* mSynch;
		ActiveShadeAttachCallback( Synchronizer* synch ) : mSynch(synch) {}
		void PreSceneAttachLight( frw::Shape& shape, INode* node );
		void PreSceneAttachLight( frw::Light& light, INode* node );
	};
protected:
	// general
	frw::Scope mScope;
	INode *mSceneINode;
	SynchronizerBridge *mBridge;
	float mMasterScale;
	MaterialParser mtlParser;
	bool mRunning = false;
	bool mFirstRun = true;

	struct Flags
	{
		bool mHasDirectDisplacements = false;
		bool mUsesNonPMLights = false;
	} mFlags;

	// tone mappers and other effects
	frw::PostEffect mWhiteBalance;
	frw::PostEffect mSimpleTonemap;
	frw::PostEffect mLinearTonemap;
	frw::PostEffect mGammaCorrection;
	
	// references
	std::map<RefTargetHandle, int> mRefsMap;
	std::map<int, RefTargetHandle> mRefsMapI;

	// tracking structures
	std::map<INode *, std::vector<SShapePtr>> mShapes;
	std::map<INode *, SShapePtr> mLightShapes;
	std::map<INode *, SLightPtr> mLights;
	std::vector<SLightPtr> mDefaultLights; // stores current default lights
	std::map<INode *, frw::Shape> mPortals;
	std::map<Mtl *, std::set<SShapePtr>> mMaterialUsers;
	std::map<Mtl *, std::vector<Mtl*>> mMultiMats; // tracks changes in multi-mat structures
	std::set<Mtl *> mEmissives; // tracks emissives being used in scene

	std::map<Mtl *, frw::Shader> mShaderCache; // caches surface shaders
	std::map<Mtl *, frw::Shader> mVolumeShaderCache; // caches volume shaders
		
	TMPropertyCallback mToneMapperCallback;
	BgPropertyCallback mBackgroundCallback;
	bool mToneMapperRunning = false;
	//int mLightSources = 0;
	bool mUsingDefaultLights = false;

	ParamsTracker mParametersTracker; // to track generic parameters, especially those that max cannot be relied on with respect to notification messages.
	
	// background tracking
	bool mMAXEnvironmentForceRebuild = false;
	BOOL mMAXEnvironmentUse = false; // caches use bg texmap
	Texmap *mMAXEnvironment = 0; // caches bg texmap
	Point3 mEnvironmentColor; // caches bg color
	HashValue mMAXEnvironmentHash; // caches bg texmap hash
	HashValue mRPREnvironmentHash;
	
	std::vector<frw::Image> mBgIBLImage;
	std::vector<frw::Image> mBgReflImage;
	std::vector<frw::Image> mBgRefrlImage;
	std::vector<frw::EnvironmentLight> mBgLight;
	std::vector<frw::EnvironmentLight> mBgReflLight;
	std::vector<frw::EnvironmentLight> mBgRefrLight;


	// change queues (commands)
	SQueue mQueue;
	SceneCallbacks callbacks;

	// changes execution
	PartID mNotifyLastPartID; // to disambiguate xform in NotifyRefChanged
	INode *mNotifyLastNode = 0; // to disambiguate xform in NotifyRefChanged
	UINT_PTR mUITimerId;
	
public:
	Synchronizer(frw::Scope scope, INode *pSceneINode, SynchronizerBridge *pBridge);
	virtual ~Synchronizer();

	void Start();
	void Stop();

	inline bool IsQueueEmpty() const
	{
		return (mQueue.begin() == mQueue.end());
	}

protected:
	
	// general
	//
	INode* GetNodeFromObject(BaseObject* obj);


	// tracking references and changes
	//

	virtual RefResult NotifyRefChanged(const Interval& iv, RefTargetHandle hTarg, PartID& partID, RefMessage msg, BOOL propagate) override;
	
	virtual int NumRefs() override;
	virtual RefTargetHandle GetReference(int i) override;
	virtual void SetReference(int i, RefTargetHandle rtarg) override;
	
	int FindReference(RefTargetHandle hTarg);
	void AddReference(RefTargetHandle hTarg);
	void RemoveReference(RefTargetHandle hTarg);

	static void NotifyProc(void *param, NotifyInfo *info);
	void MakeInitialReferences(ReferenceTarget* ref);

	// commands
	//
	
	void InsertRebuildCommand(INode *pNode);
	void InsertAssignMatCommand(INode *pNode);
	void InsertDeleteCommand(INode *pNode);
	void InsertShowCommand(INode *pNode);
	void InsertHideCommand(INode *pNode);
	void InsertXformCommand(INode *pNode);
	void InsertMatCommand(Mtl *pMat);
	void InsertStartRPREnvironmentCommand();
	void InsertModifyRPREnvironmentCommand();
	void InsertStopRPREnvironmentCommand();
	void InsertStartRPRToneMapper();
	void InsertModifyRPRToneMapper();
	void InsertStopRPRToneMapper();
	void InsertEnableAlphaChannelCommand();
	void InsertDisableAlphaChannelCommand();

	// implementation of changes executioners
	//

	static VOID CALLBACK UITimerProc(_In_ HWND hwnd, _In_ UINT uMsg, _In_ UINT_PTR idEvent, _In_ DWORD dwTime);
	void ResetTimerProc();

	virtual void CustomCPUSideSynch() // optional - called from UITimerProc
	{
	}

	bool CreateMesh(std::vector<frw::Shape>& result, bool& directlyVisible, frw::Context& context, float masterScale, TimeValue timeValue, View& view, INode* inode, Object* evaluatedObject, const int numSubmtls, size_t& meshFaces, bool flipFaces);
	std::vector<frw::Shape> parseMesh(INode* inode, Object* evaluatedObject, const int numSubmtls, size_t& meshFaces, bool flipFaces);
	void DeleteGeometry(INode *instance);
	void RebuildGeometry(const std::list<INode *> &instances);
	void RebuildMaxLight(INode *light, Object *obj);
	void AddDefaultLights();
	void RemoveDefaultLights();
	bool ResetMaterial(INode *node); // returns false if the node needs rebuilding
	void RemoveMaterialsFromNode(INode *node, bool *wasMulti = 0); // utility
	bool Show(INode *node); // returns false if the node needs rebuilding
	void Hide(INode *node);
	void RebuildUsersOfMaterial(Mtl *pMat, std::vector<INode*> &nodesToRebuild); // utility
	frw::Shader CreateShader(Mtl *pMat, INode *node, bool force = false); // utility
	void UpdateMaterial(Mtl *pMat, std::vector<INode*> &nodesToRebuild);
	void RebuildCoronaSun(INode *node, Object* evaluatedObject);
	void RebuildFRPortal(INode* node, Object* evaluatedObject);
	
	void RemoveEnvironment();
	void AddRPREnvironment();
	void UpdateRPREnvironment();
	void RemoveRPREnvironment();
	bool MAXEnvironmentNeedsUpdate(); // utility
	void UpdateMAXEnvironment();
	void UpdateEnvironmentImage();
	frw::Image CreateColorEnvironment(const Point3 &color); // utility
	void UpdateEnvXForm(INode *pNode);
	void UpdateToneMapper();
	void UpdateRenderSettings(const std::vector<int> &changes);
};

FIRERENDER_NAMESPACE_END;
