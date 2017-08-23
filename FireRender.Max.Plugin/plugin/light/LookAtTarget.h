#pragma once

#include "Common.h"
#include <object.h>

class LookAtTarget : public GeomObject
{
	class CreateCallBack;

	void GetMat(TimeValue t, INode* inode, ViewExp& vpt, Matrix3& tm);

	//  inherited virtual methods for Reference-management
	RefResult NotifyRefChanged(const Interval& changeInt, RefTargetHandle hTarget,
		PartID& partID, RefMessage message, BOOL propagate);

public:
	LookAtTarget();
	virtual ~LookAtTarget();

	// From BaseObject
	int HitTest(TimeValue t, INode* inode, int type, int crossing, int flags, IPoint2 *p, ViewExp *vpt) override;
	void Snap(TimeValue t, INode* inode, SnapInfo *snap, IPoint2 *p, ViewExp *vpt) override;
	int Display(TimeValue t, INode* inode, ViewExp *vpt, int flags) override;
	CreateMouseCallBack* GetCreateMouseCallBack() override;
	void BeginEditParams(IObjParam *ip, ULONG flags, Animatable *prev) override;
	void EndEditParams(IObjParam *ip, ULONG flags, Animatable *next) override;
	const TCHAR *GetObjectName() override;

	// From Object
	ObjectState Eval(TimeValue time) override;
	void InitNodeName(TSTR& s) override;
	int UsesWireColor() override;
	int IsRenderable() override;

	// From GeomObject
	int IntersectRay(TimeValue t, Ray& r, float& at);
	void GetWorldBoundBox(TimeValue t, INode *mat, ViewExp *vpt, Box3& box) override;
	void GetLocalBoundBox(TimeValue t, INode *mat, ViewExp *vpt, Box3& box) override;
	void GetDeformBBox(TimeValue t, Box3& box, Matrix3 *tm, BOOL useSel) override;

	// From BaseObject
	BOOL HasViewDependentBoundingBox() override;

	// From Animatable 
	void DeleteThis() override;
	Class_ID ClassID();
	void GetClassName(TSTR& s);
	int IsKeyable();
	LRESULT CALLBACK TrackViewWinProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

	// From ref.h
	RefTargetHandle Clone(RemapDir& remap) override;

	// IO
	IOResult Save(ISave *isave) override;
	IOResult Load(ILoad *iload) override;

	void AddRenderitem(
		const MaxSDK::Graphics::UpdateDisplayContext& updateDisplayContext,
		MaxSDK::Graphics::UpdateNodeContext& nodeContext,
		MaxSDK::Graphics::IRenderItemContainer& targetRenderItemContainer);

	virtual unsigned long GetObjectDisplayRequirement() const;

	virtual bool PrepareDisplay(
		const MaxSDK::Graphics::UpdateDisplayContext& prepareDisplayContext);

	virtual bool UpdatePerNodeItems(
		const MaxSDK::Graphics::UpdateDisplayContext& updateDisplayContext,
		MaxSDK::Graphics::UpdateNodeContext& nodeContext,
		MaxSDK::Graphics::IRenderItemContainer& targetRenderItemContainer);

};
