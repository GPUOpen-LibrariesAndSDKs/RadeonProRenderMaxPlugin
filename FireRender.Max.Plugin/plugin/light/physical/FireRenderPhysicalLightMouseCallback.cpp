/*********************************************************************************************************************************
* Radeon ProRender for 3ds Max plugin
* Copyright (c) 2017 AMD
* All Rights Reserved
*
* Custom IES light 3ds MAX scene node. This custom node allows the definition of IES light, using rectangular shapes
* that can be positioned and oriented in the MAX scene.
*********************************************************************************************************************************/

#include "FireRenderPhysicalLight.h"
#include "maxscript\mxsplugin\mxsPlugin.h"
#include <decomp.h>
#include "Common.h"

FIRERENDER_NAMESPACE_BEGIN

FireRenderPhysicalLight::CreateCallBack::CreateCallBack()
	: m_light(nullptr)
	, m_pBlock(nullptr)
	, state(BEGIN)
{}

const float defaultCylinderHeight = 3.0f;

const Point3 defaultSecondPos(0.0f, 10.0f, 0.0f);

bool FireRenderPhysicalLight::CreateCallBack::IntersectMouseRayWithPlane(Point3& intersectionPoint, ViewExp *vpt, const IPoint2& m)
{
	Ray intersectRay;
	vpt->MapScreenToWorldRay(m.x, m.y, intersectRay);
	Plane projPlane(intersectRay.dir, Point3(m_light->GetTargetPoint(GetCOREInterface()->GetTime())));
	return projPlane.Intersect(intersectRay, intersectionPoint);
}

