/*********************************************************************************************************************************
* Radeon ProRender for 3ds Max plugin
* Copyright (c) 2017 AMD
* All Rights Reserved
*
* Custom IES light 3ds MAX scene node. This custom node allows the definition of IES light, using rectangular shapes
* that can be positioned and oriented in the MAX scene.
*********************************************************************************************************************************/

#include "FireRenderPhysicalLight.h"
#include "Common.h"
#include "frWrap.h"

FIRERENDER_NAMESPACE_BEGIN

frw::Shape CreateMeshShape(frw::Context& context, Mesh* mesh, float masterScale)
{
	// extract data from mesh
	int countVerts = mesh->numVerts;
	int countFaces = mesh->numFaces;
	Point3*	meshVerts = mesh->verts;
	Face* meshFaces = mesh->faces;

	// setup
	std::vector<Point3> points;
	points.reserve(countVerts);
	for (int idx = 0; idx < countVerts; ++idx)
	{
		points.push_back(Point3(meshVerts[idx] * masterScale));
	}

	std::vector<int> indices;
	indices.reserve(countFaces * 3);
	for (int idx = 0; idx < countFaces; ++idx)
	{
		indices.push_back(meshFaces[idx].v[0]);
		indices.push_back(meshFaces[idx].v[1]);
		indices.push_back(meshFaces[idx].v[2]);
	}

	std::vector<int> faces;
	faces.reserve(countFaces);
	for (int idx = 0; idx < countFaces; ++idx)
	{
		faces.push_back(3);
	}

	// create mesh
	return context.CreateMesh(
		(rpr_float const*)&points[0], points.size(), sizeof(Point3),
		nullptr, 0, 0,
		nullptr, 0, 0,
		(rpr_int const*)&indices[0], sizeof(rpr_int),
		nullptr, 0,
		nullptr, 0,
		&faces[0], faces.size()
	);
}

frw::Shape CreateRectShape(frw::Context& context, Point3& corner1, Point3& corner2)
{
	// setup
	std::vector<Point3> points;
	std::vector<Point3> normals;
	std::vector<int> indices;
	std::vector<int> normal_indices;
	std::vector<int> faces;

	// compute points
	points = {
		Point3(corner1.x, corner1.y, 0.0f),
		Point3(corner2.x, corner1.y, 0.0f),
		Point3(corner1.x, corner2.y, 0.0f),
		Point3(corner2.x, corner2.y, 0.0f),
	};

	// set indices
	indices = { 0, 1, 3, 0, 3, 2 };
	faces = { 3, 3 };

	// normals
	normals.push_back(Point3(0.0f, 0.0f, -1.0f));
	normal_indices = { 0, 0, 0, 0, 0, 0 };

	// create mesh
	return context.CreateMesh(
		(rpr_float const*)&points[0], points.size(), sizeof(Point3),
		(rpr_float const*)&normals[0], normals.size(), sizeof(Point3),
		nullptr, 0, 0,
		(rpr_int const*)&indices[0], sizeof(rpr_int),
		(rpr_int const*)&normal_indices[0], sizeof(rpr_int),
		nullptr, 0,
		&faces[0], faces.size()
	);
}

constexpr size_t pointsPerArc = 16;
constexpr float da = 2.0f * PI / pointsPerArc;

size_t pointsPerCircle(ViewExp *vpt, const Matrix3& tm)
{
	float scaleFactor = vpt->NonScalingObjectSize() * vpt->GetVPWorldWidth(tm.GetTrans()) / 360.0f;

	size_t adjustedPointsPerCircle = scaleFactor * pointsPerArc;
	return adjustedPointsPerCircle;
}

