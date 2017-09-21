/*********************************************************************************************************************************
* Radeon ProRender for 3ds Max plugin
* Copyright (c) 2017 AMD
* All Rights Reserved
*
* Custom IES light 3ds MAX scene node. This custom node allows the definition of IES light, using rectangular shapes
* that can be positioned and oriented in the MAX scene.
*********************************************************************************************************************************/

#include "Common.h"
#include "Resource.h"
#include "FireRenderIESLight.h"
#include "IESprocessor.h"
#include "FireRenderIES_Profiles.h"
#include <fstream>

FIRERENDER_NAMESPACE_BEGIN

static TCHAR* IESErrorCodeToMessage(IESProcessor::ErrorCode errorCode)
{
	switch (errorCode)
	{
		case IESProcessor::ErrorCode::NO_FILE:
			return L"empty file";

		case IESProcessor::ErrorCode::NOT_IES_FILE:
			return L"not .ies file";

		case IESProcessor::ErrorCode::FAILED_TO_READ_FILE:
			return L"failed to open file";

		case IESProcessor::ErrorCode::INVALID_DATA_IN_IES_FILE:
			return L"invalid IES data";

		case IESProcessor::ErrorCode::PARSE_FAILED:
			return L"parse failed";

		case IESProcessor::ErrorCode::UNEXPECTED_END_OF_FILE:
			return L"have reached end of file before parse was complete";

		default:
			return L"Unknown error";
	}
}

void Polar2XYZ(Point3& outPoint, double verticalAngle /*polar*/, double horizontalAngle /*azimuth*/, double dist)
{
	double XTheta = cos(verticalAngle * DEG_TO_RAD);
	double YTheta = sin(verticalAngle * DEG_TO_RAD);

	double XPhi = cos(horizontalAngle * DEG_TO_RAD);
	double YPhi = sin(horizontalAngle * DEG_TO_RAD);

	outPoint.x = XTheta * XPhi * dist;
	outPoint.y = YTheta * XPhi * dist;
	outPoint.z = YPhi * dist;
}

// clones all edges in edges array, transforms cloned edges by matrTransform and inserts them to edges array
void CloneAndTransform(std::vector<std::vector<Point3> >& edges, const Matrix3 &matrTransform)
{
	size_t length = edges.size();
	edges.reserve(length * 2);
	for (size_t idx = 0; idx < length; ++idx)
	{
		edges.emplace_back();
		std::vector<Point3> &newEdge = edges.back();
		newEdge.reserve(edges[idx].size());
		for (const Point3& point : edges[idx])
		{
			newEdge.emplace_back(point * matrTransform);
		}
	}
}

void MirrorEdges(std::vector<std::vector<Point3> >& edges, const IESProcessor::IESLightData &data)
{
	if (data.IsAxiallySymmetric())
	{
		// mirror around xy plane
		Matrix3 matrMirrorXZ(
			Point3(1.0, 0.0, 0.0),
			Point3(0.0, -1.0, 0.0),
			Point3(0.0, 0.0, 1.0),
			Point3(0.0, 0.0, 0.0)
		);
		CloneAndTransform(edges, matrMirrorXZ);

		// rotate around x axis by 90 degrees
		Matrix3 matrRotateAroundX(
			Point3(1.0, 0.0, 0.0),
			Point3(0.0, 0.0, -1.0),
			Point3(0.0, 1.0, 0.0),
			Point3(0.0, 0.0, 0.0)
		);
		CloneAndTransform(edges, matrRotateAroundX);
	}

	else if (data.IsQuadrantSymmetric())
	{
		// mirror around xy plane
		Matrix3 matrMirrorXZ(
			Point3(1.0, 0.0, 0.0),
			Point3(0.0, -1.0, 0.0),
			Point3(0.0, 0.0, 1.0),
			Point3(0.0, 0.0, 0.0)
		);
		CloneAndTransform(edges, matrMirrorXZ);

		// mirror around xy plane
		Matrix3 matrMirrorXY(
			Point3(1.0, 0.0, 0.0),
			Point3(0.0, 1.0, 0.0),
			Point3(0.0, 0.0, -1.0),
			Point3(0.0, 0.0, 0.0)
		);
		CloneAndTransform(edges, matrMirrorXY);
	}

	else if (data.IsPlaneSymmetric())
	{
		// mirror around xy plane
		Matrix3 matrMirrorXY(
			Point3(1.0, 0.0, 0.0),
			Point3(0.0, 1.0, 0.0),
			Point3(0.0, 0.0, -1.0),
			Point3(0.0, 0.0, 0.0)
		);
		CloneAndTransform(edges, matrMirrorXY);
	}
}

