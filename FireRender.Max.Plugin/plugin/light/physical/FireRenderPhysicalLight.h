#pragma once

#include "IFireRenderLight.h"
#include "object.h"
#include "frScope.h"
#include <functional>
#include "FireRenderLight.h"

class LookAtTarget;

FIRERENDER_NAMESPACE_BEGIN

enum FRPhysicalLight_TexmapId 
{
	FRPhysicalLight_MAP_INTENSITY_COLOUR = 0
};

enum FRPhysicalLight_ParamID : ParamID
{
// General
	FRPhysicalLight_ISENABLED = 1000,
	FRPhysicalLight_LIGHT_TYPE,
	FRPhysicalLight_AREA_LENGTHS,
	FRPhysicalLight_AREA_WIDTHS,
	FRPhysicalLight_IS_TARGETED,
	FRPhysicalLight_TARGET_DIST,

	//- internal (and invisible)
	FRPhysicalLight_LIGHT_POINT,
	FRPhysicalLight_SECOND_POINT,
	FRPhysicalLight_THIRD_POINT, // used for cylinder
	FRPhysicalLight_TARGET_POINT,

// Intensity
	FRPhysicalLight_LIGHT_INTENSITY,
	FRPhysicalLight_INTENSITY_UNITS,
	FRPhysicalLight_LUMINOUS_EFFICACY,
	FRPhysicalLight_COLOUR_MODE,
	FRPhysicalLight_INTENSITY_COLOUR,
	FRPhysicalLight_INTENSITY_COLOUR_MAP,
	FRPhysicalLight_INTENSITY_COLOUR_USEMAP,
	FRPhysicalLight_INTENSITY_TEMPERATURE,
	FRPhysicalLight_INTENSITY_TEMPERATURE_COLOUR,

// Area Light
	FRPhysicalLight_AREALIGHT_ISVISIBLE,
	FRPhysicalLight_AREALIGHT_ISBIDIRECTIONAL,
	FRPhysicalLight_AREALIGHT_LIGHTSHAPE,
	FRPhysicalLight_AREALIGHT_LIGHTMESH,
	FRPhysicalLight_AREALIGHT_ISINTENSITYNORMALIZATION,

// Spot Light
	FRPhysicalLight_SPOTLIGHT_ISVISIBLE,
	FRPhysicalLight_SPOTLIGHT_INNERCONE,
	FRPhysicalLight_SPOTLIGHT_OUTERCONE,

// Light Decay
	FRPhysicalLight_LIGHTDECAY_TYPE,
	FRPhysicalLight_LIGHTDECAY_FALLOFF_START,
	FRPhysicalLight_LIGHTDECAY_FALLOFF_END,

// Shadows
	FRPhysicalLight_SHADOWS_ISENABLED,
	FRPhysicalLight_SHADOWS_SOFTNESS,
	FRPhysicalLight_SHADOWS_TRANSPARENCY,

// Volume
	FRPhysicalLight_VOLUME_SCALE,
};

enum FRPhysicalLight_LightType
{
	FRPhysicalLight_AREA,
	FRPhysicalLight_SPOT,
	FRPhysicalLight_POINT,
	FRPhysicalLight_DIRECTIONAL,
};

enum FRPhysicalLight_IntensityUnits
{
	FRPhysicalLight_LUMINANCE,
	FRPhysicalLight_WATTS,
	FRPhysicalLight_RADIANCE,
	FRPhysicalLight_LUMEN,
};

enum FRPhysicalLight_ColourMode
{
	FRPhysicalLight_COLOUR,
	FRPhysicalLight_TEMPERATURE,
};

enum FRPhysicalLight_AreaLight_LightShape
{
	FRPhysicalLight_DISC,
	FRPhysicalLight_CYLINDER,
	FRPhysicalLight_SPHERE,
	FRPhysicalLight_RECTANGLE,
	FRPhysicalLight_MESH,
};

enum FRPhysicalLight_LightDecayType
{
	FRPhysicalLight_NONE,
	FRPhysicalLight_INVERSESQUARE,
	FRPhysicalLight_LINEAR,
};

INode* FindNodeRef(ReferenceTarget *rt);

INode* GetNodeRef(ReferenceMaker *rm);

ClassDesc2* GetFireRenderPhysicalLightDesc(void);

const Class_ID FIRERENDER_PHYSICALLIGHT_CLASS_ID(0x64275e8c, 0x79ac5651);

