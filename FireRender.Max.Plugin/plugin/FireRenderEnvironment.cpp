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

#include "frWrap.h"
#include "Common.h"

#include "Resource.h"

#include "FireRenderEnvironment.h"

#include "utils/Utils.h"

#include <math.h>

#include <iparamb2.h>
#include <LINSHAPE.H>
#include <3dsmaxport.h> //for DLGetWindowLongPtr

#include <hsv.h> // just for RGBtoHSV

#include "ParamBlock.h"
#include "plugin/BgManager.h"


//#if MAX_PRODUCT_YEAR_NUMBER >= 2015
//#define FR_USE_IObjectDisplay2 0
//#endif
#define FR_USE_IObjectDisplay2 0

#if FR_USE_IObjectDisplay2

#include <Graphics/Utilities/MeshEdgeRenderItem.h>
#include <Graphics/Utilities/SplineRenderItem.h>
#include <Graphics/CustomRenderItemHandle.h>
#include <Graphics/RenderNodeHandle.h>

#endif

#include <decomp.h>
#include <iparamm2.h>

static void RemoveScaling(Matrix3 &tm) {
	AffineParts ap;
	decomp_affine(tm, &ap);
	tm.IdentityMatrix();
	tm.SetRotate(ap.q);
	tm.SetTrans(ap.t);
}


#include <memory>

FIRERENDER_NAMESPACE_BEGIN

TCHAR* FIRERENDER_ENVIRONMENT_CATEGORY = _T("Radeon ProRender");

TCHAR* FIRERENDER_ENVIRONMENT_OBJECT_NAME = _T("RPREnv");
TCHAR* FIRERENDER_ENVIRONMENT_INTERNAL_NAME = _T("RPREnv");
TCHAR* FIRERENDER_ENVIRONMENT_CLASS_NAME = _T("RPREnv");

enum FREnvironmentType {
	FREnvironment_Type_IBL,
	FREnvironment_Type_Analytical
};


enum FREnvironmentParamID : ParamID {
	FREnvironment_ACTIVE = 1000,
	FREnvironment_RADIUS = 1001,
	FREnvironment_MAP = 1002,
	FREnvironment_INTENSITY = 1003,
	FREnvironment_GROUND_HEIGHT = 1004,
	FREnvironment_SHADOWS = 1005,
	FREnvironment_REFLECTIONS_STRENGTH = 1006,
	FREnvironment_REFLECTIONS = 1007,
	FREnvironment_REFLECTIONS_COLOR = 1008,
	FREnvironment_REFLECTIONS_ROUGHNESS = 1009,
	FREnvironment_TYPE = 1010,
	FREnvironment_SUN_AZYMUTH = 1011,
	FREnvironment_SUN_ALTITUDE = 1012,
	FREnvironment_SKY_ALBEDO = 1013,
	FREnvironment_SKY_TURBIDITY = 1014,
	FREnvironment_BACKGROUND_MAP = 1015,
	FREnvironment_BACKGROUND = 1016,
	FREnvironment_PORTAL = 1017,
};

class FireRenderEnvironmentClassDesc : public ClassDesc2
{
public:
	int 			IsPublic() override { return 0; }
	HINSTANCE		HInstance() override { 
		FASSERT(fireRenderHInstance != NULL);
		return fireRenderHInstance;
	}

	//used in MaxScript to create objects of this class
	const TCHAR*	InternalName() override {
		return FIRERENDER_ENVIRONMENT_INTERNAL_NAME;
	}

	//used as lable in creation UI and in MaxScript to create objects of this class
	const TCHAR*	ClassName() override {
		return FIRERENDER_ENVIRONMENT_CLASS_NAME;
	}
	Class_ID 		ClassID() override { return FIRERENDER_ENVIRONMENT_CLASS_ID; }
	SClass_ID		SuperClassID() override { return LIGHT_CLASS_ID; }
	const TCHAR* 	Category() override {
		return FIRERENDER_ENVIRONMENT_CATEGORY;
	}
	void*			Create(BOOL loading) override ;
};

static FireRenderEnvironmentClassDesc desc;

static const int VERSION = 1;