bool FireRenderIESLight::CalculateBBox(void)
{
	if (m_plines.empty())
		return false;

	// get lowest and max values of X, Y, Z;
	float minX = INFINITY; 
	float minY = INFINITY;
	float minZ = INFINITY;
	float maxX = -INFINITY;
	float maxY = -INFINITY;
	float maxZ = -INFINITY;
	for (const std::vector<Point3>& it : m_plines)
	{
		for (const Point3& tPoint : it)
		{
			if (tPoint.x > maxX)
				maxX = tPoint.x;

			if (tPoint.y > maxY)
				maxY = tPoint.y;

			if (tPoint.z > maxZ)
				maxZ = tPoint.z;

			if (tPoint.x < minX)
				minX = tPoint.x;

			if (tPoint.y < minY)
				minY = tPoint.y;

			if (tPoint.z < minZ)
				minZ = tPoint.z;
		}
	}

	m_bbox[0] = Point3(minX, minY, minZ);
	m_bbox[1] = Point3(maxX, minY, minZ);
	m_bbox[2] = Point3(minX, maxY, minZ);
	m_bbox[3] = Point3(minX, minY, maxZ);
	m_bbox[4] = Point3(maxX, maxY, maxZ);
	m_bbox[5] = Point3(minX, maxY, maxZ);
	m_bbox[6] = Point3(maxX, minY, maxZ);
	m_bbox[7] = Point3(maxX, maxY, minZ);
	m_BBoxCalculated = true;

	return true;
}

