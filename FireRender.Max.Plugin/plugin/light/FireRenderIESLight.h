/*********************************************************************************************************************************
* Radeon ProRender for 3ds Max plugin
* Copyright (c) 2017 AMD
* All Rights Reserved
*
* Custom IES light 3ds MAX scene node. This custom node allows the definition of IES light, using rectangular shapes
* that can be positioned and oriented in the MAX scene.
*********************************************************************************************************************************/

#pragma once

#include "FireRenderIES_General.h"
#include "FireRenderIES_Intensity.h"
#include "FireRenderIES_Shadows.h"
#include "FireRenderIES_Volume.h"
#include "IESLightParameter.h"
#include "parser/SceneParser.h"
#include "parser/RenderParameters.h"
#include "frScope.h"

FIRERENDER_NAMESPACE_BEGIN

class FireRenderIESLight : public GenLight
{
public:
	static Class_ID GetClassId();
	static ClassDesc2* GetClassDesc();
	FireRenderIESLight();
	~FireRenderIESLight();

	CreateMouseCallBack* GetCreateMouseCallBack() override;
	GenLight* NewLight(int type) override;
	ObjectState Eval(TimeValue time) override;
	void InitNodeName(TSTR& s) override;
	const MCHAR *GetObjectName() override;
	Interval ObjectValidity(TimeValue t) override;

	BOOL UsesWireColor() override;
	int DoOwnSelectHilite() override;
	void NotifyChanged();
	void BuildVertices(bool force = false);
	RefResult NotifyRefChanged(const Interval& interval, RefTargetHandle hTarget, PartID& partId, RefMessage msg, BOOL propagate) override;

	void DeleteThis() override;
	Class_ID ClassID() override;
	void GetClassName(TSTR& s) override;
	RefTargetHandle Clone(RemapDir& remap) override;
	IParamBlock2* GetParamBlock(int i) override;
	IParamBlock2* GetParamBlockByID(BlockID id) override;
	int NumRefs() override;
	void SetReference(int i, RefTargetHandle rtarg) override;
	RefTargetHandle GetReference(int i) override;
	void DrawGeometry(ViewExp *vpt, IParamBlock2 *pblock, BOOL sel = FALSE, BOOL frozen = FALSE);
	Matrix3 GetTransformMatrix(TimeValue t, INode* inode, ViewExp* vpt);
	Color GetViewportMainColor(INode* pNode);
	Color GetViewportColor(INode* pNode, Color selectedColor);
	int Display(TimeValue t, INode* inode, ViewExp *vpt, int flags) override;
	void GetWorldBoundBox(TimeValue t, INode* inode, ViewExp* vpt, Box3& box) override;
	int HitTest(TimeValue t, INode* inode, int type, int crossing, int flags, IPoint2 *p, ViewExp *vpt) override;
	void GetLocalBoundBox(TimeValue t, INode* inode, ViewExp* vpt, Box3& box) override;

	// LightObject methods
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
	void SetTDist(TimeValue time, float f) override;
	float GetTDist(TimeValue t, Interval& valid) override;
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

	// From Light
	RefResult EvalLightState(TimeValue t, Interval& valid, LightState* cs) override;

	ObjLightDesc* CreateLightDesc(INode *inode, BOOL forceShadowBuf) override;
	int Type() override;
	void SetHSVColor(TimeValue, Point3 &) override;
	Point3 GetHSVColor(TimeValue t, Interval &valid) override;

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
	BOOL IsSpot() override;
	BOOL IsDir() override;
	void SetSpotShape(int) override;
	int GetSpotShape() override;
	void SetContrast(TimeValue, float) override;
	float GetContrast(TimeValue, Interval &) override;
	void SetUseAttenNear(int) override;
	BOOL GetUseAttenNear() override;
	void SetAttenNearDisplay(int) override;
	BOOL GetAttenNearDisplay() override;
	ExclList &GetExclusionList() override;
	void SetExclusionList(ExclList &) override;
	BOOL SetHotSpotControl(Control *) override;
	BOOL SetFalloffControl(Control *) override;
	BOOL SetColorControl(Control *) override;
	Control *GetHotSpotControl() override;
	Control *GetFalloffControl() override;
	Control *GetColorControl() override;

	void SetAffectDiffuse(BOOL onOff) override;
	BOOL GetAffectDiffuse() override;
	void SetAffectSpecular(BOOL onOff) override;
	BOOL GetAffectSpecular() override;
	void SetAmbientOnly(BOOL onOff) override;
	BOOL GetAmbientOnly() override;
	void SetDecayType(BOOL onOff) override;
	BOOL GetDecayType() override;
	void SetDecayRadius(TimeValue time, float f) override;
	float GetDecayRadius(TimeValue t, Interval& valid) override;
	void BeginEditParams(IObjParam *objParam, ULONG flags, Animatable *prev) override;
	void EndEditParams(IObjParam *objParam, ULONG flags, Animatable *next) override;

	void CreateSceneLight(const ParsedNode& node, frw::Scope scope, const RenderParameters& params);

	void FireRenderIESLight::AddTarget();

	void SetEnabled(bool value);
	bool GetEnabled() const;

	void SetTargeted(bool value);
	bool GetTargeted() const;

	void SetAreaWidth(float value);
	float GetAreaWidth() const;

	void ImportFile(const char* filename);
	void ActivateProfile(const TCHAR* profileName);

protected:
	ExclList m_exclList;

private:
	static const Class_ID m_classId;

	// Panels
	IES_General m_general;
	IES_Intensity m_intensity;
	IES_Shadows m_shadows;
	IES_Volume m_volume;

	IObjParam* m_iObjParam;
	IParamBlock2* m_pblock2;

	Point3 m_vertices[4];
	bool m_verticesBuilt;
};

FIRERENDER_NAMESPACE_END
