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
#include <array>
#include <IViewportShadingMgr.h>
#include "Common.h"

FIRERENDER_NAMESPACE_BEGIN

// Calculates z-axis transform scaling for a node;
// transforms unit vector in local space z-axis to world space, and returns world space length
float LocalToWorldScaleZ( TimeValue t, INode* node )
{
	Matrix3 tm = node->GetNodeTM(t);
	Point3  trans =			 (Point3( 0.0f, 0.0f, 0.0f ) * tm);
	Point3  transPlusUnitZ = (Point3( 0.0f, 0.0f, 1.0f ) * tm);
	return (transPlusUnitZ - trans).Length();
}

extern void CalculateSphere(float radius,
	std::vector<Point3>& vertices,
	std::vector<Point3>& normals,
	std::vector<int>& triangles);

void DrawSphereTess(ViewExp *vpt, float rad, Point3& center)
{
	std::vector<Point3> vertices;
	std::vector<Point3> normals;
	std::vector<int> triangles;

	CalculateSphere(rad, vertices, normals, triangles);

	for (Point3& tpoint : vertices)
		tpoint = tpoint + center;

	// draw each face
	size_t triangleCount = triangles.size() / 3;
	for (int idx = 0; idx < triangleCount; ++idx)
	{
		Point3 triangle[3];
		triangle[0] = vertices[triangles[idx * 3]];
		triangle[1] = vertices[triangles[idx * 3 + 1]];
		triangle[2] = vertices[triangles[idx * 3 + 2]];
		vpt->getGW()->polyline(3, triangle, NULL, NULL, TRUE, NULL);
	}
}

void DrawSphere(ViewExp *vpt, float rad, Point3& center)
{
	const size_t pointsPerArc = 16;
	const float da = 2.0f * PI / pointsPerArc;
	using CutPoints = std::array<Point3, pointsPerArc>;
	std::array<CutPoints, 3> cuts;

	// compute points
	for (int i = 0; i < pointsPerArc; i++)
	{
		// Angle
		float a = i * da;

		// Cached values
		float sn = rad * sin(a);
		float cs = rad * cos(a);

		cuts[0][i] = Point3(sn, cs, 0.0f) + center;
		cuts[1][i] = Point3(sn, 0.0f, cs) + center;
		cuts[2][i] = Point3(0.0f, sn, cs) + center;
	}

	// draw plines
	for (size_t i = 0; i < 3; ++i)
	{
		vpt->getGW()->polyline(pointsPerArc, &cuts[i][0], nullptr, nullptr, true, nullptr);
	}
}

void DrawArrow(ViewExp *vpt, const Point3& head, const Point3& tail)
{
	Point3 arrow[2] = { head, tail };
	float length = head.z - tail.z;

	Point3 arrowhead[3] = { Point3(head.x + 2.0f, head.y, head.z - (0.25f)*length), head, Point3(head.x - 2.0f, head.y, head.z - (0.25f)*length) };
	Point3 arrowhead2[3] = { Point3(head.x, head.y + 2.0f, head.z - (0.25f)*length), head, Point3(head.x, head.y - 2.0f, head.z - (0.25f)*length) };

	vpt->getGW()->polyline(2, arrow, NULL, NULL, FALSE, NULL);
	vpt->getGW()->polyline(3, arrowhead, NULL, NULL, FALSE, NULL);
	vpt->getGW()->polyline(3, arrowhead2, NULL, NULL, FALSE, NULL);
}

void DrawDisc(ViewExp *vpt, float rad, Point3& center, Matrix3* tilt = nullptr)
{
	const size_t pointsPerArc = 16;
	const float da = 2.0f * PI / pointsPerArc;
	std::array<Point3, pointsPerArc> diskPline;

	// compute points
	for (int i = 0; i < pointsPerArc; i++)
	{
		// Angle
		float a = i * da;
		float sn = rad * sin(a);
		float cs = rad * cos(a);

		diskPline[i] = Point3(cs, sn, 0.0f) + center;
	}

	// tilt
	if (tilt != nullptr)
	{
		for (Point3& tpoint : diskPline)
			tpoint = *tilt * tpoint;
	}

	// draw pline
	vpt->getGW()->polyline(pointsPerArc, &diskPline[0], NULL, NULL, TRUE, NULL);
}