frw::Shape CreateCylinderShape(frw::Context& context, float radius, float cylinderHeight)
{
	// setup
	std::vector<Point3> points;
	// - normals are auto computed so we ommit them
	std::vector<int> indices;
	std::vector<int> faces;

	// calculate points
	points.reserve((pointsPerArc + 1) * 2);
	faces.reserve(pointsPerArc * 2);
	points.push_back(Point3(0.0f, 0.0f, 0.0f));
	for (int i = 0; i < pointsPerArc; i++)
	{
		// Angle
		float a = i * da;
		float sn = radius * sin(a);
		float cs = radius * cos(a);
		points.push_back(Point3(cs, sn, 0.0f));
		faces.push_back(3);
	}
	points.push_back(Point3(0.0f, 0.0f, -cylinderHeight));
	for (int i = 0; i < pointsPerArc; i++)
	{
		// Angle
		float a = i * da;
		float sn = radius * sin(a);
		float cs = radius * cos(a);
		points.push_back(Point3(cs, sn, -cylinderHeight));
		faces.push_back(3);
	}

	// calculate indices
	indices.reserve(pointsPerArc * 12);
	// - create 2 disks
	if (cylinderHeight > 0) // (normal direction depends on disk orientation respective to each other)
	{
		for (int i = 0; i < (pointsPerArc - 1); i++)
		{
			indices.push_back(i + 1);
			indices.push_back(i + 2);
			indices.push_back(0);
		}
		indices.push_back(pointsPerArc);
		indices.push_back(1);
		indices.push_back(0);

		for (int i = pointsPerArc + 1; i < (pointsPerArc * 2); i++)
		{
			indices.push_back(i + 1);
			indices.push_back(pointsPerArc + 1);
			indices.push_back(i + 2);
		}
		indices.push_back(pointsPerArc * 2 + 1);
		indices.push_back(pointsPerArc + 1);
		indices.push_back(pointsPerArc + 2);
	}
	else
	{
		for (int i = 0; i < (pointsPerArc - 1); i++)
		{
			indices.push_back(i + 1);
			indices.push_back(0);
			indices.push_back(i + 2);
		}
		indices.push_back(pointsPerArc);
		indices.push_back(0);
		indices.push_back(1);

		for (int i = pointsPerArc + 1; i < (pointsPerArc * 2); i++)
		{
			indices.push_back(i + 1);
			indices.push_back(i + 2);
			indices.push_back(pointsPerArc + 1);
		}
		indices.push_back(pointsPerArc + 1);
		indices.push_back(pointsPerArc * 2 + 1);
		indices.push_back(pointsPerArc + 2);
	}

	// - connect 2 disks
	if (cylinderHeight > 0) // (normal direction depends on disk orientation respective to each other)
	{
		for (int i = 0; i < (pointsPerArc - 1); i++)
		{
			indices.push_back(i + 1);
			indices.push_back(pointsPerArc + i + 2);
			indices.push_back(pointsPerArc + i + 3);

			indices.push_back(i + 1);
			indices.push_back(pointsPerArc + i + 3);
			indices.push_back(i + 2);

			faces.push_back(3);
			faces.push_back(3);
		}

		indices.push_back(pointsPerArc);
		indices.push_back(2 * pointsPerArc + 1);
		indices.push_back(pointsPerArc + 2);

		indices.push_back(pointsPerArc);
		indices.push_back(pointsPerArc + 2);
		indices.push_back(1);

		faces.push_back(3);
		faces.push_back(3);
	}
	else
	{
		for (int i = 0; i < (pointsPerArc - 1); i++)
		{
			indices.push_back(i + 1);
			indices.push_back(pointsPerArc + i + 3);
			indices.push_back(pointsPerArc + i + 2);

			indices.push_back(i + 1);
			indices.push_back(i + 2);
			indices.push_back(pointsPerArc + i + 3);

			faces.push_back(3);
			faces.push_back(3);
		}

		indices.push_back(pointsPerArc);
		indices.push_back(pointsPerArc + 2);
		indices.push_back(2 * pointsPerArc + 1);

		indices.push_back(pointsPerArc);
		indices.push_back(1);
		indices.push_back(pointsPerArc + 2);

		faces.push_back(3);
		faces.push_back(3);
	}

	// create mesh
	return context.CreateMesh(
		(rpr_float const*)&points[0], points.size(), sizeof(Point3),
		nullptr, 0, 0,
		nullptr, 0, 0,
		(rpr_int const*)&indices[0], sizeof(rpr_int),
		nullptr, 0,
		nullptr, 0,
		&faces[0], faces.size()
	);
}

