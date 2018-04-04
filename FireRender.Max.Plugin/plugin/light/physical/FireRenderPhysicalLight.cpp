/*********************************************************************************************************************************
* Radeon ProRender for 3ds Max plugin
* Copyright (c) 2017 AMD
* All Rights Reserved
*
* Custom IES light 3ds MAX scene node. This custom node allows the definition of IES light, using rectangular shapes
* that can be positioned and oriented in the MAX scene.
*********************************************************************************************************************************/

#include "FireRenderPhysicalLight.h"
#include <math.h>
#include "Common.h"
#include <hsv.h>
#include "LookAtTarget.h"
#include "INodeTransformMonitor.h"

FIRERENDER_NAMESPACE_BEGIN

INode* FindNodeRef(ReferenceTarget *rt); // how about moving this function to some common block?

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

FireRenderPhysicalLight::FireRenderPhysicalLight()
	: m_pblock(nullptr)
	, m_thisNode(MakeNodeTransformMonitor())
	, m_targNode(MakeNodeTransformMonitor())
	, m_isPreview(true)
	, m_isTargetPreview(false)
{
	ReplaceLocalReference(IndirectReference::ThisNode, m_thisNode);
	ReplaceLocalReference(IndirectReference::TargetNode, m_targNode);

	GetFireRenderPhysicalLightDesc()->MakeAutoParamBlocks(this);
	FASSERT(m_pblock != NULL);
}

FireRenderPhysicalLight::~FireRenderPhysicalLight()
{
	DebugPrint(_T("FireRenderPhysicalLight::~FireRenderPhysicalLight\n"));
	DeleteAllRefs();
}

void FireRenderPhysicalLight::DeleteThis()
{
	delete this;
}

const TCHAR* FIRERENDER_PHYSICALLIGHT_OBJECT_NAME = _T("RPRPhysicalLight");
const TCHAR* FIRERENDER_PHYSICALLIGHT_TARGET_NODE_NAME = _T("Physical Light target");


Class_ID FireRenderPhysicalLight::ClassID()
{
	return FIRERENDER_PHYSICALLIGHT_CLASS_ID;
}

void FireRenderPhysicalLight::FinishPreview(void)
{ 
	m_isPreview = false; 
}

void FireRenderPhysicalLight::BeginTargetPreview(void)
{ 
	m_isTargetPreview = true; 
}

void FireRenderPhysicalLight::FinishTargetPreview(void)
{ 
	m_isTargetPreview = false; 
}

RefResult FireRenderPhysicalLight::EvalLightState(TimeValue t, Interval& valid, LightState* cs)
{
	cs->color = GetRGBColor(t, valid);
	cs->on = GetUseLight();
	cs->intens = 1.0f;
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
	cs->ambientOnly = FALSE;
	cs->affectDiffuse = TRUE;
	cs->affectSpecular = TRUE;
	cs->type = OMNI_LGT;

	return REF_SUCCEED;
}

void FireRenderPhysicalLight::GetClassName(TSTR& s)
{
	s = GetFireRenderPhysicalLightDesc()->ClassName();
}

//makes part of the node name e.g. TheName001, if method returns L"TheName"
void FireRenderPhysicalLight::InitNodeName(TSTR& s)
{
	s = FIRERENDER_PHYSICALLIGHT_OBJECT_NAME;
}

//displayed in Modifier list
const MCHAR* FireRenderPhysicalLight::GetObjectName()
{
	return FIRERENDER_PHYSICALLIGHT_OBJECT_NAME;
}

IParamBlock2* FireRenderPhysicalLight::GetParamBlock(int i)
{
	FASSERT(i == 0);
	return m_pblock;
}

IParamBlock2* FireRenderPhysicalLight::GetParamBlockByID(BlockID id)
{
	FASSERT(m_pblock->ID() == id);
	return m_pblock;
}

Interval FireRenderPhysicalLight::ObjectValidity(TimeValue t)
{
	Interval valid;
	valid.SetInfinite();
	return valid;
}

