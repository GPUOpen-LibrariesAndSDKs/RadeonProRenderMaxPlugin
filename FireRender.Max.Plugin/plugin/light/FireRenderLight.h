#pragma once

#include "IFireRenderLight.h"
#include "object.h"
#include "INodeTransformMonitor.h"

FIRERENDER_NAMESPACE_BEGIN

// Contains default implementations for some abstract methods
// which makes the final implementation simpler to read
class FireRenderLight :
	public LightObject,
	public IFireRenderLight
{
public:
	FireRenderLight(void);
	~FireRenderLight(void);

	enum StrongReference
	{
		ParamBlock = 0,

		// This should be always last
		strongRefEnd
	};

	enum IndirectReference
	{
		ThisNode = StrongReference::strongRefEnd,
		TargetNode,

		// This should be always last
		indirectRefEnd
	};

	int NumRefs() override;
	void SetReference(int i, RefTargetHandle rtarg) override;
	RefTargetHandle GetReference(int i) override;

	using BaseMaxType = LightObject;

	void SetAttenDisplay(int) override;
	BOOL GetAttenDisplay() override;
	void Enable(int) override;
	void SetMapBias(TimeValue, float) override;
	float GetMapBias(TimeValue, Interval &) override;
	void SetMapRange(TimeValue, float) override;
	float GetMapRange(TimeValue, Interval &) override;
	void SetMapSize(TimeValue, int) override;
	int  GetMapSize(TimeValue, Interval &) override;
	void SetRayBias(TimeValue, float) override;
	float GetRayBias(TimeValue, Interval &) override;
	int GetUseGlobal() override;
	void SetUseGlobal(int) override;
	int GetShadowType() override;
	void SetShadowType(int) override;
	int GetAbsMapBias() override;
	void SetAbsMapBias(int) override;

	Point3 GetRGBColor(TimeValue t, Interval& valid = Interval(0, 0)) override;
	void SetRGBColor(TimeValue t, Point3 &color) override;
	void SetUseLight(int onOff) override;
	BOOL GetUseLight() override;
	void SetHotspot(TimeValue time, float f) override;
	float GetHotspot(TimeValue t, Interval& valid) override;
	void SetFallsize(TimeValue time, float f) override;
	float GetFallsize(TimeValue t, Interval& valid) override;
	void SetAtten(TimeValue time, int which, float f) override;
	float GetAtten(TimeValue t, int which, Interval& valid) override;
	void SetConeDisplay(int s, int notify) override;
	BOOL GetConeDisplay() override;
	void SetIntensity(TimeValue time, float f) override;
	float GetIntensity(TimeValue t, Interval& valid = Interval(0, 0)) override;
	void SetUseAtten(int s) override;
	BOOL GetUseAtten() override;
	void SetAspect(TimeValue t, float f) override;
	float GetAspect(TimeValue t, Interval& valid = Interval(0, 0)) override;
	void SetOvershoot(int a) override;
	int GetOvershoot() override;
	void SetShadow(int a) override;
	int GetShadow() override;
	ObjLightDesc* CreateLightDesc(INode *inode, BOOL forceShadowBuf) override;

	void UpdateTargDistance(TimeValue t, INode* inode) override;
	void SetTDist(TimeValue time, float f) override;
	float GetTDist(TimeValue t, Interval& valid) override;

protected:
	// References
	IParamBlock2* m_pblock;
	RefTargetHandle m_thisNodeMonitor;
	RefTargetHandle m_targNodeMonitor;
};

FIRERENDER_NAMESPACE_END