frw::Shape CreateDiscShape(frw::Context& context, float radius)
{
	// setup
	std::vector<Point3> points;
	// - normals are auto computed so we ommit them
	std::vector<int> indices;
	std::vector<int> faces;

	// compute points
	points.reserve(pointsPerArc + 1);
	faces.reserve(pointsPerArc);
	points.push_back(Point3(0.0f, 0.0f, 0.0f));
	for (int i = 0; i < pointsPerArc; i++)
	{
		// Angle
		float a = i * da;
		float sn = radius * sin(a);
		float cs = radius * cos(a);
		points.push_back(Point3(cs, sn, 0.0f));
		faces.push_back(3);
	}

	// set indices
	indices.reserve(pointsPerArc * 3);
	for (int i = 0; i < (pointsPerArc - 1); i++)
	{
		indices.push_back(i + 1);
		indices.push_back(0);
		indices.push_back(i + 2);
	}
	indices.push_back(pointsPerArc);
	indices.push_back(0);
	indices.push_back(1);

	// create mesh
	return context.CreateMesh(
		(rpr_float const*)&points[0], points.size(), sizeof(Point3),
		nullptr, 0, 0,
		nullptr, 0, 0,
		(rpr_int const*)&indices[0], sizeof(rpr_int),
		nullptr, 0,
		nullptr, 0,
		&faces[0], faces.size()
	);
}

void CalculateSphere(float radius,
	std::vector<Point3>& vertices,
	std::vector<Point3>& normals,
	std::vector<int>& triangles)
{
	int nbLong = 16;
	int nbLat = 16;

	vertices.resize((nbLong + 1) * nbLat + 2);
	float _pi = PI;
	float _2pi = _pi * 2.f;

	Point3 vectorup(0.f, 1.f, 0.f);

	vertices[0] = vectorup * radius;
	for (int lat = 0; lat < nbLat; lat++)
	{
		float a1 = _pi * (float)(lat + 1) / (nbLat + 1);
		float sin1 = sin(a1);
		float cos1 = cos(a1);

		for (int lon = 0; lon <= nbLong; lon++)
		{
			float a2 = _2pi * (float)(lon == nbLong ? 0 : lon) / nbLong;
			float sin2 = sin(a2);
			float cos2 = cos(a2);
			vertices[lon + lat * (nbLong + 1) + 1] = Point3(sin1 * cos2, cos1, sin1 * sin2) * radius;
		}
	}
	vertices[vertices.size() - 1] = vectorup * -radius;

	// normals
	size_t vsize = vertices.size();

	normals.resize(vsize);
	for (size_t n = 0; n < vsize; n++)
		normals[n] = Normalize(vertices[n]);

	// triangles
	int nbTriangles = nbLong * 2 + (nbLat - 1)*nbLong * 2;//2 caps and one middle
	int nbIndexes = nbTriangles * 3;
	triangles.resize(nbIndexes);

	// Top Cap
	int i = 0;
	for (int lon = 0; lon < nbLong; lon++)
	{
		triangles[i++] = lon + 2;
		triangles[i++] = lon + 1;
		triangles[i++] = 0;
	}

	// Middle
	for (int lat = 0; lat < nbLat - 1; lat++)
	{
		for (int lon = 0; lon < nbLong; lon++)
		{
			int current = lon + lat * (nbLong + 1) + 1;
			int next = current + nbLong + 1;

			triangles[i++] = current;
			triangles[i++] = current + 1;
			triangles[i++] = next + 1;

			triangles[i++] = current;
			triangles[i++] = next + 1;
			triangles[i++] = next;
		}
	}

	// Bottom Cap
	for (int lon = 0; lon < nbLong; lon++)
	{
		triangles[i++] = vsize - 1;
		triangles[i++] = vsize - (lon + 2) - 1;
		triangles[i++] = vsize - (lon + 1) - 1;
	}
}

frw::Shape CreateSphereShape(frw::Context& context, float radius)
{
	std::vector<Point3> vertices;
	std::vector<Point3> normals;
	std::vector<int> triangles;

	CalculateSphere(radius, vertices, normals, triangles);

	std::vector<int>& normal_indices = triangles;

	std::vector<int> faces(triangles.size() / 3);
	for (size_t i = 0; i < triangles.size() / 3; i++)
		faces[i] = 3;

	return context.CreateMesh(
		(rpr_float const*)&vertices[0], vertices.size(), sizeof(Point3),
		(rpr_float const*)&normals[0], vertices.size(), sizeof(Point3),
		nullptr, 0, 0,
		(rpr_int const*)&triangles[0], sizeof(rpr_int),
		(rpr_int const*)&normal_indices[0], sizeof(rpr_int),
		nullptr, 0,
		&faces[0], triangles.size() / 3);
}

