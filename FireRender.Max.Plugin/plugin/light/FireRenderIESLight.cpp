/*********************************************************************************************************************************
* Radeon ProRender for 3ds Max plugin
* Copyright (c) 2017 AMD
* All Rights Reserved
*
* Custom IES light 3ds MAX scene node. This custom node allows the definition of IES light, using rectangular shapes
* that can be positioned and oriented in the MAX scene.
*********************************************************************************************************************************/

#include "Common.h"

#include "Resource.h"

#include "FireRenderIESLight.h"

#include "utils/Utils.h"

#include <math.h>

#include "../ParamBlock.h"
#include <iparamb2.h>
#include <LINSHAPE.H>
#include <3dsmaxport.h> //for DLGetWindowLongPtr
#include <hsv.h>

#include <decomp.h>
#include <iparamm2.h>
#include <memory>
#include <fstream>
#include <type_traits>

#include "LookAtTarget.h"

FIRERENDER_NAMESPACE_BEGIN

namespace
{
	constexpr int VERSION = 1;
	constexpr int IES_LIGHT_COLOR = 100;
	constexpr int IDC_IES_LIGHT_COLOR = 1000;
	constexpr auto FIRERENDER_IESLIGHT_CATEGORY = _T("Radeon ProRender");
	constexpr auto FIRERENDER_IESLIGHT_OBJECT_NAME = _T("RPRIESLight");
	constexpr auto FIRERENDER_IESLIGHT_INTERNAL_NAME = _T("RPRIESLight");
	constexpr auto FIRERENDER_IESLIGHT_CLASS_NAME = _T("RPRIESLight");

	class CreateCallBack : public CreateMouseCallBack
	{
	private:
		FireRenderIESLight *ob = nullptr;
		IParamBlock2 *pblock = nullptr;
		Point3 p0;
		Point3 p1;

	public:
		int proc(ViewExp *vpt, int msg, int point, int flags, IPoint2 m, Matrix3& mat) override
		{
			switch (msg)
			{
				case MOUSE_POINT:
					if (point == 0)
					{
						p0 = p1 = vpt->SnapPoint(m, m, NULL, SNAP_IN_3D);
						pblock->SetValue(IES_PARAM_P0, 0, p0);
						pblock->SetValue(IES_PARAM_P1, 0, p1);
					}
					else
					{
						ob->AddTarget();
						return 0;
					}
					break;

				case MOUSE_MOVE:
					p1 = vpt->SnapPoint(m, m, NULL, SNAP_IN_3D);
					pblock->SetValue(IES_PARAM_P1, 0, p1);
					break;

				case MOUSE_ABORT:
					return CREATE_ABORT;

				case MOUSE_FREEMOVE:
					vpt->SnapPreview(m, m, NULL, SNAP_IN_3D);
					break;
			}

			return CREATE_CONTINUE;
		}

		void SetObj(FireRenderIESLight* obj)
		{
			ob = obj;
			pblock = ob->GetParamBlock(0);
		}
	};

	class FireRenderIESLightClassDesc : public ClassDesc2
	{
	public:
		// DEACTIVATED! SET TO 1 TO REACTIVATE
		int IsPublic() override { return 1; }

		HINSTANCE HInstance() override
		{
			FASSERT(fireRenderHInstance != NULL);
			return fireRenderHInstance;
		}

		//used in MaxScript to create objects of this class
		const TCHAR* InternalName() override { return FIRERENDER_IESLIGHT_INTERNAL_NAME; }

		//used as lable in creation UI and in MaxScript to create objects of this class
		const TCHAR* ClassName() override { return FIRERENDER_IESLIGHT_CLASS_NAME; }

		Class_ID ClassID() override { return FireRenderIESLight::GetClassId(); }
		SClass_ID SuperClassID() override { return LIGHT_CLASS_ID; }
		const TCHAR* Category() override { return FIRERENDER_IESLIGHT_CATEGORY; }
		void* Create(BOOL loading) override { return new FireRenderIESLight(); }
	};