void DrawRect(ViewExp *vpt, Point3& corner1, Point3& corner2)
{
	Point3 corner3(corner1.x, corner2.y, 0.0f);
	Point3 corner4(corner2.x, corner1.y, 0.0f);

	Point3 edge[5];
	edge[0] = corner1;
	edge[1] = corner3;
	edge[2] = corner2;
	edge[3] = corner4;

	// draw border pline
	vpt->getGW()->polyline(4, edge, NULL, NULL, TRUE, NULL);

	// draw diagonals
	Point3 diag1[2] = { corner1, corner2 };
	Point3 diag2[2] = { corner3, corner4 };
	vpt->getGW()->polyline(2, diag1, NULL, NULL, FALSE, NULL);
	vpt->getGW()->polyline(2, diag2, NULL, NULL, FALSE, NULL);
}

void DrawCylinder(ViewExp *vpt, Point3& point1, float radius, Point3& secondDiskCenter)
{
	// draw disks
	DrawDisc(vpt, radius, point1);
	DrawDisc(vpt, radius, secondDiskCenter);

	// draw 4 edges
	Point3 shift(radius, 0.0f, 0.0f);
	Point3 edge1[2] = { point1 + shift, secondDiskCenter + shift };
	Point3 edge2[2] = { point1 - shift, secondDiskCenter - shift };
	shift = Point3(0.0f, radius, 0.0f);
	Point3 edge3[2] = { point1 + shift, secondDiskCenter + shift };
	Point3 edge4[2] = { point1 - shift, secondDiskCenter - shift };

	vpt->getGW()->polyline(2, edge1, NULL, NULL, FALSE, NULL);
	vpt->getGW()->polyline(2, edge2, NULL, NULL, FALSE, NULL);
	vpt->getGW()->polyline(2, edge3, NULL, NULL, FALSE, NULL);
	vpt->getGW()->polyline(2, edge4, NULL, NULL, FALSE, NULL);
}

// Draws a code oriented along the local z-axis, tip of cone at local origin
void DrawCone(ViewExp *vpt, float angle, float yonDist)
{
	Point3 centerPos(0.0f, 0.0f, 0.0f);

	// "Yon" is the far end of the cone 
	Point3 yonPos = Point3(0.0f, 0.0f, -yonDist);
	Point3 yonDir = yonPos.Normalize();

	Matrix3 tilt = RotAngleAxisMatrix(yonDir, PI / 2);

	float tanAlpha = tan(angle * PI / 180);
	float coneRad = yonDist * tanAlpha;

	DrawDisc(vpt, coneRad, yonPos, &tilt);

	Point3 points[3];
	points[0] = centerPos;

	points[1] = yonPos - Point3( coneRad, 0.0f, 0.0f );
	points[2] = yonPos + Point3( coneRad, 0.0f, 0.0f );
	vpt->getGW()->polyline(3, points, NULL, NULL, TRUE, NULL); // closed line, three points

	points[1] = yonPos;
	vpt->getGW()->polyline(2, points, NULL, NULL, FALSE, NULL); // open line, two points

	points[1] = yonPos - Point3( 0.0f, coneRad, 0.0f );
	points[2] = yonPos + Point3( 0.0f, coneRad, 0.0f );
	vpt->getGW()->polyline(3, points, NULL, NULL, TRUE, NULL); // closed line, three points
}

void DrawFaces(ViewExp *vpt, const Face* arrFaces, const Point3* verts, int countFaces)
{
	// draw each face
	for (int idx = 0; idx < countFaces; ++idx)
	{
		Point3 triangle[3];
		triangle[0] = verts[arrFaces[idx].v[0]];
		triangle[1] = verts[arrFaces[idx].v[1]];
		triangle[2] = verts[arrFaces[idx].v[2]];
		vpt->getGW()->polyline(3, triangle, NULL, NULL, TRUE, NULL);
	}
}

void DisplaySpotLight(TimeValue t, ViewExp *vpt, bool isPreview, float yonDist, IParamBlock2* pBlock)
{
	FASSERT(pBlock);

	float outerAngle = 0.0f;
	Interval valid = FOREVER;
	pBlock->GetValue(FRPhysicalLight_SPOTLIGHT_OUTERCONE, 0, outerAngle, valid);
	float innerAngle = 0.0f;
	pBlock->GetValue(FRPhysicalLight_SPOTLIGHT_INNERCONE, 0, innerAngle, valid);

	// draw cone
	if (isPreview)
	{
		DrawCone(vpt, outerAngle, yonDist);
	}
	else
	{
		DrawCone(vpt, outerAngle, yonDist);
		DrawCone(vpt, innerAngle, yonDist);
	}
}

