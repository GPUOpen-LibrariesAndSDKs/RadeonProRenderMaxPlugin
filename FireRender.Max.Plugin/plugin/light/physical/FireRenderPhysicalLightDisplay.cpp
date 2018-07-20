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
#include "Common.h"

FIRERENDER_NAMESPACE_BEGIN

namespace
{
	// Default Colours
	const Color FrozenColor = Color(0.0f, 0.0f, 1.0f);
	const Color WireColor = Color(0.0f, 1.0f, 0.0f);
	const Color SelectedColor = Color(1.0f, 0.0f, 0.0f);
}

static Color GetWireColor(bool isFrozen, bool isSelected)
{
	return isSelected ? SelectedColor : (isFrozen ? FrozenColor : WireColor);
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

void DrawConeTilted(ViewExp *vpt, float angle, Point3& center)
{
	float rx = -center.y;
	float ry = center.x;
	Point3 rdir(rx, ry, 0.0f); // rdir is vector normal to vector from zero to center of the cone
	rdir = rdir.Normalize();
	Matrix3 tilt = RotAngleAxisMatrix(rdir, PI / 2);

	float length = Point3(center).Length();

	float tanAlpha = tan(angle * PI / 180);
	float coneRad = length * tanAlpha;

	DrawDisc(vpt, coneRad, Point3(tilt * center));

	Point3 dirCone[3];
	dirCone[0] = Point3(0.0f, 0.0f, 0.0f);
	dirCone[1] = tilt * (center + rdir*coneRad);
	dirCone[2] = tilt * (center - rdir*coneRad);
	vpt->getGW()->polyline(3, dirCone, NULL, NULL, TRUE, NULL);

	Point3 dirAxis[2];
	dirAxis[0] = tilt * Point3(0.0f, 0.0f, 0.0f);
	dirAxis[1] = tilt * center;
	vpt->getGW()->polyline(2, dirAxis, NULL, NULL, FALSE, NULL);

	Matrix3 roll = RotAngleAxisMatrix(Point3(0.0f, 0.0f, -1.0f), PI / 2);
	dirCone[1] = roll * dirCone[1];
	dirCone[2] = roll * dirCone[2];
	vpt->getGW()->polyline(3, dirCone, NULL, NULL, TRUE, NULL);
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

void DisplaySpotLight(TimeValue t, ViewExp *vpt, bool isPreview, Point3 dirMesh[2], IParamBlock2* pBlock)
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
		float rx = -dirMesh[1].y;
		float ry = dirMesh[1].x;
		Point3 rdir(rx, ry, 0.0f);
		rdir = rdir.Normalize();

		float length = Point3(dirMesh[1]).Length();

		float tanAlpha = tan(outerAngle * PI / 180);
		float coneRad = length * tanAlpha;

		Matrix3 tilt = RotAngleAxisMatrix(rdir, PI / 2);
		tilt.SetTrans(dirMesh[1]);
		DrawDisc(vpt, coneRad, Point3(0.0f, 0.0f, 0.0f), &tilt);

		Point3 dirCone[3];
		dirCone[0] = Point3(0.0f, 0.0f, 0.0f);
		dirCone[1] = (dirMesh[1] + rdir*coneRad);
		dirCone[2] = (dirMesh[1] - rdir*coneRad);
		vpt->getGW()->polyline(3, dirCone, NULL, NULL, TRUE, NULL);

		Point3 dirAxis[2];
		dirAxis[0] = Point3(0.0f, 0.0f, 0.0f);
		dirAxis[1] = dirMesh[1];
		vpt->getGW()->polyline(2, dirAxis, NULL, NULL, FALSE, NULL);

		Matrix3 roll = RotAngleAxisMatrix(dirMesh[1].Normalize(), PI / 2);
		dirCone[1] = roll * dirCone[1];
		dirCone[2] = roll * dirCone[2];
		vpt->getGW()->polyline(3, dirCone, NULL, NULL, TRUE, NULL);
	}
	else
	{
		DrawConeTilted(vpt, outerAngle, dirMesh[1]);
		DrawConeTilted(vpt, innerAngle, dirMesh[1]);
	}
}

