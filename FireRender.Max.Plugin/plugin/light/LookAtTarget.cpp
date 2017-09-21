#include <array>

#include <Graphics/Utilities/MeshEdgeRenderItem.h>
#include <Graphics/CustomRenderItemHandle.h>
#include <Graphics/RenderNodeHandle.h>
#include <Graphics/IDisplayManager.h>

#include <mouseman.h>
#include <gfx.h>

#include "LookAtTarget.h"

namespace
{
	template<typename T, size_t N>
	constexpr size_t StaticArraySize(const T(&)[N])
	{
		return N;
	}

	class LookAtTargetObjectClassDesc : public ClassDesc
	{
	public:
		static const TCHAR* TargetClassName;
		static const TCHAR* TargetObjectName;
		static const TCHAR* TargetNodeName;

		int IsPublic() override { return 0; }
		void * Create(BOOL loading = FALSE) override { return new LookAtTarget; }
		const TCHAR * ClassName() override { return TargetClassName; }
		SClass_ID SuperClassID() override { return GEOMOBJECT_CLASS_ID; }
		Class_ID ClassID() override { return Class_ID(TARGET_CLASS_ID, 0); }
		const TCHAR* Category() override { return _T("PRIMITIVES"); }
	};

	const TCHAR* LookAtTargetObjectClassDesc::TargetClassName = _T("TARGET");
	const TCHAR* LookAtTargetObjectClassDesc::TargetObjectName = TargetClassName;
	const TCHAR* LookAtTargetObjectClassDesc::TargetNodeName = TargetClassName;

	class MeshCache
	{
	public:
		MeshCache() :
			m_meshBuilt(false)
		{}

		Mesh& GetMesh()
		{
			Build();
			return m_mesh;
		}

		Box3 GetBoundingBox(Matrix3 *tm = nullptr)
		{
			return GetMesh().getBoundingBox(tm);
		}

	private:
		enum class Shape
		{
			Pyramid,
			Cube
		};

		template<Shape shape>
		void BuildShape();

		template<>
		void BuildShape<Shape::Pyramid>()
		{
			const float height = ShapeSize;
			const float side = height;
			const float halfSide = side / 2;

			const std::array<float, 3> verts[]
			{
				{   0,   0,  0 },
				{  halfSide,  halfSide,  height },
				{ -halfSide,  halfSide,  height },
				{ -halfSide, -halfSide,  height },
				{  halfSide, -halfSide,  height }
			};

			const std::array<size_t, 3> sideFaces[]
			{
				{ 0, 2, 1 },
				{ 0, 3, 2 },
				{ 0, 4, 3 },
				{ 0, 1, 4 }
			};

			const std::array<size_t, 3> baseFaces[]
			{
				{ 3, 1, 2 },
				{ 1, 3, 4 }
			};

			const size_t vertsCount = StaticArraySize(verts);
			const size_t sideFacesCount = StaticArraySize(sideFaces);
			const size_t baseFacesCount = StaticArraySize(baseFaces);

			m_mesh.setNumVerts(vertsCount);
			m_mesh.setNumFaces(sideFacesCount + baseFacesCount);

			// Set vertices
			for (size_t i = 0; i < vertsCount; ++i)
			{
				const std::array<float, 3>& v = verts[i];
				m_mesh.setVert(i, v[0], v[1], v[2]);
			}

			size_t nextSmGroup = 0;
			size_t nextFaceIndex = 0;

			// Set side faces
			for (size_t i = 0; i < sideFacesCount; ++i)
			{
				const std::array<size_t, 3>& f = sideFaces[i];
				Face& meshFace = m_mesh.faces[nextFaceIndex++];

				meshFace.setVerts(f[0], f[1], f[2]);
				meshFace.setSmGroup(1 << (nextSmGroup++));
				meshFace.setEdgeVisFlags(1, 1, 1);
			}

			// Set base faces
			for (size_t i = 0; i < baseFacesCount; ++i)
			{
				const std::array<size_t, 3>& f = baseFaces[i];
				Face& meshFace = m_mesh.faces[nextFaceIndex++];

				meshFace.setVerts(f[0], f[1], f[2]);
				meshFace.setSmGroup(1 << nextSmGroup);
				meshFace.setEdgeVisFlags(0, 1, 1);
			}
		}