// mat - is the matrix of the node pivot!
// change it if you want to change node pivot!
void FireRenderPhysicalLight::CreateCallBack::ProcessMouseClicked(ViewExp *vpt, IPoint2& m, Matrix3& mat)
{
	TimeValue time = GetCOREInterface()->GetTime();
	Interval valid = FOREVER;

	// initial setup
	if (BEGIN == state)
	{
		// first click (mouse button pressed)
		INode* thisNode = FindNodeRef(m_light);
		FASSERT(thisNode);
		m_light->SetThisNode(thisNode);
		mat.SetTrans(vpt->SnapPoint(m, m, NULL, SNAP_IN_3D));

		// get light mode
		FRPhysicalLight_LightType lightTypeMode = m_light->GetLightType(time);

		switch (lightTypeMode)
		{
			case FRPhysicalLight_AREA:
			{
				state = BEGIN_AREA;
				break;
			}
			case FRPhysicalLight_SPOT:
			{
				state = BEGIN_SPOT;
				break;
			}
			case FRPhysicalLight_POINT:
			{
				state = BEGIN_POINT;
				break;
			}
			case FRPhysicalLight_DIRECTIONAL:
			{
				state = BEGIN_DIRECT;
				break;
			}
		}
	}

	// process FSM
	switch (state)
	{
		case BEGIN: // initial creation state
		{
			// should not be here!
			FASSERT(false);
			break;
		}

		case BEGIN_POINT:
		{
			// continue first click processing (mouse button pressed)
			Point3 p = vpt->SnapPoint(m, m, NULL, SNAP_IN_3D);

			m_light->SetLightPoint(p, time);
			m_light->SetSecondPoint(p + defaultSecondPos, time); // need initialize second point with some value not eqaul to first point in case point light will be changed to other type of light
			m_light->SetThirdPoint(p + Point3(0.0f, 0.0f, defaultCylinderHeight), time);
			m_light->SetTargetPoint(p, time);

			state = FIN;
			m_light->FinishPreview();
			break;
		}

		case BEGIN_SPOT: // begin create spot light
		{
			// continue first click processing (mouse button pressed)
			Point3 p = vpt->SnapPoint(m, m, NULL, SNAP_IN_3D);

			m_light->SetLightPoint(p, time);
			m_light->SetSecondPoint(p, time);
			m_light->SetThirdPoint(p + Point3(0.0f, 0.0f, defaultCylinderHeight), time);
			m_light->SetTargetPoint(p, time);

			state = SECOND_SPOT;
			break;
		}

		case BEGIN_DIRECT:
		{
			// continue first click processing (mouse button pressed)
			Point3 p = vpt->SnapPoint(m, m, NULL, SNAP_IN_3D);

			m_light->SetLightPoint(p, time);
			m_light->SetSecondPoint(p, time);
			m_light->SetThirdPoint(p + Point3(0.0f, 0.0f, defaultCylinderHeight), time);
			m_light->SetTargetPoint(p, time);

			state = SECOND_DIRECT;
			break;
		}

		case BEGIN_AREA: // begin create area light
		{
			// continue first click processing (mouse button pressed)
			Point3 p = vpt->SnapPoint(m, m, NULL, SNAP_IN_3D);

			m_light->SetLightPoint(p, time);
			m_light->SetSecondPoint(p, time);
			m_light->SetThirdPoint(p, time);
			m_light->SetTargetPoint(p, time);

			if (FRPhysicalLight_CYLINDER != m_light->GetLightShapeMode(time))
			{
				m_light->SetThirdPoint(p + Point3(0.0f, 0.0f, defaultCylinderHeight), time);
			}

			state = AREA_SECOND_POINT;
			break;
		}

		case AREA_SECOND_POINT: // second point for area light (used by all types of area light)
		{
			// mouse button released
			// get mode of light area (different light zones are set up by different number of clicks)
			FRPhysicalLight_AreaLight_LightShape areaLightShapeMode = m_light->GetLightShapeMode(time);

			if ((FRPhysicalLight_DISC == areaLightShapeMode) || (FRPhysicalLight_SPHERE == areaLightShapeMode))
			{
				// move light root so that center of disk becomes center of light
				Point3 diskCenter = m_light->GetLightPoint(time) + (m_light->GetSecondPoint(time) - m_light->GetLightPoint(time)) / 2;
				mat.SetTrans(diskCenter);

				// set ui
				float diameter = (m_light->GetSecondPoint(time) - m_light->GetLightPoint(time)).Length();
				m_light->SetLength(diameter, time);
				m_light->SetWidth(diameter, time);
			}

			if (FRPhysicalLight_RECTANGLE == areaLightShapeMode)
			{
				// move light root so that center of rectangle becomes center of light
				Point3 rectCenter = m_light->GetLightPoint(time) + (m_light->GetSecondPoint(time) - m_light->GetLightPoint(time)) / 2;
				mat.SetTrans(rectCenter);

				// set ui
				float length = abs(m_light->GetSecondPoint(time).x - m_light->GetLightPoint(time).x);
				m_light->SetLength(length, time);
				float width = abs(m_light->GetSecondPoint(time).y - m_light->GetLightPoint(time).y);
				m_light->SetWidth(width, time);

				m_light->FinishPreview();
			}

			if (FRPhysicalLight_CYLINDER == areaLightShapeMode)
			{
				// set ui
				float diameter = (m_light->GetSecondPoint(time) - m_light->GetLightPoint(time)).Length();
				m_light->SetLength(diameter, time);
				m_light->SetWidth(diameter, time);

				// set initial third point
				Point3 intersectionPoint(0.0f, 0.0f, 0.0f);
				bool gotIntersection = IntersectMouseRayWithPlane(intersectionPoint, vpt, m);
				FASSERT(gotIntersection);

				m_light->SetThirdPoint(intersectionPoint, time);

				state = AREA_THIRD_POINT;

				break;
			}

			if (m_light->IsTargeted())
			{
				state = AREA_TARGET_POINT;

				Point3 intersectionPoint(0.0f, 0.0f, 0.0f);
				bool gotIntersection = IntersectMouseRayWithPlane(intersectionPoint, vpt, m);
				FASSERT(gotIntersection);

				m_light->SetTargetPoint(intersectionPoint, time);
				m_light->BeginTargetPreview(); // to simplify display cases: light source gizmo is already created
			}
			else
			{
				state = FIN;
				m_light->FinishPreview();
			}

			break;
		}

		case SECOND_DIRECT:
		case SECOND_SPOT: // mouse button released
		{
			if (!m_light->IsTargeted())
			{
				Point3 lookDir = m_light->GetSecondPoint(time) - m_light->GetLightPoint(time);
				lookDir = lookDir.Normalize();
				Matrix3 rot1 = RotAngleAxisMatrix(Point3(0.0f, 1.0f, 0.0f), -PI/2);
				float dotProduct = DotProd(Point3(1.0f, 0.0f, 0.0f), lookDir);
				float rotAngle = acos(dotProduct);
				Point3 normal = (Point3(1.0f, 0.0f, 0.0f) ^ lookDir).Normalize(); // cross product, axis of rotation
				Matrix3 rot2 = RotAngleAxisMatrix(normal, rotAngle);
				mat = rot1 * rot2;
				mat.SetTrans(m_light->GetLightPoint(time));

				m_light->BeginTargetPreview();
			}
			// no break intentionally!
		}

		case AREA_THIRD_POINT: // optional for some area lights
							   // second click (mouse button pressed)
		{		
			if ((FRPhysicalLight_SPOT != m_light->GetLightType(time)) && (FRPhysicalLight_DIRECTIONAL != m_light->GetLightType(time)) )
			{
				// move light root so that center of disk becomes center of light
				Point3 diskCenter = m_light->GetLightPoint(time) + (m_light->GetSecondPoint(time) - m_light->GetLightPoint(time)) / 2;
				mat.SetTrans(diskCenter);
			}

			// proceed with targeted / non-targeted
			if (m_light->IsTargeted())
			{
				state = AREA_TARGET_POINT;

				Point3 intersectionPoint(0.0f, 0.0f, 0.0f);
				bool gotIntersection = IntersectMouseRayWithPlane(intersectionPoint, vpt, m);
				FASSERT(gotIntersection);

				m_light->SetTargetPoint(intersectionPoint, time);
				m_light->BeginTargetPreview();
			}
			else
			{
				state = FIN;
				m_light->FinishPreview();
			}

			break;
		}

		case AREA_TARGET_POINT: // optional if area light is targeted
		{
			// final click (mouse button pressed)
			// that is second or third click (depending on tyoe of the light)
			m_light->AddTarget(time, true);

			float distToTarg = m_light->GetTargetDistance(time);
			m_light->SetDist(distToTarg, time);

			state = FIN;
			m_light->FinishPreview();
			m_light->FinishTargetPreview();

			break;
		}
	}
}