bool DisplayAreaLight(TimeValue t, ViewExp *vpt, bool isPreview, bool isTargetPreview, Point3 dirMesh[2], FireRenderPhysicalLight* light)
{
	FASSERT(light);

	FRPhysicalLight_AreaLight_LightShape mode = light->GetLightShapeMode(t);
	switch (mode)
	{
		case FRPhysicalLight_DISC:
		{
			float radius = (dirMesh[1] - dirMesh[0]).Length() / 2;
			if (isPreview && !isTargetPreview)
			{
				Point3 diskCenter = dirMesh[0] + (dirMesh[1] - dirMesh[0]) / 2;
				DrawDisc(vpt, radius, diskCenter);
			}
			else
			{
				DrawDisc(vpt, radius, Point3(0.0f, 0.0f, 0.0f));

				// draw arrow
				if (!light->IsTargeted())
				{
					const Point3 targ(0.0f, 0.0f, -radius * 2);
					DrawArrow(vpt, targ, Point3(0.0f, 0.0f, 0.0f));
				}
			}

			break;
		}

		case FRPhysicalLight_RECTANGLE:
		{
			if (isPreview && !isTargetPreview)
			{
				DrawRect(vpt, dirMesh[0], dirMesh[1]);
			}
			else
			{
				// do the shift 
				Point3 rectCenter = dirMesh[0] + (dirMesh[1] - dirMesh[0]) / 2;
				DrawRect(vpt, dirMesh[0] - rectCenter, dirMesh[1] - rectCenter);

				// draw arrow
				if (!light->IsTargeted())
				{
					const float dist = (dirMesh[1] - dirMesh[0]).Length();
					const Point3 targ(0.0f, 0.0f, -dist);
					DrawArrow(vpt, targ, Point3(0.0f, 0.0f, 0.0f));
				}
			}
			break;
		}

		case FRPhysicalLight_CYLINDER:
		{
			float radius = (dirMesh[1] - dirMesh[0]).Length() / 2;
			Point3 cylinderCenter = Point3(0.0f, 0.0f, 0.0f);
			if (isPreview && !isTargetPreview)
			{
				cylinderCenter = dirMesh[0] + (dirMesh[1] - dirMesh[0]) / 2;
			}

			Point3 secondDiskCenter(cylinderCenter);
			secondDiskCenter.z = light->GetThirdPoint(t).z;

			if (!light->IsTargeted() || (!light->GetTargetNode()))
			{
				DrawCylinder(vpt, cylinderCenter, radius, secondDiskCenter);
			}
			else
			{
				if (light->GetTargetPoint(t).z < light->GetThirdPoint(t).z)
				{
					secondDiskCenter = -secondDiskCenter;
				}

				DrawCylinder(vpt, cylinderCenter, radius, -secondDiskCenter);
			}

			if ((isPreview))
			{
				Point3 dirMesh2[2];
				dirMesh2[0] = dirMesh[1];
				dirMesh2[1] = dirMesh[1];
				dirMesh2[1].z = light->GetThirdPoint(t).z;
				vpt->getGW()->polyline(2, dirMesh2, NULL, NULL, FALSE, NULL); // draw line from second click point to third one (cylinder height)
			}

			bool drawGizmoNormals = !isPreview && !light->IsTargeted();
			if  (drawGizmoNormals)
			{
				// draw normals
				float cylinderHeight = light->GetThirdPoint(t).z - cylinderCenter.z;
				if (cylinderHeight > 0)
				{
					Point3 targ1 (0.0f, 0.0f, -radius * 2);
					Point3 targ2 (0.0f, 0.0f, cylinderHeight  + radius * 2);
					DrawArrow(vpt, targ1, Point3(0.0f, 0.0f, 0.0f));
					DrawArrow(vpt, targ2, secondDiskCenter);
				}
				else
				{
					Point3 targ1(0.0f, 0.0f, radius * 2);
					Point3 targ2(0.0f, 0.0f, cylinderHeight - radius * 2);
					DrawArrow(vpt, targ1, Point3(0.0f, 0.0f, 0.0f));
					DrawArrow(vpt, targ2, secondDiskCenter);
				}
			}

			break;
		}

		case FRPhysicalLight_SPHERE:
		{
			float radius = (dirMesh[1] - dirMesh[0]).Length() / 2;
			if (isPreview && !isTargetPreview)
			{
				Point3 diskCenter = dirMesh[0] + (dirMesh[1] - dirMesh[0]) / 2;
				DrawSphereTess(vpt, radius, diskCenter);
			}
			else
			{
				DrawSphereTess(vpt, radius, dirMesh[0]);
			}
			
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

void DisplayTarget(TimeValue t, ViewExp *vpt, bool isPreview, bool isTargetPreview, FireRenderPhysicalLight* light)
{
	FASSERT(light);

	if (!light->IsTargeted())
		return;

	INode* targetNode = light->GetTargetNode();

	if (isPreview && isTargetPreview)
	{
		// during light creation
		Point3 dirTarg[2];
		dirTarg[0] = Point3(0.0f, 0.0f, 0.0f);
		dirTarg[1] = Point3(0.0f, 0.0f, 0.0f);

		FRPhysicalLight_LightType lightType = light->GetLightType(t);
		if (FRPhysicalLight_SPOT == lightType)
		{
			Point3 dir = (light->GetSecondPoint(t) - light->GetLightPoint(t)).Normalize();
			float length = (light->GetTargetPoint(t) - light->GetLightPoint(t)).Length();
			dirTarg[1] = dir*length;
		}
		else
		{
			dirTarg[1].z = light->GetTargetPoint(t).z;
		}
		vpt->getGW()->polyline(2, dirTarg, NULL, NULL, FALSE, NULL); // draw line from pos of light origin to target

		DrawSphere(vpt, 1.0f, dirTarg[1]);
	}
	else if (targetNode)
	{
		// regular display
		INode* thisNode = light->GetThisNode();
		FASSERT(thisNode);
		Matrix3 thisTM = thisNode->GetNodeTM(t);
		Point3 thisPos = thisTM.GetTrans();

		FASSERT(targetNode);
		Matrix3 targTM = targetNode->GetNodeTM(t);
		Point3 targetPos = targTM.GetTrans();

		Matrix3 scaleMatr(thisNode->GetNodeTM(t)); // to cancel out scaling of light representation
		scaleMatr.NoScale();
		scaleMatr.Invert();
		scaleMatr = thisNode->GetNodeTM(t) * scaleMatr;
		scaleMatr.Invert();

		float distToTarg = (Point3(targetPos - thisPos) * scaleMatr).Length();
		Point3 dirTarg[2];
		dirTarg[0] = Point3(0.0f, 0.0f, 0.0f);
		dirTarg[1] = Point3(0.0f, 0.0f, -distToTarg);
		DrawArrow(vpt, dirTarg[1], dirTarg[0]); // draw arrow from pos of light origin to target
	}
}

bool FireRenderPhysicalLight::DisplayLight(TimeValue t, INode* inode, ViewExp *vpt, int flags)
{
	// do draw
	GraphicsWindow* graphicsWindow = vpt->getGW();
	graphicsWindow->setColor(LINE_COLOR, GetWireColor( bool_cast(inode->Selected()), bool_cast(inode->IsFrozen()) ));

	Point3 dirMesh[2];
	dirMesh[0] = Point3(0.0f, 0.0f, 0.0f);
	dirMesh[1] = GetSecondPoint(t) - GetLightPoint(t);

	if (m_isPreview && !m_isTargetPreview)
	{
		vpt->getGW()->polyline(2, dirMesh, NULL, NULL, FALSE, NULL); // draw line from pos of first click to pos of second click
	}
	
	bool success = true;

	FRPhysicalLight_LightType type = GetLightType(t);
	switch(type)
	{
		case FRPhysicalLight_SPOT:
		{
			DisplaySpotLight(t, vpt, m_isPreview, dirMesh, m_pblock);

			break;
		}

		case FRPhysicalLight_POINT:
		{
			DrawSphere(vpt, 2.0f, dirMesh[0]);
			DrawSphere(vpt, 4.0f, dirMesh[0]);

			break;
		}

		case FRPhysicalLight_DIRECTIONAL:
		{
			if (m_isPreview)
			{
				DrawArrow(vpt, dirMesh[1], dirMesh[0]);
			}
			else
			{
				float length = Point3(dirMesh[1]).Length();
				DrawArrow(vpt, Point3(0.0f, 0.0f, -length), dirMesh[0]);
			}

			break;
		}

		case FRPhysicalLight_AREA:
		{
			success = DisplayAreaLight(t, vpt, m_isPreview, m_isTargetPreview, dirMesh, this);

			break;
		}
	}

	// draw line to target
	if (IsTargeted())
	{
		DisplayTarget(t, vpt, m_isPreview, m_isTargetPreview, this);
	}

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
