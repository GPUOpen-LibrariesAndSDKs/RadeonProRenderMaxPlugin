/*********************************************************************************************************************************
* Radeon ProRender for 3ds Max plugin
* Copyright (c) 2017 AMD
* All Rights Reserved
*
* Custom analytical sun 3ds MAX scene node. The Analytical sun allows to manually control azimuth and elevation of the sun,
* when the sky system is set to analytical mode.
*********************************************************************************************************************************/

#include "frWrap.h"
#include "Common.h"

#include "Resource.h"

#include "plugin/FireRenderAnalyticalSun.h"

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

TCHAR* FIRERENDER_ANALYTICALSUN_CATEGORY = _T("Radeon ProRender");

TCHAR* FIRERENDER_ANALYTICALSUN_OBJECT_NAME = _T("RPRAnalyticalSun");
TCHAR* FIRERENDER_ANALYTICALSUN_INTERNAL_NAME = _T("RPRAnalyticalSun");
TCHAR* FIRERENDER_ANALYTICALSUN_CLASS_NAME = _T("RPRAnalyticalSun");

class FireRenderAnalyticalSunClassDesc : public ClassDesc2
{
public:
	int 			IsPublic() override { return 0; }
	HINSTANCE		HInstance() override
	{ 
		FASSERT(fireRenderHInstance != NULL);
		return fireRenderHInstance;
	}

	//used in MaxScript to create objects of this class
	const TCHAR*	InternalName() override {
		return FIRERENDER_ANALYTICALSUN_INTERNAL_NAME;
	}

	//used as lable in creation UI and in MaxScript to create objects of this class
	const TCHAR*	ClassName() override {
		return FIRERENDER_ANALYTICALSUN_CLASS_NAME;
	}
	Class_ID 		ClassID() override { return FIRERENDER_ANALYTICALSUN_CLASS_ID; }
	SClass_ID		SuperClassID() override { return LIGHT_CLASS_ID; }
	const TCHAR* 	Category() override {
		return FIRERENDER_ANALYTICALSUN_CATEGORY;
	}
	void*			Create(BOOL loading) override ;
};

static FireRenderAnalyticalSunClassDesc desc;

static const int VERSION = 1;

static ParamBlockDesc2 paramBlock(0, _T("AnalyticalSunPbDesc"), 0, &desc, P_AUTO_CONSTRUCT + P_VERSION + P_AUTO_UI, 
	VERSION, // for P_VERSION
	0, //for P_AUTO_CONSTRUCT
	//P_AUTO_UI
	IDD_FIRERENDER_ANALYTICALSUN, IDS_FR_ENV, 0, 0, NULL,

	p_end
);

ClassDesc2* GetFireRenderAnalyticalSunDesc()
{
	return &desc; 
}


class FireRenderAnalyticalSun : public GenLight
{
	class CreateCallBack : public CreateMouseCallBack
	{
		FireRenderAnalyticalSun *ob;
	public:
		int proc(ViewExp *vpt, int msg, int point, int flags, IPoint2 m, Matrix3& mat)
		{
			if (!vpt || !vpt->IsAlive())
			{
				// why are we here
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

			if (msg == MOUSE_POINT || msg == MOUSE_MOVE)
			{
				// Since we're now allowing the user to set the color of
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

				if (point == 1)
				{
					if (msg == MOUSE_POINT)
						return 0;
				}
			}
			else if (msg == MOUSE_ABORT)
				return CREATE_ABORT;

			return TRUE;
		}
		void SetObj(FireRenderAnalyticalSun* obj) { ob = obj; }
	};

public:
	Mesh displayShape;
	