bool DisplayAreaLight(TimeValue t, ViewExp *vpt, bool drawNormals, bool isUpright, Box3& bbox, FireRenderPhysicalLight* light)
{
	FASSERT(light);

	Point3 span = bbox.pmax - bbox.pmin;
	Point3 centerPos = (bbox.pmin + bbox.pmax)/2.0f;
	float longest = MAX(span.x,span.y);
	float diameter = span.y;
	float radius = diameter/2.0f;

	FRPhysicalLight_AreaLight_LightShape mode = light->GetLightShapeMode(t);
	switch (mode)
	{
		case FRPhysicalLight_DISC:
		{
			DrawDisc(vpt, radius, centerPos);
			if (drawNormals)
				DrawArrow(vpt, centerPos+Point3(0.0f,0.0f,-diameter), centerPos);
			break;
		}

		case FRPhysicalLight_RECTANGLE:
		{
			DrawRect(vpt, bbox.pmin, bbox.pmax);
			if (drawNormals)
				DrawArrow(vpt, centerPos+Point3(0.0f,0.0f,-longest), centerPos);
			break;
		}

		case FRPhysicalLight_CYLINDER:
		{
			Point3 firstDiskCenterPos = centerPos, secondDiskCenterPos = centerPos;
			if ( isUpright )
				 firstDiskCenterPos.z = bbox.pmin.z, secondDiskCenterPos.z = bbox.pmax.z;
			else firstDiskCenterPos.z = bbox.pmax.z, secondDiskCenterPos.z = bbox.pmin.z;

			DrawCylinder(vpt, firstDiskCenterPos, radius, secondDiskCenterPos);

			if  (drawNormals)
			{
				Point3 targ1 = firstDiskCenterPos;
				Point3 targ2 = secondDiskCenterPos;
				// draw normals
				if ( isUpright )
				{
					targ1.z -= diameter;
					targ2.z += diameter;
				}
				else
				{
					targ1.z += diameter;
					targ2.z -= diameter;
				}
				DrawArrow(vpt, targ1, firstDiskCenterPos);
				DrawArrow(vpt, targ2, secondDiskCenterPos);
			}

			break;
		}

		case FRPhysicalLight_SPHERE:
		{
			DrawSphereTess(vpt, radius, centerPos);
			break;
		}

		case FRPhysicalLight_MESH:
		{
			ReferenceTarget* pRef = nullptr;
			TimeValue time = GetCOREInterface()->GetTime();
			Interval valid = FOREVER;
			light->GetParamBlockByID(0)->GetValue(FRPhysicalLight_AREALIGHT_LIGHTMESH, time, pRef, valid);

			if (pRef == nullptr)
				return false;

			INode* pNode = static_cast<INode*>(pRef);
			if (pNode == nullptr)
			{
				return false;
			}

			Object* pObj = pNode->GetObjectRef();
			if (pObj == nullptr)
			{
				return false;
			}

			ShapeObject* pShape = static_cast<ShapeObject*>(pObj);
			FASSERT(pShape);

			Mesh* mesh = nullptr;
			BOOL needDelete = FALSE;
			FireRender::RenderParameters params;
			mesh = pShape->GetRenderMesh(t, pNode, params.view, needDelete);
			int countVerts = mesh->numVerts;
			int countFaces = mesh->numFaces;
			Point3*	verts = mesh->verts;
			Face* faces = mesh->faces;
			DrawFaces(vpt, faces, verts, countFaces);

			break;
		}

		default:
			break;
	}

	return true;
}

void DisplayTarget(TimeValue t, ViewExp *vpt, bool isPreview, Point3 centerPos, float targetDist, FireRenderPhysicalLight* light)
{
	FASSERT(light);

	if (!light->IsTargeted())
		return;

	Point3 targetPos = centerPos;
	targetPos.z += targetDist;

	// Draw target; sphere in create mode during mouse drag (preview), arrow otherwise
	if( isPreview )
	{
		Point3 points[2];
		points[0] = centerPos;
		points[1] = targetPos;
		vpt->getGW()->polyline(2, points, NULL, NULL, FALSE, NULL); // draw line from pos of light origin to target
		DrawSphere(vpt, 1.0f, targetPos);
	}
	else
	{
		DrawArrow(vpt, targetPos, centerPos );
	}
}