float FireRenderPhysicalLight::GetLightSourceArea(void) const
{
	float area = 1.0f;

	TimeValue time = GetCOREInterface()->GetTime();

	FRPhysicalLight_LightType lightType = GetLightType(time);
	if (FRPhysicalLight_AREA != lightType)
		return area; // no area for non-physical light sources

	// proceed with calculating diferent area for different light geometry
	FRPhysicalLight_AreaLight_LightShape lightShape = GetLightShapeMode(time);
	switch (lightShape)
	{
		case FRPhysicalLight_DISC:
		{
			float radius = (GetSecondPoint(time) - GetLightPoint(time)).Length();
			area = PI * radius * radius;
			break;
		}

		case FRPhysicalLight_CYLINDER:
		{
			float radius = (GetSecondPoint(time) - GetLightPoint(time)).Length();
			float height = abs(GetThirdPoint(time).z - GetLightPoint(time).z);
			area = 2 * PI * radius * radius + height * 2 * PI * radius;
			break;
		}

		case FRPhysicalLight_SPHERE:
		{
			float radius = (GetSecondPoint(time) - GetLightPoint(time)).Length();
			area = 4 * PI * radius * radius;
			break;
		}

		case FRPhysicalLight_RECTANGLE:
		{
			Point3 corner = GetSecondPoint(time) - GetLightPoint(time);
			float length = abs(corner.y);
			float width = abs(corner.x);
			area = length * width;
			break;
		}

		case FRPhysicalLight_MESH:
		{
			// back-offs
			ReferenceTarget* pRef = nullptr;
			m_pblock->GetValue(FRPhysicalLight_AREALIGHT_LIGHTMESH, time, pRef, Interval(FOREVER));
			if (pRef == nullptr)
				break;

			INode* pNode = static_cast<INode*>(pRef);
			if (pNode == nullptr)
				break;

			Object* pObj = pNode->GetObjectRef();
			if (pObj == nullptr)
				break;

			ShapeObject* pShape = static_cast<ShapeObject*>(pObj);
			FASSERT(pShape);
			
			// get mesh
			Mesh* mesh = nullptr;
			BOOL needDelete = FALSE;
			FireRender::RenderParameters params;
			mesh = pShape->GetRenderMesh(time, pNode, params.view, needDelete);
			int countVerts = mesh->numVerts;
			int countFaces = mesh->numFaces;
			Point3*	verts = mesh->verts;
			Face* faces = mesh->faces;

			area = 0;

			// foreach triangle
			for (int idx = 0; idx < countFaces; ++idx)
			{
				const Point3& v0 = verts[faces[idx].v[0]];
				Point3 v1 = verts[faces[idx].v[1]] - v0;
				Point3 v2 = verts[faces[idx].v[2]] - v0;
				float polygonSurface = (v1 ^ v2).Length() / 2; // cross product / 2
				area += polygonSurface;
			}

			break;
		}
	}

	float masterScale = GetUnitScale();
	area *= masterScale;

	return area;
}

float FireRenderPhysicalLight::GetIntensity(void) const
{
	float maxEfficacy = 684.0f;

	TimeValue time = GetCOREInterface()->GetTime();

	float intensity = 0.0f;
	Interval valid = FOREVER;
	m_pblock->GetValue(FRPhysicalLight_LIGHT_INTENSITY, time, intensity, valid);

	int value;
	m_pblock->GetValue(FRPhysicalLight_INTENSITY_UNITS, time, value, valid);
	FRPhysicalLight_IntensityUnits typeOfIntensityUnits = static_cast<FRPhysicalLight_IntensityUnits>(value);
	switch (typeOfIntensityUnits)
	{
		case FRPhysicalLight_WATTS:
		{
			// intensity * efficiency / area
			float efficiency = 1.0f;
			m_pblock->GetValue(FRPhysicalLight_LUMINOUS_EFFICACY, time, efficiency, valid);

			intensity = (intensity * efficiency) / GetLightSourceArea();

			break;
		}

		case FRPhysicalLight_LUMINANCE:
		{
			// as is
			intensity = intensity;

			break;
		}

		case FRPhysicalLight_RADIANCE:
		{
			// intensity * efficiency
			float efficiency = 1.0f;
			m_pblock->GetValue(FRPhysicalLight_LUMINOUS_EFFICACY, time, efficiency, valid);

			intensity = intensity * efficiency;

			break;
		}

		case FRPhysicalLight_LUMEN:
		{
			// intensity by area
			intensity /= GetLightSourceArea();

			break;
		}
	}

	return intensity /= maxEfficacy;
}

