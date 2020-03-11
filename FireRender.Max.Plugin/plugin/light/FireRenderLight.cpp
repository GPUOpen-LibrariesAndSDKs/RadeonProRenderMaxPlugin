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


#include "FireRenderLight.h"

FIRERENDER_NAMESPACE_BEGIN

ReferenceTarget* MakeNodeTransformMonitor()
{
	Interface* ip = GetCOREInterface();
	return static_cast<ReferenceTarget*>(ip->CreateInstance(REF_TARGET_CLASS_ID, NODETRANSFORMMONITOR_CLASS_ID));
}

FireRenderLight::FireRenderLight()
	: m_pblock(nullptr)
	, m_thisNodeMonitor(MakeNodeTransformMonitor())
	, m_targNodeMonitor(MakeNodeTransformMonitor())
{}

FireRenderLight::~FireRenderLight()
{}

void FireRenderLight::SetAttenDisplay(int) 
{ 
	/* empty */ 
}

BOOL FireRenderLight::GetAttenDisplay() 
{ 
	return FALSE; 
}

void FireRenderLight::Enable(int) 
{}

void FireRenderLight::SetMapBias(TimeValue, float) 
{ 
	/* empty */ 
}

float FireRenderLight::GetMapBias(TimeValue, Interval &) 
{ 
	return 0; 
}

void FireRenderLight::SetMapRange(TimeValue, float) 
{ 
	/* empty */ 
}

float FireRenderLight::GetMapRange(TimeValue, Interval &) 
{ 
	return 0; 
}

void FireRenderLight::SetMapSize(TimeValue, int) 
{ 
	/* empty */ 
}

int  FireRenderLight::GetMapSize(TimeValue, Interval &) 
{ 
	return 0; 
}

void FireRenderLight::SetRayBias(TimeValue, float) 
{ 
	/* empty */ 
}

float FireRenderLight::GetRayBias(TimeValue, Interval &) 
{ 
	return 0; 
}

int FireRenderLight::GetUseGlobal() 
{ 
	return FALSE; 
}

void FireRenderLight::SetUseGlobal(int) 
{ 
	/* empty */ 
}

int FireRenderLight::GetShadowType() 
{ 
	return -1; 
}

void FireRenderLight::SetShadowType(int) 
{ 
	/* empty */ 
}

int FireRenderLight::GetAbsMapBias() 
{ 
	return FALSE; 
}

void FireRenderLight::SetAbsMapBias(int) 
{ 
	/* empty */ 
}

Point3 FireRenderLight::GetRGBColor(TimeValue t, Interval& valid) 
{ 
	return Color(1.f, 1.f, 1.f); 
}

void FireRenderLight::SetRGBColor(TimeValue t, Point3 &color) 
{}

void FireRenderLight::SetUseLight(int onOff) 
{ 
	/* empty */ 
}

BOOL FireRenderLight::GetUseLight() 
{ 
	return TRUE; 
}

void FireRenderLight::SetHotspot(TimeValue time, float f) 
{ 
	/* empty */ 
}

float FireRenderLight::GetHotspot(TimeValue t, Interval& valid) 
{ 
	return -1.0f; 
}

void FireRenderLight::SetFallsize(TimeValue time, float f) 
{ 
	/* empty */ 
}

float FireRenderLight::GetFallsize(TimeValue t, Interval& valid) 
{ 
	return -1.0f; 
}

void FireRenderLight::SetAtten(TimeValue time, int which, float f) 
{ 
	/* empty */ 
}

float FireRenderLight::GetAtten(TimeValue t, int which, Interval& valid) 
{ 
	return 0.0f; 
}

void FireRenderLight::SetConeDisplay(int s, int notify) 
{ 
	/* empty */ 
}

BOOL FireRenderLight::GetConeDisplay() 
{ 
	return FALSE; 
}

void FireRenderLight::SetIntensity(TimeValue time, float f) 
{}

float FireRenderLight::GetIntensity(TimeValue t, Interval& valid) 
{ 
	return 1.f; 
}

void FireRenderLight::SetUseAtten(int s) 
{ 
	/* empty */ 
}

BOOL FireRenderLight::GetUseAtten() 
{ 
	return FALSE; 
}

void FireRenderLight::SetAspect(TimeValue t, float f) 
{ 
	/* empty */ 
}

float FireRenderLight::GetAspect(TimeValue t, Interval& valid) 
{ 
	return -1.0f; 
}

void FireRenderLight::SetOvershoot(int a) 
{ 
	/* empty */ 
}

int FireRenderLight::GetOvershoot() 
{ 
	return 0; 
}

void FireRenderLight::SetShadow(int a) 
{ 
	/* empty */ 
}

int FireRenderLight::GetShadow() 
{ 
	return 0; 
}

ObjLightDesc* FireRenderLight::CreateLightDesc(INode *inode, BOOL forceShadowBuf) 
{ 
	return nullptr; 
}

void FireRenderLight::UpdateTargDistance(TimeValue t, INode* inode) 
{}

void FireRenderLight::SetTDist(TimeValue time, float f) 
{}

float FireRenderLight::GetTDist(TimeValue t, Interval& valid) 
{ 
	return 1.f;
}

int FireRenderLight::NumRefs()
{
	return LightObject::NumRefs() + static_cast<int>(IndirectReference::indirectRefEnd);
}

void FireRenderLight::SetReference(int i, RefTargetHandle rtarg)
{
	FASSERT(i < NumRefs());

	int baseRefs = BaseMaxType::NumRefs();

	if (i < baseRefs)
	{
		BaseMaxType::SetReference(i, rtarg);
		return;
	}

	int local_i = i - baseRefs;

	switch (static_cast<StrongReference>(local_i))
	{
		case StrongReference::ParamBlock:
			m_pblock = dynamic_cast<IParamBlock2*>(rtarg);
			break;
	}

	switch (static_cast<IndirectReference>(local_i))
	{
		case IndirectReference::ThisNode:
			m_thisNodeMonitor = rtarg;
			break;

		case IndirectReference::TargetNode:
			m_targNodeMonitor = rtarg;
			break;
	}
}

RefTargetHandle FireRenderLight::GetReference(int i)
{
	FASSERT(i < NumRefs());

	int baseRefs = BaseMaxType::NumRefs();

	if (i < baseRefs)
	{
		return BaseMaxType::GetReference(i);
	}

	int local_i = i - baseRefs;

	RefTargetHandle result = nullptr;

	switch (static_cast<StrongReference>(local_i))
	{
		case StrongReference::ParamBlock:
		{
			result = m_pblock;
			break;
		}
	}

	switch (static_cast<IndirectReference>(local_i))
	{
		case IndirectReference::ThisNode:
		{
			result = m_thisNodeMonitor; // NodeMonitor
			break;
		}

		case IndirectReference::TargetNode:
		{
			result = m_targNodeMonitor; // NodeMonitor
			break;
		}
	}

	return result;
}




FIRERENDER_NAMESPACE_END