	static FireRenderIESLightClassDesc desc;

	static ParamBlockDesc2 paramBlock(0, _T("IESLightPbDesc"), 0, &desc, P_AUTO_CONSTRUCT + P_VERSION,
		VERSION, // for P_VERSION
		0, //for P_AUTO_CONSTRUCT

		IES_PARAM_P0, _T("p0"), TYPE_POINT3, 0, 0,
			p_default, Point3(0.f, 0.f, 0.f),
			PB_END,

		IES_PARAM_P1, _T("p1"), TYPE_POINT3, 0, 0,
			p_default, Point3(0.f, 0.f, 0.f),
			PB_END,

		// COLOR
		IES_LIGHT_COLOR, _T("Color"), TYPE_RGBA, P_ANIMATABLE, 0,
			p_default, Color(1.f, 1.f, 1.f),
			p_ui, TYPE_COLORSWATCH, IDC_IES_LIGHT_COLOR,
			PB_END,

		// Enabled parameter
		IES_PARAM_ENABLED, _T("Enabled"), TYPE_BOOL, P_ANIMATABLE, 0,
			p_default, TRUE,
			PB_END,

		// Area width parameter
		IES_PARAM_AREA_WIDTH, _T("AreaWidth"), TYPE_FLOAT, P_ANIMATABLE, 0,
			p_default, 1.0f,
			PB_END,

		IES_PARAM_TARGETED, _T("Targeted"), TYPE_BOOL, P_ANIMATABLE, 0,
			p_default, FALSE,
			PB_END,

		PB_END
	);

	// For type inference
	template<IESLightParameter p, typename Enabled = void>
	struct GetBlockValueHelper;

	template<IESLightParameter p>
	struct GetBlockValueHelper<p,
		std::enable_if_t<
			p == IES_PARAM_ENABLED ||
			p == IES_PARAM_TARGETED
		>>
	{
		using T1 = BOOL;
		using T2 = bool;
	};

	template<IESLightParameter p>
	struct GetBlockValueHelper<p,
		std::enable_if_t<
			p == IES_PARAM_AREA_WIDTH
		>>
	{
		using T1 = FLOAT;
		using T2 = float;
	};

	template<typename T>
	void SetBlockValue(IParamBlock2* pBlock, IESLightParameter parameter, T value)
	{
		auto res = pBlock->SetValue(parameter, 0, value);
		FASSERT(res);
	}

	template<IESLightParameter parameter>
	decltype(auto) GetBlockValue(IParamBlock2* pBlock)
	{
		using Helper = GetBlockValueHelper<parameter>;
		typename Helper::T1 result;

		auto ok = pBlock->GetValue(parameter, 0, result, FOREVER);
		FASSERT(ok);

		return static_cast<typename Helper::T2>(result);
	}
}

const Class_ID FireRenderIESLight::m_classId(0x7ab5467f, 0x1c96049f);

Class_ID FireRenderIESLight::GetClassId()
{
	return m_classId;
}

ClassDesc2* FireRenderIESLight::GetClassDesc()
{
	return &desc;
}

FireRenderIESLight::FireRenderIESLight() :
	m_general(this),
	m_intensity(this),
	m_shadows(this),
	m_volume(this),
	m_iObjParam(nullptr),
	m_pblock2(nullptr),
	m_verticesBuilt(false)
{
	GetClassDesc()->MakeAutoParamBlocks(this);
}

FireRenderIESLight::~FireRenderIESLight()
{
    DebugPrint(_T("FireRenderIESLight::~FireRenderIESLight\n"));
    DeleteAllRefs();
}

CreateMouseCallBack* FireRenderIESLight::GetCreateMouseCallBack()
{
	static CreateCallBack createCallback;
	createCallback.SetObj(this);
	return &createCallback;
}