bool FireRenderPhysicalLight::DisplayLight(TimeValue t, INode* inode, ViewExp *vpt, int flags)
{
	bool success = true;

	// do draw
	GraphicsWindow* gw = vpt->getGW();
	DWORD rlim = gw->getRndLimits();
	gw->setRndLimits(GW_WIREFRAME|GW_EDGES_ONLY|GW_BACKCULL|(gw->getRndMode() & GW_Z_BUFFER));

	// Set the wireframe gizmo color
	// By default, white if selected, yellow if unselected, black if disabled
	if (inode->Selected())
		gw->setColor( LINE_COLOR, GetSelColor() );
	else if(!inode->IsFrozen() && !inode->Dependent())
	{
		if(IsEnabled())
		{
			Color color = GetIViewportShadingMgr()->GetLightIconColor(*inode);
			gw->setColor( LINE_COLOR, color );
		}
		else
			gw->setColor( LINE_COLOR, 0.0f, 0.0f, 0.0f );
	}
	else if(inode->IsFrozen())
	{
		gw->setColor( LINE_COLOR, GetFreezeColor() );
	}


	// Initial preview mode: During initial mouse drag, before target positioning mouse drag.
	// In this mode, extra indicator lines are drawn, but target is not drawn
	bool isInitialPreview = m_isPreview && !m_isTargetPreview;
	bool isTargetPreview = m_isPreview && m_isTargetPreview;
	bool isEditMode = !m_isPreview;
	bool isDisplayTarget = (isEditMode && HasTargetNode()) || (isTargetPreview && IsTargeted());
	// draw arrow to indicate facing direction for non-targeted lights
	bool drawNormals = (!m_isPreview) && (!isDisplayTarget);
	bool isUpright = GetAreaLightUpright(t);

	Point3 centerPos( 0.0f, 0.0f, 0.0f );

	FRPhysicalLight_LightType lightType = GetLightType(t);
	switch(lightType)
	{
		case FRPhysicalLight_SPOT:
		{
			float yonDist = GetSpotLightYon(t);
			DisplaySpotLight(t, vpt, m_isPreview, yonDist, m_pblock);

			break;
		}

		case FRPhysicalLight_POINT:
		{
			DrawSphere(vpt, 2.0f, centerPos);
			DrawSphere(vpt, 4.0f, centerPos);

			break;
		}

		case FRPhysicalLight_DIRECTIONAL:
		{
			float yonDist = GetSpotLightYon(t);
			Point3 yonPos(0.0f,0.0f,-yonDist);
			DrawArrow(vpt, yonPos, centerPos);

			break;
		}

		case FRPhysicalLight_AREA:
		{
			Box3 bbox = GetAreaLightBoundBox(t);
			centerPos = (bbox.pmax + bbox.pmin)/2.0f;
			centerPos.z = 0;

			FRPhysicalLight_AreaLight_LightShape mode = GetLightShapeMode(t);

			if( isInitialPreview )
			{
				// Initial preview mode, draw a line from first click position to current mouse drag position
				Point3 span = (GetSecondPoint(t)-GetLightPoint(t));
				Point3 halfSpan = span/2.0f;
				Point3 points[2];
				points[0] = centerPos-halfSpan;
				points[1] = centerPos+halfSpan;
				vpt->getGW()->polyline(2, points, NULL, NULL, FALSE, NULL);
				// ... And for cylinder, draw line from second click point to third one (cylinder height)
				if ( mode==FRPhysicalLight_CYLINDER )
				{
					float height = (bbox.pmax.z - bbox.pmin.z);
					points[0] = points[1];
					points[1].z += height;
					vpt->getGW()->polyline(2, points, NULL, NULL, FALSE, NULL); 
				}
			}

			success = DisplayAreaLight( t, vpt, drawNormals, isUpright, bbox, this );

			break;
		}
	}

	// Draw target: preview sphere during light creation, arrow otherwise
	if (isDisplayTarget)
	{
		float targetDist = 0.0f;
		if( isTargetPreview )
			 targetDist = GetDist(t); // Preview mode, target node doesn't exist, but GetDist() is set
		else targetDist = GetTargetDistance(t); // Regular mode, calculate distance from target node

		targetDist  = -targetDist; // z-axis points opposite direction, away from target
		targetDist /= LocalToWorldScaleZ( t, inode );

		DisplayTarget(t, vpt, m_isPreview, centerPos, targetDist, this);
	}

	gw->setRndLimits(rlim);
	return success;
}

int FireRenderPhysicalLight::Display(TimeValue t, INode* inode, ViewExp *vpt, int flags)
{
	if (!vpt || !vpt->IsAlive())
	{
		DbgAssert(!_T("Invalid viewport!"));
		return FALSE;
	}

	// save viewport transform
	Matrix3 prevtm = vpt->getGW()->getTransform();
	Matrix3 tm = prevtm;

	bool isLightDisplayed = DisplayLight(t, inode, vpt, flags);

	if (!isLightDisplayed)
	{
		// draw some default gizmo
		DrawSphere(vpt, 2.0f, Point3(0.0f, 0.0f, 0.0f));
	}

	// return viewport transform to its original state
	vpt->getGW()->setTransform(prevtm);

	return TRUE;
}

FIRERENDER_NAMESPACE_END