void FireRenderPhysicalLight::CreateCallBack::ProcessMouseMove(ViewExp *vpt, IPoint2& m, Matrix3& mat)
{
	TimeValue time = GetCOREInterface()->GetTime();
	Interval valid = FOREVER;

	// process FSM
	switch (state)
	{
		case AREA_SECOND_POINT:
		case SECOND_SPOT:
		case SECOND_DIRECT:
		{
			Point3 p = vpt->SnapPoint(m, m, NULL, SNAP_IN_3D);
			m_light->SetSecondPoint(p, time);

			break;
		}

		case AREA_THIRD_POINT:
		{
			Point3 intersectionPoint(0.0f, 0.0f, 0.0f);
			bool gotIntersection = IntersectMouseRayWithPlane(intersectionPoint, vpt, m);
			FASSERT(gotIntersection);

			m_light->SetThirdPoint(intersectionPoint, time);

			break;
		}

		case AREA_TARGET_POINT:
		{
			Point3 intersectionPoint(0.0f, 0.0f, 0.0f);
			bool gotIntersection = IntersectMouseRayWithPlane(intersectionPoint, vpt, m);
			FASSERT(gotIntersection);

			m_light->SetTargetPoint(intersectionPoint, time);

			break;
		}
	}
}

int FireRenderPhysicalLight::CreateCallBack::proc(ViewExp *vpt, int msg, int point, int flags, IPoint2 m, Matrix3& mat)
{
	int result = CREATE_CONTINUE;

	switch (msg)
	{
		case MOUSE_POINT:
		{
			ProcessMouseClicked(vpt, m, mat); // all click handle logic is done here
			if (FIN == state)
				result = CREATE_STOP;

			break;
		}

		case MOUSE_MOVE:
		{
			ProcessMouseMove(vpt, m, mat); // fsm state is NOT changed here

			break;
		}

		case MOUSE_ABORT:
		{
			result = CREATE_ABORT;
			break;
		}

		case MOUSE_FREEMOVE:
		{
			vpt->SnapPreview(m, m, NULL, SNAP_IN_3D);
			break;
		}
	}

	return result;
}

void FireRenderPhysicalLight::CreateCallBack::SetLight(FireRenderPhysicalLight* light)
{
	FASSERT(light);
	m_light = light;
	m_pBlock = light->GetParamBlock(0);
	FASSERT(m_pBlock);
	state = BEGIN;
}

FIRERENDER_NAMESPACE_END