		template<>
		void BuildShape<Shape::Cube>()
		{
			const float halfEdge = ShapeSize / 2;

			const std::array<float, 3> verts[]
			{
				{ -halfEdge, -halfEdge, -halfEdge },
				{  halfEdge, -halfEdge, -halfEdge },
				{ -halfEdge,  halfEdge, -halfEdge },
				{  halfEdge,  halfEdge, -halfEdge },
				{ -halfEdge, -halfEdge,  halfEdge },
				{  halfEdge, -halfEdge,  halfEdge },
				{ -halfEdge,  halfEdge,  halfEdge },
				{  halfEdge,  halfEdge,  halfEdge },
			};

			const std::array<size_t, 3> faces[]
			{
				// XY FACES (-Z)
				{ 2, 1, 0 },
				{ 1, 2, 3 },

				// XY FACES (+Z)
				{ 5, 6, 4 },
				{ 6, 5, 7 },

				// XZ FACES (-Y)
				{ 1, 4, 0 },
				{ 4, 1, 5 },
				
				// XZ FACES (+Y)
				{ 6, 3, 2 },
				{ 3, 6, 7 },
				
				// YZ FACES (-X)
				{ 4, 2, 0 },
				{ 2, 4, 6 },
				
				// YZ FACES (+X)
				{ 3, 5, 1 },
				{ 5, 3, 7 }
			};

			const size_t vertsCount = StaticArraySize(verts);
			const size_t facesCount = StaticArraySize(faces);

			m_mesh.setNumVerts(vertsCount);
			m_mesh.setNumFaces(facesCount);

			for (size_t i = 0; i < vertsCount; ++i)
			{
				const std::array<float, 3>& v = verts[i];
				m_mesh.setVert(i, v[0], v[1], v[2]);
			}

			for (size_t i = 0; i < facesCount; ++i)
			{
				const std::array<size_t, 3>& f = faces[i];
				Face& meshFace = m_mesh.faces[i];

				meshFace.setVerts(f[0], f[1], f[2]);
				meshFace.setSmGroup(1 << (i / 2));
				meshFace.setEdgeVisFlags(0, 1, 1);
			}
		}

		static constexpr Shape DefaultMeshShape = Shape::Cube;
		static const float ShapeSize;

		void Build()
		{
			if (m_meshBuilt)
			{
				return;
			}

			BuildShape<DefaultMeshShape>();

			m_mesh.buildNormals();
			m_mesh.EnableEdgeList(1);

			m_meshBuilt = true;
		}

		Mesh m_mesh;
		bool m_meshBuilt;
	};

	const float MeshCache::ShapeSize = 5.f;

	static LookAtTargetObjectClassDesc lookAtTargetObjDesc;
	static MeshCache meshCache;
}

void LookAtTarget::BeginEditParams( IObjParam *ip, ULONG flags,Animatable *prev){}
void LookAtTarget::EndEditParams( IObjParam *ip, ULONG flags,Animatable *next ){}
const TCHAR* LookAtTarget::GetObjectName() { return LookAtTargetObjectClassDesc::TargetObjectName; }

class LookAtTarget::CreateCallBack :
	public CreateMouseCallBack
{
	LookAtTarget *ob;
public:
	int proc(ViewExp *vpt, int msg, int point, int flags, IPoint2 m, Matrix3& mat)
	{
		if (!vpt || !vpt->IsAlive())
		{
			DbgAssert(!_T("Invalid viewport!"));
			return FALSE;
		}


		switch (msg)
		{
			case MOUSE_POINT:
			case MOUSE_MOVE:
			{
				Point3 c;

				switch (point)
				{
					case 0:
						c = vpt->GetPointOnCP(m);
						mat.SetTrans(c);
						return CREATE_STOP;
				}
			}
			break;

			case MOUSE_ABORT:
				return CREATE_ABORT;
		}

		return TRUE;
	}

	void SetObj(LookAtTarget *obj) { ob = obj; }

	static CreateCallBack* GetInstance()
	{
		static CreateCallBack instance;
		return &instance;
	}
};

CreateMouseCallBack* LookAtTarget::GetCreateMouseCallBack()
{
	CreateCallBack* instance = CreateCallBack::GetInstance();
	instance->SetObj(this);
	return instance;
}

void LookAtTarget::AddRenderitem(
	const MaxSDK::Graphics::UpdateDisplayContext& updateDisplayContext,
	MaxSDK::Graphics::UpdateNodeContext& nodeContext,
	MaxSDK::Graphics::IRenderItemContainer& targetRenderItemContainer)
{}