static ParamBlockDesc2 paramBlock(0, _T("EnvironmentPbDesc"), 0, &desc, P_AUTO_CONSTRUCT + P_VERSION + P_AUTO_UI, 
	VERSION, // for P_VERSION
	0, //for P_AUTO_CONSTRUCT
	//P_AUTO_UI
	IDD_FIRERENDER_ENVIRONMENT, IDS_FR_ENV, 0, 0, NULL,

	FREnvironment_ACTIVE, _T("active"), TYPE_BOOL, P_INVISIBLE | P_OBSOLETE | P_RESET_DEFAULT, 0,
	p_default, TRUE, 
	p_end,
	
	FREnvironment_RADIUS, _T("input radius"), TYPE_FLOAT, P_INVISIBLE | P_OBSOLETE | P_RESET_DEFAULT, 0,
	p_default, 0.0,
	p_range, 0.0, FLT_MAX,
	p_end,

	FREnvironment_MAP, _T("map"), TYPE_TEXMAP, P_INVISIBLE | P_OBSOLETE, 0,
	p_subtexno, 0, 
	p_end,
	
	FREnvironment_INTENSITY, _T("intensity"), TYPE_FLOAT, P_INVISIBLE | P_OBSOLETE | P_RESET_DEFAULT, 0,
	p_default, 1.0,
	p_range, 0.0, FLT_MAX,
	p_end,

	FREnvironment_GROUND_HEIGHT, _T("groundHeight"), TYPE_FLOAT, P_INVISIBLE | P_OBSOLETE | P_RESET_DEFAULT, 0,
	p_default, 0.0,
	p_range, -FLT_MAX, FLT_MAX,
	p_end,

	FREnvironment_SHADOWS, _T("shadows"), TYPE_BOOL, P_INVISIBLE | P_OBSOLETE | P_RESET_DEFAULT, 0,
	p_default, FALSE, 
	p_end,
	
	FREnvironment_REFLECTIONS_STRENGTH, _T("reflectionsStrength"), TYPE_FLOAT, P_INVISIBLE | P_OBSOLETE | P_RESET_DEFAULT, 0,
	p_default, 0.5,
	p_range, 0.0, 1.0,
	p_end,
	
	FREnvironment_REFLECTIONS, _T("reflections"), TYPE_BOOL, P_INVISIBLE | P_OBSOLETE | P_RESET_DEFAULT, 0,
	p_default, FALSE, 
	p_end,

	FREnvironment_REFLECTIONS_COLOR, _T("reflectionsColor"), TYPE_RGBA, P_INVISIBLE | P_OBSOLETE | P_RESET_DEFAULT, 0,
	p_default, Color(0.5f, 0.5f, 0.5f), 
	p_end,
	
	FREnvironment_REFLECTIONS_ROUGHNESS, _T("reflectionsRoughness"), TYPE_FLOAT, P_INVISIBLE | P_OBSOLETE | P_RESET_DEFAULT, 0,
	p_default, 0.0,
	p_range, 0.0, FLT_MAX,
	p_end,
	
	FREnvironment_TYPE, _T("type"), TYPE_INT, P_INVISIBLE | P_OBSOLETE, 0,
	p_default, FREnvironment_Type_IBL,
	p_end,
	
	FREnvironment_SUN_AZYMUTH, _T("sunAzymuth"), TYPE_ANGLE, P_INVISIBLE | P_OBSOLETE | P_RESET_DEFAULT, 0,
	p_default, 0.0,
	p_range, -360.0, 360.0,
	p_end,

	FREnvironment_SUN_ALTITUDE, _T("sunAltitude"), TYPE_ANGLE, P_INVISIBLE | P_OBSOLETE | P_RESET_DEFAULT, 0,
	p_default, 0.0,
	p_range, -90.0, 90.0,
	p_end,

	FREnvironment_SKY_ALBEDO, _T("skyAlbedo"), TYPE_FLOAT, P_INVISIBLE | P_OBSOLETE | P_RESET_DEFAULT, 0,
	p_default, 1.0,
	p_range, -FLT_MAX, FLT_MAX,
	p_end,

	FREnvironment_SKY_TURBIDITY, _T("skyTurbidity"), TYPE_FLOAT, P_INVISIBLE | P_OBSOLETE | P_RESET_DEFAULT, 0,
	p_default, 1.0,
	p_range, -FLT_MAX, FLT_MAX,
	p_end,

	FREnvironment_BACKGROUND_MAP, _T("backgroundMap"), TYPE_TEXMAP, P_INVISIBLE | P_OBSOLETE, 0,
	p_subtexno, 0, 
	p_end,

	FREnvironment_BACKGROUND, _T("background"), TYPE_BOOL, P_INVISIBLE | P_OBSOLETE | P_RESET_DEFAULT, 0,
	p_default, FALSE, 
	p_end,

	FREnvironment_PORTAL, _T("portal"), TYPE_INODE, P_INVISIBLE | P_OBSOLETE, 0,
	p_end,

	p_end
);

ClassDesc2* GetFireRenderEnvironmentDesc() { 

	return &desc; 
}

