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
#include "FireRenderIES_Profiles.h"

#include "IESprocessor.h"

FIRERENDER_NAMESPACE_BEGIN

namespace
{
	constexpr int VERSION = 1;
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
						mat.SetTrans(vpt->SnapPoint(m, m, NULL, SNAP_IN_3D));
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

		// Enabled parameter
		IES_PARAM_ENABLED, _T("Enabled"), TYPE_BOOL, P_ANIMATABLE, 0,
			p_default, FireRenderIESLight::DefaultEnabled,
			PB_END,

		// Profile name parameter
		IES_PARAM_PROFILE,  _T("Profile"), TYPE_STRING, 0, 0,
			p_default, "",
			PB_END,

		// Area width parameter
		IES_PARAM_AREA_WIDTH, _T("AreaWidth"), TYPE_FLOAT, P_ANIMATABLE, 0,
			p_default, FireRenderIESLight::AreaWidthSettings::Default,
			PB_END,

		// Targeted parameter
		IES_PARAM_TARGETED, _T("Targeted"), TYPE_BOOL, P_ANIMATABLE, 0,
			p_default, FireRenderIESLight::DefaultTargeted,
			PB_END,

		// Light intensity parameter
		IES_PARAM_INTENSITY, _T("Intensity"), TYPE_FLOAT, P_ANIMATABLE, 0,
			p_default, FireRenderIESLight::IntensitySettings::Default,
			PB_END,

		// Color mode parameter
		IES_PARAM_COLOR_MODE, _T("ColorMode"), TYPE_INT, P_ANIMATABLE, 0,
			p_default, FireRenderIESLight::DefaultColorMode,
			PB_END,

		// COLOR
		IES_PARAM_COLOR, _T("Color"), TYPE_RGBA, P_ANIMATABLE, 0,
			p_default, Color(1.f, 1.f, 1.f),
			PB_END,

		// Temperature in Kelvin
		IES_PARAM_TEMPERATURE, _T("Temperature"), TYPE_FLOAT, P_ANIMATABLE, 0,
			p_default, DefaultKelvin,
			p_range, MinKelvin, MaxKelvin,
			PB_END,

		// Shadows enabled parameter
		IES_PARAM_SHADOWS_ENABLED, _T("ShadowsEnabled"), TYPE_BOOL, P_ANIMATABLE, 0,
			p_default, FireRenderIESLight::DefaultShadowsEnabled,
			PB_END,

		// Shadows softness parameter
		IES_PARAM_SHADOWS_SOFTNESS, _T("ShadowsSoftness"), TYPE_FLOAT, P_ANIMATABLE, 0,
			p_default, FireRenderIESLight::ShadowsSoftnessSettings::Default,
			PB_END,

		// Shadows transparency parameter
		IES_PARAM_SHADOWS_TRANSPARENCY, _T("ShadowsTransparency"), TYPE_FLOAT, P_ANIMATABLE, 0,
			p_default, FireRenderIESLight::ShadowsTransparencySettings::Default,
			PB_END,

		// Volume scale parameter
		IES_PARAM_VOLUME_SCALE, _T("VolumeScale"), TYPE_FLOAT, P_ANIMATABLE, 0,
			p_default, FireRenderIESLight::VolumeScaleSettings::Default,
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
			p == IES_PARAM_TARGETED ||
			p == IES_PARAM_SHADOWS_ENABLED
		>>
	{
		using T1 = BOOL;
		using T2 = bool;
	};

	template<IESLightParameter p>
	struct GetBlockValueHelper<p,
		std::enable_if_t<
			p == IES_PARAM_COLOR_MODE
		>>
	{
		using T1 = INT;
		using T2 = int;
	};

	template<IESLightParameter p>
	struct GetBlockValueHelper<p,
		std::enable_if_t<
			p == IES_PARAM_AREA_WIDTH ||
			p == IES_PARAM_INTENSITY ||
			p == IES_PARAM_TEMPERATURE ||
			p == IES_PARAM_SHADOWS_SOFTNESS ||
			p == IES_PARAM_SHADOWS_TRANSPARENCY ||
			p == IES_PARAM_VOLUME_SCALE
		>>
	{
		using T1 = FLOAT;
		using T2 = float;
	};

	template<IESLightParameter p>
	struct GetBlockValueHelper<p,
		std::enable_if_t<
			p == IES_PARAM_COLOR
		>>
	{
		using T1 = Color;
		using T2 = T1;
	};

	template<IESLightParameter p>
	struct GetBlockValueHelper<p,
		std::enable_if_t<
			p == IES_PARAM_PROFILE
		>>
	{
		using T1 = const TCHAR*;
		using T2 = T1;
	};

	template<typename T>
	void SetBlockValue(IParamBlock2* pBlock, IESLightParameter parameter, T value, TimeValue time = 0)
	{
		auto res = pBlock->SetValue(parameter, time, value);
		FASSERT(res);
	}