unsigned long LookAtTarget::GetObjectDisplayRequirement() const
{
	return 1;
}

bool LookAtTarget::PrepareDisplay(const MaxSDK::Graphics::UpdateDisplayContext& displayContext)
{
	return true;
}

bool LookAtTarget::UpdatePerNodeItems(
	const MaxSDK::Graphics::UpdateDisplayContext& /*updateDisplayContext*/,
	MaxSDK::Graphics::UpdateNodeContext& /*nodeContext*/,
	MaxSDK::Graphics::IRenderItemContainer& /*targetRenderItemContainer*/)
{
	return true;
}

LookAtTarget::LookAtTarget() = default;
LookAtTarget::~LookAtTarget() = default;

void LookAtTarget::GetMatrix(TimeValue t, INode* inode, ViewExp& vpt, Matrix3& tm)
{
	if ( !vpt.IsAlive() )
	{
		tm.Zero();
		return;
	}
	
	tm = inode->GetObjectTM(t);
	tm.NoScale();
}

void LookAtTarget::GetDeformBBox(TimeValue t, Box3& box, Matrix3 *tm, BOOL useSel )
{
	box = meshCache.GetBoundingBox(tm);
}

BOOL LookAtTarget::HasViewDependentBoundingBox()
{
	return true;
}

void LookAtTarget::DeleteThis()
{
	delete this;
}

Class_ID LookAtTarget::ClassID()
{
	return Class_ID(TARGET_CLASS_ID, 0);
}

void LookAtTarget::GetClassName(TSTR& s)
{
	s = LookAtTargetObjectClassDesc::TargetClassName;
}

LRESULT CALLBACK LookAtTarget::TrackViewWinProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	return 0;
}

void LookAtTarget::GetLocalBoundBox(TimeValue t, INode* inode, ViewExp* vpt, Box3& box )
{
	if ( ! vpt || ! vpt->IsAlive() )
	{
		box.Init();
		return;
	}
	
	Matrix3 m = inode->GetObjectTM(t);
	box = meshCache.GetBoundingBox();
}

void LookAtTarget::GetWorldBoundBox(TimeValue t, INode* inode, ViewExp* vpt, Box3& box)
{
	if ( !vpt || !vpt->IsAlive() )
	{
		box.Init();
		return;
	}

	Mesh& mesh = meshCache.GetMesh();

	Matrix3 m;
	GetMatrix(t, inode, *vpt, m);

	int vertsCount = mesh.getNumVerts();
	box.Init();
	
	for (int i = 0; i < vertsCount; i++)
	{
		box += m * mesh.getVert(i);
	}
}

int LookAtTarget::HitTest(TimeValue t, INode *inode, int type, int crossing, int flags, IPoint2 *p, ViewExp *vpt)
{
	if ( ! vpt || ! vpt->IsAlive() )
	{
		FASSERT(!_T("Invalid viewport!"));
		return FALSE;
	}
	
	HitRegion hitRegion;
	MakeHitRegion(hitRegion,type,crossing,4,p);

	DWORD savedLimits;
	GraphicsWindow* graphicsWindow = vpt->getGW();
	graphicsWindow->setRndLimits(((savedLimits = graphicsWindow->getRndLimits()) | GW_PICK) & ~GW_ILLUM);

	Matrix3 m;
	GetMatrix(t,inode,*vpt,m);
	graphicsWindow->setTransform(m);

	if (meshCache.GetMesh().select(graphicsWindow, graphicsWindow->getMaterial(), &hitRegion, flags & HIT_ABORTONHIT))
	{
		return TRUE;
	}

	graphicsWindow->setRndLimits(savedLimits);

	return FALSE;

#if 0
	graphicsWindow->setHitRegion(&hitRegion);
	graphicsWindow->clearHitCode();
	graphicsWindow->fWinMarker(&pt, HOLLOW_BOX_MRKR);
	return graphicsWindow->checkHitCode();
#endif
}