namespace
{
	template<class CB>
	class NodeIterator {
		CB callback;
	public:
		NodeIterator(CB callback) : callback(callback) {}

		void traverse(INode* node) {
			callback(node);
			traverseDescendants(node);
		}

		void traverseDescendants(INode* node) {
			for (int i = 0; i < node->NumberOfChildren(); ++i) {
				traverse(node->GetChildNode(i));
			}
		}
	};

	template<class CB>
	void traverseNode(INode* node, CB callback) {
		NodeIterator<CB>(callback).traverse(node);
	}
};

class FireRenderEnvironment : public GenLight
{
	class CreateCallBack : public CreateMouseCallBack
	{
		FireRenderEnvironment *ob;
	public:
		int proc(ViewExp *vpt, int msg, int point, int flags, IPoint2 m, Matrix3& mat) {
			if (!vpt || !vpt->IsAlive())
			{
				// this shouldn't happen
				DbgAssert(!_T("Invalid viewport!"));
				return FALSE;
			}

			Point3 p0;
#ifdef _OSNAP
			if (msg == MOUSE_FREEMOVE)
			{
#ifdef _3D_CREATE
				vpt->SnapPreview(m, m, NULL, SNAP_IN_3D);
#else
				vpt->SnapPreview(m, m, NULL, SNAP_IN_PLANE);
#endif
			}
#endif

			if (msg == MOUSE_POINT || msg == MOUSE_MOVE) {

				// 6/19/01 11:37am --MQM-- wire-color changes
				// since we're now allowing the user to set the color of
				// the light wire-frames, we need to set the initial light 
				// color to yellow, instead of the default random object color.
				if (point == 0)
				{
					ULONG handle;
					ob->NotifyDependents(FOREVER, (PartID)&handle, REFMSG_GET_NODE_HANDLE);
					INode * node;
					node = GetCOREInterface()->GetINodeByHandle(handle);
					if (node)
					{
						Point3 color(1, 1, 0);	// yellow wire color
						node->SetWireColor(RGB(color.x*255.0f, color.y*255.0f, color.z*255.0f));
					}
				}

#ifdef _3D_CREATE
				mat.SetTrans(vpt->SnapPoint(m, m, NULL, SNAP_IN_3D));
#else
				mat.SetTrans(vpt->SnapPoint(m, m, NULL, SNAP_IN_PLANE));
#endif

				if (point == 1) {
					if (msg == MOUSE_POINT)
						return 0;
				}
			}
			else if (msg == MOUSE_ABORT)
				return CREATE_ABORT;

			return TRUE;
		}
		void SetObj(FireRenderEnvironment* obj) { ob = obj; }
	};

public:
	PolyLine displayShape;
#define NumRosePoints 17
	Point3 roseVerts[NumRosePoints];
	
	FireRenderEnvironment() 
	{
		GetFireRenderEnvironmentDesc()->MakeAutoParamBlocks(this);

		const float ro = 150.f;
		const float ri = 50.f;
		const float w = ro - ri;
		const float rt = w * 1.f / 3.3333333333333f;
				
		Point2 rosePoints[NumRosePoints] = {
			{ 0.f, ro},
			{ 27.f, rt },
			{ 45.f, ri },
			{ 63.f, rt },
			{ 90.f, ro },
			{ 117.f, rt },
			{ 135.f, ri },
			{ 153.f, rt },
			{ 180.f, ro },
			{ 207.f, rt },
			{ 225.f, ri },
			{ 243.f, rt },
			{ 270.f, ro },
			{ 297.f, rt },
			{ 315.f, ri },
			{ 333.f, rt },
			{ 0.f, ro },
		};
		// create polyline
		for (int i = 0; i < NumRosePoints; i++)
		{
			float angle = rosePoints[i].x * PI / 180.f;
			float r = rosePoints[i].y;
			roseVerts[i].x = r * cos(angle);
			roseVerts[i].y = r * sin(angle);
			roseVerts[i].z = 0.f;
			displayShape.Append(PolyPt(roseVerts[i]));
		}
		
		float radius = 0.f;
		BgManagerMax::TheManager.GetProperty(PARAM_GROUND_RADIUS, radius);
		if(radius==0){
			Box3 sceneBB = GetSceneBB();
			if(!sceneBB.IsEmpty()){
				radius = (sceneBB.Max()-sceneBB.Min()).Length()*4.0f;
			}
		}

		if(radius<0.1f){
			radius = 10.0f;
		}
		BgManagerMax::TheManager.SetProperty(PARAM_GROUND_RADIUS, radius);

		BOOL IsActive = FALSE;
		BgManagerMax::TheManager.GetProperty(PARAM_GROUND_ACTIVE, IsActive);
		handleActivated( bool_cast(IsActive) );
	}
	
