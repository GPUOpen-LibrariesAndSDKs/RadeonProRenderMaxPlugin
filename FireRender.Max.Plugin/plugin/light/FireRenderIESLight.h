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
#include "IFireRenderLight.h"
#include "FireRenderIES_Intensity.h"
#include "FireRenderIES_Shadows.h"
#include "FireRenderIES_Volume.h"
#include "FireRenderLight.h"
#include "IESLightParameter.h"
#include "parser/SceneParser.h"
#include "parser/RenderParameters.h"
#include "frScope.h"

class LookAtTarget;

FIRERENDER_NAMESPACE_BEGIN

class FireRenderIESLight :
	public FireRenderLight
{
public:
	struct IntensitySettings :
		public MaxSpinner::DefaultFloatSettings
	{
		static const float Default;
	};
	using AreaWidthSettings = MaxSpinner::DefaultFloatSettings;
	using LightRotateSettings = MaxSpinner::DefaultRotationSettings;
	using ShadowsSoftnessSettings = MaxSpinner::DefaultFloatSettings;
	using ShadowsTransparencySettings = MaxSpinner::DefaultFloatSettings;
	using VolumeScaleSettings = MaxSpinner::DefaultFloatSettings;
	using TargetDistanceSettings = MaxSpinner::DefaultFloatSettings;

	static Class_ID GetClassId();
	static ClassDesc2* GetClassDesc();
	static Color GetWireColor(bool isFrozen, bool isSelected);

	static const bool DefaultEnabled;
	static const bool DefaultTargeted;
	static const bool DefaultShadowsEnabled;
	static const IESLightColorMode DefaultColorMode;

	static const bool EnableGeneralPanel;
	static const bool EnableIntensityPanel;
	static const bool EnableShadowsPanel;
	static const bool EnableVolumePanel;

	static const size_t SphereCirclePointsCount;
	static const size_t IES_ImageWidth;
	static const size_t IES_ImageHeight;

	FireRenderIESLight();
	~FireRenderIESLight();

	CreateMouseCallBack* GetCreateMouseCallBack() override;
	ObjectState Eval(TimeValue time) override;
	void InitNodeName(TSTR& s) override;
	const MCHAR *GetObjectName() override;
	Interval ObjectValidity(TimeValue t) override;

	BOOL UsesWireColor() override;
	int DoOwnSelectHilite() override;
	RefResult NotifyRefChanged(const Interval& interval, RefTargetHandle hTarget, PartID& partId, RefMessage msg, BOOL propagate) override;

	void DeleteThis() override;
	Class_ID ClassID() override;
	void GetClassName(TSTR& s) override;
	RefTargetHandle Clone(RemapDir& remap) override;

	IParamBlock2* GetParamBlock(int i) override;

	IParamBlock2* GetParamBlockByID(BlockID id) override;

	void DrawSphere(TimeValue t, ViewExp *vpt, BOOL sel = FALSE, BOOL frozen = FALSE);
	bool DrawWeb(TimeValue t, ViewExp *pVprt, bool isSelected = false, bool isFrozen = false); 

	Color GetViewportMainColor(INode* pNode);
	Color GetViewportColor(INode* pNode, Color selectedColor);
	
	int Display(TimeValue t, INode* inode, ViewExp *vpt, int flags) override;
	void GetWorldBoundBox(TimeValue t, INode* inode, ViewExp* vpt, Box3& box) override;
	int HitTest(TimeValue t, INode* inode, int type, int crossing, int flags, IPoint2 *p, ViewExp *vpt) override;
	void GetLocalBoundBox(TimeValue t, INode* inode, ViewExp* vpt, Box3& box) override;
	RefResult EvalLightState(TimeValue t, Interval& valid, LightState* cs) override;

	void BeginEditParams(IObjParam *objParam, ULONG flags, Animatable *prev) override;
	void EndEditParams(IObjParam *objParam, ULONG flags, Animatable *next) override;

	void CreateSceneLight(
		TimeValue t, const ParsedNode& node, frw::Scope scope,
		SceneAttachCallback* sceneAttachCallback ) override;
	bool DisplayLight(TimeValue t, INode* inode, ViewExp *vpt, int flags) override;
	bool CalculateLightRepresentation(const TCHAR* profileName);
	bool CalculateBBox(void) override;

	bool SetLightPoint(Point3 value, TimeValue t);
	Point3 GetLightPoint(TimeValue t, Interval& valid = FOREVER) const;

	bool SetTargetPoint(Point3 value, TimeValue t);
	Point3 GetTargetPoint(TimeValue t, Interval& valid = FOREVER) const;

	bool SetEnabled(bool value, TimeValue t);
	bool GetEnabled(TimeValue t, Interval& valid = FOREVER) const;

	bool SetTargeted(bool value, TimeValue t);
	bool GetTargeted(TimeValue t, Interval& valid = FOREVER) const;

	bool SetShadowsEnabled(bool value, TimeValue t);
	bool GetShadowsEnabled(TimeValue t, Interval& valid = FOREVER) const;

	bool SetTargetDistance(float value, TimeValue t);
	float GetTargetDistance(TimeValue t, Interval& valid = FOREVER) const;

	bool SetAreaWidth(float value, TimeValue t);
	float GetAreaWidth(TimeValue t, Interval& valid = FOREVER) const;

	bool SetRotationX(float value, TimeValue t);
	float GetRotationX(TimeValue t, Interval& valid = FOREVER) const;

	bool SetRotationY(float value, TimeValue t);
	float GetRotationY(TimeValue t, Interval& valid = FOREVER) const;

	bool SetRotationZ(float value, TimeValue t);
	float GetRotationZ(TimeValue t, Interval& valid = FOREVER) const;

	bool SetIntensity(float value, TimeValue t);
	float GetIntensity(TimeValue t, Interval& valid = FOREVER) const;

	bool SetTemperature(float value, TimeValue t);
	float GetTemperature(TimeValue t, Interval& valid = FOREVER) const;

	bool SetShadowsSoftness(float value, TimeValue t);
	float GetShadowsSoftness(TimeValue t, Interval& valid = FOREVER) const;

	bool SetVolumeScale(float value, TimeValue t);
	float GetVolumeScale(TimeValue t, Interval& valid = FOREVER) const;

	bool SetShadowsTransparency(float value, TimeValue t);
	float GetShadowsTransparency(TimeValue t, Interval& valid = FOREVER) const;

	bool SetActiveProfile(const TCHAR* value, TimeValue t);
	const TCHAR* GetActiveProfile(TimeValue t, Interval& valid = FOREVER) const;

	bool SetColor(Color value, TimeValue t);
	Color GetColor(TimeValue t, Interval& valid = FOREVER) const;

	bool SetColorMode(IESLightColorMode value, TimeValue t);
	IESLightColorMode GetColorMode(TimeValue t, Interval& valid = FOREVER) const;

	bool ProfileIsSelected(TimeValue t) const;

	// Result depends on color mode (but without intensity)
	Color GetFinalColor(TimeValue t = 0, Interval& valid = FOREVER) const;

protected:
	class CreateCallback;

	void AddTarget(TimeValue t, bool fromCreateCallback);
	void RemoveTarget(TimeValue t);
	void OnTargetedChanged(TimeValue t, bool fromCreateCallback);

	void SetThisNode(INode*);
	void SetTargetNode(INode*);
	INode* GetThisNode();
	INode* GetTargetNode();

	ExclList m_exclList;

private:
	static const Class_ID m_classId;
	static const Color FrozenColor;
	static const Color WireColor;
	static const Color SelectedColor;

	template<typename T_Id>
	void ReplaceLocalReference(T_Id id, RefTargetHandle handle)
	{
		RefResult ret = ReplaceReference(static_cast<int>(id) + BaseMaxType::NumRefs(), handle);
		FASSERT(ret == REF_SUCCEED);
	}

	// Panels
	IES_General m_general;
	IES_Intensity m_intensity;
	IES_Shadows m_shadows;
	IES_Volume m_volume;

	IObjParam* m_iObjParam;

	std::vector<std::vector<Point3> > m_plines;
	Point3 m_defaultUp; // up vector of IES light
	std::vector<Point3> m_bbox; // need all 8 points to support proper transformation
	bool m_invlidProfileMessageShown;
	bool m_BBoxCalculated;
	std::vector<std::vector<Point3> > m_preview_plines;
	bool m_isPreviewGraph; // photometric web should be rotated differently when look target is not created yet (during preview) compared to when look at target is reomved/disabled
};

#undef IES_DELCARE_PARAM_SET
#undef IES_DELCARE_PARAM_GET
#undef IES_DECLARE_PARAM

FIRERENDER_NAMESPACE_END