GenLight *FireRenderIESLight::NewLight(int type)
{
    return new FireRenderIESLight();
}

// From Object
ObjectState FireRenderIESLight::Eval(TimeValue time)
{
    return ObjectState(this);
}

//makes part of the node name e.g. TheName001, if method returns L"TheName"
void FireRenderIESLight::InitNodeName(TSTR& s)
{
	s = FIRERENDER_IESLIGHT_OBJECT_NAME;
}

//displayed in Modifier list
const MCHAR* FireRenderIESLight::GetObjectName()
{
	return FIRERENDER_IESLIGHT_OBJECT_NAME;
}

Interval FireRenderIESLight::ObjectValidity(TimeValue t)
{
    Interval valid;
    valid.SetInfinite();
    return valid;
}

BOOL FireRenderIESLight::UsesWireColor()
{
	return 1;
}

int FireRenderIESLight::DoOwnSelectHilite()
{
	return 1;
}

void FireRenderIESLight::NotifyChanged()
{
	NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
}

// inherited virtual methods for Reference-management
void FireRenderIESLight::BuildVertices(bool force)
{
    if (m_verticesBuilt && (!force))
        return;

    Point3 _p0, _p1, p0, p1;

	m_pblock2->GetValue(IES_PARAM_P0, 0, p0, FOREVER);
	m_pblock2->GetValue(IES_PARAM_P1, 0, p1, FOREVER);

    _p0.x = std::min(p0.x, p1.x);
    _p0.y = std::min(p0.y, p1.y);
    _p0.z = std::min(p0.z, p1.z);
    _p1.x = std::max(p0.x, p1.x);
    _p1.y = std::max(p0.y, p1.y);
    _p1.z = std::max(p0.z, p1.z);

    m_vertices[0] = p0;
    m_vertices[1] = Point3(p1.x, p0.y, p0.z);
    m_vertices[2] = p1;
    m_vertices[3] = Point3(p0.x, p1.y, p0.z);

	m_verticesBuilt = true;
}

RefResult FireRenderIESLight::NotifyRefChanged(const Interval& interval, RefTargetHandle hTarget, PartID& partId, RefMessage msg, BOOL propagate)
{
    if ((REFMSG_CHANGE==msg)  && (hTarget == m_pblock2))
    {
        auto p = m_pblock2->LastNotifyParamID();
        if (p == IES_PARAM_P0 || p == IES_PARAM_P1)
            BuildVertices(true);
		//some params have changed - should redraw all
		if(GetCOREInterface()){
			GetCOREInterface()->RedrawViews(GetCOREInterface()->GetTime());
		}
    }
    return REF_SUCCEED;
}

void FireRenderIESLight::DeleteThis()
{ 
    delete this; 
}

Class_ID FireRenderIESLight::ClassID()
{
    return GetClassId();
}

//
void FireRenderIESLight::GetClassName(TSTR& s)
{
    FIRERENDER_IESLIGHT_CLASS_NAME;
}

RefTargetHandle FireRenderIESLight::Clone(RemapDir& remap)
{
    FireRenderIESLight* newob = new FireRenderIESLight();
    newob->m_verticesBuilt = m_verticesBuilt;
    newob->m_vertices[0] = m_vertices[0];
    newob->m_vertices[1] = m_vertices[1];
    newob->m_vertices[2] = m_vertices[2];
    newob->m_vertices[3] = m_vertices[3];
    newob->ReplaceReference(0, remap.CloneRef(m_pblock2));
    BaseClone(this, newob, remap);
    return(newob);
}

IParamBlock2* FireRenderIESLight::GetParamBlock(int i)
{
    FASSERT(i == 0);
    return m_pblock2;
}

IParamBlock2* FireRenderIESLight::GetParamBlockByID(BlockID id)
{
    FASSERT(m_pblock2->ID() == id);
    return m_pblock2;
}

int FireRenderIESLight::NumRefs()
{
    return 1;
}

