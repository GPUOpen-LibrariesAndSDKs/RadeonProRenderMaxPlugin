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

const float DEG2RAD = PI / 180.0;

void Polar2XYZ(Point3& outPoint, double verticalAngle /*polar*/, double horizontalAngle /*azimuth*/, double dist)
{
	double XTheta = cos(verticalAngle * DEG2RAD);
	double YTheta = sin(verticalAngle * DEG2RAD);

	double XPhi = cos(horizontalAngle * DEG2RAD);
	double YPhi = sin(horizontalAngle * DEG2RAD);

	outPoint.x = XTheta * XPhi * dist;
	outPoint.y = YTheta * XPhi * dist;
	outPoint.z = YPhi * dist;
}

Color GetEdgeColor(bool isFrozen, bool isSelected)
{
	// determine color of object to be drawn
	Color color = isFrozen ? Color(0.0f, 0.0f, 1.0f) : Color(0.0f, 1.0f, 0.0f);
	if (isSelected)
	{
		color = Color(1.0f, 0.0f, 0.0f);
	}

	return color;
}

const float SCALE_WEB = 0.01f;

// clones all edges in edges array, transforms cloned edges them by matrTransform and inserts them to edges array
void CloneAndTransform(std::vector<std::vector<Point3> >& edges, const Matrix3 &matrTransform)
{
	size_t length = edges.size();
	edges.reserve(length * 2);
	for (size_t idx = 0; idx < length; ++idx)
	{
		edges.emplace_back();
		std::vector<Point3> &newEdge = edges.back();
		newEdge.reserve(edges[idx].size());
		for (auto& point : edges[idx])
		{
			newEdge.emplace_back(point * matrTransform);
		}
	}
}

bool MirrorEdges(std::vector<std::vector<Point3> >& edges, const IESProcessor::IESLightData &data)
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

		return true;
	}

	if (data.IsQuadrantSymmetric())
	{
		// NIY
		return false;
	}

	if (data.IsPlaneSymmetric())
	{
		// mirror around xy plane
		Matrix3 matrMirrorXY(
			Point3(1.0, 0.0, 0.0),
			Point3(0.0, 1.0, 0.0),
			Point3(0.0, 0.0, -1.0),
			Point3(0.0, 0.0, 0.0)
		);
		CloneAndTransform(edges, matrMirrorXY);

		return true;
	}

	return true;
}

bool FireRenderIESLight::CalculateLightRepresentation(std::vector<std::vector<Point3> >& edges) const
{
	// load IES data
	auto profilePath = FireRenderIES_Profiles::ProfileNameToPath(GetActiveProfile());
	std::string iesFilename((std::istreambuf_iterator<char>(std::ifstream(profilePath))), {});

	// ************** DEBUG ***************
	//iesFilename = "C:\\Users\\endar\\Downloads\\L04148904.IES";
	//iesFilename = "C:\\Users\\endar\\Downloads\\LLI-16162-2-R01.IES";
	//iesFilename = "C:\\Users\\endar\\Downloads\\T31177.IES";
	// ************ END DEBUG *************

	// get .ies light params
	IESProcessor parser;
	IESProcessor::IESLightData data;
	std::string errorMsg;
	bool isSuccessfull = parser.Parse(data, iesFilename.c_str(), errorMsg);
	if (!isSuccessfull)
		return false;

	// calculate points of the mesh
	// - verticle angles count is number of columns in candela values table
	// - horizontal angles count is number of rows in candela values table
	std::vector<Point3> candela_XYZ;
	candela_XYZ.reserve(data.CandelaValues().size());
	auto it_candela = data.CandelaValues().begin();
	for (double horizontalAngle : data.HorizontalAngles())
	{
		for (double verticleAngle : data.VerticalAngles())
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
	size_t valuesPerRow = data.VerticalAngles().size(); // verticle angles count is number of columns in candela values table
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

	if (edges.empty())
		return false;

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
					//--i; // want to view continuous line
				}
			}

			if (!pline.empty())
			{
				edges.push_back(pline);
			}
		}

	}
#endif

	// mirror edges if necessary
	if (!MirrorEdges(edges, data))
	{
		return false;
	}

	return true;
}

INode* FindNodeRef(ReferenceTarget *rt);