	FireRenderAnalyticalSun()
	{
		GetFireRenderAnalyticalSunDesc()->MakeAutoParamBlocks(this);

		float radius = 15.f;
		int nbLong = 8;
		int nbLat = 8;

		std::vector<Point3> vertices((nbLong + 1) * nbLat + 2);
		float _pi = PI;
		float _2pi = _pi * 2.f;

		Point3 vectorup(0.f, 1.f, 0.f);

		vertices[0] = vectorup * radius;
		for (int lat = 0; lat < nbLat; lat++)
		{
			float a1 = _pi * (float)(lat + 1) / (nbLat + 1);
			float sin1 = sin(a1);
			float cos1 = cos(a1);

			for (int lon = 0; lon <= nbLong; lon++)
			{
				float a2 = _2pi * (float)(lon == nbLong ? 0 : lon) / nbLong;
				float sin2 = sin(a2);
				float cos2 = cos(a2);
				vertices[lon + lat * (nbLong + 1) + 1] = Point3(sin1 * cos2, cos1, sin1 * sin2) * radius;
			}
		}
		vertices[vertices.size() - 1] = vectorup * -radius;
		
		size_t vsize = vertices.size();
		
		displayShape.setNumVerts( int_cast(vsize) );

		// triangles
		int nbTriangles = nbLong * 2 + (nbLat - 1)*nbLong * 2;//2 caps and one middle
		int nbIndexes = nbTriangles * 3;
		std::vector<int> triangles(nbIndexes);

		//Top Cap
		int i = 0;
		for (int lon = 0; lon < nbLong; lon++)
		{
			triangles[i++] = lon + 2;
			triangles[i++] = lon + 1;
			triangles[i++] = 0;
		}

		//Middle
		for (int lat = 0; lat < nbLat - 1; lat++)
		{
			for (int lon = 0; lon < nbLong; lon++)
			{
				int current = lon + lat * (nbLong + 1) + 1;
				int next = current + nbLong + 1;

				triangles[i++] = current;
				triangles[i++] = current + 1;
				triangles[i++] = next + 1;

				triangles[i++] = current;
				triangles[i++] = next + 1;
				triangles[i++] = next;
			}
		}

		//Bottom Cap
		for (int lon = 0; lon < nbLong; lon++)
		{
			// TODO: triangles is int32 but vsize is int64, change vsize to 32?
			triangles[i++] = int_cast(vsize - 1);
			triangles[i++] = int_cast(vsize - (lon + 2) - 1);
			triangles[i++] = int_cast(vsize - (lon + 1) - 1);
		}


		for (int i = 0; i < vertices.size(); i++)
			displayShape.setVert(i, vertices[i]);

		displayShape.setNumFaces(nbTriangles);
		
		for (int i = 0; i < nbTriangles; i++)
		{
			Face* f = displayShape.faces + i;
			f->v[0] = triangles[i * 3];
			f->v[1] = triangles[i * 3 + 1];
			f->v[2] = triangles[i * 3 + 2];
			f->smGroup = 0;
			f->flags = EDGE_ALL;
		}
	}
	
	~FireRenderAnalyticalSun() {
		DebugPrint(_T("FireRenderAnalyticalSun::~FireRenderAnalyticalSun\n"));
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
		return new FireRenderAnalyticalSun();
	}

	// From Object
	ObjectState 	Eval(TimeValue time) {
		return ObjectState(this);
	}

	//makes part of the node name e.g. TheName001, if method returns L"TheName"
	void InitNodeName(TSTR& s) { s = FIRERENDER_ANALYTICALSUN_OBJECT_NAME; }

	//displayed in Modifier list
	const MCHAR *GetObjectName() override { return FIRERENDER_ANALYTICALSUN_OBJECT_NAME; }

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
		if ((REFMSG_CHANGE==msg)  && (hTarget == pblock2))
		{
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
		return FIRERENDER_ANALYTICALSUN_CLASS_ID;
	}

	//
	void GetClassName(TSTR& s) override { 
		FIRERENDER_ANALYTICALSUN_CLASS_NAME;
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
		FireRenderAnalyticalSun*	p;
		PLCB(FireRenderAnalyticalSun* pp) { p = pp; }

		void proc(ILoad *iload)
		{
			BgManagerMax &man = BgManagerMax::TheManager;

			INode* node = p->FindNodeRef(p);
			man.SetAnalyticalSunNode(node);

			delete this;
		}
	};

	IOResult Load(ILoad *iload) override
	{
		PLCB* plcb = new PLCB(this);
		iload->RegisterPostLoadCallback(plcb);
		return IO_OK;
	}

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
		if (!vpt || !vpt->IsAlive())
		{
			// why are we here
			DbgAssert(!_T("Invalid viewport!"));
			return FALSE;
		}