void FireRenderIESLight::SetReference(int i, RefTargetHandle rtarg)
{
    FASSERT(0 == i);
	m_pblock2 = (IParamBlock2*)rtarg;
}

RefTargetHandle FireRenderIESLight::GetReference(int i)
{
    FASSERT(0 == i);
    return (RefTargetHandle)m_pblock2;
}

#define NUM_CIRC_PTS 28

void GetAttenPoints(TimeValue t, float rad, Point3 *q, Point3& center)
{
	double a;
	float sn;
	float cs;

	for (int i = 0; i < NUM_CIRC_PTS; i++)
	{
		a = (double)i * 2.0 * 3.1415926 / (double) NUM_CIRC_PTS;
		sn = rad * (float)sin(a);
		cs = rad * (float)cos(a);
		q[i + 0 * NUM_CIRC_PTS] = Point3(sn, cs, 0.0f) + center;
		q[i + 1 * NUM_CIRC_PTS] = Point3(sn, 0.0f, cs) + center;
		q[i + 2 * NUM_CIRC_PTS] = Point3(0.0f, sn, cs) + center;
	}
}

void DrawSphereArcs(TimeValue t, GraphicsWindow *gw, float r, Point3 *q, Point3& center)
{
	GetAttenPoints(t, r, q, center);
	gw->polyline(NUM_CIRC_PTS, q, NULL, NULL, TRUE, NULL);
	gw->polyline(NUM_CIRC_PTS, q + NUM_CIRC_PTS, NULL, NULL, TRUE, NULL);
	gw->polyline(NUM_CIRC_PTS, q + 2 * NUM_CIRC_PTS, NULL, NULL, TRUE, NULL);
}

void FireRenderIESLight::DrawGeometry(ViewExp *vpt, IParamBlock2 *pblock, BOOL sel, BOOL frozen)
{
	GraphicsWindow* gw = vpt->getGW();
	
	BuildVertices();

	Color color = frozen ? Color(0.0f, 0.0f, 1.0f) : Color(0.0f, 1.0f, 0.0f);

    if (sel)
    {
        color =  Color(1.0f, 0.0f, 0.0f);
    }

	gw->setColor(LINE_COLOR, color);
        
	Point3 sphereMesh[3 * NUM_CIRC_PTS];
	Point3 dirMesh[2];

	pblock->GetValue(IES_PARAM_P0, 0, dirMesh[0], FOREVER);
	pblock->GetValue(IES_PARAM_P1, 0, dirMesh[1], FOREVER);

	// light source
	DrawSphereArcs(0, gw, 2.0, sphereMesh, dirMesh[0]);

	// draw direction
	gw->polyline(2, dirMesh, NULL, NULL, FALSE, NULL);

	// light lookAt
	DrawSphereArcs(0, gw, 1.0, sphereMesh, dirMesh[1]);


	// marker
	//gw->marker(const_cast<Point3*>(&Point3::Origin), X_MRKR);
}

Matrix3 FireRenderIESLight::GetTransformMatrix(TimeValue t, INode* inode, ViewExp* vpt)
{
    Matrix3 tm = inode->GetObjectTM(t);
    return tm;
}

Color FireRenderIESLight::GetViewportMainColor(INode* pNode)
{
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
    return Color(1,1,0);
}

Color FireRenderIESLight::GetViewportColor(INode* pNode, Color selectedColor)
{
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
    return selectedColor * 0.5f;
}

int FireRenderIESLight::Display(TimeValue t, INode* inode, ViewExp *vpt, int flags)
{
    if (!vpt || !vpt->IsAlive())
    {
        // why are we here
        DbgAssert(!_T("Invalid viewport!"));
        return FALSE;
    }

    Matrix3 prevtm = vpt->getGW()->getTransform();
    Matrix3 tm = inode->GetObjectTM(t);
    vpt->getGW()->setTransform(tm);
    DrawGeometry(vpt, GetParamBlock(0), inode->Selected(), inode->IsFrozen());
    vpt->getGW()->setTransform(prevtm);

    return(0);
}
    