static INode* GetNodeRef(ReferenceMaker *rm) {
	if (rm->SuperClassID() == BASENODE_CLASS_ID) return (INode *)rm;
	else return rm->IsRefTarget() ? FindNodeRef((ReferenceTarget *)rm) : NULL;
}

static INode* FindNodeRef(ReferenceTarget *rt) {
	DependentIterator di(rt);
	ReferenceMaker *rm = NULL;
	INode *nd = NULL;
	while ((rm = di.Next()) != NULL) {
		nd = GetNodeRef(rm);
		if (nd) return nd;
	}
	return NULL;
}

void FireRenderIESLight::DrawWeb(ViewExp *pVprt, IParamBlock2 *pPBlock, bool isSelected /*= false*/, bool isFrozen /*= false*/)
{
	bool doWantCalculateLightRepresentation = m_plines.empty();

	if (doWantCalculateLightRepresentation)
	{
		// calculate IES web
		bool success = CalculateLightRepresentation(m_plines);
		FASSERT(success);

		m_preview_plines = m_plines;

		// align light representation to look at controller (-Z axis is up vector)
		// - rotate around x axis by 90 degrees
		Matrix3 matrRotateAroundX(
			Point3(1.0, 0.0, 0.0),
			Point3(0.0, 0.0, -1.0),
			Point3(0.0, 1.0, 0.0),
			Point3(0.0, 0.0, 0.0)
		);

		for (auto& pline : m_plines)
		{
			for (Point3& point : pline)
				point = point * matrRotateAroundX;
		}
	}

	GraphicsWindow* gw = pVprt->getGW();
	gw->setColor(LINE_COLOR, GetEdgeColor(isFrozen, isSelected));

	// draw line from light source to target
	Point3 dirMesh[2];
	pPBlock->GetValue(IES_PARAM_P0, 0, dirMesh[0], FOREVER);
	pPBlock->GetValue(IES_PARAM_P1, 0, dirMesh[1], FOREVER);

	INode *nd = FindNodeRef(this);
	Control* pLookAtController = nd->GetTMController();

	if ((pLookAtController == nullptr) || (pLookAtController->GetTarget() == nullptr))
	{
		// no look at controller => user have not put target yet (have not released mouse buttion yet)
		dirMesh[1] = dirMesh[1] - dirMesh[0];
		dirMesh[0] = Point3(0.0f, 0.0f, 0.0f);

		// draw line from light source to target
		gw->polyline(2, dirMesh, NULL, NULL, FALSE, NULL);

		// rotate light web so it would look at mouse target (around Z axis)
		// - between prev look at vector and current one (Y axis is up vector for light representation)
		Point3 normDir = dirMesh[1].Normalize();
		float dotProduct = prevUp % normDir; // Dot product, for angle between vectors
		float cosAngle = dotProduct; // vectors are normalized
		float angle = acos(cosAngle);
		Matrix3 rotateMx = RotAngleAxisMatrix(Point3(0.0f, 0.0f, 1.0f), angle);

		auto preview_plines = m_preview_plines;

		for (auto& pline : preview_plines)
		{
			for (Point3& point : pline)
				point = point * rotateMx;
		}

		// draw web
		for (auto& pline : preview_plines)
		{
			gw->polyline(pline.size(), &pline.front(), NULL, NULL, false /*isClosed*/, NULL);
		}
	}
	else
	{
		// have controller => need to calculate dist between ligh root and target
		INode* pTargNode = pLookAtController->GetTarget();
		Matrix3 targTransform = pTargNode->GetObjectTM(0.0);
		Point3 targVector(0.0f, 0.0f, 0.0f);
		targVector = targVector * targTransform;

		Matrix3 rootTransform = nd->GetObjectTM(0.0);
		Point3 rootVector(0.0f, 0.0f, 0.0f);
		rootVector = rootVector * rootTransform;

		float dist = (targVector - rootVector).FLength();
		dirMesh[1] = Point3(0.0f, 0.0f, -dist);
		dirMesh[0] = Point3(0.0f, 0.0f, 0.0f);

		// draw line from light source to target
		gw->polyline(2, dirMesh, NULL, NULL, FALSE, NULL);

		// draw web
		for (auto& pline : m_plines)
		{
			gw->polyline(pline.size(), &pline.front(), NULL, NULL, false /*isClosed*/, NULL);
		}
	}
}

FIRERENDER_NAMESPACE_END