RefTargetHandle FireRenderPhysicalLight::Clone(RemapDir& remap)
{
	FireRenderPhysicalLight* newob = new FireRenderPhysicalLight();

	// Clone base class data
	BaseClone(this, newob, remap);

	// Clone this class references
	int baseRefs = LightObject::NumRefs();

	for (int i = 0; i < NumRefs(); ++i)
	{
		int refIndex = baseRefs + i;
		RefTargetHandle ref = GetReference(refIndex);
		newob->ReplaceReference(refIndex, remap.CloneRef(ref));
	}

	return newob;
}

int FireRenderPhysicalLight::NumRefs()
{
	return LightObject::NumRefs() + static_cast<int>(IndirectReference::indirectRefEnd);
}

void FireRenderPhysicalLight::SetReference(int i, RefTargetHandle rtarg)
{
	FASSERT(unsigned(i) < unsigned(NumRefs()));
	this->m_pblock = dynamic_cast<IParamBlock2*>(rtarg);
}

RefTargetHandle FireRenderPhysicalLight::GetReference(int i)
{
	FASSERT(unsigned(i) < unsigned(NumRefs()));
	return m_pblock;
}

void FireRenderPhysicalLight::BeginEditParams(IObjParam *ip, ULONG flags, Animatable *prev)
{
	GetFireRenderPhysicalLightDesc()->BeginEditParams(ip, this, flags, prev);
}

void FireRenderPhysicalLight::EndEditParams(IObjParam *ip, ULONG flags, Animatable *next)
{
	GetFireRenderPhysicalLightDesc()->EndEditParams(ip, this, flags, next);
}

bool FireRenderPhysicalLight::SetTargetDistance(float value, TimeValue time)
{
	INode* thisNode = GetThisNode(); 
	FASSERT(thisNode);
	INode* targetNode = GetTargetNode();
	FASSERT(targetNode);

	if (thisNode == nullptr || targetNode == nullptr ||
		std::abs(GetTargetDistance(time) - value) < std::numeric_limits<float>::epsilon())
	{
		return false;
	}

	// Compute offset from light to target
	Point3 lightPos = thisNode->GetNodeTM(time).GetTrans();
	Point3 targPos = targetNode->GetNodeTM(time).GetTrans();
	Point3 dir = (targPos - lightPos).Normalize();
	targPos = dir * value;

	Matrix3 thisTm = thisNode->GetNodeTM(time);
	Point3 thisTrans = thisTm.GetTrans();

	Matrix3 targetTm(true);
	targetTm.SetTrans(thisTrans + dir * value);

	// Update target node position
	targetNode->SetNodeTM(time, targetTm);

	// Update UI
	SetTargetPoint(targPos+lightPos, time);
	SetDist(GetTargetDistance(time), time);

	return true;
}

float FireRenderPhysicalLight::GetTargetDistance(TimeValue t) const
{
	const INode* thisNode = GetThisNode();
	const INode* targetNode = GetTargetNode();
	FASSERT(thisNode);
	FASSERT(targetNode);

	Matrix3 thisTm = const_cast<INode*>(thisNode)->GetNodeTM(t);
	Point3 thisTrans = thisTm.GetTrans();
	Matrix3 targTm = const_cast<INode*>(targetNode)->GetNodeTM(t);
	Point3 targTrans = targTm.GetTrans();
	float dist = (targTrans - thisTrans).Length();

	return dist;
}

CreateMouseCallBack* FireRenderPhysicalLight::GetCreateMouseCallBack()
{
	static CreateCallBack createCallback;
	createCallback.SetLight(this);
	return &createCallback;
}

ObjectState FireRenderPhysicalLight::Eval(TimeValue time)
{
	return ObjectState(this);
}

void FireRenderPhysicalLight::SetHSVColor(TimeValue, Point3 &)
{

}

Point3 FireRenderPhysicalLight::GetHSVColor(TimeValue t, Interval &valid)
{
	int h, s, v;
	Point3 rgbf = GetRGBColor(t, valid);
	DWORD rgb = RGB((int)(rgbf[0] * 255.0f), (int)(rgbf[1] * 255.0f), (int)(rgbf[2] * 255.0f));
	RGBtoHSV(rgb, &h, &s, &v);
	return Point3(h / 255.0f, s / 255.0f, v / 255.0f);
}

void FireRenderPhysicalLight::SetThisNode(INode* node)
{
	FASSERT(m_thisNode);
	dynamic_cast<INodeTransformMonitor*>(m_thisNode)->SetNode(node);
}