void FireRenderIESLight::GetWorldBoundBox(TimeValue t, INode* inode, ViewExp* vpt, Box3& box)
{
    if (!vpt || !vpt->IsAlive())
    {
        box.Init();
        return;
    }

    Matrix3 worldTM = inode->GetNodeTM(t);
    Point3 _Vertices[4];
    for (int i = 0; i < 4; i++)
        _Vertices[i] = m_vertices[i] * worldTM;

    box.Init();
    for (int i = 0; i < 4; i++)
    {
        box.pmin.x = std::min(box.pmin.x, _Vertices[i].x);
        box.pmin.y = std::min(box.pmin.y, _Vertices[i].y);
        box.pmin.z = std::min(box.pmin.z, _Vertices[i].z);
        box.pmax.x = std::max(box.pmin.x, _Vertices[i].x);
        box.pmax.y = std::max(box.pmin.y, _Vertices[i].y);
        box.pmax.z = std::max(box.pmin.z, _Vertices[i].z);
    }
}

int FireRenderIESLight::HitTest(TimeValue t, INode* inode, int type, int crossing, int flags, IPoint2 *p, ViewExp *vpt)
{
    if (!vpt || !vpt->IsAlive())
    {
        // why are we here
        DbgAssert(!_T("Invalid viewport!"));
        return FALSE;
    }

    HitRegion hitRegion;
    DWORD savedLimits;

    GraphicsWindow *gw = vpt->getGW();
    Matrix3 prevtm = gw->getTransform();

    gw->setTransform(idTM);
    MakeHitRegion(hitRegion, type, crossing, 8, p);
    savedLimits = gw->getRndLimits();

    gw->setRndLimits((savedLimits | GW_PICK) & ~GW_ILLUM & ~GW_Z_BUFFER);
    gw->setHitRegion(&hitRegion);
    gw->clearHitCode();

    Matrix3 tm = inode->GetObjectTM(t);
    gw->setTransform(tm);

    DrawGeometry(vpt, GetParamBlock(0));

    int res = gw->checkHitCode();

    gw->setRndLimits(savedLimits);
    gw->setTransform(prevtm);

    return res;
}

void FireRenderIESLight::GetLocalBoundBox(TimeValue t, INode* inode, ViewExp* vpt, Box3& box)
{
    Matrix3 nodeTM = inode->GetNodeTM(t);
    Matrix3 parentTM = inode->GetParentNode()->GetNodeTM(t);
    Matrix3 localTM = nodeTM*Inverse(parentTM);

    Point3 _Vertices[4];
    for (int i = 0; i < 4; i++)
        _Vertices[i] = m_vertices[i] * localTM;

    box.Init();
    for (int i = 0; i < 4; i++)
    {
        box.pmin.x = std::min(box.pmin.x, _Vertices[i].x);
        box.pmin.y = std::min(box.pmin.y, _Vertices[i].y);
        box.pmin.z = std::min(box.pmin.z, _Vertices[i].z);
        box.pmax.x = std::max(box.pmin.x, _Vertices[i].x);
        box.pmax.y = std::max(box.pmin.y, _Vertices[i].y);
        box.pmax.z = std::max(box.pmin.z, _Vertices[i].z);
    }
}

// LightObject methods

