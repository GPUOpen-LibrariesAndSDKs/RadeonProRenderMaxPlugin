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

#include <array>
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

	public:
		int proc(ViewExp *vpt, int msg, int point, int flags, IPoint2 m, Matrix3& mat) override
		{
			switch (msg)
			{
				case MOUSE_POINT:
					if (point == 0)
					{
						mat.SetTrans(vpt->SnapPoint(m, m, NULL, SNAP_IN_3D));
						Point3 p = vpt->SnapPoint(m, m, NULL, SNAP_IN_3D);
						ob->SetLightPoint(p);
						ob->SetTargetPoint(p);
					}
					else
					{
						ob->AddTarget();
						return 0;
					}
					break;

				case MOUSE_MOVE:
					pblock->SetValue(IES_PARAM_P1, 0,
						vpt->SnapPoint(m, m, NULL, SNAP_IN_3D));
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
		void* Create(BOOL loading) override;

	};

	static FireRenderIESLightClassDesc desc;
	static bool parametersCacheEnabled = false;

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

		// Distance from light to target
		IES_PARAM_TARGET_DISTANCE, _T("TargetDistance"), TYPE_FLOAT, P_ANIMATABLE, 0,
			p_default, FireRenderIESLight::TargetDistanceSettings::Default,
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
		using TCache = T2;
	};

	template<IESLightParameter p>
	struct GetBlockValueHelper<p,
		std::enable_if_t<
			p == IES_PARAM_COLOR_MODE
		>>
	{
		using T1 = INT;
		using T2 = int;
		using TCache = T2;
	};

	template<IESLightParameter p>
	struct GetBlockValueHelper<p,
		std::enable_if_t<
			p == IES_PARAM_AREA_WIDTH ||
			p == IES_PARAM_INTENSITY ||
			p == IES_PARAM_TEMPERATURE ||
			p == IES_PARAM_SHADOWS_SOFTNESS ||
			p == IES_PARAM_SHADOWS_TRANSPARENCY ||
			p == IES_PARAM_VOLUME_SCALE ||
			p == IES_PARAM_TARGET_DISTANCE
		>>
	{
		using T1 = FLOAT;
		using T2 = float;
		using TCache = T2;
	};

	template<IESLightParameter p>
	struct GetBlockValueHelper<p,
		std::enable_if_t<
			p == IES_PARAM_COLOR
		>>
	{
		using T1 = Color;
		using T2 = T1;
		using TCache = T2;
	};

	template<IESLightParameter p>
	struct GetBlockValueHelper<p,
		std::enable_if_t<
			p == IES_PARAM_P0 ||
			p == IES_PARAM_P1
		>>
	{
		using T1 = Point3;
		using T2 = T1;
		using TCache = T2;
	};

	template<IESLightParameter p>
	struct GetBlockValueHelper<p,
		std::enable_if_t<
			p == IES_PARAM_PROFILE
		>>
	{
		using T1 = const TCHAR*;
		using T2 = T1;
		using TCache = std::basic_string<TCHAR>;
	};

	template<IESLightParameter parameter>
	decltype(auto) GetBlockValue(IParamBlock2* pBlock, TimeValue t = 0, Interval valid = FOREVER)
	{
		using Helper = GetBlockValueHelper<parameter>;
		typename Helper::T1 result;

		auto ok = pBlock->GetValue(parameter, t, result, valid);
		FASSERT(ok);

		return static_cast<typename Helper::T2>(result);
	}

	template<typename From, typename To>
	To CastParameter(From& from)
	{
		return static_cast<To>(from);
	}

	template<>
	const TCHAR* CastParameter(std::basic_string<TCHAR>& from)
	{
		return from.c_str();
	}

	template<IESLightParameter p>
	struct ParameterCache
	{
		using T = typename GetBlockValueHelper<p>::TCache;

		static T& GetValue(IParamBlock2* pBlock)
		{
			static T value = GetBlockValue<p>(pBlock);
			return value;
		}

		static void SetValue(IParamBlock2* pBlock, T value)
		{
			if (parametersCacheEnabled)
			{
				GetValue(pBlock) = value;
			}
		}
	};

	template<IESLightParameter parameter>
	void SetBlockValue(IParamBlock2* pBlock,
		typename GetBlockValueHelper<parameter>::T1 value,
		TimeValue time = 0)
	{
		ParameterCache<parameter>::SetValue(pBlock, value);
		auto res = pBlock->SetValue(parameter, time, value);
		FASSERT(res);
	}

	template<IESLightParameter p>
	void InitializeDefaultBlockValue(IParamBlock2* pBlock)
	{
		using From = typename GetBlockValueHelper<p>::TCache;
		using To = typename GetBlockValueHelper<p>::T1;

		SetBlockValue<p>(pBlock,
			CastParameter<From, To>(
				ParameterCache<p>::GetValue(pBlock)));
	}

	void* FireRenderIESLightClassDesc::Create(BOOL loading)
	{
		auto instance = new FireRenderIESLight();
		auto pBlock = instance->GetParamBlock(0);

		InitializeDefaultBlockValue<IES_PARAM_ENABLED>(pBlock);
		InitializeDefaultBlockValue<IES_PARAM_PROFILE>(pBlock);
		InitializeDefaultBlockValue<IES_PARAM_AREA_WIDTH>(pBlock);
		InitializeDefaultBlockValue<IES_PARAM_TARGETED>(pBlock);
		InitializeDefaultBlockValue<IES_PARAM_TARGET_DISTANCE>(pBlock);
		InitializeDefaultBlockValue<IES_PARAM_INTENSITY>(pBlock);
		InitializeDefaultBlockValue<IES_PARAM_COLOR_MODE>(pBlock);
		InitializeDefaultBlockValue<IES_PARAM_COLOR>(pBlock);
		InitializeDefaultBlockValue<IES_PARAM_TEMPERATURE>(pBlock);
		InitializeDefaultBlockValue<IES_PARAM_SHADOWS_ENABLED>(pBlock);
		InitializeDefaultBlockValue<IES_PARAM_SHADOWS_SOFTNESS>(pBlock);
		InitializeDefaultBlockValue<IES_PARAM_SHADOWS_TRANSPARENCY>(pBlock);
		InitializeDefaultBlockValue<IES_PARAM_VOLUME_SCALE>(pBlock);

		if (instance->ProfileIsSelected())
		{
			instance->CalculateLightRepresentation(instance->GetActiveProfile());
		}

		return instance;
	}

	// Computes sphere points for three cuts:
	//	cuts[0] - XY
	//	cuts[1] - XZ
	//	cuts[2] - YZ
	//	'numCircPts' - number of points in one cut (approximation level)
	template<size_t numCircPts>
	void ComputeSphereCutPoints(
		TimeValue t, float rad, Point3& center,
		std::array<std::array<Point3, numCircPts>, 3>& cuts)
	{
		// Delta angle
		constexpr auto da = 2.0f * PI / numCircPts;

		for (int i = 0; i < numCircPts; i++)
		{
			// Angle
			auto a = i * da;

			// Cached values
			auto sn = rad * sin(a);
			auto cs = rad * cos(a);

			cuts[0][i] = Point3(sn, cs, 0.0f) + center;
			cuts[1][i] = Point3(sn, 0.0f, cs) + center;
			cuts[2][i] = Point3(0.0f, sn, cs) + center;
		}
	}

	// Draws XY, XZ and YZ sphere cuts
	//	'numCircPts' - number of points in one cut (approximation level)
	template<size_t numCircPts>
	void DrawSphereArcs(TimeValue t, GraphicsWindow *gw, float r, Point3& center)
	{
		// Compute points
		using CutPoints = std::array<Point3, numCircPts>;
		std::array<CutPoints, 3> xyz_cuts;
		ComputeSphereCutPoints(t, r, center, xyz_cuts);

		// Draw polylines
		for (size_t i = 0; i < 3; ++i)
		{
			gw->polyline(numCircPts, &xyz_cuts[i][0], nullptr, nullptr, true, nullptr);
		}
	}

	INode* FindNodeRef(ReferenceTarget *rt);

	INode* GetNodeRef(ReferenceMaker *rm)
	{
		if (rm->SuperClassID() == BASENODE_CLASS_ID)
		{
			return (INode *)rm;
		}

		return rm->IsRefTarget() ? FindNodeRef((ReferenceTarget *)rm) : nullptr;
	}

	INode* FindNodeRef(ReferenceTarget *rt)
	{
		DependentIterator di(rt);
		ReferenceMaker *rm = nullptr;

		while ((rm = di.Next()) != nullptr)
		{
			if (auto nd = GetNodeRef(rm))
			{
				return nd;
			}
		}

		return nullptr;
	}

	enum class IES_Light_Reference
	{
		ParamBlock = 0,

		// This should be always last
		__last
	};
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
	prevUp(0.0f, 1.0f, 0.0f),
	m_preview_plines(),
	m_bbox(),
	m_BBoxCalculated(false),
	m_plines()
{
	GetClassDesc()->MakeAutoParamBlocks(this);

	m_bbox.resize(8, Point3(0.0f, 0.0f, 0.0f));
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

RefResult FireRenderIESLight::NotifyRefChanged(const Interval& interval, RefTargetHandle hTarget, PartID& partId, RefMessage msg, BOOL propagate)
{
    if ((REFMSG_CHANGE==msg)  && (hTarget == m_pblock2))
    {
        auto p = m_pblock2->LastNotifyParamID();

		if (m_general.IsInitialized())
		{
			switch (p)
			{
				case IES_PARAM_P0:
				case IES_PARAM_P1:
				{
					auto p0 = GetLightPoint();
					auto p1 = GetTargetPoint();
					SetTargetDistance((p0 - p1).Length());
					m_general.UpdateTargetDistanceUi();
				}
				break;
			}
		}

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
    auto newob = new FireRenderIESLight();
    newob->ReplaceReference(BaseMaxType::NumRefs() + 0, remap.CloneRef(m_pblock2));
    BaseClone(this, newob, remap);
    return newob;
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
	return BaseMaxType::NumRefs() + 
		static_cast<int>(IES_Light_Reference::__last);
}

void FireRenderIESLight::SetReference(int i, RefTargetHandle rtarg)
{
	auto baseRefs = BaseMaxType::NumRefs();

	if (i < baseRefs)
	{
		BaseMaxType::SetReference(i, rtarg);
		return;
	}

	auto refId = static_cast<IES_Light_Reference>(i - baseRefs);

	static_assert(
		static_cast<int>(IES_Light_Reference::__last) == 1,
		"Light references enumeration has been updated. Please, implement here");

	switch (refId)
	{
		case IES_Light_Reference::ParamBlock:
			m_pblock2 = dynamic_cast<IParamBlock2*>(rtarg);
			break;

		default:
			FASSERT(!"Invalid reference request");
			break;
	}
}

RefTargetHandle FireRenderIESLight::GetReference(int i)
{
	auto baseRefs = BaseMaxType::NumRefs();

	if (i < baseRefs)
	{
		return BaseMaxType::GetReference(i);
	}

	auto refId = static_cast<IES_Light_Reference>(i - baseRefs);

	RefTargetHandle result = nullptr;

	static_assert(
		static_cast<int>(IES_Light_Reference::__last) == 1,
		"Light references enumeration has been updated. Please, implement here");

	switch (refId)
	{
		case IES_Light_Reference::ParamBlock:
			result = m_pblock2;
			break;

		default:
			FASSERT(!"Invalid reference request");
			break;
	}

	return result;
}

void FireRenderIESLight::DrawSphere(ViewExp *vpt, BOOL sel, BOOL frozen)
{
	GraphicsWindow* gw = vpt->getGW();
	Color color = frozen ? Color(0.0f, 0.0f, 1.0f) : Color(0.0f, 1.0f, 0.0f);

	if (sel)
	{
		color = Color(1.0f, 0.0f, 0.0f);
	}

	gw->setColor(LINE_COLOR, color);

	Point3 dirMesh[]
	{
		GetLightPoint(),
		GetTargetPoint()
	};

	INode *nd = FindNodeRef(this);
	Control* pLookAtController = nd->GetTMController();

	if ((pLookAtController == nullptr) || (pLookAtController->GetTarget() == nullptr))
	{
		dirMesh[1] -= dirMesh[0];
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

		auto dist = (targVector - rootVector).FLength();
		dirMesh[1] = Point3(0.0f, 0.0f, -dist);
	}

	dirMesh[0] = Point3(0.0f, 0.0f, 0.0f);

	// light source
	DrawSphereArcs<SphereCirclePointsCount>(0, gw, 2.0, dirMesh[0]);

	// draw direction
	gw->polyline(2, dirMesh, NULL, NULL, FALSE, NULL);

	// light lookAt
	DrawSphereArcs<SphereCirclePointsCount>(0, gw, 1.0, dirMesh[1]);
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
	Matrix3 prevtm = vpt->getGW()->getTransform();
	Matrix3 tm = inode->GetObjectTM(t);

	// apply scaling
#define IES_IGNORE_SCENE_SCALING
#ifndef IES_IGNORE_SCENE_SCALING
	float scaleFactor = vpt->NonScalingObjectSize() * vpt->GetVPWorldWidth(tm.GetTrans()) / 360.0f;
	if (scaleFactor!=(float)1.0)
		tm.Scale(Point3(scaleFactor,scaleFactor,scaleFactor));
#endif
	float scaleFactor;
	GetParamBlock(0)->GetValue(IES_PARAM_AREA_WIDTH, 0, scaleFactor, FOREVER);
	tm.Scale(Point3(scaleFactor, scaleFactor, scaleFactor));

	vpt->getGW()->setTransform(tm);

	auto result = DrawWeb(vpt, GetParamBlock(0), inode->Selected(), inode->IsFrozen());

	vpt->getGW()->setTransform(prevtm);

	return result;
}

int FireRenderIESLight::Display(TimeValue t, INode* inode, ViewExp *vpt, int flags)
{
	if (!vpt || !vpt->IsAlive())
	{
		// why are we here
		DbgAssert(!_T("Invalid viewport!"));
		return FALSE;
	}

	if (!DisplayLight(t, inode, vpt, flags))
	{
		Matrix3 prevtm = vpt->getGW()->getTransform();
		Matrix3 tm = inode->GetObjectTM(t);
		vpt->getGW()->setTransform(tm);
		DrawSphere(vpt, inode->Selected(), inode->IsFrozen());
		vpt->getGW()->setTransform(prevtm);
	}

    return TRUE;
}

void FireRenderIESLight::GetWorldBoundBox(TimeValue t, INode* inode, ViewExp* vpt, Box3& box)
{
	//set default output result
	box.Init();

	// back-off
    if (!vpt || !vpt->IsAlive())
    {
        return;
    }

	// calculate BBox for web
	if (!m_BBoxCalculated)
		bool haveBBox = CalculateBBox();

	if (!m_BBoxCalculated)
		return;

	// transform BBox with current web transformation
	// - node transform
	Matrix3 tm = inode->GetObjectTM(t);
	// - scale
	float scaleFactor;
	GetParamBlock(0)->GetValue(IES_PARAM_AREA_WIDTH, 0, scaleFactor, FOREVER);
	tm.Scale(Point3(scaleFactor, scaleFactor, scaleFactor));
	// - apply transformation
	std::vector<Point3> bbox(m_bbox);
	for (Point3& it : bbox)
		it = it * tm;

	// re-calculate BBox to form that 3DMax can accept (Xmin < Xmax, Ymin < Ymax, Zmin < Zmax)
	Point3 bboxMin (INFINITY, INFINITY, INFINITY);
	Point3 bboxMax (-INFINITY, -INFINITY, -INFINITY);
	for (Point3& tPoint : bbox)
	{
		if (tPoint.x > bboxMax.x)
			bboxMax.x = tPoint.x;

		if (tPoint.y > bboxMax.y)
			bboxMax.y = tPoint.y;

		if (tPoint.z > bboxMax.z)
			bboxMax.z = tPoint.z;

		if (tPoint.x < bboxMin.x)
			bboxMin.x = tPoint.x;

		if (tPoint.y < bboxMin.y)
			bboxMin.y = tPoint.y;

		if (tPoint.z < bboxMin.z)
			bboxMin.z = tPoint.z;
	}

	// output result
	box.pmin.x = bboxMin.x;
	box.pmin.y = bboxMin.y;
	box.pmin.z = bboxMin.z;
	box.pmax.x = bboxMax.x;
	box.pmax.y = bboxMax.y;
	box.pmax.z = bboxMax.z;

	// DEBUG draw BBox
#ifdef IES_DRAW_LIGHT_BBOX
	vpt->getGW()->setColor(LINE_COLOR, Point3(0.0f, 1.0f, 1.0f));
	Point3 bboxFaces[20]; // 4 faces enough
	bboxFaces[0] = Point3(bboxMin.x, bboxMin.y, bboxMin.z); // 1-st face
	bboxFaces[1] = Point3(bboxMax.x, bboxMin.y, bboxMin.z);
	bboxFaces[2] = Point3(bboxMax.x, bboxMin.y, bboxMax.z);
	bboxFaces[3] = Point3(bboxMin.x, bboxMin.y, bboxMax.z);
	bboxFaces[4] = Point3(bboxMin.x, bboxMin.y, bboxMin.z);

	bboxFaces[5] = Point3(bboxMin.x, bboxMin.y, bboxMin.z); // 2-nd face
	bboxFaces[6] = Point3(bboxMin.x, bboxMax.y, bboxMin.z);
	bboxFaces[7] = Point3(bboxMin.x, bboxMax.y, bboxMax.z);
	bboxFaces[8] = Point3(bboxMin.x, bboxMin.y, bboxMax.z);
	bboxFaces[9] = Point3(bboxMin.x, bboxMin.y, bboxMin.z);

	bboxFaces[10] = Point3(bboxMax.x, bboxMax.y, bboxMax.z); // 3-d face
	bboxFaces[11] = Point3(bboxMax.x, bboxMax.y, bboxMin.z);
	bboxFaces[12] = Point3(bboxMax.x, bboxMin.y, bboxMin.z);
	bboxFaces[13] = Point3(bboxMax.x, bboxMin.y, bboxMax.z);
	bboxFaces[14] = Point3(bboxMax.x, bboxMax.y, bboxMax.z);

	bboxFaces[15] = Point3(bboxMax.x, bboxMax.y, bboxMax.z); // 4-th face
	bboxFaces[16] = Point3(bboxMin.x, bboxMax.y, bboxMax.z);
	bboxFaces[17] = Point3(bboxMin.x, bboxMax.y, bboxMin.z);
	bboxFaces[18] = Point3(bboxMax.x, bboxMax.y, bboxMin.z);
	bboxFaces[19] = Point3(bboxMax.x, bboxMax.y, bboxMax.z);

	vpt->getGW()->polyline(5, bboxFaces, NULL, NULL, FALSE, NULL);
	vpt->getGW()->polyline(5, bboxFaces+5, NULL, NULL, FALSE, NULL);
	vpt->getGW()->polyline(5, bboxFaces+10, NULL, NULL, FALSE, NULL);
	vpt->getGW()->polyline(5, bboxFaces+15, NULL, NULL, FALSE, NULL);
#endif
}

int FireRenderIESLight::HitTest(TimeValue t, INode* inode, int type, int crossing, int flags, IPoint2 *p, ViewExp *vpt)
{
#define WEB_ENABLED
#ifdef WEB_ENABLED
	/*HitRegion hitRegion;
	DWORD savedLimits;
	GraphicsWindow *gw = vpt->getGW();

	gw->setTransform(idTM);
	MakeHitRegion(hitRegion, type, crossing, 8, p);
	savedLimits = gw->getRndLimits();

	gw->setRndLimits((savedLimits | GW_PICK) & ~GW_ILLUM & ~GW_Z_BUFFER);
	gw->setHitRegion(&hitRegion);
	gw->clearHitCode();*/

	// draw web
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
    box.Init();
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
	SetBlockValue<IES_PARAM_INTENSITY>(m_pblock2, f, time);
}

float FireRenderIESLight::GetIntensity(TimeValue t, Interval& valid)
{
	return GetBlockValue<IES_PARAM_INTENSITY>(m_pblock2, t, valid);
}

void FireRenderIESLight::SetRGBColor(TimeValue time, Point3 &color)
{
	SetBlockValue<IES_PARAM_COLOR>(m_pblock2, color, time);
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

	// Enable parameters caching when we are in the 'Create' tab
	parametersCacheEnabled = (flags & BEGIN_EDIT_CREATE) != 0;

	auto beginEdit = [&](auto& panel)
	{
		panel.BeginEdit(objParam, flags, prev);
	};

	beginEdit(m_general);
	beginEdit(m_intensity);
	beginEdit(m_shadows);
	beginEdit(m_volume);

	using namespace ies_panel_utils;
	EnableControl<EnableGeneralPanel>(m_general);
	EnableControl<EnableIntensityPanel>(m_intensity);
	EnableControl<EnableShadowsPanel>(m_shadows);
	EnableControl<EnableVolumePanel>(m_volume);
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

	parametersCacheEnabled = false;
}

void FireRenderIESLight::CreateSceneLight(const ParsedNode& node, frw::Scope scope, const RenderParameters& params)
{
	// create light
	frw::IESLight light = scope.GetContext().CreateIESLight();
	auto activeProfile = GetActiveProfile();

	if (ProfileIsSelected())
	{
		// profile is ok, load IES data
		std::wstring profilePath = FireRenderIES_Profiles::ProfileNameToPath(activeProfile);

		// apply scaling to photometric web if necessary
		float scaleFactor;
		GetParamBlock(0)->GetValue(IES_PARAM_AREA_WIDTH, 0, scaleFactor, FOREVER);
		if (std::fabs(scaleFactor - 1.0f) > 0.01f)
		{
			// parse IES file
			std::string iesFilename(profilePath.begin(), profilePath.end());
			IESProcessor parser;
			IESProcessor::IESLightData data;
			std::string errorMsg;
			bool parseOK = parser.Parse(data, iesFilename.c_str(), errorMsg);

			// scale photometric web
			IESProcessor::IESUpdateRequest req;
			req.m_scale = scaleFactor;
			parser.Update(data, req);

			// pass IES data to RPR
			std::string iesData = parser.ToString(data);

			light.SetImageFromData(iesData.c_str(), 256, 256);
		}
		else
		{
			std::string iesData(
				(std::istreambuf_iterator<char>(std::ifstream(profilePath))),
				std::istreambuf_iterator<char>());

			// pass IES data to RPR
			light.SetImageFromData(iesData.c_str(), 256, 256);
		}
		
	}
	else
	{
		MessageBox(
			GetCOREInterface()->GetMAXHWnd(),
			_T("No profile is selected!"),
			_T("Warning"),
			MB_ICONWARNING | MB_OK);
	}

	// setup color & intensity
	auto color = GetFinalColor(params.t);
	float intensity = GetIntensity(params.t);
	// - convert physical values
	intensity *= ((intensity / 682.069f) / 683.f) / 2.5f;
	color *= intensity;

	light.SetRadiantPower(color);

	// setup position
	Matrix3 tm = node.tm; // INode* inode = FindNodeRef(this); inode->GetObjectTM(0); <= this method doesn't take masterScale into acount, thus returns wrong transform!

	// rotate by 90 degrees around X axis
	Matrix3 matrRotateAroundX(
		Point3(1.0, 0.0, 0.0),
		Point3(0.0, 0.0, -1.0),
		Point3(0.0, 1.0, 0.0),
		Point3(0.0, 0.0, 0.0)
	);
	tm = matrRotateAroundX * tm;

	// rotate by -90 degrees around Z axis (to make representation of web in Max align with RPR up-vector)
	Matrix3 matrRotateAroundZ(
		Point3(0.0, 1.0, 0.0),
		Point3(-1.0, 0.0, 0.0),
		Point3(0.0, 0.0, 1.0),
		Point3(0.0, 0.0, 0.0)
	);
	tm = matrRotateAroundZ * tm;

	// create RPR matrix
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

	Point3 p1 = GetTargetPoint();
	Point3 p0 = GetLightPoint();
	Point3 r = p1 - p0;

	Matrix3 tm = nd->GetNodeTM(t);
	Matrix3 targtm = tm;
	
	targtm.PreTranslate(r);

	auto targObject = new LookAtTarget;
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

bool FireRenderIESLight::ProfileIsSelected() const
{
	auto activeProfile = GetActiveProfile();

	return
		activeProfile != nullptr &&
		_tcscmp(activeProfile, _T(""));
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

void FireRenderIESLight::SetActiveProfile(const TCHAR* profileName)
{
	SetBlockValue<IES_PARAM_PROFILE>(m_pblock2, profileName);
	CalculateLightRepresentation(profileName);
	CalculateBBox();
}

// Makes default implementation for parameter setter
#define IES_DEFINE_PARAM_SET($paramName, $paramType, $enum)	\
void FireRenderIESLight::Set##$paramName($paramType value)	\
{															\
	SetBlockValue<$enum>(m_pblock2, value);					\
}

// Makes default implementation for parameter getter
#define IES_DEFINE_PARAM_GET($paramName, $paramType, $enum)				\
$paramType FireRenderIESLight::Get##$paramName() const					\
{																		\
	return static_cast<$paramType>(GetBlockValue<$enum>(m_pblock2));	\
}

// Makes default implementations for parameter setter and getter
#define IES_DEFINE_PARAM($paramName, $paramType, $enum)\
	IES_DEFINE_PARAM_SET($paramName, $paramType, $enum)\
	IES_DEFINE_PARAM_GET($paramName, $paramType, $enum)

IES_DEFINE_PARAM(LightPoint, Point3, IES_PARAM_P0)
IES_DEFINE_PARAM(TargetPoint, Point3, IES_PARAM_P1)
IES_DEFINE_PARAM(Enabled, bool, IES_PARAM_ENABLED)
IES_DEFINE_PARAM(Targeted, bool, IES_PARAM_TARGETED)
IES_DEFINE_PARAM(ShadowsEnabled, bool, IES_PARAM_SHADOWS_ENABLED)
IES_DEFINE_PARAM(TargetDistance, float, IES_PARAM_TARGET_DISTANCE)
IES_DEFINE_PARAM(AreaWidth, float, IES_PARAM_AREA_WIDTH)
IES_DEFINE_PARAM(Intensity, float, IES_PARAM_INTENSITY)
IES_DEFINE_PARAM(Temperature, float, IES_PARAM_TEMPERATURE)
IES_DEFINE_PARAM(ShadowsSoftness, float, IES_PARAM_SHADOWS_SOFTNESS)
IES_DEFINE_PARAM(ShadowsTransparency, float, IES_PARAM_SHADOWS_TRANSPARENCY)
IES_DEFINE_PARAM(VolumeScale, float, IES_PARAM_VOLUME_SCALE)
IES_DEFINE_PARAM(Color, Color, IES_PARAM_COLOR)
IES_DEFINE_PARAM(ColorMode, IESLightColorMode, IES_PARAM_COLOR_MODE)
IES_DEFINE_PARAM_GET(ActiveProfile, const TCHAR*, IES_PARAM_PROFILE)

FIRERENDER_NAMESPACE_END
