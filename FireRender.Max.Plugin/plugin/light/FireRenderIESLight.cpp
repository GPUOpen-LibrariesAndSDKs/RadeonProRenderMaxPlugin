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

const float FireRenderIESLight::IntensitySettings::Default = 100.f;

const bool FireRenderIESLight::DefaultEnabled = true;
const bool FireRenderIESLight::DefaultTargeted = true;
const bool FireRenderIESLight::DefaultShadowsEnabled = true;
const IESLightColorMode FireRenderIESLight::DefaultColorMode = IES_LIGHT_COLOR_MODE_COLOR;

const bool FireRenderIESLight::EnableGeneralPanel = true;
const bool FireRenderIESLight::EnableIntensityPanel = true;
const bool FireRenderIESLight::EnableShadowsPanel = false;
const bool FireRenderIESLight::EnableVolumePanel = false;

const size_t FireRenderIESLight::SphereCirclePointsCount = 28;
const size_t FireRenderIESLight::IES_ImageWidth = 256;
const size_t FireRenderIESLight::IES_ImageHeight = 256;

const Color FireRenderIESLight::FrozenColor = Color(0, 0, 1);
const Color FireRenderIESLight::WireColor = Color(0, 1, 0);
const Color FireRenderIESLight::SelectedColor = Color(1, 0, 0);

namespace
{
	const int VERSION = 1;
	const TCHAR* FIRERENDER_IESLIGHT_CATEGORY = _T("Radeon ProRender");
	const TCHAR* FIRERENDER_IESLIGHT_OBJECT_NAME = _T("RPRIESLight");
	const TCHAR* FIRERENDER_IESLIGHT_INTERNAL_NAME = _T("RPRIESLight");
	const TCHAR* FIRERENDER_IESLIGHT_CLASS_NAME = _T("RPRIESLight");
	const TCHAR* FIRERENDER_IESLIG_TARGET_NODE_NAME = _T("IES target");

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

		// X Rotation of IES light parameter
		IES_PARAM_LIGHT_ROTATION_X, _T("RotationX"), TYPE_FLOAT, P_ANIMATABLE, 0,
			p_default, FireRenderIESLight::LightRotateSettings::Default,
			PB_END,

		// Y Rotation of IES light parameter
		IES_PARAM_LIGHT_ROTATION_Y, _T("RotationY"), TYPE_FLOAT, P_ANIMATABLE, 0,
			p_default, FireRenderIESLight::LightRotateSettings::Default,
			PB_END,

		// Z Rotation of IES light parameter
		IES_PARAM_LIGHT_ROTATION_Z, _T("RotationZ"), TYPE_FLOAT, P_ANIMATABLE, 0,
			p_default, FireRenderIESLight::LightRotateSettings::Default,
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
			p == IES_PARAM_LIGHT_ROTATION_X ||
			p == IES_PARAM_LIGHT_ROTATION_Y ||
			p == IES_PARAM_LIGHT_ROTATION_Z ||
			p == IES_PARAM_INTENSITY ||
			p == IES_PARAM_TEMPERATURE ||
			p == IES_PARAM_SHADOWS_SOFTNESS ||
			p == IES_PARAM_SHADOWS_TRANSPARENCY ||
			p == IES_PARAM_VOLUME_SCALE
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

		BOOL ok = pBlock->GetValue(parameter, t, result, valid);
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