void FireRenderPhysicalLight::SetTargetNode(INode* node)
{
	FASSERT(m_targNode);
	dynamic_cast<INodeTransformMonitor*>(m_targNode)->SetNode(node);
}

INode* FireRenderPhysicalLight::GetThisNode()
{
	// back-off
	if (m_thisNode == nullptr)
		return nullptr;

	return dynamic_cast<INodeTransformMonitor*>(m_thisNode)->GetNode();
}

INode* FireRenderPhysicalLight::GetTargetNode()
{
	// back-off
	if (m_targNode == nullptr)
		return nullptr;

	return dynamic_cast<INodeTransformMonitor*>(m_targNode)->GetNode();
}

const INode* FireRenderPhysicalLight::GetThisNode() const
{
	// back-off
	if (m_targNode == nullptr)
		return nullptr;

	return dynamic_cast<INodeTransformMonitor*>(m_thisNode)->GetNode();
}

const INode* FireRenderPhysicalLight::GetTargetNode() const
{
	// back-off
	if (m_targNode == nullptr)
		return nullptr;

	return dynamic_cast<INodeTransformMonitor*>(m_targNode)->GetNode();
}

void FireRenderPhysicalLight::SetLightPoint(const Point3& p, const TimeValue& time)
{
	m_pblock->SetValue(FRPhysicalLight_LIGHT_POINT, time, p);
}

const Point3 FireRenderPhysicalLight::GetLightPoint(const TimeValue& time) const
{
	Point3 lightPoint (0.0f, 0.0f, 0.0f);
	Interval valid = FOREVER;
	bool success = m_pblock->GetValue(FRPhysicalLight_LIGHT_POINT, time, lightPoint, valid);
	FASSERT(success);
	return lightPoint;
}

void FireRenderPhysicalLight::SetTargetPoint(const Point3& p, const TimeValue& time)
{
	m_pblock->SetValue(FRPhysicalLight_TARGET_POINT, time, p);
}

const Point3 FireRenderPhysicalLight::GetTargetPoint(const TimeValue& time) const
{
	Point3 targPoint (0.0f, 0.0f, 0.0f);
	Interval valid = FOREVER;
	bool success = m_pblock->GetValue(FRPhysicalLight_TARGET_POINT, time, targPoint, valid);
	FASSERT(success);
	return targPoint;
}

void FireRenderPhysicalLight::SetSecondPoint(const Point3& p, const TimeValue& time)
{
	m_pblock->SetValue(FRPhysicalLight_SECOND_POINT, time, p);
}

const Point3 FireRenderPhysicalLight::GetSecondPoint(const TimeValue& time) const
{
	Point3 secondPoint (0.0f, 0.0f, 0.0f);
	Interval valid = FOREVER;
	bool success = m_pblock->GetValue(FRPhysicalLight_SECOND_POINT, time, secondPoint, valid);
	FASSERT(success);
	return secondPoint;
}

void FireRenderPhysicalLight::SetThirdPoint(const Point3& p, const TimeValue& time)
{
	m_pblock->SetValue(FRPhysicalLight_THIRD_POINT, time, p);
}

const Point3 FireRenderPhysicalLight::GetThirdPoint(const TimeValue& time) const
{
	Point3 thirdPoint (0.0f, 0.0f, 0.0f);
	Interval valid = FOREVER;
	bool success = m_pblock->GetValue(FRPhysicalLight_THIRD_POINT, time, thirdPoint, valid);
	FASSERT(success);
	return thirdPoint;
}

void FireRenderPhysicalLight::SetLength(float length, const TimeValue& time)
{
	m_pblock->SetValue(FRPhysicalLight_AREA_LENGTHS, time, length);
}

float FireRenderPhysicalLight::GetLength(const TimeValue& time) const
{
	float length = 0.0f;
	Interval valid = FOREVER;
	bool success = m_pblock->GetValue(FRPhysicalLight_AREA_LENGTHS, time, length, valid);
	FASSERT(success);
	return length;
}

void FireRenderPhysicalLight::SetWidth(float length, const TimeValue& time)
{
	m_pblock->SetValue(FRPhysicalLight_AREA_WIDTHS, time, length);
}