bool FireRenderIESLight::CalculateLightRepresentation(const TCHAR* profileName)
{
	const float SCALE_WEB = 0.05f;

	m_plines.clear();
	std::vector<std::vector<Point3>>& edges = m_plines;

	// load IES data
	std::wstring profilePath = FireRenderIES_Profiles::ProfileNameToPath(profileName);
	std::string iesFilename(profilePath.begin(), profilePath.end());

	// get .ies light params
	IESProcessor parser;
	IESProcessor::IESLightData data;

	const TCHAR* failReason = _T("Internal error");
	std::basic_string<TCHAR> temp;

	IESProcessor::ErrorCode parseRes = parser.Parse(data, iesFilename.c_str());
	bool failed = parseRes != IESProcessor::ErrorCode::SUCCESS;
	if (failed)
	{
		temp = _T("Failed to parse IES profile: ");
		temp += IESErrorCodeToMessage(parseRes);
		failReason = temp.c_str();
	}

	if (!failed)
	{
		// calculate points of the mesh
		// - verticle angles count is number of columns in candela values table
		// - horizontal angles count is number of rows in candela values table
		std::vector<Point3> candela_XYZ;
		candela_XYZ.reserve(data.m_candelaValues.size());
		auto it_candela = data.m_candelaValues.begin();
		for (double horizontalAngle : data.m_horizontalAngles)
		{
			for (double verticleAngle : data.m_verticalAngles)
			{
				// get world coordinates from polar representation in .ies
				candela_XYZ.emplace_back(0.0f, 0.0f, 0.0f);
				Polar2XYZ(candela_XYZ.back(), verticleAngle, horizontalAngle, *it_candela);
				candela_XYZ.back() *= SCALE_WEB;

				++it_candela;
			}
		}

		// generate edges for each verticle angle (slices)
		const size_t MAX_POINTS_PER_POLYLINE = 32; // this is 3DMax limitation!
		size_t valuesPerRow = data.m_verticalAngles.size(); // verticle angles count is number of columns in candela values table
		auto it_points = candela_XYZ.begin();
		while (it_points != candela_XYZ.end())
		{
			auto endRow = it_points + valuesPerRow;
			std::vector<Point3> pline; pline.reserve(MAX_POINTS_PER_POLYLINE);
			bool isClosed = true;
			for (; it_points != endRow; ++it_points)
			{
				pline.push_back(*it_points);
				if (pline.size() == MAX_POINTS_PER_POLYLINE)
				{
					edges.push_back(pline);
					pline.clear();
					isClosed = false;
					--it_points; // want to view continuous line
				}
			}

			if (!pline.empty())
			{
				edges.push_back(pline);
			}
		}
	}

	if (!failed && edges.empty())
	{
		failed = true;
		failReason = _T("Wrong IES file (edges list is empty).");
	}

	if (!failed)
	{
		// generate edges for mesh (edges crossing slices)
#define IES_LIGHT_DONT_GEN_EXTRA_EDGES
#ifndef IES_LIGHT_DONT_GEN_EXTRA_EDGES
		if (!data.IsAxiallySymmetric()) // axially simmetric light field is a special case (should be processed after mirroring)
		{
			size_t columnCount = data.VerticalAngles().size();
			size_t rowCount = data.HorizontalAngles().size();

			for (size_t idx_column = 0; idx_column < columnCount; idx_column++) // 0 - 180
			{
				std::vector<Point3> pline; pline.reserve(MAX_POINTS_PER_POLYLINE);
				for (size_t idx_row = 0; idx_row < rowCount; idx_row++) // 0 - 360
				{
					Point3 &tpoint = candela_XYZ[idx_row*columnCount + idx_column];
					pline.push_back(tpoint);

					if (pline.size() == MAX_POINTS_PER_POLYLINE)
					{
						edges.push_back(pline);
						pline.clear();
						--i; // want to view continuous line
					}
				}

				if (!pline.empty())
				{
					edges.push_back(pline);
				}
			}

		}
#endif
	}

	// mirror edges if necessary
	if (!failed)
	{
		MirrorEdges(edges, data);
	}

	if (!failed)
	{
		// preview graphic (before mouse button is released) should be aligned differently
		m_preview_plines = m_plines;

		// align light representation
		const Matrix3 matrRotateAroundZ(
			Point3(0.0, -1.0, 0.0),
			Point3(1.0, 0.0, 0.0),
			Point3(0.0, 0.0, 1.0),
			Point3(0.0, 0.0, 0.0)
		);
		const Matrix3 matrRotateAroundMinusY(
			Point3(0.0, 0.0, -1.0),
			Point3(0.0, 1.0, 0.0),
			Point3(1.0, 0.0, 0.0),
			Point3(0.0, 0.0, 0.0)
		);

		for (std::vector<Point3>& pline : m_plines)
		{
			for (Point3& point : pline)
				point = point * matrRotateAroundZ;
		}
		for (std::vector<Point3>& pline : m_preview_plines)
		{
			for (Point3& point : pline)
			{
				point = point * matrRotateAroundMinusY;
				point = point * matrRotateAroundZ;
			}
		}
	}

	if (failed)
	{
		MessageBox(
			GetCOREInterface()->GetMAXHWnd(),
			failReason,
			_T("Error"),
			MB_ICONERROR | MB_OK);
	}

	return !failed;
}

INode* FindNodeRef(ReferenceTarget *rt);

static INode* GetNodeRef(ReferenceMaker *rm) 
{
	if (rm->SuperClassID() == BASENODE_CLASS_ID) return (INode *)rm;
	else return rm->IsRefTarget() ? FindNodeRef((ReferenceTarget *)rm) : NULL;
}