void LookAtTarget::Snap(TimeValue t, INode* inode, SnapInfo *snap, IPoint2 *p, ViewExp *vpt)
{	
	if ( !vpt || ! vpt->IsAlive() )
	{
		DbgAssert(!_T("Invalid viewport!"));
		return;
	}

	// Make sure the vertex priority is active and at least as important as the best snap so far
	if (snap->vertPriority > 0 && snap->vertPriority <= snap->priority)
	{
		Matrix3 tm = inode->GetObjectTM(t);	
		GraphicsWindow *gw = vpt->getGW();	
		gw->setTransform(tm);

		Matrix3 invPlane = Inverse(snap->plane);

		Point2 fp = Point2((float) p->x, (float) p->y);
		IPoint3 screen3;
		Point2 screen2;

		Point3 thePoint(0,0,0);
		// If constrained to the plane, make sure this point is in it!
		if (snap->snapType == SNAP_2D || snap->flags & SNAP_IN_PLANE)
		{
			Point3 test = thePoint * tm * invPlane;
			if(fabs(test.z) > 0.0001)	// Is it in the plane (within reason)?
				return;
		}

		gw->wTransPoint(&thePoint,&screen3);
		screen2.x = (float)screen3.x;
		screen2.y = (float)screen3.y;
		// Are we within the snap radius?
		int len = (int)Length(screen2 - fp);

		if (len <= snap->strength)
		{
			// Is this priority better than the best so far?
			if(snap->vertPriority < snap->priority)
			{
				snap->priority = snap->vertPriority;
				snap->bestWorld = thePoint * tm;
				snap->bestScreen = screen2;
				snap->bestDist = len;
			}
			else if(len < snap->bestDist)
			{
				snap->priority = snap->vertPriority;
				snap->bestWorld = thePoint * tm;
				snap->bestScreen = screen2;
				snap->bestDist = len;
			}
		}
	}
}

int LookAtTarget::Display(TimeValue t, INode* inode, ViewExp *vpt, int flags)
{
	if (!vpt || !vpt->IsAlive())
	{
		DbgAssert(!_T("Invalid viewport!"));
		return FALSE;
	}
	
	if (MaxSDK::Graphics::IsRetainedModeEnabled() &&
		vpt != nullptr &&
		vpt->GetViewCamera() != nullptr &&
		vpt->GetViewCamera()->GetTarget() == inode)
	{
		return 0;
	}

	Matrix3 m;
	GraphicsWindow* graphicsWindow = vpt->getGW();
	GetMatrix(t,inode,*vpt,m);
	graphicsWindow->setTransform(m);

	DWORD rlim = graphicsWindow->getRndLimits();
	graphicsWindow->setRndLimits(GW_WIREFRAME|GW_EDGES_ONLY|GW_BACKCULL| (rlim&GW_Z_BUFFER) );
	
	if (inode->Selected())
	{
		graphicsWindow->setColor(LINE_COLOR, GetSelColor());
	}
	else if (!inode->IsFrozen() && !inode->Dependent() && inode->GetLookatNode())
	{
		const ObjectState& os = inode->GetLookatNode()->EvalWorldState(t);
		Object* ob = os.obj;

		if (ob != nullptr && ( ( ob->SuperClassID() == LIGHT_CLASS_ID ) || ( ob->SuperClassID() == CAMERA_CLASS_ID ) ) )
		{													
			Color color(inode->GetWireColor());
			graphicsWindow->setColor( LINE_COLOR, color );
		}
		else
			graphicsWindow->setColor( LINE_COLOR, GetUIColor(COLOR_CAMERA_OBJ)); // default target color, just use camera targ color
	}

	meshCache.GetMesh().render(graphicsWindow, graphicsWindow->getMaterial(), NULL, COMP_ALL);
	graphicsWindow->setRndLimits(rlim);
	
	return 0;
}

// This is only called if the object MAKES references to other things.
RefResult LookAtTarget::NotifyRefChanged(const Interval& changeInt, RefTargetHandle hTarget,
	PartID& partID, RefMessage message, BOOL propagate ) 
{
	switch (message)
	{
		case REFMSG_NODE_WIRECOLOR_CHANGED:
			Beep(1000, 500);
	}

	return REF_SUCCEED;
}

ObjectState LookAtTarget::Eval(TimeValue time)
{
	return ObjectState(this);
}

void LookAtTarget::InitNodeName(TSTR& s)
{
	s = LookAtTargetObjectClassDesc::TargetNodeName;
}

int LookAtTarget::UsesWireColor()
{
	return 1;
}

int LookAtTarget::IsRenderable()
{
	return 0;
}

RefTargetHandle LookAtTarget::Clone(RemapDir& remap)
{
	LookAtTarget* newObject = new LookAtTarget();
	BaseClone(this, newObject, remap);
	
	return newObject;
}

IOResult LookAtTarget::Save(ISave* isave)
{
	return IO_OK;
}

IOResult LookAtTarget::Load(ILoad* iload)
{
	return IO_OK;
}