float FireRenderPhysicalLight::GetWidth(const TimeValue& time) const
{
	float width = 0.0f;
	Interval valid = FOREVER;
	bool success = m_pblock->GetValue(FRPhysicalLight_AREA_WIDTHS, time, width, valid);
	FASSERT(success);
	return width;
}

void FireRenderPhysicalLight::SetDist(float length, const TimeValue& time)
{
	m_pblock->SetValue(FRPhysicalLight_TARGET_DIST, time, length);
}

float FireRenderPhysicalLight::GetDist(const TimeValue& time) const
{
	float dist = 0.0f;
	Interval valid = FOREVER;
	bool success = m_pblock->GetValue(FRPhysicalLight_TARGET_DIST, time, dist, valid);
	FASSERT(success);
	return dist;
}

FRPhysicalLight_AreaLight_LightShape FireRenderPhysicalLight::GetLightShapeMode(const TimeValue& time) const
{
	int value;
	Interval valid = FOREVER;
	m_pblock->GetValue(FRPhysicalLight_AREALIGHT_LIGHTSHAPE, time, value, valid);
	return static_cast<FRPhysicalLight_AreaLight_LightShape>(value);
}

FRPhysicalLight_LightType FireRenderPhysicalLight::GetLightType(const TimeValue& time) const
{
	int lightType;
	Interval valid = FOREVER;
	m_pblock->GetValue(FRPhysicalLight_LIGHT_TYPE, time, lightType, valid);
	return static_cast<FRPhysicalLight_LightType>(lightType);
}

bool FireRenderPhysicalLight::IsTargeted() const 
{
	Interval valid = FOREVER;
	int boolValue;
	m_pblock->GetValue(FRPhysicalLight_IS_TARGETED, 0, boolValue, valid);
	return static_cast<bool>(boolValue);
}

bool FireRenderPhysicalLight::IsEnabled() const
{
	Interval valid = FOREVER;
	int boolValue;
	m_pblock->GetValue(FRPhysicalLight_ISENABLED, 0, boolValue, valid);
	return static_cast<bool>(boolValue);
}

void FireRenderPhysicalLight::AddTarget(TimeValue t, bool fromCreateCallback)
{
	INode* thisNode = GetThisNode();
	Interface* core = GetCOREInterface();
	INode* targNode = core->CreateObjectNode(new LookAtTarget);

	// Make target node name
	TSTR targName = thisNode->GetName();
	targName += FIRERENDER_PHYSICALLIGHT_TARGET_NODE_NAME;
	targNode->SetName(targName);

	// Setup look at control
	Control* laControl = CreateLookatControl();
	laControl->SetTarget(targNode);
	laControl->Copy(thisNode->GetTMController());
	thisNode->SetTMController(laControl);
	targNode->SetIsTarget(TRUE);

	// Track target node changes
	SetTargetNode(targNode);

	// Set target node transform
	Point3 p1 = GetLightPoint(t);
	FRPhysicalLight_LightType lightType = GetLightType(t);
	if (FRPhysicalLight_SPOT == lightType)
	{
		Point3 dir = (GetSecondPoint(t) - GetLightPoint(t)).Normalize();
		float rx = -dir.y;
		float ry = dir.x;
		Point3 rdir(rx, ry, 0.0f); // rdir is vector normal to vector from zero to center of the cone
		rdir = rdir.Normalize();
		Matrix3 tilt = RotAngleAxisMatrix(rdir, PI / 2);
		dir = tilt * dir;

		if (!fromCreateCallback)
		{
			float dist = (GetSecondPoint(t) - GetLightPoint(t)).Length() * 2;
			p1 = dir * dist;
		}
		else
		{
			float dist = (GetTargetPoint(t) - GetLightPoint(t)).Length();
			p1 = dir * dist;
		}
	}
	else
	{
		if (fromCreateCallback)
		{
			p1.z = GetTargetPoint(t).z;
		}
		else
		{
			float dist = (GetSecondPoint(t) - GetLightPoint(t)).Length();
			Point3 dir2targ (0.0f, 0.0f, -1.0f);
			p1.z = (dir2targ*dist).z;
		}
	}

	Point3 p0 = GetLightPoint(t);
	Point3 r = p1 - p0;

	Matrix3 targtm = thisNode->GetNodeTM(t);
	targtm.PreTranslate(r);

	targNode->SetNodeTM(t, targtm);
	targNode->InvalidateTM();

	// Colour
	Color lightWireColor(thisNode->GetWireColor());
	targNode->SetWireColor(lightWireColor.toRGB());

	targNode->NotifyDependents(FOREVER, PART_OBJ, REFMSG_CHANGE);
	targNode->NotifyDependents(FOREVER, PART_OBJ, REFMSG_NUM_SUBOBJECTTYPES_CHANGED);

	SetTargetPoint(p1, t);
}