Point3 FireRenderIESLight::GetRGBColor(TimeValue t, Interval &valid) { return Point3(1.0f, 1.0f, 1.0f); }
void FireRenderIESLight::SetRGBColor(TimeValue t, Point3 &color)  { /* empty */ }
void FireRenderIESLight::SetUseLight(int onOff)  { /* empty */ }
BOOL FireRenderIESLight::GetUseLight()  { return TRUE; }
void FireRenderIESLight::SetHotspot(TimeValue time, float f)  { /* empty */ }
float FireRenderIESLight::GetHotspot(TimeValue t, Interval& valid)  { return -1.0f; }
void FireRenderIESLight::SetFallsize(TimeValue time, float f)  { /* empty */ }
float FireRenderIESLight::GetFallsize(TimeValue t, Interval& valid)  { return -1.0f; }
void FireRenderIESLight::SetAtten(TimeValue time, int which, float f)  { /* empty */ }
float FireRenderIESLight::GetAtten(TimeValue t, int which, Interval& valid)  { return 0.0f; }
void FireRenderIESLight::SetTDist(TimeValue time, float f)  { /* empty */ }
float FireRenderIESLight::GetTDist(TimeValue t, Interval& valid)  { return 0.0f; }
void FireRenderIESLight::SetConeDisplay(int s, int notify)  { /* empty */ }
BOOL FireRenderIESLight::GetConeDisplay()  { return FALSE; }
void FireRenderIESLight::SetIntensity(TimeValue time, float f)  { /* empty */ }
float FireRenderIESLight::GetIntensity(TimeValue t, Interval& valid)  { return 1.0f; }
void FireRenderIESLight::SetUseAtten(int s)  { /* empty */ }
BOOL FireRenderIESLight::GetUseAtten()  { return FALSE; }
void FireRenderIESLight::SetAspect(TimeValue t, float f)  { /* empty */ }
float FireRenderIESLight::GetAspect(TimeValue t, Interval& valid)  { return -1.0f; }
void FireRenderIESLight::SetOvershoot(int a)  { /* empty */ }
int FireRenderIESLight::GetOvershoot()  { return 0; }
void FireRenderIESLight::SetShadow(int a)  { /* empty */ }
int FireRenderIESLight::GetShadow()  { return 0; }

// From Light
RefResult FireRenderIESLight::EvalLightState(TimeValue t, Interval& valid, LightState* cs)
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

ObjLightDesc* FireRenderIESLight::CreateLightDesc(INode *inode, BOOL forceShadowBuf)  { return NULL; }

int FireRenderIESLight::Type()  { return OMNI_LIGHT; }

void FireRenderIESLight::SetHSVColor(TimeValue, Point3 &)
{
}

Point3 FireRenderIESLight::GetHSVColor(TimeValue t, Interval &valid)
{
    int h, s, v;
    Point3 rgbf = GetRGBColor(t, valid);
    DWORD rgb = RGB((int)(rgbf[0] * 255.0f), (int)(rgbf[1] * 255.0f), (int)(rgbf[2] * 255.0f));
    RGBtoHSV(rgb, &h, &s, &v);
    return Point3(h / 255.0f, s / 255.0f, v / 255.0f);
}


void FireRenderIESLight::SetAttenDisplay(int)  { /* empty */ }
BOOL FireRenderIESLight::GetAttenDisplay()  { return FALSE; }
void FireRenderIESLight::Enable(int)  {}
void FireRenderIESLight::SetMapBias(TimeValue, float)  { /* empty */ }
float FireRenderIESLight::GetMapBias(TimeValue, Interval &)  { return 0; }
void FireRenderIESLight::SetMapRange(TimeValue, float)  { /* empty */ }
float FireRenderIESLight::GetMapRange(TimeValue, Interval &)  { return 0; }
void FireRenderIESLight::SetMapSize(TimeValue, int)  { /* empty */ }
int  FireRenderIESLight::GetMapSize(TimeValue, Interval &)  { return 0; }
void FireRenderIESLight::SetRayBias(TimeValue, float)  { /* empty */ }
float FireRenderIESLight::GetRayBias(TimeValue, Interval &)  { return 0; }
int FireRenderIESLight::GetUseGlobal()  { return FALSE; }
void FireRenderIESLight::SetUseGlobal(int)  { /* empty */ }
int FireRenderIESLight::GetShadowType()  { return -1; }
void FireRenderIESLight::SetShadowType(int)  { /* empty */ }
int FireRenderIESLight::GetAbsMapBias()  { return FALSE; }
void FireRenderIESLight::SetAbsMapBias(int)  { /* empty */ }
BOOL FireRenderIESLight::IsSpot()  { return FALSE; }
BOOL FireRenderIESLight::IsDir()  { return FALSE; }
void FireRenderIESLight::SetSpotShape(int)  { /* empty */ }
int FireRenderIESLight::GetSpotShape()  { return CIRCLE_LIGHT; }
void FireRenderIESLight::SetContrast(TimeValue, float)  { /* empty */ }
float FireRenderIESLight::GetContrast(TimeValue, Interval &)  { return 0; }
void FireRenderIESLight::SetUseAttenNear(int)  { /* empty */ }
BOOL FireRenderIESLight::GetUseAttenNear()  { return FALSE; }
void FireRenderIESLight::SetAttenNearDisplay(int)  { /* empty */ }
BOOL FireRenderIESLight::GetAttenNearDisplay()  { return FALSE; }
ExclList & FireRenderIESLight::GetExclusionList()  { return m_exclList; }
void FireRenderIESLight::SetExclusionList(ExclList &)  { /* empty */ }