static INode* FindNodeRef(ReferenceTarget *rt) 
{
	DependentIterator di(rt);
	ReferenceMaker *rm = NULL;
	INode *nd = NULL;
	while ((rm = di.Next()) != NULL) 
	{
		nd = GetNodeRef(rm);
		if (nd) return nd;
	}
	return NULL;
}

bool FireRenderIESLight::DrawWeb(TimeValue t, ViewExp *pVprt, bool isSelected /*= false*/, bool isFrozen /*= false*/)
{
	if (m_plines.empty())
	{
		// try regen web
		if (!CalculateLightRepresentation(GetActiveProfile(t)))
			return false;
	}

	GraphicsWindow* gw = pVprt->getGW();
	gw->setColor(LINE_COLOR, GetWireColor(isFrozen, isSelected));

	// draw line from light source to target
	Point3 dirMesh[2]
	{
		GetLightPoint(t),
		GetTargetPoint(t)
	};

	INode *nd = FindNodeRef(this);
	Control* pLookAtController = nd->GetTMController();

	float scaleFactor = GetAreaWidth(t);

	bool hasLookAtTarget = (pLookAtController != nullptr) && (pLookAtController->GetTarget() != nullptr);
	bool isPreviewMode = !hasLookAtTarget && m_isPreviewGraph;

	if (isPreviewMode)
	{
		// no look at controller => user have not put target yet (have not released mouse buttion yet)
		dirMesh[1] = dirMesh[1] - dirMesh[0];
		dirMesh[1] = dirMesh[1] / scaleFactor; // to cancel out scaling of graphic window
		dirMesh[0] = Point3(0.0f, 0.0f, 0.0f);

		// draw line from light source to target
		gw->polyline(2, dirMesh, NULL, NULL, FALSE, NULL);

		// rotate light web so it would look at mouse target (around Z axis)
		// - between prev look at vector and current one (Y axis is up vector for light representation)
		Point3 normDir = dirMesh[1].Normalize();
		normDir = normDir.Normalize();
		Point3 normal = m_defaultUp ^ normDir; // cross product, axis of rotation
		normal = normal.Normalize();
		float dotProduct = DotProd(m_defaultUp, normDir);
		float cosAngle = dotProduct; // vectors are normalized
		float angle = acos(cosAngle);
		Matrix3 rotateMx = RotAngleAxisMatrix(normal, angle);

		std::vector<std::vector<Point3>> preview_plines = m_preview_plines;

		for (std::vector<Point3>& pline : preview_plines)
		{
			for (Point3& point : pline)
				point = point * rotateMx;
		}

		// draw web
		for (std::vector<Point3>& pline : preview_plines)
		{
			gw->polyline(pline.size(), &pline.front(), NULL, NULL, false, NULL);
		}

		return true;
	}

	m_isPreviewGraph = false;

	if (hasLookAtTarget)
	{
		// look at controller is not disabled => draw line from light source to look at target
		float dist = GetTargetDistance(t);
		dist = dist / scaleFactor; // to cancel out scaling of graphic window

		dirMesh[1] = Point3(0.0f, 0.0f, -dist);
		dirMesh[0] = Point3(0.0f, 0.0f, 0.0f);

		// draw line from light source to target
		gw->polyline(2, dirMesh, NULL, NULL, FALSE, NULL);
	}

	std::vector<std::vector<Point3>> plines = m_plines;

	// transform light web representation by input params
	Matrix3 rotationToUpVector;
	float angles[3] = { GetRotationX(t)*DEG_TO_RAD, GetRotationY(t)*DEG_TO_RAD, GetRotationZ(t)*DEG_TO_RAD };
	EulerToMatrix(angles, rotationToUpVector, EULERTYPE_XYZ);

	for (std::vector<Point3>& pline : plines)
	{
		for (Point3& point : pline)
			point = rotationToUpVector * point;
	}

	// draw web
	for (std::vector<Point3>& pline : plines)
	{
		gw->polyline(pline.size(), &pline.front(), NULL, NULL, false, NULL);
	}	

	return true;
}

FIRERENDER_NAMESPACE_END