class FireRenderPhysicalLight :
	public FireRenderLight
{
protected:
	static std::map<int, std::pair<ParamID, MCHAR*>> TEXMAP_MAPPING;

	class CreateCallBack : public CreateMouseCallBack
	{
	private:
		IParamBlock2* m_pBlock;
		FireRenderPhysicalLight* m_light;

		enum PhysicalLightCreationState // light creation FSM
		{
			BEGIN,
			BEGIN_AREA,
			AREA_SECOND_POINT,
			AREA_THIRD_POINT,
			AREA_TARGET_POINT,
			BEGIN_SPOT,
			SECOND_SPOT,
			BEGIN_POINT,
			BEGIN_DIRECT,
			SECOND_DIRECT,
			FIN,
		} state;

		void ProcessMouseClicked(ViewExp *vpt, IPoint2& m, Matrix3& mat);
		void ProcessMouseMove(ViewExp *vpt, IPoint2& m, Matrix3& mat);
		bool IntersectMouseRayWithPlane(Point3& intersectionPoint, ViewExp *vpt, const IPoint2& m);

	public:
		CreateCallBack(void);

		int proc(ViewExp *vpt, int msg, int point, int flags, IPoint2 m, Matrix3& mat) override;		

		void SetLight(FireRenderPhysicalLight* light);
	};

	void AddTarget(TimeValue t, bool fromCreateCallback);
	void RemoveTarget(TimeValue t);
	bool IsEnabled(void) const;

	float GetIntensity(void) const;
	float GetLightSourceArea(void) const;
	Color GetLightColour(void) const;

public:
	FireRenderPhysicalLight(void);
	~FireRenderPhysicalLight(void);

	virtual bool DisplayLight(TimeValue t, INode* inode, ViewExp *vpt, int flags);
	virtual bool CalculateBBox(void);

	INode* GetThisNode(void);
	bool IsTargeted(void) const;
	INode* GetTargetNode(void);
	const INode* GetThisNode(void) const;
	const INode* GetTargetNode(void) const;

	void SetThisNode(INode*);
	void SetTargetNode(INode*);

	void SetLightPoint(const Point3& p, const TimeValue& time);
	const Point3 GetLightPoint(const TimeValue& time) const;
	void SetTargetPoint(const Point3& p, const TimeValue& time);
	const Point3 GetTargetPoint(const TimeValue& time) const;
	void SetSecondPoint(const Point3& p, const TimeValue& time);
	const Point3 GetSecondPoint(const TimeValue& time) const;
	void SetThirdPoint(const Point3& p, const TimeValue& time);
	const Point3 GetThirdPoint(const TimeValue& time) const;
	void SetLength(float length, const TimeValue& time);
	float GetLength(const TimeValue& time) const;
	void SetWidth(float width, const TimeValue& time);
	float GetWidth(const TimeValue& time) const;
	void SetDist(float dist, const TimeValue& time);
	float GetDist(const TimeValue& time) const;

	bool SetTargetDistance(float value, TimeValue t);
	float GetTargetDistance(TimeValue t) const;

	FRPhysicalLight_AreaLight_LightShape GetLightShapeMode(const TimeValue& time) const;
	FRPhysicalLight_LightType GetLightType(const TimeValue& time) const;

	void FinishPreview(void);
	void BeginTargetPreview(void);
	void FinishTargetPreview(void);

	CreateMouseCallBack* GetCreateMouseCallBack(void) override;
	void InitNodeName(TSTR& s) override;
	const MCHAR *GetObjectName(void) override;
	Interval ObjectValidity(TimeValue t) override;

	RefResult NotifyRefChanged(NOTIFY_REF_CHANGED_PARAMETERS) override;

	void DeleteThis(void) override;
	Class_ID ClassID(void) override;
	void GetClassName(TSTR& s) override;
	RefTargetHandle Clone(RemapDir& remap) override;

	IParamBlock2* GetParamBlock(int i) override;

	IParamBlock2* GetParamBlockByID(BlockID id) override;

	void BeginEditParams(IObjParam *objParam, ULONG flags, Animatable *prev) override;
	void EndEditParams(IObjParam *objParam, ULONG flags, Animatable *next) override;

	int Display(TimeValue t, INode* inode, ViewExp *vpt, int flags) override;
	void CreateSceneLight(const ParsedNode& node, frw::Scope scope, const RenderParameters& params) override;
	void GetWorldBoundBox(TimeValue t, INode* inode, ViewExp* vpt, Box3& box) override;
	int HitTest(TimeValue t, INode* inode, int type, int crossing, int flags, IPoint2 *p, ViewExp *vpt) override;

	static ClassDesc2* GetClassDesc();

	// From Object
	ObjectState Eval(TimeValue time) override;

	// From Light
	RefResult EvalLightState(TimeValue t, Interval& valid, LightState* cs);

	virtual void SetHSVColor(TimeValue, Point3 &);
	virtual Point3 GetHSVColor(TimeValue t, Interval &valid);

protected:
	ExclList m_exclList;

private:

	bool m_isPreview; // light representation should be displayed in a different way during preview
	bool m_isTargetPreview; // same for target

	using BaseMaxType = LightObject;
	template<typename T_Id>
	void ReplaceLocalReference(T_Id id, RefTargetHandle handle)
	{
		RefResult ret = ReplaceReference(static_cast<int>(id) + BaseMaxType::NumRefs(), handle);
		FASSERT(ret == REF_SUCCEED);
	}
};