	// Sets parameter value in the parameter block
	// Returns 'true' if value is updated
	// It doesn't change the parameter block value if input is the same
	// which prevents unnecessary updates
	template<IESLightParameter parameter>
	bool SetBlockValue(IParamBlock2* pBlock,
		typename GetBlockValueHelper<parameter>::T2 value,
		TimeValue time = 0)
	{
		if (GetBlockValue<parameter>(pBlock, time) == value)
		{
			return false;
		}
		
		ParameterCache<parameter>::SetValue(pBlock, value);
		BOOL res = pBlock->SetValue(parameter, time, value);
		FASSERT(res);

		return true;
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
		FireRenderIESLight* instance = new FireRenderIESLight();
		IParamBlock2* pBlock = instance->GetParamBlock(0);

		InitializeDefaultBlockValue<IES_PARAM_ENABLED>(pBlock);
		InitializeDefaultBlockValue<IES_PARAM_PROFILE>(pBlock);
		InitializeDefaultBlockValue<IES_PARAM_AREA_WIDTH>(pBlock);
		InitializeDefaultBlockValue<IES_PARAM_LIGHT_ROTATION_X>(pBlock);
		InitializeDefaultBlockValue<IES_PARAM_LIGHT_ROTATION_Y>(pBlock);
		InitializeDefaultBlockValue<IES_PARAM_LIGHT_ROTATION_Z>(pBlock);
		InitializeDefaultBlockValue<IES_PARAM_TARGETED>(pBlock);
		InitializeDefaultBlockValue<IES_PARAM_INTENSITY>(pBlock);
		InitializeDefaultBlockValue<IES_PARAM_COLOR_MODE>(pBlock);
		InitializeDefaultBlockValue<IES_PARAM_COLOR>(pBlock);
		InitializeDefaultBlockValue<IES_PARAM_TEMPERATURE>(pBlock);
		InitializeDefaultBlockValue<IES_PARAM_SHADOWS_ENABLED>(pBlock);
		InitializeDefaultBlockValue<IES_PARAM_SHADOWS_SOFTNESS>(pBlock);
		InitializeDefaultBlockValue<IES_PARAM_SHADOWS_TRANSPARENCY>(pBlock);
		InitializeDefaultBlockValue<IES_PARAM_VOLUME_SCALE>(pBlock);

		TimeValue t = GetCOREInterface()->GetTime();
		if (instance->ProfileIsSelected(t))
		{
			instance->CalculateLightRepresentation(instance->GetActiveProfile(t));
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
		const float da = 2.0f * PI / numCircPts;

		for (int i = 0; i < numCircPts; i++)
		{
			// Angle
			float a = i * da;

			// Cached values
			float sn = rad * sin(a);
			float cs = rad * cos(a);

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
			if (INode* nd = GetNodeRef(rm))
			{
				return nd;
			}
		}

		return nullptr;
	}

	ReferenceTarget* MakeNodeTransformMonitor()
	{
		Interface* ip = GetCOREInterface();
		return static_cast<ReferenceTarget*>(ip->CreateInstance(REF_TARGET_CLASS_ID, NODETRANSFORMMONITOR_CLASS_ID));
	}
}

class FireRenderIESLight::CreateCallback : public CreateMouseCallBack
{
public:
	CreateCallback() :
		m_light(nullptr),
		m_pBlock(nullptr)
	{}

	int proc(ViewExp *vpt, int msg, int point, int flags, IPoint2 m, Matrix3& mat) override
	{
		int result = CREATE_CONTINUE;

		switch (msg)
		{
			case MOUSE_POINT:
			{
				TimeValue time = GetCOREInterface()->GetTime();

				// First click
				if (point == 0)
				{
					INode* thisNode = FindNodeRef(m_light);
					m_light->SetThisNode(thisNode);

					mat.SetTrans(vpt->SnapPoint(m, m, NULL, SNAP_IN_3D));

					Point3 p = vpt->SnapPoint(m, m, NULL, SNAP_IN_3D);

					m_light->SetLightPoint(p, time);

					if (m_light->GetTargeted(time))
					{
						m_light->SetTargetPoint(p, time);
					}
					else
					{
						// Just to avoid zero target distance
						m_light->SetTargetPoint(p + Point3(1, 0, 0), time);

						// Don't need to wait for the second point
						result = CREATE_STOP;
					}
				}
				// Second click
				else
				{
					m_light->OnTargetedChanged(time, true);
					result = CREATE_STOP;
				}
			}
			break;

			case MOUSE_MOVE:
				m_pBlock->SetValue(IES_PARAM_P1, 0, vpt->SnapPoint(m, m, NULL, SNAP_IN_3D));
				break;

			case MOUSE_ABORT:
				result = CREATE_ABORT;
				break;

			case MOUSE_FREEMOVE:
				vpt->SnapPreview(m, m, NULL, SNAP_IN_3D);
				break;
		}

		return result;
	}