BOOL FireRenderIESLight::SetHotSpotControl(Control *)  { return FALSE; }
BOOL FireRenderIESLight::SetFalloffControl(Control *)  { return FALSE; }
BOOL FireRenderIESLight::SetColorControl(Control *)  { return FALSE; }
Control * FireRenderIESLight::GetHotSpotControl()  { return NULL; }
Control * FireRenderIESLight::GetFalloffControl()  { return NULL; }
Control * FireRenderIESLight::GetColorControl()  { return NULL; }

void FireRenderIESLight::SetAffectDiffuse(BOOL onOff)  { /* empty */ }
BOOL FireRenderIESLight::GetAffectDiffuse()  { return TRUE; }
void FireRenderIESLight::SetAffectSpecular(BOOL onOff)  { /* empty */ }
BOOL FireRenderIESLight::GetAffectSpecular()  { return TRUE; }
void FireRenderIESLight::SetAmbientOnly(BOOL onOff)  { /* empty */ }
BOOL FireRenderIESLight::GetAmbientOnly()  { return FALSE; }

void FireRenderIESLight::SetDecayType(BOOL onOff)  {}
BOOL FireRenderIESLight::GetDecayType()  { return 1; }
void FireRenderIESLight::SetDecayRadius(TimeValue time, float f)  {}
float FireRenderIESLight::GetDecayRadius(TimeValue t, Interval& valid)  { return 40.0f; } //! check different values

void FireRenderIESLight::BeginEditParams(IObjParam* objParam, ULONG flags, Animatable* prev)
{
	m_iObjParam = objParam;
	bool inCreate = (flags & BEGIN_EDIT_CREATE) ? 1 : 0;

	auto beginEdit = [&](auto& panel)
	{
		panel.BeginEdit(objParam, flags, prev);
	};

	beginEdit(m_general);
	beginEdit(m_intensity);
	beginEdit(m_shadows);
	beginEdit(m_volume);
}

void FireRenderIESLight::EndEditParams(IObjParam* objParam, ULONG flags, Animatable* next)
{
	auto endEdit = [&](auto& panel)
	{
		panel.EndEdit(objParam, flags, next);
	};

	endEdit(m_general);
	endEdit(m_intensity);
	endEdit(m_shadows);
	endEdit(m_volume);
}

INode* FindNodeRef(ReferenceTarget *rt);

static INode* GetNodeRef(ReferenceMaker *rm) {
	if (rm->SuperClassID() == BASENODE_CLASS_ID) return (INode *)rm;
	else return rm->IsRefTarget() ? FindNodeRef((ReferenceTarget *)rm) : NULL;
}