Color FireRenderPhysicalLight::GetLightColour(void) const
{
	Color colour (0.0f, 0.0f, 0.0f);

	TimeValue time = GetCOREInterface()->GetTime();

	int value;
	Interval valid = FOREVER;
	m_pblock->GetValue(FRPhysicalLight_COLOUR_MODE, time, value, valid);
	FRPhysicalLight_ColourMode typeOfIntensityUnits = static_cast<FRPhysicalLight_ColourMode>(value);
	switch (typeOfIntensityUnits)
	{
		case FRPhysicalLight_COLOUR:
		{
			bool ok = m_pblock->GetValue(FRPhysicalLight_INTENSITY_COLOUR, time, colour, valid);
			FASSERT(ok);
			break;
		}

		case FRPhysicalLight_TEMPERATURE:
		{
			bool ok = m_pblock->GetValue(FRPhysicalLight_INTENSITY_TEMPERATURE_COLOUR, time, colour, valid);
			FASSERT(ok);
			break;
		}
	}

	return colour;
}

void FireRenderPhysicalLight::CreateSceneLight(const ParsedNode& node, frw::Scope scope, const RenderParameters& params)
{
	// back-off
	if (!IsEnabled())
		return;

	INode* thisNode = GetThisNode();
	FASSERT(thisNode);

	// setup position
	// - should depend on targeted/not targeted because of how look at contoller works
	//Matrix3 tm = thisNode->GetNodeTM(TimeValue(0)); // <= this method doesn't take masterScale into acount, thus returns wrong transform!
	Matrix3 tm = node.tm;
	// - create RPR matrix
	float frTm[16];
	CreateFrMatrix(fxLightTm(tm), frTm);

	TimeValue time = GetCOREInterface()->GetTime();

	// intensity
	float watts = GetIntensity();
	Color colour = GetLightColour();

	// create light
	frw::Context context = scope.GetContext();
	// different type of RPR lights should be created for different types of physical lights
	FRPhysicalLight_LightType lightType = GetLightType(time);
	switch (lightType)
	{
		case FRPhysicalLight_AREA:
		{
			frw::Shape shape = NULL;
			FRPhysicalLight_AreaLight_LightShape lightShape = GetLightShapeMode(time);
			switch (lightShape)
			{
				case FRPhysicalLight_DISC:
				{
					// create geom
					float radius = (GetSecondPoint(time) - GetLightPoint(time)).Length() / 2;
					float masterScale = GetUnitScale();
					radius *= masterScale;
					shape = CreateDiscShape(context, radius);

					break;
				}

				case FRPhysicalLight_CYLINDER:
				{
					// create geom
					// - radius
					float radius = (GetSecondPoint(time) - GetLightPoint(time)).Length() / 2;
					float masterScale = GetUnitScale();
					radius *= masterScale;
					// - height
					float height = GetThirdPoint(time).z - GetSecondPoint(time).z;
					height *= masterScale;
					if (!IsTargeted())
						height = -height;
					else if (GetTargetPoint(time).z < GetThirdPoint(time).z)
						height = -height;
					shape = CreateCylinderShape(context, radius, height);
					break;
				}

				case FRPhysicalLight_SPHERE:
				{
					// create geom
					float radius = (GetSecondPoint(time) - GetLightPoint(time)).Length() / 2;
					float masterScale = GetUnitScale();
					radius *= masterScale;
					shape = CreateSphereShape(context, radius);
					break;
				}

				case FRPhysicalLight_RECTANGLE:
				{
					// create geom
					float masterScale = GetUnitScale();
					Point3 rectCenter = Point3(0.0f, 0.0f, 0.0f) + (GetSecondPoint(time) - GetLightPoint(time)) / 2;
					rectCenter *= masterScale;
					Point3 corner = GetSecondPoint(time) - GetLightPoint(time);
					corner *= masterScale;
					shape = CreateRectShape(context, Point3(0.0f, 0.0f, 0.0f) - rectCenter, corner - rectCenter);

					break;
				}

				case FRPhysicalLight_MESH:
				{
					// create geom
					ReferenceTarget* pRef = nullptr;
					TimeValue time = GetCOREInterface()->GetTime();
					Interval valid = FOREVER;
					m_pblock->GetValue(FRPhysicalLight_AREALIGHT_LIGHTMESH, time, pRef, valid);
					if (pRef != nullptr)
					{
						INode* pNode = static_cast<INode*>(pRef);
						if (pNode == nullptr)
						{
							break;
						}

						Object* pObj = pNode->GetObjectRef();
						if (pObj == nullptr)
						{
							break;
						}

						ShapeObject* pShape = static_cast<ShapeObject*>(pObj);
						FASSERT(pShape);

						Mesh* mesh = nullptr;
						BOOL needDelete = FALSE;
						FireRender::RenderParameters params;
						mesh = pShape->GetRenderMesh(time, pNode, params.view, needDelete);

						float masterScale = GetUnitScale();
						shape = CreateMeshShape(context, mesh, masterScale);
					}

					break;
				}
			}

			if (!shape.IsValid())
				return;

			// create emissive material
			frw::MaterialSystem ms = scope.GetMaterialSystem();
			frw::EmissiveShader material(ms);

			{
				// - light emmiter double sided
				int value;
				Interval valid = FOREVER;
				m_pblock->GetValue(FRPhysicalLight_AREALIGHT_ISBIDIRECTIONAL, TimeValue(0), value, valid);
				bool isDoubleSided = static_cast<bool>(value);
				// not supported in Core!
			}

			// intensity
			colour *= watts;
			material.SetColor(frw::Value(colour.r, colour.g, colour.b));

			shape.SetShader(material);
			shape.SetShadowFlag(false);

			{
				// - light emmiter visibility
				int value;
				Interval valid = FOREVER;
				m_pblock->GetValue(FRPhysicalLight_AREALIGHT_ISVISIBLE, TimeValue(0), value, valid);
				bool isVisible = static_cast<bool>(value);
				shape.SetPrimaryVisibility(isVisible);
			}

			// attach to scene
			shape.SetTransform(frTm, false);
			std::wstring name = thisNode->GetName();
			std::string name_s(name.begin(), name.end());
			shape.SetName(name_s.c_str());
			scope.GetScene().Attach(shape);
			break;
		}
		case FRPhysicalLight_SPOT:
		{
			rpr_int res = 0;
			rpr_light fireLight = 0;
			res = rprContextCreateSpotLight(context.Handle(), &fireLight);
			FCHECK(res);

			Interval valid = FOREVER;

			float iAngle = 0.0f;
			{
				bool success = m_pblock->GetValue(FRPhysicalLight_SPOTLIGHT_INNERCONE, time, iAngle, valid);
				FASSERT(success);
			}
			iAngle *= DEG_TO_RAD * 0.5f;

			float oAngle = 0.0f;
			{
				bool success = m_pblock->GetValue(FRPhysicalLight_SPOTLIGHT_OUTERCONE, time, oAngle, valid);
				FASSERT(success);
			}
			oAngle *= DEG_TO_RAD * 0.5f;

			colour *= watts;
			res = rprSpotLightSetRadiantPower3f(fireLight, colour.r, colour.g, colour.b);
			FCHECK(res);
			res = rprSpotLightSetConeShape(fireLight, iAngle, oAngle);
			FCHECK(res);

			frw::Light light(fireLight, context);
			light.SetTransform(frTm, false);
			std::wstring name = thisNode->GetName();
			std::string name_s(name.begin(), name.end());
			light.SetName(name_s.c_str());
			scope.GetScene().Attach(light);

			break;
		}

		case FRPhysicalLight_POINT:
		{
			frw::PointLight light = context.CreatePointLight();
			colour *= watts;
			light.SetRadiantPower(colour.r, colour.g, colour.b);
			light.SetTransform(frTm, false);
			std::wstring name = thisNode->GetName();
			std::string name_s(name.begin(), name.end());
			light.SetName(name_s.c_str());
			scope.GetScene().Attach(light);

			break;
		}

		case FRPhysicalLight_DIRECTIONAL:
		{
			frw::DirectionalLight light = context.CreateDirectionalLight();
			colour *= watts;
			light.SetRadiantPower(colour.r, colour.g, colour.b);
			light.SetTransform(frTm, false);
			std::wstring name = thisNode->GetName();
			std::string name_s(name.begin(), name.end());
			light.SetName(name_s.c_str());
			scope.GetScene().Attach(light);

			break;
		}
	}
}


FIRERENDER_NAMESPACE_END