void FireRenderPhysicalLight::RemoveTarget(TimeValue t)
{
	// back-off
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
			Matrix3 tm = node->GetNodeTM(t);

			if (INode* target = node->GetTarget())
			{
				core->DeleteNode(target);
			}

			node->SetTMController(NewDefaultMatrix3Controller());
			node->SetNodeTM(t, tm);
		}
	}

	SetTargetNode(nullptr);
}

int FireRenderPhysicalLight::HitTest(TimeValue t, INode* inode, int type, int crossing, int flags, IPoint2 *p, ViewExp *vpt)
{
	return 1;
}

bool FireRenderPhysicalLight::CalculateBBox(void)
{
	// NIY
	// is supposed to cache bbox so that bbox won't have to be re-calculated every frame

	return true;
}

void CalculateBBoxForSpotLight(Point3& bboxMin, Point3& bboxMax, const TimeValue& time, FireRenderPhysicalLight* light)
{
	if (light == nullptr)
		return;

	float angle = 0.0f;
	Interval valid = FOREVER;
	light->GetParamBlockByID(0)->GetValue(FRPhysicalLight_SPOTLIGHT_OUTERCONE, 0, angle, valid);

	float length = (light->GetSecondPoint(time) - light->GetLightPoint(time)).Length();
	float tanAlpha = tan(angle * PI / 180);
	float coneRad = length * tanAlpha;

	bboxMin = { -coneRad, -coneRad, -length };
	bboxMax = { coneRad, coneRad, 0.0f };
}

void CalculateBBoxForAreaLight(Point3& bboxMin, Point3& bboxMax, const TimeValue& time, FireRenderPhysicalLight* light)
{
	FRPhysicalLight_AreaLight_LightShape lightShape = light->GetLightShapeMode(time);
	switch (lightShape)
	{
		case FRPhysicalLight_DISC:
		{
			float radius = (light->GetSecondPoint(time) - light->GetLightPoint(time)).Length() / 2;
			bboxMin = { -radius, -radius, 0.0f };
			bboxMax = { radius, radius, 0.1f };

			break;
		}

		case FRPhysicalLight_CYLINDER:
		{
			float radius = (light->GetSecondPoint(time) - light->GetLightPoint(time)).Length() / 2;
			float height = light->GetThirdPoint(time).z - light->GetSecondPoint(time).z;
			if (light->IsTargeted())
			{
				height = -height; // because cylinder is "mirrored" for inteface convinience when target is present
				if (light->GetTargetPoint(time).z < light->GetThirdPoint(time).z)
					height = -height;
			}
			bboxMin = { -radius, -radius, (height <= 0) ? height : 0.0f };
			bboxMax = { radius, radius, (height > 0) ? height : 0.0f };

			break;
		}

		case FRPhysicalLight_SPHERE:
		{
			float radius = (light->GetSecondPoint(time) - light->GetLightPoint(time)).Length() / 2;
			bboxMin = { -radius, -radius, -radius };
			bboxMax = { radius, radius, radius };

			break;
		}

		case FRPhysicalLight_RECTANGLE:
		{
			Point3 rectCenter = Point3(0.0f, 0.0f, 0.0f) + (light->GetSecondPoint(time) - light->GetLightPoint(time)) / 2;
			std::vector<Point3> bbox = { Point3(0.0f, 0.0f, 0.0f) - rectCenter , light->GetSecondPoint(time) - light->GetLightPoint(time) - rectCenter };
			// 3DMax can accept ONLY (Xmin < Xmax, Ymin < Ymax, Zmin < Zmax)
			bboxMin = { INFINITY, INFINITY, INFINITY };
			bboxMax = { -INFINITY, -INFINITY, -INFINITY };

			for (const Point3& tPoint : bbox)
			{
				if (tPoint.x > bboxMax.x)
					bboxMax.x = tPoint.x;

				if (tPoint.y > bboxMax.y)
					bboxMax.y = tPoint.y;

				if (tPoint.x < bboxMin.x)
					bboxMin.x = tPoint.x;

				if (tPoint.y < bboxMin.y)
					bboxMin.y = tPoint.y;
			}
			bboxMin.z = 0.0f;
			bboxMax.z = 0.1f;

			break;
		}

		case FRPhysicalLight_MESH:
		{
			// should not be handled here

			break; 
		}
	}
}