static INode* FindNodeRef(ReferenceTarget *rt) {
	DependentIterator di(rt);
	ReferenceMaker *rm = NULL;
	INode *nd = NULL;
	while ((rm = di.Next()) != NULL) {
		nd = GetNodeRef(rm);
		if (nd) return nd;
	}
	return NULL;
}

void FireRenderIESLight::CreateSceneLight(const ParsedNode& node, frw::Scope scope, const RenderParameters& params)
{
	// create light
	rpr_light light = 0;
	rpr_int res = rprContextCreateIESLight(scope.GetContext().Handle(), &light);
	FASSERT(res == RPR_SUCCESS);

	// load IES data
	std::string iesFile("D:\\download\\IES light\\10817_AWF.ies");

	std::string iesData((std::istreambuf_iterator<char>(std::ifstream(iesFile))),
		std::istreambuf_iterator<char>());

	res = rprIESLightSetImageFromIESdata(light, iesData.c_str(), 256, 256);
	FASSERT(res == RPR_SUCCESS);

	// setup color & intensity
	Point3 color = GetRGBColor(params.t);
	float intensity = GetIntensity(params.t);

	color *= intensity * 100;

	res = rprIESLightSetRadiantPower3f(light, color.x, color.y, color.z);
	FASSERT(res == RPR_SUCCESS);

	// setup position
	Matrix3 tm;
	tm.IdentityMatrix();

	float frTm[16];
	CreateFrMatrix(fxLightTm(tm), frTm);
	
	res = rprLightSetTransform(light, false, frTm);
	FASSERT(res == RPR_SUCCESS);

	// attach to scene	
	auto fireLight = frw::Light(light, scope.GetContext());
	fireLight.SetUserData(node.id);
	scope.GetScene().Attach(fireLight);
}

void FireRenderIESLight::AddTarget()
{
	INode *nd = FindNodeRef(this);

	Interface *iface = GetCOREInterface();
	TimeValue t = iface->GetTime();

	Point3 p;
	m_pblock2->GetValue(IES_PARAM_P1, 0, p, FOREVER);

	Matrix3 tm = nd->GetNodeTM(t);
	Matrix3 targtm = tm;
	
	targtm.PreTranslate(p);

	Object *targObject = new LookAtTarget;
	INode *targNode = iface->CreateObjectNode(targObject);
	
	TSTR targName;
	targName = nd->GetName();
	targName += _T("DOT_TARGET");
	targNode->SetName(targName);
	
	Control *laControl = CreateLookatControl();
	targNode->SetNodeTM(0, targtm);
	laControl->SetTarget(targNode);
	laControl->Copy(nd->GetTMController());
	nd->SetTMController(laControl);

	targNode->SetIsTarget(TRUE);

	Color lightWireColor(nd->GetWireColor());
	targNode->SetWireColor(lightWireColor.toRGB());
}

void FireRenderIESLight::SetEnabled(bool value)
{
	SetBlockValue(m_pblock2, IES_PARAM_ENABLED, value);
}

bool FireRenderIESLight::GetEnabled() const
{
	return GetBlockValue<IES_PARAM_ENABLED>(m_pblock2);
}

void FireRenderIESLight::SetTargeted(bool value)
{
	SetBlockValue(m_pblock2, IES_PARAM_TARGETED, value);
}

bool FireRenderIESLight::GetTargeted() const
{
	return GetBlockValue<IES_PARAM_TARGETED>(m_pblock2);
}

void FireRenderIESLight::SetAreaWidth(float value)
{

	SetBlockValue(m_pblock2, IES_PARAM_AREA_WIDTH, value);
}

float FireRenderIESLight::GetAreaWidth() const
{
	return GetBlockValue<IES_PARAM_AREA_WIDTH>(m_pblock2);
}

void FireRenderIESLight::ActivateProfile(const TCHAR* profileName)
{
	//TODO:
	FASSERT(!"Not implemented");
}

FIRERENDER_NAMESPACE_END
