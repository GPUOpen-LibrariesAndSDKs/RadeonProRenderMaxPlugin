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

FireRenderPhysicalLight::FireRenderPhysicalLight()
	: m_isPreview(false)
	, m_isTargetPreview(false)
	, m_isNotifyMoveLight(false)
	, m_isNotifyMoveTarget(false)
{
	ReplaceLocalReference(IndirectReference::ThisNode, m_thisNodeMonitor);
	ReplaceLocalReference(IndirectReference::TargetNode, m_targNodeMonitor);

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

void FireRenderPhysicalLight::BeginPreview(void)
{
	m_isPreview = true;
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

IParamBlock2* FireRenderPhysicalLight::GetParamBlockByID(BlockID id) const
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

void FireRenderPhysicalLight::BeginEditParams(IObjParam *ip, ULONG flags, Animatable *prev)
{
	GetFireRenderPhysicalLightDesc()->BeginEditParams(ip, this, flags, prev);
	UpdateUI();
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

	// Allow safe handling when no target exists yet;
	// Occurs in create mode during taget positioning mouse drag, and may occur via MaxScript
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
	FASSERT(m_thisNodeMonitor);
	dynamic_cast<INodeTransformMonitor*>(m_thisNodeMonitor)->SetNode(node);
}

void FireRenderPhysicalLight::SetTargetNode(INode* node)
{
	FASSERT(m_targNodeMonitor);
	dynamic_cast<INodeTransformMonitor*>(m_targNodeMonitor)->SetNode(node);
}

INode* FireRenderPhysicalLight::GetThisNode()
{
	// back-off
	if (m_thisNodeMonitor == nullptr)
		return nullptr;

	return dynamic_cast<INodeTransformMonitor*>(m_thisNodeMonitor)->GetNode();
}

INode* FireRenderPhysicalLight::GetTargetNode()
{
	// back-off
	if (m_targNodeMonitor == nullptr)
		return nullptr;

	return dynamic_cast<INodeTransformMonitor*>(m_targNodeMonitor)->GetNode();
}

const INode* FireRenderPhysicalLight::GetThisNode() const
{
	// back-off
	if (m_targNodeMonitor == nullptr)
		return nullptr;

	return dynamic_cast<INodeTransformMonitor*>(m_thisNodeMonitor)->GetNode();
}

const INode* FireRenderPhysicalLight::GetTargetNode() const
{
	// back-off
	if (m_targNodeMonitor == nullptr)
		return nullptr;

	return dynamic_cast<INodeTransformMonitor*>(m_targNodeMonitor)->GetNode();
}

void FireRenderPhysicalLight::SetLightPoint(const Point3& p, const TimeValue& time)
{
	if(m_pblock!= NULL)
	{
		m_pblock->SetValue(FRPhysicalLight_LIGHT_POINT, time, p);
	}
}

const Point3 FireRenderPhysicalLight::GetLightPoint(const TimeValue& time) const
{
	Point3 lightPoint (0.0f, 0.0f, 0.0f);
	if(m_pblock!= NULL)
	{
		Interval valid = FOREVER;
		BOOL success = m_pblock->GetValue(FRPhysicalLight_LIGHT_POINT, time, lightPoint, valid);
		FASSERT(success);
	}

	return lightPoint;
}

void FireRenderPhysicalLight::SetTargetPoint(const Point3& p, const TimeValue& time)
{
	if(m_pblock!= NULL)
	{
		m_pblock->SetValue(FRPhysicalLight_TARGET_POINT, time, p);
	}
}

const Point3 FireRenderPhysicalLight::GetTargetPoint(const TimeValue& time) const
{
	Point3 targPoint (0.0f, 0.0f, 0.0f);
	if(m_pblock!= NULL)
	{
		Interval valid = FOREVER;
		BOOL success = m_pblock->GetValue(FRPhysicalLight_TARGET_POINT, time, targPoint, valid);
		FASSERT(success);
	}

	return targPoint;
}

void FireRenderPhysicalLight::SetSecondPoint(const Point3& p, const TimeValue& time)
{
	if(m_pblock!= NULL)
	{
		m_pblock->SetValue(FRPhysicalLight_SECOND_POINT, time, p);
	}
}

const Point3 FireRenderPhysicalLight::GetSecondPoint(const TimeValue& time) const
{
	Point3 secondPoint (0.0f, 0.0f, 0.0f);
	if(m_pblock!= NULL)
	{
		Interval valid = FOREVER;
		BOOL success = m_pblock->GetValue(FRPhysicalLight_SECOND_POINT, time, secondPoint, valid);
		FASSERT(success);
	}

	return secondPoint;
}

void FireRenderPhysicalLight::SetThirdPoint(const Point3& p, const TimeValue& time)
{
	if(m_pblock!= NULL)
	{
		m_pblock->SetValue(FRPhysicalLight_THIRD_POINT, time, p);
	}
}

const Point3 FireRenderPhysicalLight::GetThirdPoint(const TimeValue& time) const
{
	Point3 thirdPoint (0.0f, 0.0f, 0.0f);
	if(m_pblock!= NULL)
	{
		Interval valid = FOREVER;
		BOOL success = m_pblock->GetValue(FRPhysicalLight_THIRD_POINT, time, thirdPoint, valid);
		FASSERT(success);
	}

	return thirdPoint;
}

void FireRenderPhysicalLight::SetLength(float length, const TimeValue& time)
{
	if(m_pblock!= NULL)
	{
		m_pblock->SetValue(FRPhysicalLight_AREA_LENGTHS, time, length);
	}
}

float FireRenderPhysicalLight::GetLength(const TimeValue& time) const
{
	float length = 0.0f;
	if(m_pblock!= NULL)
	{
		Interval valid = FOREVER;
		BOOL success = m_pblock->GetValue(FRPhysicalLight_AREA_LENGTHS, time, length, valid);
		FASSERT(success);
	}

	return length;
}

void FireRenderPhysicalLight::SetWidth(float length, const TimeValue& time)
{
	if(m_pblock!= NULL)
	{
		m_pblock->SetValue(FRPhysicalLight_AREA_WIDTHS, time, length);
	}
}

float FireRenderPhysicalLight::GetWidth(const TimeValue& time) const
{
	float width = 0.0f;
	if(m_pblock!=NULL)
	{
		Interval valid = FOREVER;
		BOOL success = m_pblock->GetValue(FRPhysicalLight_AREA_WIDTHS, time, width, valid);
		FASSERT(success);
	}

	return width;
}

void FireRenderPhysicalLight::SetDist(float length, const TimeValue& time)
{
	if(m_pblock!=NULL)
	{
		m_pblock->SetValue(FRPhysicalLight_TARGET_DIST, time, length);
	}
}

float FireRenderPhysicalLight::GetDist(const TimeValue& time) const
{
	float dist = 0.0f;
	if(m_pblock!=NULL)
	{
		Interval valid = FOREVER;
		BOOL success = m_pblock->GetValue(FRPhysicalLight_TARGET_DIST, time, dist, valid);
		FASSERT(success);
	}

	return dist;
}

FRPhysicalLight_AreaLight_LightShape FireRenderPhysicalLight::GetLightShapeMode(const TimeValue& time) const
{
	int value = FRPhysicalLight_DISC;
	if(m_pblock!=NULL)
	{
		Interval valid = FOREVER;
		m_pblock->GetValue(FRPhysicalLight_AREALIGHT_LIGHTSHAPE, time, value, valid);
	}

	return static_cast<FRPhysicalLight_AreaLight_LightShape>(value);
}

FRPhysicalLight_LightType FireRenderPhysicalLight::GetLightType(const TimeValue& time) const
{
	int lightType = FRPhysicalLight_AREA;
	if(m_pblock!=NULL)
	{
		Interval valid = FOREVER;
		m_pblock->GetValue(FRPhysicalLight_LIGHT_TYPE, time, lightType, valid);
	}

	return static_cast<FRPhysicalLight_LightType>(lightType);
}

bool FireRenderPhysicalLight::IsTargeted() const 
{
	int boolValue = 0;
	if(m_pblock!=NULL)
	{
		Interval valid = FOREVER;
		m_pblock->GetValue(FRPhysicalLight_IS_TARGETED, 0, boolValue, valid);
	}

	return bool_cast(boolValue);
}

bool FireRenderPhysicalLight::HasTargetNode() const
{
	return (IsTargeted() && (GetTargetNode()!=nullptr));
}

bool FireRenderPhysicalLight::IsEnabled() const
{
	int boolValue = 0;
	if(m_pblock!=NULL)
	{
		Interval valid = FOREVER;
		m_pblock->GetValue(FRPhysicalLight_ISENABLED, 0, boolValue, valid);
	}

	return bool_cast(boolValue);
}

void FireRenderPhysicalLight::AddTarget(TimeValue t, bool fromCreateCallback)
{
	INode* thisNode = GetThisNode();
	if (!thisNode)
	{
		// in case node is not initialized 
		thisNode = FindNodeRef(this);
		FASSERT(thisNode);
		SetThisNode(thisNode);
	}

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
	float targetDist;
	FRPhysicalLight_LightType lightType = GetLightType(t);
	if( (!fromCreateCallback) && (FRPhysicalLight_AREA == lightType) )
	{	// Target added after creation for an Area Light, calculate appropriate target distance
		Box3 bbox = GetAreaLightBoundBox(t);
		float length = bbox.pmax.x-bbox.pmin.x;
		float width = bbox.pmax.y-bbox.pmin.y;
		targetDist = MAX(length,width);
	}
	else if( (!fromCreateCallback) && ((FRPhysicalLight_SPOT == lightType) || (FRPhysicalLight_DIRECTIONAL == lightType)) )
	{	// Target added after creation for a Spot Light, calculate appropriate target distance
		targetDist = GetSpotLightYon(t) * 2.0f;
	}
	else // all other cases
		targetDist = GetDist(t);

	Point3 p1( 0.0f, 0.0f, -targetDist );

	Matrix3 mat = thisNode->GetNodeTM(t);
	mat.PreTranslate(p1); // Current position in world space, plus local space offset to target 

	targNode->SetNodeTM(t, mat);
	targNode->InvalidateTM();

	// Colour
	Color lightWireColor(thisNode->GetWireColor());
	targNode->SetWireColor(lightWireColor.toRGB());

	targNode->NotifyDependents(FOREVER, PART_OBJ, REFMSG_CHANGE);
	targNode->NotifyDependents(FOREVER, PART_OBJ, REFMSG_NUM_SUBOBJECTTYPES_CHANGED);
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


// For spot and directional, distance to end of cone of viewport gizmo
float FireRenderPhysicalLight::GetSpotLightYon(TimeValue t) const
{
	FRPhysicalLight_LightType lightType = GetLightType(t);
	if( (lightType==FRPhysicalLight_SPOT) || (lightType==FRPhysicalLight_DIRECTIONAL) )
	{
		// Special case: "Yon" value for cone length is set during create mode and cannot be changed.
		// Calculated from creation mouse click points, because no other official param exists.
		float yon = (GetSecondPoint(t)-GetLightPoint(t)).Length();
		return yon;
	}
	return 0.0f;
}

// For cylinder area lights, whether shape topside is upward in local z-axis
bool FireRenderPhysicalLight::GetAreaLightUpright(TimeValue t) const
{
	// Always true for shapes other than cylinder.
	// Normally true for cylinder light without target.
	// Normally false for cylinder with target, unless flipped (see below)
	// Note that if cylinder is targeted and height is negative, it may look upright when actually not.
	bool upright = true;

	bool isTargetPreview = m_isPreview && m_isTargetPreview;
	bool isEditMode = !m_isPreview;
	bool isDisplayTarget = (isEditMode && HasTargetNode()) || (isTargetPreview && IsTargeted());

	if( isDisplayTarget )
	{
		if( isTargetPreview )
			upright = false;
		else
			// Special case: If target cylinder light has target below the object, orientation flips.
			// Check using creation mouse click points, because no formal param exists.
			// Note local z-axis is downward when target point is set; higher value is mouse downward.
			upright = (GetTargetPoint(t).z <= GetLightPoint(t).z);
	}
	return upright;
}

Box3 FireRenderPhysicalLight::GetSpotLightBoundBox(TimeValue t) const
{
	float angle = 0.0f;
	Interval valid = FOREVER;
	GetParamBlockByID(0)->GetValue(FRPhysicalLight_SPOTLIGHT_OUTERCONE, 0, angle, valid);

	float length = GetSpotLightYon(t);
	float tanAlpha = tan(angle * PI / 180);
	float coneRad = length * tanAlpha;

	return Box3(
		Point3( -coneRad, -coneRad, -length ),
		Point3(  coneRad,  coneRad, 0.0f ) );
}


// For area lights, corner-to-corner dimensions in local space of viewport gizmo
Box3 FireRenderPhysicalLight::GetAreaLightBoundBox(TimeValue t) const
{
	Point3 p[2] = { Point3( 0.0f, 0.0f, 0.0f ), Point3( 0.0f, 0.0f, 0.0f ) };
	FRPhysicalLight_LightType lightType = GetLightType(t);
	FRPhysicalLight_AreaLight_LightShape lightShape = GetLightShapeMode(t);

	// mesh not handled here because we'd need to call GetWorldBoundBox on object mesh light refers to
	if( (lightType==FRPhysicalLight_AREA) && (lightShape!=FRPhysicalLight_MESH) )
	{
		// Special case: Offset transform axis during create mode (preview)
		// Offset applies to Area lights only, not Spot or directional lights
		// Offset applies during initial mouse drag, before target positioning mouse drag
		// During this time, transform axis is located at "upper left" corner of the shape
		bool isInitialPreview = m_isPreview && !m_isTargetPreview;
		//if( isInitialPreview )
		//	centerPos = Point3( previewSpan.x/2.0f, previewSpan.y/2.0f, 0.0f ); // offset in x and y only

		Point3 origin( 0.0f, 0.0f, 0.0f );
		Point3 preview1Span = (GetSecondPoint(t)-GetLightPoint(t));
		Point3 preview2Span = (GetThirdPoint(t)-GetSecondPoint(t));
		Point3 preview1Center = preview1Span/2.0f;
		float preview1Length = preview1Span.Length();

		// Set the x-value and y-value for rectangle
		if( lightShape==FRPhysicalLight_RECTANGLE )
		{
			if( isInitialPreview ) // Use mouse click points in create mode during mouse drag (preview)
			{
				p[0] = origin;
				p[1] = Point3( preview1Span.x, preview1Span.y, 0.0f );
			}
			else {
				Point3  offsetFromCenter = Point3( GetLength(t)/2.0f, GetWidth(t)/2.0f, 0.0f );
				p[0] = -offsetFromCenter;
				p[1] =  offsetFromCenter;
			}
		}

		// Set the x-value and y-value for circular gizmos
		if( (lightShape==FRPhysicalLight_DISC) || (lightShape==FRPhysicalLight_SPHERE) || (lightShape==FRPhysicalLight_CYLINDER) )
		{
			if( isInitialPreview ) // Use mouse click points in create mode during mouse drag (preview)
			{
				Point3 offsetFromCenter( preview1Length/2.0f, preview1Length/2.0f, 0.0f );
				p[0] = preview1Center - offsetFromCenter;
				p[1] = preview1Center + offsetFromCenter;
			}
			else
			{
				Point3  offsetFromCenter = Point3( GetWidth(t)/2.0f, GetWidth(t)/2.0f, 0.0f );
				p[0] = -offsetFromCenter;
				p[1] =  offsetFromCenter;
			}
		}

		// Set the z-value for cylinder only
		if( lightShape==FRPhysicalLight_CYLINDER )
		{
			if( isInitialPreview ) // Use mouse click points in create mode during mouse drag (preview)
				 p[1].z = p[0].z + preview2Span.z;
			else if( GetAreaLightUpright(t) )
				 p[1].z = p[0].z + GetLength(t);
			else p[1].z = p[0].z - GetLength(t);
		}
	}

	Box3 bbox;
	bbox.IncludePoints(p,2);

	return bbox;
}

void FireRenderPhysicalLight::GetWorldBoundBox(TimeValue t, INode* inode, ViewExp* vpt, Box3& bbox)
{
	//set default output result
	bbox.Init();

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
				if(m_pblock!=NULL)
				{
					Interval valid = FOREVER;
					m_pblock->GetValue(FRPhysicalLight_AREALIGHT_LIGHTMESH, time, pRef, valid);
				}

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

				bbox.pmin = thisTM * objBox.Min();
				bbox.pmin = thisTM * objBox.Max();
			}
			else
			{
				bbox = GetAreaLightBoundBox(t);
			}

			break;
		}

		case FRPhysicalLight_SPOT:
		{
			bbox = GetSpotLightBoundBox(t);

			break;
		}

		case FRPhysicalLight_POINT:
		case FRPhysicalLight_DIRECTIONAL:
		{
			bbox.pmin = { -2.0f, -2.0f, -2.0f };
			bbox.pmax = {  2.0f,  2.0f,  2.0f };

			break;
		}
	}

	// rotate calculated bbox
	std::vector<Point3> bboxPts = {
		Point3(bbox.pmin.x, bbox.pmin.y, bbox.pmin.z),
		Point3(bbox.pmax.x, bbox.pmin.y, bbox.pmin.z),
		Point3(bbox.pmin.x, bbox.pmax.y, bbox.pmin.z),
		Point3(bbox.pmin.x, bbox.pmin.y, bbox.pmax.z),
		Point3(bbox.pmax.x, bbox.pmax.y, bbox.pmax.z),
		Point3(bbox.pmin.x, bbox.pmax.y, bbox.pmax.z),
		Point3(bbox.pmax.x, bbox.pmin.y, bbox.pmax.z),
		Point3(bbox.pmax.x, bbox.pmax.y, bbox.pmin.z)
	};

	for (Point3& tPoint : bboxPts)
	{
		tPoint = tPoint * tm;
	}

	// adjust box after it is rotated
	{
		bbox.pmin = {  INFINITY,  INFINITY,  INFINITY };
		bbox.pmax = { -INFINITY, -INFINITY, -INFINITY };
		for (const Point3& tPoint : bboxPts)
		{
			if (tPoint.x > bbox.pmax.x)
				bbox.pmax.x = tPoint.x;

			if (tPoint.y > bbox.pmax.y)
				bbox.pmax.y = tPoint.y;

			if (tPoint.z > bbox.pmax.z)
				bbox.pmax.z = tPoint.z;

			if (tPoint.x < bbox.pmin.x)
				bbox.pmin.x = tPoint.x;

			if (tPoint.y < bbox.pmin.y)
				bbox.pmin.y = tPoint.y;

			if (tPoint.z < bbox.pmin.z)
				bbox.pmin.z = tPoint.z;
		}
	}
}

void PhysLightsVolumeDlgProc::DeleteThis()
{}

FIRERENDER_NAMESPACE_END