	~FireRenderEnvironment() {
		DebugPrint(_T("FireRenderEnvironment::~FireRenderEnvironment\n"));
		DeleteAllRefs();
	}

	CreateMouseCallBack *BaseObject::GetCreateMouseCallBack(void)
	{
		static CreateCallBack createCallback;
		createCallback.SetObj(this);
		return &createCallback;
	}

	GenLight *NewLight(int type)
	{
		return new FireRenderEnvironment();
	}

	// From Object
	ObjectState 	Eval(TimeValue time) {
		return ObjectState(this);
	}

	//makes part of the node name e.g. TheName001, if method returns L"TheName"
	void InitNodeName(TSTR& s) { s = FIRERENDER_ENVIRONMENT_OBJECT_NAME; }

	//displayed in Modifier list
	const MCHAR *GetObjectName() override { return FIRERENDER_ENVIRONMENT_OBJECT_NAME; }

	Interval 		ObjectValidity(TimeValue t) override
	{
		Interval valid;
		valid.SetInfinite();
		return valid;
	}
	BOOL 			UsesWireColor() { return 1; }		// 6/18/01 2:51pm --MQM-- now we can set object color
	int 			DoOwnSelectHilite() { return 1; }

	void NotifyChanged() { NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE); }

	// inherited virtual methods for Reference-management

	RefResult NotifyRefChanged(NOTIFY_REF_CHANGED_PARAMETERS) override
	{
		// implement texmap changed handling
		// i.e. recreate RPR texture

		if ((REFMSG_CHANGE==msg)  && (hTarget == pblock2))
		{
			auto p = pblock2->LastNotifyParamID();
			//some params have changed - should redraw all
			if(GetCOREInterface()){
				GetCOREInterface()->RedrawViews(GetCOREInterface()->GetTime());
			}
		}
		return REF_SUCCEED;
	}

	IParamBlock2* pblock2 = NULL;

	void DeleteThis() override
	{ 
		delete this; 
	}

	Class_ID ClassID() override { 
		return FIRERENDER_ENVIRONMENT_CLASS_ID; 
	}

	//
	void GetClassName(TSTR& s) override { 
		FIRERENDER_ENVIRONMENT_CLASS_NAME; 
	}

	IParamBlock2* GetParamBlock(int i) override { 
		FASSERT(i == 0);
		return pblock2; 
	}

	IParamBlock2* GetParamBlockByID(BlockID id) override { 
		FASSERT(pblock2->ID() == id);
		return pblock2; 
	}

	int NumRefs() override {
		return 1;
	}

	void SetReference(int i, RefTargetHandle rtarg) override {
		FASSERT(0 == i);
		pblock2 = (IParamBlock2*)rtarg;
	}

	RefTargetHandle GetReference(int i) override {
		FASSERT(0 == i);
		return (RefTargetHandle)pblock2;
	}

#if FR_USE_IObjectDisplay2
	unsigned long GetObjectDisplayRequirement() const override {
		return 0;
	}

	bool PrepareDisplay(
		const MaxSDK::Graphics::UpdateDisplayContext& prepareDisplayContext) override {
		GetDisplayMeshKey().SetFixedSize(true);
		return true;
	}

	bool UpdatePerViewItems(
		const MaxSDK::Graphics::UpdateDisplayContext& updateDisplayContext,
		MaxSDK::Graphics::UpdateNodeContext& nodeContext,
		MaxSDK::Graphics::UpdateViewContext& viewContext,
		MaxSDK::Graphics::IRenderItemContainer& targetRenderItemContainer) override {

		return UpdatePerNodeItems(updateDisplayContext, nodeContext, targetRenderItemContainer);
	}

	bool UpdatePerNodeItems(
		const MaxSDK::Graphics::UpdateDisplayContext& updateDisplayContext,
		MaxSDK::Graphics::UpdateNodeContext& nodeContext,
		MaxSDK::Graphics::IRenderItemContainer& targetRenderItemContainer) override {

		INode* pNode = nodeContext.GetRenderNode().GetMaxNode();
		MaxSDK::Graphics::Utilities::MeshEdgeRenderItem* pMeshItem = new MaxSDK::Graphics::Utilities::MeshEdgeRenderItem(&displayMesh, true, false);
		if (pNode->Dependent())
		{
			pMeshItem->SetColor(Color(ColorMan()->GetColor(kViewportShowDependencies)));
		}
		else if (pNode->Selected()) 
		{
			pMeshItem->SetColor(Color(GetSelColor()));
		}
		else if (pNode->IsFrozen())
		{
			pMeshItem->SetColor(Color(GetFreezeColor()));
		}
		else
		{
			pMeshItem->SetColor(Color(1,0,1));
		}

		MaxSDK::Graphics::CustomRenderItemHandle meshHandle;
		meshHandle.Initialize();
		meshHandle.SetVisibilityGroup(MaxSDK::Graphics::RenderItemVisible_Gizmo);
		meshHandle.SetCustomImplementation(pMeshItem);
		MaxSDK::Graphics::ConsolidationData data;
		data.Strategy = &MaxSDK::Graphics::Utilities::MeshEdgeConsolidationStrategy::GetInstance();

		data.Key = &GetDisplayMeshKey();
		meshHandle.SetConsolidationData(data);
		targetRenderItemContainer.AddRenderItem(meshHandle);

		return true;

	}

	MaxSDK::Graphics::Utilities::MeshEdgeKey& GetDisplayMeshKey(){
		static MaxSDK::Graphics::Utilities::MeshEdgeKey meshKey;
		return meshKey;
	}