using MsgProc = INT_PTR(*) (TimeValue t, IParamMap2* map, HWND hWnd, WPARAM wParam, LPARAM lParam);
class PhysLightGeneralParamsDlgProc : public ParamMap2UserDlgProc
{
public:
	virtual INT_PTR DlgProc(TimeValue t, IParamMap2* map, HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) override;
	virtual void DeleteThis(void) override;

private:
	static INT_PTR MsgProcInitDialog(TimeValue t, IParamMap2* map, HWND hWnd, WPARAM wParam, LPARAM lParam);
	static INT_PTR MsgProcClose(TimeValue t, IParamMap2* map, HWND hWnd, WPARAM wParam, LPARAM lParam);
	static INT_PTR MsgProcCommand(TimeValue t, IParamMap2* map, HWND hWnd, WPARAM wParam, LPARAM lParam);
};

class PhysLightIntensityDlgProc : public ParamMap2UserDlgProc
{
public:
	virtual INT_PTR DlgProc(TimeValue t, IParamMap2* map, HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) override;
	virtual void DeleteThis(void) override;

private:
	static INT_PTR MsgProcInitDialog(TimeValue t, IParamMap2* map, HWND hWnd, WPARAM wParam, LPARAM lParam);
	static INT_PTR MsgProcClose(TimeValue t, IParamMap2* map, HWND hWnd, WPARAM wParam, LPARAM lParam);
	static INT_PTR MsgProcCommand(TimeValue t, IParamMap2* map, HWND hWnd, WPARAM wParam, LPARAM lParam);
};

class PhysLightsAreaLightsDlgProc : public ParamMap2UserDlgProc
{
public:
	virtual INT_PTR DlgProc(TimeValue t, IParamMap2* map, HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) override;
	virtual void DeleteThis(void) override;

private:
	static INT_PTR MsgProcInitDialog(TimeValue t, IParamMap2* map, HWND hWnd, WPARAM wParam, LPARAM lParam);
	static INT_PTR MsgProcClose(TimeValue t, IParamMap2* map, HWND hWnd, WPARAM wParam, LPARAM lParam);
	static INT_PTR MsgProcCommand(TimeValue t, IParamMap2* map, HWND hWnd, WPARAM wParam, LPARAM lParam);
};

class PhysLightsSpotLightsDlgProc : public ParamMap2UserDlgProc
{
public:
	virtual INT_PTR DlgProc(TimeValue t, IParamMap2* map, HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) override;
	virtual void DeleteThis(void) override;

private:
	static INT_PTR MsgProcInitDialog(TimeValue t, IParamMap2* map, HWND hWnd, WPARAM wParam, LPARAM lParam);
	static INT_PTR MsgProcClose(TimeValue t, IParamMap2* map, HWND hWnd, WPARAM wParam, LPARAM lParam);
	static INT_PTR MsgProcCommand(TimeValue t, IParamMap2* map, HWND hWnd, WPARAM wParam, LPARAM lParam);
};

class PhysLightsVolumeDlgProc : public ParamMap2UserDlgProc
{
public:
	virtual INT_PTR DlgProc(TimeValue t, IParamMap2* map, HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) override;
	virtual void DeleteThis(void) override;
};

FIRERENDER_NAMESPACE_END