		GraphicsWindow *gw = vpt->getGW();

		DWORD rlim = gw->getRndLimits();

		//wireframe
		gw->setRndLimits(GW_WIREFRAME | GW_EDGES_ONLY | GW_TWO_SIDED | GW_ALL_EDGES | (rlim&GW_Z_BUFFER));

		gw->setColor(LINE_COLOR, GetViewportColor(inode, Color(0.8f, 0.8f, 0.0f)));

		// display sun shape
		Matrix3 tm = GetTransformMatrix(t, inode, vpt);
		gw->setTransform(tm);
		displayShape.render(gw, gw->getMaterial(), NULL, COMP_ALL, 1);

		// display line between sun and environment (parent node)
		// revert to world coordinate system
		Matrix3 identity;
		identity.IdentityMatrix();
		gw->setTransform(identity);
		// draw the line
		Matrix3 parentTm = inode->GetParentTM(t);
		Point3 v[3];
		v[0] = parentTm.GetTrans();
		v[1] = tm.GetTrans();
		gw->polyline(2, v, NULL, NULL, FALSE, NULL);

		gw->setRndLimits(rlim);

		return TRUE;
	}

	void GetWorldBoundBox(TimeValue t, INode* inode, ViewExp* vpt, Box3& box)
	{
		if (!vpt || !vpt->IsAlive())
		{
			box.Init();
			return;
		}

		Matrix3 tm;

		tm = inode->GetObjectTM(t);
		RemoveScaling(tm);
		float scaleFactor = vpt->NonScalingObjectSize() * vpt->GetVPWorldWidth(tm.GetTrans()) / 360.0f;

		Point3 loc = tm.GetTrans();

		box = displayShape.getBoundingBox(&tm);
		box.Scale(scaleFactor*0.5);
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

		HitRegion hitRegion;
		GraphicsWindow *gw = vpt->getGW();
		DWORD savedLimits = gw->getRndLimits();
		Material *mtl = gw->getMaterial();
		MakeHitRegion(hitRegion, type, crossing, 4, p);
		gw->setRndLimits((savedLimits | GW_PICK) & ~GW_ILLUM);

		gw->setTransform(GetTransformMatrix(t, inode, vpt));

		gw->clearHitCode();
		BOOL res = displayShape.select(gw, mtl, &hitRegion, flags & HIT_ABORTONHIT);
		
		gw->setRndLimits(savedLimits);
		return res;
	};


	int extDispFlags = 0;
	void SetExtendedDisplay(int flags)
	{
		extDispFlags = flags;
	}

	void GetLocalBoundBox(TimeValue t, INode* inode, ViewExp* vpt, Box3& box)
	{
		Interval ivalid = FOREVER;

		box = displayShape.getBoundingBox();

		Point3 loc = inode->GetObjectTM(t).GetTrans();
		float scaleFactor = vpt->NonScalingObjectSize()*vpt->GetVPWorldWidth(loc) / 360.0f;
		box.Scale(scaleFactor);
	}
		
	TimeValue getEditTime() {
		return GetCOREInterface()->GetTime();
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
		cs->type = OMNI_LGT;

		return REF_SUCCEED;
	}

	ObjLightDesc* CreateLightDesc(INode *inode, BOOL forceShadowBuf) override { return NULL; }
	// virtual GenLight *NewLight(int type);
	int Type() override { return OMNI_LIGHT; }

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
	BOOL GetAmbientOnly() override { return FALSE; }

	void SetDecayType(BOOL onOff) override {}
	BOOL GetDecayType() override { return 1; }
	void SetDecayRadius(TimeValue time, float f) override {}
	float GetDecayRadius(TimeValue t, Interval& valid) override { return 40.0f; } //! check different values
protected:
	ExclList m_exclList;
};

void*	FireRenderAnalyticalSunClassDesc::Create(BOOL loading) {
	return new FireRenderAnalyticalSun();
}

FIRERENDER_NAMESPACE_END