#else //FR_USE_IObjectDisplay2

	Matrix3 GetTransformMatrix(TimeValue t, INode* inode, ViewExp* vpt){
		Matrix3 tm;
		if ( ! vpt->IsAlive() )
		{
			tm.Zero();
			return tm;
		}
	
		tm = inode->GetObjectTM(t);
		
		RemoveScaling(tm);
		float scaleFactor = vpt->NonScalingObjectSize() * vpt->GetVPWorldWidth(tm.GetTrans()) / 360.0f;
		if (scaleFactor!=(float)1.0)
			tm.Scale(Point3(scaleFactor,scaleFactor,scaleFactor));
		return tm;
	}

	Color GetViewportMainColor(INode* pNode) {
		if (pNode->Dependent())
		{
			return Color(ColorMan()->GetColor(kViewportShowDependencies));
		}
		else if (pNode->Selected()) 
		{
			return Color(GetSelColor());
		}
		else if (pNode->IsFrozen())
		{
			return Color(GetFreezeColor());
		}
		return Color(1,0,1);
	}

	Color GetViewportColor(INode* pNode, Color selectedColor) {
		if (pNode->Dependent())
		{
			return Color(ColorMan()->GetColor(kViewportShowDependencies));
		}
		else if (pNode->Selected()) 
		{
			return selectedColor;
		}
		else if (pNode->IsFrozen())
		{
			return Color(GetFreezeColor());
		}
		return selectedColor*0.5f;
	}

	int Display(TimeValue t, INode* inode, ViewExp *vpt, int flags) override {
		if ( ! vpt || ! vpt->IsAlive() )
		{
			// why are we here
			DbgAssert(!_T("Invalid viewport!"));
			return FALSE;
		}

		GraphicsWindow *gw = vpt->getGW();
		//if (!enable) return  0;

		DWORD rlim = gw->getRndLimits();

		//wireframe
		gw->setRndLimits(GW_WIREFRAME|GW_EDGES_ONLY|GW_BACKCULL| (rlim&GW_Z_BUFFER) );

		gw->setColor( LINE_COLOR, GetViewportColor(inode, Color(0.6f, 0.6f, 0.6f)));

		gw->setTransform(GetTransformMatrix(t, inode, vpt));

		gw->text(&roseVerts[0], L"E");
		gw->text(&roseVerts[4], L"N");
		gw->text(&roseVerts[8], L"W");
		gw->text(&roseVerts[12], L"S");
				
		BitArray ba(16);
		ba.SetAll();
		displayShape.Render(gw, gw->getMaterial(), 1, FALSE, ba, TRUE);

		gw->setRndLimits(rlim);

		return TRUE;
	}

	//w're using retained mode, so we don't need to implement this
	int HitTest(TimeValue t, INode* inode, int type, int crossing, int flags, IPoint2 *p, ViewExp *vpt) override
	{
		if ( ! vpt || ! vpt->IsAlive() )
		{
			// why are we here
			DbgAssert(!_T("Invalid viewport!"));
			return FALSE;
		}
		return TRUE;
	};