	template<IESLightParameter parameter>
	decltype(auto) GetBlockValue(IParamBlock2* pBlock, TimeValue t = 0, Interval valid = FOREVER)
	{
		using Helper = GetBlockValueHelper<parameter>;
		typename Helper::T1 result;

		auto ok = pBlock->GetValue(parameter, t, result, valid);
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
	m_verticesBuilt(false),
	prevUp(0.0f, 1.0f, 0.0f),
	m_preview_plines(),
	m_plines()
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

const IParamBlock2* FireRenderIESLight::GetParamBlock(int i) const
{
	return const_cast<const IParamBlock2*>
		(const_cast<FireRenderIESLight*>(this)->GetParamBlock(i));
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

INode* FindNodeRef(ReferenceTarget *rt);

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

	INode *nd = FindNodeRef(this);
	Control* pLookAtController = nd->GetTMController();
	if ((pLookAtController == nullptr) || (pLookAtController->GetTarget() == nullptr))
	{
		dirMesh[1] = dirMesh[1] - dirMesh[0];
	}
	else
	{
		INode* pTargNode = pLookAtController->GetTarget();
		Matrix3 targTransform = pTargNode->GetObjectTM(0.0);
		Point3 targVector(0.0f, 0.0f, 0.0f);
		targVector = targVector * targTransform;

		Matrix3 rootTransform = nd->GetObjectTM(0.0);
		Point3 rootVector(0.0f, 0.0f, 0.0f);
		rootVector = rootVector * rootTransform;

		float dist = (targVector - rootVector).FLength();
		dirMesh[1] = Point3(0.0f, 0.0f, -dist);
	}
	dirMesh[0] = Point3(0.0f, 0.0f, 0.0f);

	// light source
	DrawSphereArcs(0, gw, 2.0, sphereMesh, dirMesh[0]);

	// draw direction
	gw->polyline(2, dirMesh, NULL, NULL, FALSE, NULL);

	// light lookAt
	DrawSphereArcs(0, gw, 1.0, sphereMesh, dirMesh[1]);
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

bool FireRenderIESLight::DisplayLight(TimeValue t, INode* inode, ViewExp *vpt, int flags)
{
	if (!vpt || !vpt->IsAlive())
	{
		// why are we here
		DbgAssert(!_T("Invalid viewport!"));
		return false;
	}

	Matrix3 prevtm = vpt->getGW()->getTransform();
	Matrix3 tm = inode->GetObjectTM(t);

	// apply scaling
	float scaleFactor = vpt->NonScalingObjectSize() * vpt->GetVPWorldWidth(tm.GetTrans()) / 360.0f;
	if (scaleFactor!=(float)1.0)
		tm.Scale(Point3(scaleFactor,scaleFactor,scaleFactor));

	vpt->getGW()->setTransform(tm);

	DrawWeb(vpt, GetParamBlock(0), inode->Selected(), inode->IsFrozen());

	vpt->getGW()->setTransform(prevtm);

	return true;
}

#define WEB_ENABLED
int FireRenderIESLight::Display(TimeValue t, INode* inode, ViewExp *vpt, int flags)
{
#ifdef WEB_ENABLED
	DisplayLight(t, inode, vpt, flags);
#else
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
#endif

    return(1);
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
#ifdef WEB_ENABLED
	DisplayLight(t, inode, vpt, flags);
#else
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
#endif
    return (1);
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

// LightObject dummy implementation
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
void FireRenderIESLight::SetUseAtten(int s)  { /* empty */ }
BOOL FireRenderIESLight::GetUseAtten()  { return FALSE; }
void FireRenderIESLight::SetAspect(TimeValue t, float f)  { /* empty */ }
float FireRenderIESLight::GetAspect(TimeValue t, Interval& valid)  { return -1.0f; }
void FireRenderIESLight::SetOvershoot(int a)  { /* empty */ }
int FireRenderIESLight::GetOvershoot()  { return 0; }
void FireRenderIESLight::SetShadow(int a)  { /* empty */ }
int FireRenderIESLight::GetShadow()  { return 0; }

// LightObject custom implementation
void FireRenderIESLight::SetIntensity(TimeValue time, float f)
{
	SetBlockValue(m_pblock2, IES_PARAM_INTENSITY, f, time);
}

float FireRenderIESLight::GetIntensity(TimeValue t, Interval& valid)
{
	return GetBlockValue<IES_PARAM_INTENSITY>(m_pblock2, t, valid);
}

void FireRenderIESLight::SetRGBColor(TimeValue time, Point3 &color)
{
	SetBlockValue(m_pblock2, IES_PARAM_COLOR, color, time);
}

Point3 FireRenderIESLight::GetRGBColor(TimeValue t, Interval &valid)
{
	return GetBlockValue<IES_PARAM_COLOR>(m_pblock2, t, valid);
}


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
	auto light = scope.GetContext().CreateIESLight();

	// load IES data
	auto profilePath = FireRenderIES_Profiles::ProfileNameToPath(GetActiveProfile());

	std::string iesData(
		(std::istreambuf_iterator<char>(std::ifstream(profilePath))),
		std::istreambuf_iterator<char>());

	light.SetImageFromData(iesData.c_str(), 256, 256);

	// setup color & intensity
	auto color = GetFinalColor(params.t) * GetIntensity(params.t) * 100;

	light.SetRadiantPower(color);

	// setup position
	Matrix3 tm;
	tm.IdentityMatrix();

	float frTm[16];
	CreateFrMatrix(fxLightTm(tm), frTm);
	
	light.SetTransform(frTm, false);

	// attach to scene
	light.SetUserData(node.id);
	scope.GetScene().Attach(light);
}

void FireRenderIESLight::AddTarget()
{
	INode *nd = FindNodeRef(this);

	Interface *iface = GetCOREInterface();
	TimeValue t = iface->GetTime();

	Point3 p1;
	m_pblock2->GetValue(IES_PARAM_P1, 0, p1, FOREVER);
	Point3 p0;
	m_pblock2->GetValue(IES_PARAM_P0, 0, p0, FOREVER);
	Point3 r = p1 - p0;

	Matrix3 tm = nd->GetNodeTM(t);
	Matrix3 targtm = tm;
	
	targtm.PreTranslate(r);

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

void FireRenderIESLight::SetIntensity(float value)
{
	SetBlockValue(m_pblock2, IES_PARAM_INTENSITY, value);
}

float FireRenderIESLight::GetIntensity() const
{
	return GetBlockValue<IES_PARAM_INTENSITY>(m_pblock2);
}

void FireRenderIESLight::SetTemperature(float value)
{
	SetBlockValue(m_pblock2, IES_PARAM_TEMPERATURE, value);
}

float FireRenderIESLight::GetTemperature() const
{
	return GetBlockValue<IES_PARAM_TEMPERATURE>(m_pblock2);
}

void FireRenderIESLight::SetColor(Color value)
{
	SetBlockValue(m_pblock2, IES_PARAM_COLOR, value);
}

Color FireRenderIESLight::GetColor() const
{
	return GetBlockValue<IES_PARAM_COLOR>(m_pblock2);
}

void FireRenderIESLight::SetColorMode(IESLightColorMode value)
{
	SetBlockValue(m_pblock2, IES_PARAM_COLOR_MODE, value);
}

IESLightColorMode FireRenderIESLight::GetColorMode() const
{
	return
		static_cast<IESLightColorMode>(
			GetBlockValue<IES_PARAM_COLOR_MODE>(m_pblock2));
}

void FireRenderIESLight::SetShadowsEnabled(bool value)
{
	SetBlockValue(m_pblock2, IES_PARAM_SHADOWS_ENABLED, value);
}

bool FireRenderIESLight::GetShadowsEnabled() const
{
	return GetBlockValue<IES_PARAM_SHADOWS_ENABLED>(m_pblock2);
}

void FireRenderIESLight::SetShadowsSoftness(float value)
{
	SetBlockValue(m_pblock2, IES_PARAM_SHADOWS_SOFTNESS, value);
}

float FireRenderIESLight::GetShadowsSoftness() const
{
	return GetBlockValue<IES_PARAM_SHADOWS_SOFTNESS>(m_pblock2);
}

void FireRenderIESLight::SetShadowsTransparency(float value)
{
	SetBlockValue(m_pblock2, IES_PARAM_SHADOWS_TRANSPARENCY, value);
}

float FireRenderIESLight::GetShadowsTransparency() const
{
	return GetBlockValue<IES_PARAM_SHADOWS_TRANSPARENCY>(m_pblock2);
}

void FireRenderIESLight::SetVolumeScale(float value)
{
	SetBlockValue(m_pblock2, IES_PARAM_VOLUME_SCALE, value);
}

float FireRenderIESLight::GetVolumeScale() const
{
	return GetBlockValue<IES_PARAM_VOLUME_SCALE>(m_pblock2);
}

void FireRenderIESLight::SetActiveProfile(const TCHAR* profileName)
{
	SetBlockValue(m_pblock2, IES_PARAM_PROFILE, profileName);
}

const TCHAR* FireRenderIESLight::GetActiveProfile() const
{
	return GetBlockValue<IES_PARAM_PROFILE>(m_pblock2);
}

// Result depends on color mode
Color FireRenderIESLight::GetFinalColor(TimeValue t, Interval& i) const
{
	Color result(1.f, 1.f, 1.f);

	switch (GetColorMode())
	{
		case IES_LIGHT_COLOR_MODE_COLOR:
			result = GetBlockValue<IES_PARAM_COLOR>(m_pblock2, t, i);
			break;

		case IES_LIGHT_COLOR_MODE_TEMPERATURE:
			result = KelvinToColor(GetBlockValue<IES_PARAM_TEMPERATURE>(m_pblock2, t, i));
			break;

		default:
			FASSERT(!"Not implemented!");
			break;
	}

	return result;
}

FIRERENDER_NAMESPACE_END