void FireRenderPhysicalLight::GetWorldBoundBox(TimeValue t, INode* inode, ViewExp* vpt, Box3& box)
{
	//set default output result
	box.Init();

	// back-off
	if (!vpt || !vpt->IsAlive())
	{
		return;
	}

	TimeValue time = GetCOREInterface()->GetTime();

	// node transform
	Matrix3 tm = inode->GetObjectTM(t);

	// different bbox for different types of light
	FRPhysicalLight_LightType lightType = GetLightType(time);
	Point3 bboxMin(0.0f, 0.0f, 0.0f);
	Point3 bboxMax(0.0f, 0.0f, 0.0f);
	switch (lightType)
	{
		case FRPhysicalLight_AREA:
		{
			FRPhysicalLight_AreaLight_LightShape lightShape = GetLightShapeMode(time);

			// mesh is handled here because we call GetWorldBoundBox on object mesh light refers to
			if (FRPhysicalLight_MESH == lightShape)
			{
				// get original object
				ReferenceTarget* pRef = nullptr;
				TimeValue time = GetCOREInterface()->GetTime();
				Interval valid = FOREVER;
				m_pblock->GetValue(FRPhysicalLight_AREALIGHT_LIGHTMESH, time, pRef, valid);

				if (pRef == nullptr)
					break;

				INode* pNode = static_cast<INode*>(pRef);
				if (pNode == nullptr)
				{
					break;
				}

				Object* pObj = pNode->GetObjectRef();
				if (pObj == nullptr)
				{
					break;
				}

				// get original object bbox
				Box3 objBox;
				objBox.Init();
				pObj->GetWorldBoundBox(t, inode, vpt, objBox);

				// apply transform
				INode* thisNode = GetThisNode();
				FASSERT(thisNode);
				Matrix3 thisTM = thisNode->GetNodeTM(t);
				thisTM.Invert();

				bboxMin = thisTM * objBox.Min();
				bboxMax = thisTM * objBox.Max();
			}
			else
			{
				CalculateBBoxForAreaLight(bboxMin, bboxMax, time, this);
			}

			break;
		}

		case FRPhysicalLight_SPOT:
		{
			CalculateBBoxForSpotLight(bboxMin, bboxMax, time, this);

			break;
		}

		case FRPhysicalLight_POINT:
		case FRPhysicalLight_DIRECTIONAL:
		{
			bboxMin = { -2.0f, -2.0f, -2.0f };
			bboxMax = { 2.0f, 2.0f, 2.0f };

			break;
		}
	}

	// rotate calculated bbox
	std::vector<Point3> bbox = {
		Point3(bboxMin.x, bboxMin.y, bboxMin.z),
		Point3(bboxMax.x, bboxMin.y, bboxMin.z),
		Point3(bboxMin.x, bboxMax.y, bboxMin.z),
		Point3(bboxMin.x, bboxMin.y, bboxMax.z),
		Point3(bboxMax.x, bboxMax.y, bboxMax.z),
		Point3(bboxMin.x, bboxMax.y, bboxMax.z),
		Point3(bboxMax.x, bboxMin.y, bboxMax.z),
		Point3(bboxMax.x, bboxMax.y, bboxMin.z)
	};

	for (Point3& tPoint : bbox)
	{
		tPoint = tPoint * tm;
	}

	// adjust box after it is rotated
	{
		bboxMin = { INFINITY, INFINITY, INFINITY };
		bboxMax = { -INFINITY, -INFINITY, -INFINITY };
		for (const Point3& tPoint : bbox)
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
	}

	// output result
	box.pmin = bboxMin;
	box.pmax = bboxMax;
}

void PhysLightsVolumeDlgProc::DeleteThis()
{}

FIRERENDER_NAMESPACE_END
