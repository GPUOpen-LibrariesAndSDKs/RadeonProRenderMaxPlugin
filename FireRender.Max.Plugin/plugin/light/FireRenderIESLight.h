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
#include "INodeTransformMonitor.h"

class LookAtTarget;

FIRERENDER_NAMESPACE_BEGIN

#define IES_DELCARE_PARAM_SET($paramName, $paramType) bool Set##$paramName($paramType value, TimeValue t)
#define IES_DECLARE_PARAM_GET($paramName, $paramType) $paramType Get##$paramName(TimeValue t, Interval& valid = FOREVER) const
#define IES_DECLARE_PARAM($paramName, $paramType)  \
	IES_DELCARE_PARAM_SET($paramName, $paramType); \
	IES_DECLARE_PARAM_GET($paramName, $paramType)

class FireRenderIESLight :
	public FireRenderLight
{
public:
	enum class StrongReference
	{
		ParamBlock = 0,

		// This should be always last
		__last
	};

	enum class IndirectReference
	{
		ThisNode = static_cast<std::underlying_type_t<StrongReference>>(StrongReference::__last),
		TargetNode,

		// This should be always last
		__last
	};

	using IntensitySettings = MaxSpinner::DefaultFloatSettings;
	using AreaWidthSettings = MaxSpinner::DefaultFloatSettings;
	using LightRotateSettings = MaxSpinner::DefaultRotationSettings;
	using ShadowsSoftnessSettings = MaxSpinner::DefaultFloatSettings;
	using ShadowsTransparencySettings = MaxSpinner::DefaultFloatSettings;
	using VolumeScaleSettings = MaxSpinner::DefaultFloatSettings;
	using TargetDistanceSettings = MaxSpinner::DefaultFloatSettings;

	static Class_ID GetClassId();
	static ClassDesc2* GetClassDesc();

	static constexpr bool DefaultEnabled = true;
	static constexpr bool DefaultTargeted = true;
	static constexpr bool DefaultShadowsEnabled = true;
	static constexpr auto DefaultColorMode = IES_LIGHT_COLOR_MODE_COLOR;

	static constexpr bool EnableGeneralPanel = true;
	static constexpr bool EnableIntensityPanel = true;
	static constexpr bool EnableShadowsPanel = false;
	static constexpr bool EnableVolumePanel = false;

	static constexpr auto SphereCirclePointsCount = 28u;

	FireRenderIESLight();
	~FireRenderIESLight();

	CreateMouseCallBack* GetCreateMouseCallBack() override;
	ObjectState Eval(TimeValue time) override;
	void InitNodeName(TSTR& s) override;
	const MCHAR *GetObjectName() override;
	Interval ObjectValidity(TimeValue t) override;

	BOOL UsesWireColor() override;
	int DoOwnSelectHilite() override;
	void NotifyChanged();
	RefResult NotifyRefChanged(const Interval& interval, RefTargetHandle hTarget, PartID& partId, RefMessage msg, BOOL propagate) override;

	void DeleteThis() override;
	Class_ID ClassID() override;
	void GetClassName(TSTR& s) override;
	RefTargetHandle Clone(RemapDir& remap) override;

	//********************** can move this to IFace?
	IParamBlock2* GetParamBlock(int i) override; // override??? function not in the base class
	const IParamBlock2* GetParamBlock(int i) const;

	IParamBlock2* GetParamBlockByID(BlockID id) override;
	int NumRefs() override;
	void SetReference(int i, RefTargetHandle rtarg) override;
	RefTargetHandle GetReference(int i) override;

	void DrawSphere(TimeValue t, ViewExp *vpt, BOOL sel = FALSE, BOOL frozen = FALSE);
	bool DrawWeb(TimeValue t, ViewExp *pVprt, bool isSelected = false, bool isFrozen = false);
	//***************************************************************************************
	Color GetViewportMainColor(INode* pNode);
	Color GetViewportColor(INode* pNode, Color selectedColor);
	//***************************************************************************************
	int Display(TimeValue t, INode* inode, ViewExp *vpt, int flags) override;
	void GetWorldBoundBox(TimeValue t, INode* inode, ViewExp* vpt, Box3& box) override;
	int HitTest(TimeValue t, INode* inode, int type, int crossing, int flags, IPoint2 *p, ViewExp *vpt) override;
	void GetLocalBoundBox(TimeValue t, INode* inode, ViewExp* vpt, Box3& box) override;
	RefResult EvalLightState(TimeValue t, Interval& valid, LightState* cs) override;

	void BeginEditParams(IObjParam *objParam, ULONG flags, Animatable *prev) override;
	void EndEditParams(IObjParam *objParam, ULONG flags, Animatable *next) override;

	void CreateSceneLight(const ParsedNode& node, frw::Scope scope, const RenderParameters& params) override;
	bool DisplayLight(TimeValue t, INode* inode, ViewExp *vpt, int flags) override;
	bool CalculateLightRepresentation(const TCHAR* profileName) override;
	bool CalculateBBox(void) override;

	IES_DECLARE_PARAM(LightPoint, Point3);
	IES_DECLARE_PARAM(TargetPoint, Point3);
	IES_DECLARE_PARAM(Enabled, bool);
	IES_DECLARE_PARAM(Targeted, bool);
	IES_DECLARE_PARAM(TargetDistance, float);
	IES_DECLARE_PARAM(AreaWidth, float);
	IES_DECLARE_PARAM(RotationX, float);
	IES_DECLARE_PARAM(RotationY, float);
	IES_DECLARE_PARAM(RotationZ, float);
	IES_DECLARE_PARAM(ActiveProfile, const TCHAR*);
	IES_DECLARE_PARAM(Intensity, float);
	IES_DECLARE_PARAM(Temperature, float);
	IES_DECLARE_PARAM(Color, Color);
	IES_DECLARE_PARAM(ColorMode, IESLightColorMode);
	IES_DECLARE_PARAM(ShadowsEnabled, bool);
	IES_DECLARE_PARAM(ShadowsSoftness, float);
	IES_DECLARE_PARAM(ShadowsTransparency, float);
	IES_DECLARE_PARAM(VolumeScale, float);

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

	template<typename T_Id>
	void ReplaceLocalReference(T_Id id, RefTargetHandle handle)
	{
		auto ret = ReplaceReference(static_cast<int>(id) + BaseMaxType::NumRefs(), handle);
		FASSERT(ret == REF_SUCCEED);
	}

	// Panels
	IES_General m_general;
	IES_Intensity m_intensity;
	IES_Shadows m_shadows;
	IES_Volume m_volume;

	IObjParam* m_iObjParam;

	std::vector<std::vector<Point3> > m_plines;
	Point3 m_prevUp; // up vector of IES light
	std::vector<Point3> m_bbox; // need all 8 points to support proper transformation
	bool m_BBoxCalculated;
	std::vector<std::vector<Point3> > m_preview_plines;

	// References
	IParamBlock2* m_pblock2;
	ReferenceTarget* m_thisNodeMonitor;
	ReferenceTarget* m_targNodeMonitor;
};

#undef IES_DELCARE_PARAM_SET
#undef IES_DELCARE_PARAM_GET
#undef IES_DECLARE_PARAM

FIRERENDER_NAMESPACE_END