	void SetLight(FireRenderIESLight* light)
	{
		m_light = light;
		m_pBlock = light->GetParamBlock(0);
	}

private:
	FireRenderIESLight* m_light;
	IParamBlock2* m_pBlock;
};

const Class_ID FireRenderIESLight::m_classId(0x7ab5467f, 0x1c96049f);

Class_ID FireRenderIESLight::GetClassId()
{
	return m_classId;
}

ClassDesc2* FireRenderIESLight::GetClassDesc()
{
	return &desc;
}

Color FireRenderIESLight::GetWireColor(bool isFrozen, bool isSelected)
{
	return isSelected ? SelectedColor : (isFrozen ? FrozenColor : WireColor);
}

FireRenderIESLight::FireRenderIESLight() :
	m_general(this),
	m_intensity(this),
	m_shadows(this),
	m_volume(this),
	m_iObjParam(nullptr),
	m_defaultUp(0.0f, 1.0f, 0.0f),
	m_isPreviewGraph(true),
	m_bbox(),
	m_BBoxCalculated(false),
	m_pblock2(nullptr),
	m_thisNodeMonitor(MakeNodeTransformMonitor()),
	m_targNodeMonitor(MakeNodeTransformMonitor())
{
	ReplaceLocalReference(IndirectReference::ThisNode, m_thisNodeMonitor);
	ReplaceLocalReference(IndirectReference::TargetNode, m_targNodeMonitor);

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
	static CreateCallback createCallback;
	createCallback.SetLight(this);
	return &createCallback;
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

// inherited virtual methods for Reference-management

RefResult FireRenderIESLight::NotifyRefChanged(const Interval& interval, RefTargetHandle hTarget, PartID& partId, RefMessage msg, BOOL propagate)
{
	if (REFMSG_CHANGE == msg)
	{
		auto updatePoint = [&](
			INode* (FireRenderIESLight::* nodeGetter)(),
			bool (FireRenderIESLight::* pointSetter)(Point3, TimeValue))
		{
			TimeValue time = GetCOREInterface()->GetTime();
			INode* node = (this->*nodeGetter)();
			Control* nodeController = node->GetTMController();

			if (!nodeController->TestAFlag(A_HELD))
			{
				Matrix3 thisTm = node->GetNodeTM(time);
				(this->*pointSetter)(thisTm.GetTrans(), time);
			}
		};

		if (hTarget == m_thisNodeMonitor)
		{
			updatePoint(&FireRenderIESLight::GetThisNode, &FireRenderIESLight::SetLightPoint);
		}
		else if (hTarget == m_targNodeMonitor)
		{
			updatePoint(&FireRenderIESLight::GetTargetNode, &FireRenderIESLight::SetTargetPoint);
		}
		else if (hTarget == m_pblock2)
		{
			ParamID p = m_pblock2->LastNotifyParamID();
			TimeValue time = GetCOREInterface()->GetTime();

			switch (p)
			{
				case IES_PARAM_PROFILE:
				{
					if (ProfileIsSelected(time))
					{
						if (!CalculateLightRepresentation(GetActiveProfile(time)))
						{
							SetActiveProfile(_T(""), time);
						}

						CalculateBBox();
					}
					else
					{
						m_plines.clear();
						m_preview_plines.clear();
					}
				}
				break;

				case IES_PARAM_TARGETED:
					if(!theHold.RestoreOrRedoing())
						OnTargetedChanged(time, false);
					break;
			}

			// Update panels
			switch (p)
			{
				case IES_PARAM_P0:
				case IES_PARAM_P1:
				case IES_PARAM_ENABLED:
				case IES_PARAM_PROFILE:
				case IES_PARAM_AREA_WIDTH:
				case IES_PARAM_LIGHT_ROTATION_X:
				case IES_PARAM_LIGHT_ROTATION_Y:
				case IES_PARAM_LIGHT_ROTATION_Z:
				case IES_PARAM_TARGETED:
					m_general.UpdateUI(time);
					break;

				case IES_PARAM_INTENSITY:
				case IES_PARAM_COLOR_MODE:
				case IES_PARAM_COLOR:
				case IES_PARAM_TEMPERATURE:
					m_intensity.UpdateUI(time);
					break;

				case IES_PARAM_SHADOWS_ENABLED:
				case IES_PARAM_SHADOWS_SOFTNESS:
				case IES_PARAM_SHADOWS_TRANSPARENCY:
					m_shadows.UpdateUI(time);
					break;

				case IES_PARAM_VOLUME_SCALE:
					m_volume.UpdateUI(time);
					break;
			}
		}
		
		//some params have changed - should redraw all
		if(GetCOREInterface())
		{
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

	// Clone base class data
	BaseClone(this, newob, remap);

	// Clone this class references
	int baseRefs = BaseMaxType::NumRefs();

	for (int i = 0; i < NumRefs(); ++i)
	{
		int refIndex = baseRefs + i;
		RefTargetHandle ref = GetReference(refIndex);
		newob->ReplaceReference(refIndex, remap.CloneRef(ref));
	}

	return newob;
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
	return BaseMaxType::NumRefs() + static_cast<int>(IndirectReference::indirectRefEnd);
}

void FireRenderIESLight::SetReference(int i, RefTargetHandle rtarg)
{
	int baseRefs = BaseMaxType::NumRefs();

	if (i < baseRefs)
	{
		BaseMaxType::SetReference(i, rtarg);
		return;
	}

	int local_i = i - baseRefs;
	bool referenceFound = false;

	static_assert(
		static_cast<int>(IndirectReference::indirectRefEnd) == 3,
		"Light references enumeration has been updated. Please, implement here");

	switch (static_cast<StrongReference>(local_i))
	{
		case StrongReference::ParamBlock:
			m_pblock2 = dynamic_cast<IParamBlock2*>(rtarg);
			referenceFound = true;
			break;
	}

	switch (static_cast<IndirectReference>(local_i))
	{
		case IndirectReference::ThisNode:
			m_thisNodeMonitor = rtarg;
			referenceFound = true;
			break;

		case IndirectReference::TargetNode:
			m_targNodeMonitor = rtarg;
			referenceFound = true;
			break;
	}

	if (!referenceFound)
	{
		FASSERT(!"Invalid reference request");
	}
}

RefTargetHandle FireRenderIESLight::GetReference(int i)
{
	int baseRefs = BaseMaxType::NumRefs();

	if (i < baseRefs)
	{
		return BaseMaxType::GetReference(i);
	}

	int local_i = i - baseRefs;

	RefTargetHandle result = nullptr;
	bool referenceFound = false;

	static_assert(
		static_cast<int>(IndirectReference::indirectRefEnd) == 3,
		"Light references enumeration has been updated. Please, implement here");

	switch (static_cast<StrongReference>(local_i))
	{
		case StrongReference::ParamBlock:
			result = m_pblock2;
			referenceFound = true;
			break;
	}

	switch (static_cast<IndirectReference>(local_i))
	{
		case IndirectReference::ThisNode:
			result = m_thisNodeMonitor;
			referenceFound = true;
			break;

		case IndirectReference::TargetNode:
			result = m_targNodeMonitor;
			referenceFound = true;
			break;
	}

	if (!referenceFound)
	{
		FASSERT(!"Invalid reference request");
	}

	return result;
}

void FireRenderIESLight::DrawSphere(TimeValue t, ViewExp *vpt, BOOL sel, BOOL frozen)
{
	GraphicsWindow* graphicsWindow = vpt->getGW();

	graphicsWindow->setColor(LINE_COLOR, GetWireColor(frozen, sel));

	Point3 dirMesh[]
	{
		GetLightPoint(t),
		GetTargetPoint(t)
	};

	INode* thisNode = FindNodeRef(this);
	INode* targNode = GetTargetNode();
	
	if (targNode == nullptr)
	{
		dirMesh[1] -= dirMesh[0];
	}
	else
	{
		Matrix3 targTm = targNode->GetNodeTM(t);
		float dist = GetTargetDistance(t);
		dirMesh[1] = Point3(0.0f, 0.0f, -dist);
	}

	dirMesh[0] = Point3(0.0f, 0.0f, 0.0f);

	// light source
	DrawSphereArcs<SphereCirclePointsCount>(0, graphicsWindow, 2.0, dirMesh[0]);

	if (GetTargeted(t))
	{
		// draw direction
		graphicsWindow->polyline(2, dirMesh, NULL, NULL, FALSE, NULL);

		// light lookAt
		DrawSphereArcs<SphereCirclePointsCount>(0, graphicsWindow, 1.0, dirMesh[1]);
	}
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
		return FALSE;
	}

	Matrix3 prevtm = vpt->getGW()->getTransform();
	Matrix3 tm = prevtm;
	BOOL result = TRUE;

	if (ProfileIsSelected(t))
	{
		GraphicsWindow* graphicsWindow = vpt->getGW();

		// apply scaling
#define IES_IGNORE_SCENE_SCALING
#ifndef IES_IGNORE_SCENE_SCALING
		float scaleFactor = vpt->NonScalingObjectSize() * vpt->GetVPWorldWidth(tm.GetTrans()) / 360.0f;
		if (scaleFactor != (float)1.0)
			tm.Scale(Point3(scaleFactor, scaleFactor, scaleFactor));
#endif
		float scaleFactor = GetAreaWidth(t);
		tm.Scale(Point3(scaleFactor, scaleFactor, scaleFactor));

		graphicsWindow->setTransform(tm);
		result = DrawWeb(t, vpt, inode->Selected(), inode->IsFrozen());
	}
	else
	{
		vpt->getGW()->setTransform(tm);
		DrawSphere(t, vpt, inode->Selected(), inode->IsFrozen());
	}

	vpt->getGW()->setTransform(prevtm);

	return result;
}

int FireRenderIESLight::Display(TimeValue t, INode* inode, ViewExp *vpt, int flags)
{
	return DisplayLight(t, inode, vpt, flags);
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
}

int FireRenderIESLight::HitTest(TimeValue t, INode* inode, int type, int crossing, int flags, IPoint2 *p, ViewExp *vpt)
{
	Display(t, inode, vpt, flags);
	return 1;
}

void FireRenderIESLight::GetLocalBoundBox(TimeValue t, INode* inode, ViewExp* vpt, Box3& box)
{
	box.Init();
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
    cs->type = OMNI_LGT;

    return REF_SUCCEED;
}

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
	if (!ProfileIsSelected(params.t))
	{
		MessageBox(
			GetCOREInterface()->GetMAXHWnd(),
			_T("No profile is selected!"),
			_T("Warning"),
			MB_ICONWARNING | MB_OK);

		return;
	}

	if (!GetEnabled(params.t))
	{
		return;
	}

	// create light
	frw::IESLight light = scope.GetContext().CreateIESLight();
	const TCHAR* activeProfile = GetActiveProfile(params.t);

	// Load profile
	{
		// profile is ok, load IES data
		std::wstring profilePath = FireRenderIES_Profiles::ProfileNameToPath(activeProfile);

		// apply scaling to photometric web if necessary
		float scaleFactor;
		GetParamBlock(0)->GetValue(IES_PARAM_AREA_WIDTH, 0, scaleFactor, FOREVER);

		std::string iesData;

		// parse IES file
		IESProcessor parser;
		IESProcessor::IESLightData data;
		bool parseOK = parser.Parse(data, profilePath.c_str()) == IESProcessor::ErrorCode::SUCCESS;
		
		if (!parseOK)
		{
			// throw?
			MessageBox(
				GetCOREInterface()->GetMAXHWnd(),
				_T("Failed to export IES light source!"),
				_T("Warning"),
				MB_ICONWARNING | MB_OK);

			return;
		}

		// scale photometric web
		IESProcessor::IESUpdateRequest req;
		req.m_scale = scaleFactor;

		bool updateOK = parser.Update(data, req) == IESProcessor::ErrorCode::SUCCESS;

		if (!updateOK)
		{
			MessageBox(
				GetCOREInterface()->GetMAXHWnd(),
				_T("Failed to export IES light source!"),
				_T("Warning"),
				MB_ICONWARNING | MB_OK);

			return;
		}

		iesData = parser.ToString(data);

		// pass IES data to RPR
		light.SetIESData(iesData.c_str(), IES_ImageWidth, IES_ImageHeight);
	}

	// setup color & intensity
	Color color = GetFinalColor(params.t);
	float intensity = GetIntensity(params.t);
	// - convert physical values
	intensity *= ((intensity / 682.069f) / 683.f) / 2.5f;
	color *= intensity;

	light.SetRadiantPower(color);

	// setup position
	Matrix3 tm = node.tm; // INode* inode = FindNodeRef(this); inode->GetObjectTM(0); <= this method doesn't take masterScale into acount, thus returns wrong transform!

	Matrix3 localRot;
	float angles[3] = { GetRotationX(params.t)*DEG_TO_RAD, GetRotationY(params.t)*DEG_TO_RAD, GetRotationZ(params.t)*DEG_TO_RAD };
	EulerToMatrix(angles, localRot, EULERTYPE_XYZ);
		
	tm = localRot * tm;

	// create RPR matrix
	float frTm[16];
	CreateFrMatrix(fxLightTm(tm), frTm);
	
	// pass this matrix to renderer
	light.SetTransform(frTm, false);

	// attach to scene
	light.SetUserData(node.id);
	scope.GetScene().Attach(light);
}

void FireRenderIESLight::AddTarget(TimeValue t, bool fromCreateCallback)
{
	INode* thisNode = GetThisNode();
	Interface* core = GetCOREInterface();
	INode* targNode = core->CreateObjectNode(new LookAtTarget);
	
	// Make target node name
	TSTR targName = thisNode->GetName();
	targName += FIRERENDER_IESLIG_TARGET_NODE_NAME;
	targNode->SetName(targName);

	// Set up look at control
	Control* laControl = CreateLookatControl();
	laControl->SetTarget(targNode);
	laControl->Copy(thisNode->GetTMController());
	thisNode->SetTMController(laControl);
	targNode->SetIsTarget(TRUE);

	// Track target node changes
	SetTargetNode(targNode);

	// Set target node transform
	if (fromCreateCallback)
	{
		Point3 p1 = GetTargetPoint(t);
		Point3 p0 = GetLightPoint(t);
		Point3 r = p1 - p0;

		Matrix3 targtm = thisNode->GetNodeTM(t);
		targtm.PreTranslate(r);

		targNode->SetNodeTM(t, targtm);
	}
	else
	{
		Matrix3 targtm(1);
		targtm.SetTrans(GetTargetPoint(t));
		targNode->SetNodeTM(t, targtm);
	}

	Color lightWireColor(thisNode->GetWireColor());
	targNode->SetWireColor(lightWireColor.toRGB());

	targNode->NotifyDependents(FOREVER, PART_OBJ, REFMSG_CHANGE);
	targNode->NotifyDependents(FOREVER, PART_OBJ, REFMSG_NUM_SUBOBJECTTYPES_CHANGED);
}

void FireRenderIESLight::RemoveTarget(TimeValue t)
{
	INode* thisNode = GetThisNode();
	INode* targNode = GetTargetNode();

	if (!thisNode || !targNode)
	{
		return;
	}

	Interface* core = GetCOREInterface();
	DependentIterator di(this);

	// iterate through the instances
	while (ReferenceMaker* rm = di.Next())
	{
		if (INode* node = GetNodeRef(rm))
		{
			if (INode* target = node->GetTarget())
			{
				core->DeleteNode(target);
			}

			Point3 p0 = GetLightPoint(t);
			Point3 p1 = GetTargetPoint(t);
			Point3 dir = (p1 - p0).Normalize();

			Matrix3 tm(1);
			tm.SetAngleAxis(dir, 0);
			tm.SetTrans(p0);

			node->SetTMController(NewDefaultMatrix3Controller());
			node->SetNodeTM(t, tm);
		}
	}

	SetTargetNode(nullptr);
}

void FireRenderIESLight::OnTargetedChanged(TimeValue t, bool fromCreateCallback)
{
	if (GetTargeted(t))
	{
		AddTarget(t, fromCreateCallback);
	}
	else
	{
		RemoveTarget(t);
	}

	NotifyDependents(FOREVER, PART_OBJ, REFMSG_CHANGE);
	NotifyDependents(FOREVER, PART_OBJ, REFMSG_NUM_SUBOBJECTTYPES_CHANGED);
	GetCOREInterface()->RedrawViews(t);
}

bool FireRenderIESLight::ProfileIsSelected(TimeValue t) const
{
	const TCHAR* activeProfile = GetActiveProfile(t);
	return activeProfile != nullptr && _tcscmp(activeProfile, _T(""));
}

// Result depends on color mode
Color FireRenderIESLight::GetFinalColor(TimeValue t, Interval& i) const
{
	Color result(1.f, 1.f, 1.f);

	switch (GetColorMode(t))
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

bool FireRenderIESLight::SetTargetDistance(float value, TimeValue time)
{
	if (INode* thisNode = GetThisNode())
	{
		if (GetTargetDistance(time) != value)
		{
			// Compute offset from light to target
			Point3 p0 = GetLightPoint(time);
			Point3 p1 = GetTargetPoint(time);
			Point3 targetOffset = (p1 - p0).Normalize() * value;

			// Get target node from controller
			INode* targetNode = GetTargetNode();

			Matrix3 thisTm = thisNode->GetNodeTM(time);
			Point3 thisTrans = thisTm.GetTrans();

			Matrix3 targetTm(true);
			targetTm.SetTrans(thisTrans + targetOffset);

			// Update target node position
			targetNode->SetNodeTM(time, targetTm);

			return true;
		}
	}

	return false;
}

float FireRenderIESLight::GetTargetDistance(TimeValue t, Interval& valid) const
{
	return (GetTargetPoint(t, valid) - GetLightPoint(t, valid)).Length();
}

INode* FireRenderIESLight::GetThisNode()
{
	return dynamic_cast<INodeTransformMonitor*>(m_thisNodeMonitor)->GetNode();
}

INode* FireRenderIESLight::GetTargetNode()
{
	return dynamic_cast<INodeTransformMonitor*>(m_targNodeMonitor)->GetNode();
}

void FireRenderIESLight::SetThisNode(INode* node)
{
	dynamic_cast<INodeTransformMonitor*>(m_thisNodeMonitor)->SetNode(node);
}

void FireRenderIESLight::SetTargetNode(INode* node)
{
	dynamic_cast<INodeTransformMonitor*>(m_targNodeMonitor)->SetNode(node);
}

// Makes default implementation for parameter setter
#define IES_DEFINE_PARAM_SET($paramName, $paramType, $enum)				\
bool FireRenderIESLight::Set##$paramName($paramType value, TimeValue t)	\
{																		\
	return SetBlockValue<$enum>(m_pblock2, value, t);					\
}

// Makes default implementation for parameter getter
#define IES_DEFINE_PARAM_GET($paramName, $paramType, $enum)							\
$paramType FireRenderIESLight::Get##$paramName(TimeValue t, Interval& valid) const	\
{																					\
	return static_cast<$paramType>(GetBlockValue<$enum>(m_pblock2, t, valid));		\
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
IES_DEFINE_PARAM(AreaWidth, float, IES_PARAM_AREA_WIDTH)
IES_DEFINE_PARAM(RotationX, float, IES_PARAM_LIGHT_ROTATION_X)
IES_DEFINE_PARAM(RotationY, float, IES_PARAM_LIGHT_ROTATION_Y)
IES_DEFINE_PARAM(RotationZ, float, IES_PARAM_LIGHT_ROTATION_Z)
IES_DEFINE_PARAM(Intensity, float, IES_PARAM_INTENSITY)
IES_DEFINE_PARAM(Temperature, float, IES_PARAM_TEMPERATURE)
IES_DEFINE_PARAM(ShadowsSoftness, float, IES_PARAM_SHADOWS_SOFTNESS)
IES_DEFINE_PARAM(ShadowsTransparency, float, IES_PARAM_SHADOWS_TRANSPARENCY)
IES_DEFINE_PARAM(VolumeScale, float, IES_PARAM_VOLUME_SCALE)
IES_DEFINE_PARAM(Color, Color, IES_PARAM_COLOR)
IES_DEFINE_PARAM(ColorMode, IESLightColorMode, IES_PARAM_COLOR_MODE)
IES_DEFINE_PARAM(ActiveProfile, const TCHAR*, IES_PARAM_PROFILE)

FIRERENDER_NAMESPACE_END
