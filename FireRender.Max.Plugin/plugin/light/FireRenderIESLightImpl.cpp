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

		// rotate around x axis
		Matrix3 matrRotateX(
			Point3(1.0, 0.0, 0.0),
			Point3(0.0, 0.0, -1.0),
			Point3(0.0, 1.0, 0.0),
			Point3(0.0, 0.0, 0.0)
		);
		CloneAndTransform(edges, matrRotateX);

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
	// ************** DEBUG ***************
	std::string iesFilename = "C:\\Users\\endar\\Downloads\\L04148904.IES";
	//std::string iesFilename = "C:\\Users\\endar\\Downloads\\LLI-16162-2-R01.IES";
	//std::string iesFilename = "C:\\Users\\endar\\Downloads\\T31177.IES";
	// ************ END DEBUG *************

	// get .ies light params
	IESProcessor parser;
	IESProcessor::IESLightData data;
	std::string errorMsg;
	bool isSuccessfull = parser.Parse(data, /*m_iesFilename.c_str()*/ iesFilename.c_str(), errorMsg);
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

	// mirror edges if necessary
	if (!MirrorEdges(edges, data))
	{
		return false;
	}

	return true;
}

void FireRenderIESLight::DrawWeb(ViewExp *pVprt, IParamBlock2 *pPBlock, bool isSelected /*= false*/, bool isFrozen /*= false*/)
{
	bool doWantCalculateLightRepresentation = m_plines.empty();
	if (doWantCalculateLightRepresentation)
	{
		bool success = CalculateLightRepresentation(m_plines);
		FASSERT(success);
	}

	GraphicsWindow* gw = pVprt->getGW();
	gw->setColor(LINE_COLOR, GetEdgeColor(isFrozen, isSelected));

	// draw web (mesh)
	for (auto& pline : m_plines)
	{
		gw->polyline(pline.size(), &pline.front(), NULL, NULL, false /*isClosed*/, NULL);
	}
}

FIRERENDER_NAMESPACE_END