#endif //FR_USE_IObjectDisplay2

	int extDispFlags = 0;
	void SetExtendedDisplay(int flags)
	{
		extDispFlags = flags;
	}

	void GetLocalBoundBox(TimeValue t, INode* inode, ViewExp* vpt, Box3& box)
	{
		Interval ivalid = FOREVER;

		box = displayShape.GetBoundingBox();

		Point3 loc = inode->GetObjectTM(t).GetTrans();
		float scaleFactor = vpt->NonScalingObjectSize()*vpt->GetVPWorldWidth(loc) / 360.0f;
		box.Scale(scaleFactor);
	}

	void GetWorldBoundBox(TimeValue t, INode* inode, ViewExp* vpt, Box3& box)
	{
		if ( ! vpt || ! vpt->IsAlive() )
		{
			box.Init();
			return;
		}
	
		Matrix3 tm;
		
		tm = inode->GetObjectTM(t);
		RemoveScaling(tm);
		float scaleFactor = vpt->NonScalingObjectSize() * vpt->GetVPWorldWidth(tm.GetTrans()) / 360.0f;

		Point3 loc = tm.GetTrans();

		box = displayShape.GetBoundingBox(&tm);
		box.Scale(scaleFactor*0.5);
	}

	TimeValue getEditTime() {
		return GetCOREInterface()->GetTime();
	}

	void handleActivated(bool value) {
		auto obj = this;
		if (value) {
			auto visit = [obj](INode* node) {
				ObjectState state = node->EvalWorldState(0);
				Object* otherObj = state.obj;
				if (otherObj && (otherObj->ClassID() == FIRERENDER_ENVIRONMENT_CLASS_ID))
				{
					auto other = dynamic_cast<FireRenderEnvironment*>(otherObj);

					if (other != obj) {
						other->pblock2->SetValue(FREnvironment_ACTIVE, other->getEditTime(), false);
					}
				}
			};
			traverseNode(GetCOREInterface()->GetRootNode(), visit);
		}
	}

	Box3 GetSceneBB()
	{
		Box3 result;

		auto visit = [this, &result](INode* node)
		{
			if (!node)
				return;

			TimeValue t = getEditTime();

			ObjectState state = node->EvalWorldState(t);

			if (state.obj)
			{
				if (!node->IsNodeHidden()
					&& node->Renderable()
					&& state.obj->IsRenderable()
					)
				{
					Box3 bb;
					Matrix3 tm = node->GetObjTMAfterWSM(t);

					state.obj->GetDeformBBox(t, bb, &tm);
					result += bb;
				}
			}
		};

		traverseNode(GetCOREInterface()->GetRootNode(), visit);

		return result;
	}

	INode* GetNodeRef(ReferenceMaker *rm)
	{
		if (rm->SuperClassID() == BASENODE_CLASS_ID)
			return (INode *)rm;
		else
			return rm->IsRefTarget() ? FindNodeRef((ReferenceTarget *)rm) : NULL;
	}

	INode* FindNodeRef(ReferenceTarget *rt)
	{
		DependentIterator di(rt);
		ReferenceMaker *rm;
		INode *nd = NULL;
		while (bool_cast(rm = di.Next()))
		{
			nd = GetNodeRef(rm);
			if (nd) return nd;
		}
		return NULL;
	}

	class PLCB : public PostLoadCallback
	{
	public:
		FireRenderEnvironment*	p;
		PLCB(FireRenderEnvironment* pp) { p = pp; }
		
		void proc(ILoad *iload)
		{
			BgManagerMax &man = BgManagerMax::TheManager;

			Texmap* bgMap = 0;
			p->pblock2->GetValue(FREnvironment_MAP, p->getEditTime(), bgMap, Interval());
			man.SetProperty(PARAM_BG_IBL_MAP, bgMap);
			if (bgMap)
				man.SetProperty(PARAM_BG_IBL_MAP_USE, TRUE);

			float intensity = 1.f;
			p->pblock2->GetValue(FREnvironment_INTENSITY, p->getEditTime(), intensity, Interval());
			man.SetProperty(PARAM_BG_IBL_INTENSITY, intensity);

			Texmap* plateMap = 0;
			p->pblock2->GetValue(FREnvironment_BACKGROUND_MAP, p->getEditTime(), plateMap, Interval());
			man.SetProperty(PARAM_BG_IBL_BACKPLATE, plateMap);
				
			BOOL usePlateMap = FALSE;
			p->pblock2->GetValue(FREnvironment_BACKGROUND, p->getEditTime(), usePlateMap, Interval());
			man.SetProperty(PARAM_BG_IBL_BACKPLATE_USE, usePlateMap);

			man.SetProperty(PARAM_GROUND_ACTIVE, TRUE);

			float groundRadius = 0.f;
			p->pblock2->GetValue(FREnvironment_RADIUS, p->getEditTime(), groundRadius, Interval());
			man.SetProperty(PARAM_GROUND_RADIUS, groundRadius);

			float groundHeight = 0.f;
			p->pblock2->GetValue(FREnvironment_GROUND_HEIGHT, p->getEditTime(), groundHeight, Interval());
			man.SetProperty(PARAM_GROUND_GROUND_HEIGHT, groundHeight);

			BOOL groundShadows = FALSE;
			p->pblock2->GetValue(FREnvironment_SHADOWS, p->getEditTime(), groundShadows, Interval());
			man.SetProperty(PARAM_GROUND_SHADOWS, groundShadows);

			float groundReflStrenght = 0.f;
			p->pblock2->GetValue(FREnvironment_REFLECTIONS_STRENGTH, p->getEditTime(), groundReflStrenght, Interval());
			man.SetProperty(PARAM_GROUND_REFLECTIONS_STRENGTH, groundReflStrenght);

			BOOL groundRefl = FALSE;
			p->pblock2->GetValue(FREnvironment_REFLECTIONS, p->getEditTime(), groundRefl, Interval());
			man.SetProperty(PARAM_GROUND_REFLECTIONS, groundRefl);

			Color groundReflColor;
			p->pblock2->GetValue(FREnvironment_REFLECTIONS_COLOR, p->getEditTime(), groundReflColor, Interval());
			man.SetProperty(PARAM_GROUND_REFLECTIONS_COLOR, groundReflColor);

			float groundReflRough = 0.f;
			p->pblock2->GetValue(FREnvironment_REFLECTIONS_ROUGHNESS, p->getEditTime(), groundReflRough, Interval());
			man.SetProperty(PARAM_GROUND_REFLECTIONS_ROUGHNESS, groundReflRough);

			INode* node = p->FindNodeRef(p);
			man.SetEnvironmentNode(node);
				
			delete this;
		}
	};
	
	IOResult Load(ILoad *iload) override
	{
		PLCB* plcb = new PLCB(this);
		iload->RegisterPostLoadCallback(plcb);
		return IO_OK;
	}

	// LightObject methods

	Point3 GetRGBColor(TimeValue t, Interval &valid) override { return Point3(1.0f, 1.0f, 1.0f); }
	void SetRGBColor(TimeValue t, Point3 &color) override { /* empty */ }
	void SetUseLight(int onOff) override { /* empty */ }
	BOOL GetUseLight() override
	{
		return TRUE;
	}
	void SetHotspot(TimeValue time, float f) override { /* empty */ }
	float GetHotspot(TimeValue t, Interval& valid) override { return -1.0f; }
	void SetFallsize(TimeValue time, float f) override { /* empty */ }
	float GetFallsize(TimeValue t, Interval& valid) override { return -1.0f; }
	void SetAtten(TimeValue time, int which, float f) override { /* empty */ }
	float GetAtten(TimeValue t, int which, Interval& valid) override { return 0.0f; }
	void SetTDist(TimeValue time, float f) override { /* empty */ }
	float GetTDist(TimeValue t, Interval& valid) override { return 0.0f; }
	void SetConeDisplay(int s, int notify) override { /* empty */ }
	BOOL GetConeDisplay() override { return FALSE; }
	void SetIntensity(TimeValue time, float f) override { /* empty */ }
	float GetIntensity(TimeValue t, Interval& valid) override { return 1.0f; }
	void SetUseAtten(int s) override { /* empty */ }
	BOOL GetUseAtten() override { return FALSE; }
	void SetAspect(TimeValue t, float f) override { /* empty */ }
	float GetAspect(TimeValue t, Interval& valid) override { return -1.0f; }
	void SetOvershoot(int a) override { /* empty */ }
	int GetOvershoot() override { return 0; }
	void SetShadow(int a) override { /* empty */ }
	int GetShadow() override { return 0; }

	// From Light
	RefResult EvalLightState(TimeValue t, Interval& valid, LightState* cs)
	{
		cs->color = GetRGBColor(t, valid);
		cs->on = GetUseLight();
		cs->intens = GetIntensity(t, valid);
		cs->hotsize = GetHotspot(t, valid);
		cs->fallsize = GetFallsize(t, valid);
		cs->useNearAtten = FALSE;
		cs->useAtten = GetUseAtten();
		cs->nearAttenStart = GetAtten(t, ATTEN1_START, valid);
		cs->nearAttenEnd = GetAtten(t, ATTEN1_END, valid);
		cs->attenStart = GetAtten(t, ATTEN_START, valid);
		cs->attenEnd = GetAtten(t, ATTEN_END, valid);
		cs->aspect = GetAspect(t, valid);
		cs->overshoot = GetOvershoot();
		cs->shadow = GetShadow();
		cs->shape = CIRCLE_LIGHT;
		cs->ambientOnly = GetAmbientOnly();
		cs->affectDiffuse = GetAffectDiffuse();
		cs->affectSpecular = GetAffectSpecular();
		cs->type = AMBIENT_LGT;

		return REF_SUCCEED;
	}

	ObjLightDesc* CreateLightDesc(INode *inode, BOOL forceShadowBuf) override { return NULL; }

	int Type() override { return DIR_LIGHT; }

	virtual void SetHSVColor(TimeValue, Point3 &)
	{
	}
	virtual Point3 GetHSVColor(TimeValue t, Interval &valid)
	{
		int h, s, v;
		Point3 rgbf = GetRGBColor(t, valid);
		DWORD rgb = RGB((int)(rgbf[0] * 255.0f), (int)(rgbf[1] * 255.0f), (int)(rgbf[2] * 255.0f));
		RGBtoHSV(rgb, &h, &s, &v);
		return Point3(h / 255.0f, s / 255.0f, v / 255.0f);
	}


	void SetAttenDisplay(int) override { /* empty */ }
	BOOL GetAttenDisplay() override { return FALSE; }
	void Enable(int) override {}
	void SetMapBias(TimeValue, float) override { /* empty */ }
	float GetMapBias(TimeValue, Interval &) override { return 0; }
	void SetMapRange(TimeValue, float) override { /* empty */ }
	float GetMapRange(TimeValue, Interval &) override { return 0; }
	void SetMapSize(TimeValue, int) override { /* empty */ }
	int  GetMapSize(TimeValue, Interval &) override { return 0; }
	void SetRayBias(TimeValue, float) override { /* empty */ }
	float GetRayBias(TimeValue, Interval &) override { return 0; }
	int GetUseGlobal() override { return FALSE; }
	void SetUseGlobal(int) override { /* empty */ }
	int GetShadowType() override { return -1; }
	void SetShadowType(int) override { /* empty */ }
	int GetAbsMapBias() override { return FALSE; }
	void SetAbsMapBias(int) override { /* empty */ }
	BOOL IsSpot() override { return FALSE; }
	//BOOL IsDir() override { return TRUE; }
	BOOL IsDir() override { return FALSE; }
	void SetSpotShape(int) override { /* empty */ }
	int GetSpotShape() override { return CIRCLE_LIGHT; }
	void SetContrast(TimeValue, float) override { /* empty */ }
	float GetContrast(TimeValue, Interval &) override { return 0; }
	void SetUseAttenNear(int) override { /* empty */ }
	BOOL GetUseAttenNear() override { return FALSE; }
	void SetAttenNearDisplay(int) override { /* empty */ }
	BOOL GetAttenNearDisplay() override { return FALSE; }
	ExclList &GetExclusionList() override { return m_exclList; }
	void SetExclusionList(ExclList &) override { /* empty */ }

	BOOL SetHotSpotControl(Control *) override { return FALSE; }
	BOOL SetFalloffControl(Control *) override { return FALSE; }
	BOOL SetColorControl(Control *) override { return FALSE; }
	Control *GetHotSpotControl() override { return NULL; }
	Control *GetFalloffControl() override { return NULL; }
	Control *GetColorControl() override { return NULL; }

	void SetAffectDiffuse(BOOL onOff) override { /* empty */ }
	BOOL GetAffectDiffuse() override { return TRUE; }
	void SetAffectSpecular(BOOL onOff) override { /* empty */ }
	BOOL GetAffectSpecular() override { return TRUE; }
	void SetAmbientOnly(BOOL onOff) override { /* empty */ }
	BOOL GetAmbientOnly() override { return TRUE; }

	void SetDecayType(BOOL onOff) override {}
	BOOL GetDecayType() override { return DECAY_NONE; }
	void SetDecayRadius(TimeValue time, float f) override {}
	float GetDecayRadius(TimeValue t, Interval& valid) override { return 40.0f; } //! check different values
		
protected:
	ExclList m_exclList;
};

void*	FireRenderEnvironmentClassDesc::Create(BOOL loading) {
	return new FireRenderEnvironment();
}

FIRERENDER_NAMESPACE_END

#include <maxscript\maxscript.h>
#include <maxscript\macros\define_instantiation_functions.h>

#ifdef _DEBUG

def_visible_primitive(rprCrash, "rprCrash" );

Value* rprCrash_cf(Value** arg_list, int count)
{
   check_arg_count(rprCrash, 0, count);
   volatile int* p;
   p = 0;
   *p = 666;
   return &ok;
}

#endif
