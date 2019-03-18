/*********************************************************************************************************************************
* Radeon ProRender for 3ds Max plugin
* Copyright (c) 2017 AMD
* All Rights Reserved
*
* Implementation of Light-related reactions of the Synchronizer class
*********************************************************************************************************************************/

#include "Synchronizer.h"
#include "CoronaDeclarations.h"
#include "plugin/FireRenderPortalLight.h"
#include "plugin/ParamBlock.h"
#include "plugin/BgManager.h"
#include <lslights.h>
#include <ICustAttribContainer.h>
#include <fstream>

FIRERENDER_NAMESPACE_BEGIN;

//////////////////////////////////////////////////////////////////////////////
// Builds a RPR light out of a MAX light
//

void Synchronizer::RebuildMaxLight(INode *node, Object *evaluatedObject)
{
	// remove the light's counterpart from RPR scene (if exists)
	auto ll = mLights.find(node);
	if (ll != mLights.end())
	{
		mScope.GetScene().Detach(ll->second->Get());
		mLights.erase(ll);
	}
	else
	{
		auto el = mLightShapes.find(node);
		if (el != mLightShapes.end())
		{
			mScope.GetScene().Detach(el->second->Get());
			mLightShapes.erase(el);
		}
	}
	
	auto tm = node->GetObjTMAfterWSM(mBridge->t());
	tm.SetTrans(tm.GetTrans() * mMasterScale);
	BOOL parity = tm.Parity();

	auto t = mBridge->t();

	auto context = mScope.GetContext();

	// Try to match a RPR point light object to 3ds Max native light shader.
	// note: Corona area lights are handled separately.
#ifndef DISABLE_MAX_LIGHTS

	GenLight* light = dynamic_cast<GenLight*>(evaluatedObject);
	if (!light || (light && !light->GetUseLight())) {
		return;
	}

	float intensity = light->GetIntensity(t);

	rpr_int res = 0;
	rpr_light fireLight = 0; // output light. A specific type used is determined by the input light type

	// PHOTOMETRIC LIGHTS

	if (LightscapeLight *phlight = dynamic_cast<LightscapeLight*>(evaluatedObject))
	{
		BOOL visibleInRender = TRUE;

		if (auto cont = evaluatedObject->GetCustAttribContainer())
		{
			for (int i = 0; i < cont->GetNumCustAttribs(); i++)
			{
				if (auto * ca = cont->GetCustAttrib(i))
				{
					if (0 == wcscmp(ca->GetName(), L"Area Light Sampling Custom Attribute"))
					{
						if (auto aca = phlight->GetAreaLightCustAttrib(ca))
							visibleInRender = aca->IsLightShapeRenderingEnabled(t);
						break;
					}
				}
			}
		}

		int renderable = phlight->IsRenderable();

		intensity = phlight->GetResultingIntensity(t); // intensity  in Cd

		int type = phlight->Type();

		if (phlight->GetDistribution() == LightscapeLight::SPOTLIGHT_DIST)
		{
			if (type != LightscapeLight::TARGET_POINT_TYPE && type != LightscapeLight::POINT_TYPE)
			{
				MessageBox(0, L"Spotlight Distribution is supported with Point light shape only", L"Radeon ProRender Error", MB_ICONERROR | MB_OK);
			}
		}

		switch (phlight->GetDistribution())
		{
		case LightscapeLight::ISOTROPIC_DIST:
		{
			if (type == LightscapeLight::TARGET_POINT_TYPE || type == LightscapeLight::POINT_TYPE)
			{
				res = rprContextCreatePointLight(context.Handle(), &fireLight);
				FCHECK_CONTEXT(res, context.Handle(), "rprContextCreatePointLight");

				// isotropic spherical light source, total surface yields 4PI steradians,
				// but for some reason FR wants PI instead of 4PI, the engine is probably making some assumptions.
				float lumens = PI * intensity;
				float watts = lumens / 683.f; // photopic
				Point3 color = phlight->GetRGBFilter(t) * phlight->GetRGBColor(t) * watts;

				res = rprPointLightSetRadiantPower3f(fireLight, color.x, color.y, color.z);
				FCHECK(res);
			}
			else if (type == LightscapeLight::AREA_TYPE || type == LightscapeLight::TARGET_AREA_TYPE)
			{
				float width = phlight->GetWidth(t) * mMasterScale;
				float length = phlight->GetLength(t) * mMasterScale;
				float w2 = width * 0.5f;
				float h2 = length * 0.5f;

				Point3 points[8] = {
					Point3(-w2, -h2, 0.0005f),
					Point3(w2, -h2, 0.0005f),
					Point3(w2, h2, 0.0005f),
					Point3(-w2, h2, 0.0005f),

					Point3(-w2, -h2, -0.0005f),
					Point3(w2, -h2, -0.0005f),
					Point3(w2, h2, -0.0005f),
					Point3(-w2, h2, -0.0005f)
				};

				Point3 normals[2] = {
					Point3(0.f, 0.f, 1.f),
					Point3(0.f, 0.f, -1.f)
				};

				rpr_int indices[] = {
					0, 1, 2,
					0, 2, 3,
					6, 5, 4,
					7, 6, 4
				};

				rpr_int normal_indices[] = {
					0, 0, 0,
					0, 0, 0,
					1, 1, 1,
					1, 1, 1
				};

				rpr_int num_face_vertices[] = { 3, 3, 3, 3 };

				auto shape = context.CreateMesh(
					(rpr_float const*)&points[0], 8, sizeof(Point3),
					(rpr_float const*)&normals[0], 2, sizeof(Point3),
					nullptr, 0, 0,
					(rpr_int const*)indices, sizeof(rpr_int),
					(rpr_int const*)normal_indices, sizeof(rpr_int),
					nullptr, 0,
					num_face_vertices, 4);

				shape.SetTransform(tm);

				auto ms = mtlParser.materialSystem;
				frw::EmissiveShader material(ms);

				float lumens = phlight->GetResultingFlux(t);
				float watts = lumens / 683.f;

				float area = 2.f * width * length;
				watts /= area;

				Point3 color = phlight->GetRGBFilter(t) * phlight->GetRGBColor(t) * watts;
				material.SetColor(frw::Value(color.x, color.y, color.z));
				shape.SetShader(material);

				shape.SetShadowFlag(false);
				shape.SetPrimaryVisibility( bool_cast(visibleInRender) );

				auto ll = mLightShapes.find(node);
				if (ll != mLightShapes.end())
					mLightShapes.erase(ll);
				mLightShapes.insert(std::make_pair(node, new SShape(shape, node)));

				SetNameFromNode(node, shape);
				mScope.GetScene().Attach(shape);

				return;
			}
			else if (type == LightscapeLight::DISC_TYPE || type == LightscapeLight::TARGET_DISC_TYPE)
			{
				int numpoints = phlight->GetShape(0, 0);
				std::vector<Point3> points((numpoints << 1) + 2);
				phlight->GetShape(&points[0], numpoints);
				for (int i = 0; i < numpoints; i++)
				{
					points[i] *= mMasterScale;
					points[numpoints + i] = points[i];
					points[i].z += 0.0005f;
					points[numpoints + i].z -= 0.0005f;
				}

				int centerTop = int_cast( points.size() - 1 );
				int centerBottom = centerTop - 1;
				points[centerTop] = Point3(0.f, 0.f, 0.0005f);
				points[centerBottom] = Point3(0.f, 0.f, -0.0005f);

				std::vector<int> indices;
				std::vector<int> normal_indices;
				std::vector<int> faces;

				for (int i = 0; i < numpoints; i++)
				{
					int next = (i + 1) < numpoints ? (i + 1) : 0;

					indices.push_back(i); normal_indices.push_back(0);
					indices.push_back(centerTop); normal_indices.push_back(0);
					indices.push_back(next); normal_indices.push_back(0);
					faces.push_back(3);

					indices.push_back(i + numpoints); normal_indices.push_back(1);
					indices.push_back(next + numpoints); normal_indices.push_back(1);
					indices.push_back(centerBottom); normal_indices.push_back(1);
					faces.push_back(3);
				}

				Point3 normals[2] = {
					Point3(0.f, 0.f, 1.f),
					Point3(0.f, 0.f, -1.f)
				};

				auto shape = context.CreateMesh(
					(rpr_float const*)&points[0], points.size(), sizeof(Point3),
					(rpr_float const*)&normals[0], 2, sizeof(Point3),
					nullptr, 0, 0,
					(rpr_int const*)&indices[0], sizeof(rpr_int),
					(rpr_int const*)&normal_indices[0], sizeof(rpr_int),
					nullptr, 0,
					&faces[0], faces.size());

				shape.SetTransform(tm);

				auto ms = mtlParser.materialSystem;
				frw::EmissiveShader material(ms);

				float lumens = phlight->GetResultingFlux(t);
				float watts = lumens / 683.f;

				float radius = phlight->GetRadius(t) * mMasterScale;;
				float area = 2.f * PI * radius * radius; // both sides so we do not divide by two
				watts /= area;

				Point3 color = phlight->GetRGBFilter(t) * phlight->GetRGBColor(t) * watts;
				material.SetColor(frw::Value(color.x, color.y, color.z));
				shape.SetShader(material);

				shape.SetShadowFlag(false);
				shape.SetPrimaryVisibility( bool_cast(visibleInRender) );

				auto ll = mLightShapes.find(node);
				if (ll != mLightShapes.end())
					mLightShapes.erase(ll);
				mLightShapes.insert(std::make_pair(node, new SShape(shape, node)));

				SetNameFromNode(node, shape);
				mScope.GetScene().Attach(shape);

				return;
			}
			else if (type == LightscapeLight::SPHERE_TYPE || type == LightscapeLight::TARGET_SPHERE_TYPE)
			{
				float radius = phlight->GetRadius(t) * mMasterScale;
				int nbLong = 16;
				int nbLat = 16;

				std::vector<Point3> vertices((nbLong + 1) * nbLat + 2);
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

				std::vector<Point3> normals(vsize);
				for (size_t n = 0; n < vsize; n++)
					normals[n] = Normalize(vertices[n]);

				// triangles
				int nbTriangles = nbLong * 2 + (nbLat - 1)*nbLong * 2;//2 caps and one middle
				int nbIndexes = nbTriangles * 3;
				std::vector<int> triangles(nbIndexes);

				//Top Cap
				int i = 0;
				for (int lon = 0; lon < nbLong; lon++)
				{
					triangles[i++] = lon + 2;
					triangles[i++] = lon + 1;
					triangles[i++] = 0;
				}

				//Middle
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

				//Bottom Cap
				for (int lon = 0; lon < nbLong; lon++)
				{
					// TODO: triangles is int32 but vsize is int64, change vsize of int32?
					triangles[i++] = int_cast(vsize - 1);
					triangles[i++] = int_cast(vsize - (lon + 2) - 1);
					triangles[i++] = int_cast(vsize - (lon + 1) - 1);
				}

				std::vector<int> normal_indices;
				for (int i = 0; i < nbIndexes; i++)
					normal_indices.push_back(triangles[i]);

				std::vector<int> faces(nbTriangles);
				for (size_t i = 0; i < nbTriangles; i++)
					faces[i] = 3;

				auto shape = context.CreateMesh(
					(rpr_float const*)&vertices[0], vsize, sizeof(Point3),
					(rpr_float const*)&normals[0], vsize, sizeof(Point3),
					nullptr, 0, 0,
					(rpr_int const*)&triangles[0], sizeof(rpr_int),
					(rpr_int const*)&normal_indices[0], sizeof(rpr_int),
					nullptr, 0,
					&faces[0], nbTriangles);

				shape.SetTransform(tm);

				auto ms = mtlParser.materialSystem;

				frw::EmissiveShader material(ms);

				float lumens = phlight->GetResultingFlux(t);
				float watts = lumens / 683.f;

				float area = 4 * PI * radius * radius;
				watts /= area;

				Point3 color = phlight->GetRGBFilter(t) * phlight->GetRGBColor(t) * watts;
				material.SetColor(frw::Value(color.x, color.y, color.z));
				shape.SetShader(material);

				shape.SetShadowFlag(false);
				shape.SetPrimaryVisibility( bool_cast(visibleInRender) );

				auto ll = mLightShapes.find(node);
				if (ll != mLightShapes.end())
					mLightShapes.erase(ll);
				mLightShapes.insert(std::make_pair(node, new SShape(shape, node)));

				SetNameFromNode(node, shape);
				mScope.GetScene().Attach(shape);

				return;
			}
			else if (type == LightscapeLight::CYLINDER_TYPE || type == LightscapeLight::TARGET_CYLINDER_TYPE)
			{
				int segments = 16;
				float length = phlight->GetLength(t) * mMasterScale;
				float hlength = length * 0.5f;
				float radius = phlight->GetRadius(t) * mMasterScale;
				Point3 centerTop(0.f, -hlength, 0.f);
				Point3 centerBottom(0.f, hlength, 0.f);
				float step = (2.f * PI) / float(segments);
				float angle = 0.f;
				std::vector<Point3> pshape;
				for (int i = 0; i < segments; i++, angle += step)
					pshape.push_back(Point3(radius * sin(angle), 0.f, radius * cos(angle)));

				int numPointsShape = int_cast(pshape.size());
				int numPoints = numPointsShape << 1;
				std::vector<Point3> points(numPoints);
				std::vector<Point3> normals(numPoints);
				for (int i = 0; i < numPointsShape; i++)
				{
					points[i] = Point3(pshape[i].x, centerTop.y, pshape[i].z);
					normals[i] = Normalize(points[i] - centerTop);
					points[i + numPointsShape] = Point3(pshape[i].x, centerBottom.y, pshape[i].z);
					normals[i + numPointsShape] = Normalize(points[i + numPointsShape] - centerBottom);
				}

				std::vector<int> indices;
				std::vector<int> normal_indices;
				std::vector<int> faces;

				// sides

				for (int i = 0; i < numPointsShape; i++)
				{
					indices.push_back(i + numPointsShape);
					indices.push_back(i);
					int next = (i + 1) < numPointsShape ? (i + 1) : 0;
					indices.push_back(next);
					faces.push_back(3);

					indices.push_back(next);
					indices.push_back(next + numPointsShape);
					indices.push_back(i + numPointsShape);
					faces.push_back(3);
				}

				for (auto ii = indices.begin(); ii != indices.end(); ii++)
					normal_indices.push_back(*ii);

				auto shape = context.CreateMesh(
					(rpr_float const*)&points[0], points.size(), sizeof(Point3),
					(rpr_float const*)&normals[0], normals.size(), sizeof(Point3),
					nullptr, 0, 0,
					(rpr_int const*)&indices[0], sizeof(rpr_int),
					(rpr_int const*)&normal_indices[0], sizeof(rpr_int),
					nullptr, 0,
					&faces[0], faces.size());

				shape.SetTransform(tm);

				auto ms = mtlParser.materialSystem;
				frw::EmissiveShader material(ms);

				float lumens = phlight->GetResultingFlux(t);
				float watts = lumens / 683.f;

				float area = 2 * PI * radius * length;
				watts /= area;

				Point3 color = phlight->GetRGBFilter(t) * phlight->GetRGBColor(t) * watts;
				material.SetColor(frw::Value(color.x, color.y, color.z));
				shape.SetShader(material);

				shape.SetShadowFlag(false);
				shape.SetPrimaryVisibility( bool_cast(visibleInRender) );

				auto ll = mLightShapes.find(node);
				if (ll != mLightShapes.end())
					mLightShapes.erase(ll);
				mLightShapes.insert(std::make_pair(node, new SShape(shape, node)));

				SetNameFromNode(node, shape);
				mScope.GetScene().Attach(shape);

				return;
			}
			else if (type == LightscapeLight::LINEAR_TYPE || type == LightscapeLight::TARGET_LINEAR_TYPE)
			{
				int segments = 16;
				float length = phlight->GetLength(t) * mMasterScale;
				float hlength = length * 0.5f;
				float radius = 0.003f;
				Point3 centerTop(0.f, -hlength, 0.f);
				Point3 centerBottom(0.f, hlength, 0.f);
				float step = (2.f * PI) / float(segments);
				float angle = 0.f;
				std::vector<Point3> pshape;
				for (int i = 0; i < segments; i++, angle += step)
					pshape.push_back(Point3(radius * sin(angle), 0.f, radius * cos(angle)));

				int numPointsShape = int_cast(pshape.size());
				int numPoints = numPointsShape << 1;
				std::vector<Point3> points(numPoints);
				std::vector<Point3> normals(numPoints);
				for (int i = 0; i < numPointsShape; i++)
				{
					points[i] = Point3(pshape[i].x, centerTop.y, pshape[i].z);
					normals[i] = Normalize(points[i] - centerTop);
					points[i + numPointsShape] = Point3(pshape[i].x, centerBottom.y, pshape[i].z);
					normals[i + numPointsShape] = Normalize(points[i + numPointsShape] - centerBottom);
				}

				std::vector<int> indices;
				std::vector<int> normal_indices;
				std::vector<int> faces;

				// sides

				for (int i = 0; i < numPointsShape; i++)
				{
					indices.push_back(i + numPointsShape);
					indices.push_back(i);
					int next = (i + 1) < numPointsShape ? (i + 1) : 0;
					indices.push_back(next);
					faces.push_back(3);

					indices.push_back(next);
					indices.push_back(next + numPointsShape);
					indices.push_back(i + numPointsShape);
					faces.push_back(3);
				}

				for (auto ii = indices.begin(); ii != indices.end(); ii++)
					normal_indices.push_back(*ii);

				auto shape = context.CreateMesh(
					(rpr_float const*)&points[0], points.size(), sizeof(Point3),
					(rpr_float const*)&normals[0], normals.size(), sizeof(Point3),
					nullptr, 0, 0,
					(rpr_int const*)&indices[0], sizeof(rpr_int),
					(rpr_int const*)&normal_indices[0], sizeof(rpr_int),
					nullptr, 0,
					&faces[0], faces.size());

				shape.SetTransform(tm);

				auto ms = mtlParser.materialSystem;
				frw::EmissiveShader material(ms);

				float lumens = phlight->GetResultingFlux(t);
				float watts = lumens / 683.f;

				float area = 2 * PI * radius * length;
				watts /= area;

				Point3 color = phlight->GetRGBFilter(t) * phlight->GetRGBColor(t) * watts;
				material.SetColor(frw::Value(color.x, color.y, color.z));
				shape.SetShader(material);

				shape.SetShadowFlag(false);
				shape.SetPrimaryVisibility( bool_cast(visibleInRender) );

				auto ll = mLightShapes.find(node);
				if (ll != mLightShapes.end())
					mLightShapes.erase(ll);
				mLightShapes.insert(std::make_pair(node, new SShape(shape, node)));

				SetNameFromNode(node, shape);
				mScope.GetScene().Attach(shape);

				return;
			}
			else
				return; // unsupported by FR at the moment
		}
		break;

		case LightscapeLight::SPOTLIGHT_DIST:
		{
			if (type == LightscapeLight::TARGET_POINT_TYPE || type == LightscapeLight::POINT_TYPE)
			{
				res = rprContextCreateSpotLight(context.Handle(), &fireLight);
				FCHECK_CONTEXT(res, context.Handle(), "rprContextCreateSpotLight");
				const float iAngle = DEG_TO_RAD*light->GetHotspot(t) * 0.5f;
				const float oAngle = DEG_TO_RAD*light->GetFallsize(t) * 0.5f;
				
				// maxlumens = phlight->GetResultingFlux(t);
				// apex is the angle between the vector at which flux is 100% and the one at which it is 50%
				// apex = ????; RPR does not seem to have a linear exponential angular decay
				// steradians = 2.f * PI * (1.f - cos(apex));
				// lumens = intensity * steradians;
				// watts = lumens / ((683.f * steradians) / (4.f * PI));

				float lumens = phlight->GetResultingFlux(t);
				float steradians = lumens / intensity;
				float watts = lumens / ((1700.f * steradians) / (4.f * PI)); // scotopic
				Point3 color = phlight->GetRGBFilter(t) * phlight->GetRGBColor(t) * watts;

				res = rprSpotLightSetRadiantPower3f(fireLight, color.x, color.y, color.z);
				FCHECK(res);
				res = rprSpotLightSetConeShape(fireLight, iAngle, oAngle);
				FCHECK(res);
			}
			// other types are currently not supported by RPR
			else
				return;
		}
		break;

		case LightscapeLight::WEB_DIST:
		{
			if (type == LightscapeLight::TARGET_POINT_TYPE || type == LightscapeLight::POINT_TYPE)
			{
				try {
					std::string iesData((std::istreambuf_iterator<char>(std::ifstream(phlight->GetFullWebFileName()))),
						std::istreambuf_iterator<char>());

					// we don't need to be brutal, web lights can have an empty filename especially while being created and active shade is on
					if (!iesData.empty())
					{
						res = rprContextCreateIESLight(context.Handle(), &fireLight);
						FCHECK_CONTEXT(res, context.Handle(), "rprContextCreateIESLight");

						res = rprIESLightSetImageFromIESdata(fireLight, iesData.c_str(), 256, 256);
						if (RPR_SUCCESS != res) {
							throw std::runtime_error("rprIESLightSetImageFromIESdata failed with code:" + std::to_string(res));
						}

						// we are passing a simple scale factor (1 Lm to 1 W) however we are missing geometrical
						// information for an accurate conversion. Specifically we would need the light's Apex angle.
						// Perhaps the rendering engine could return it as a light query option. Or perhaps it could
						// handle the conversion altogether, so the rprIESLightSetRadiantPower3f would actually be used
						// to pass an optional dimming/scaling factor and coloration.
						Point3 originalColor = phlight->GetRGBFilter(t);
						Point3 color = originalColor * ((intensity / 682.069f) / 683.f) / 2.5f;

						res = rprIESLightSetRadiantPower3f(fireLight, color.x, color.y, color.z);

						FCHECK(res);

						Matrix3 r;
						r.IdentityMatrix();
						r.PreRotateX(-DegToRad(phlight->GetWebRotateX()));
						r.PreRotateY(DegToRad(phlight->GetWebRotateY()));
						r.PreRotateZ(DegToRad(phlight->GetWebRotateZ()));

						// rotation doesn't affect ies lights in RPR
						// however:
						// - use GetWebRotateX/Y/Z 
						// - translate rotation from MAX to RPR

						Matrix3 tx;
						tx.IdentityMatrix();
						// flip yz
						tx.PreRotateX(M_PI_2);
						tx.PreRotateY(M_PI);

						tm = tx*r*tm;
					}
				}
				catch (std::exception& error)
				{
					if (fireLight)
						fireLight = 0;
					std::string errorMsg(error.what());

					MessageBox(0,
						(std::wstring(node->GetName()) + L" - failed to create IES light:\n" + std::wstring(errorMsg.begin(), errorMsg.end())).c_str(), L"Radeon ProRender Error", MB_ICONERROR | MB_OK);
				}

			}
			else
			{
				MessageBox(0,
					(std::wstring(node->GetName()) + L" - Web Distribution is supported with Point light shape only").c_str(), L"Radeon ProRender Error", MB_ICONERROR | MB_OK);

				fireLight = 0;
			}
		}
		break;

		case LightscapeLight::DIFFUSE_DIST:
		{
			if (type == LightscapeLight::AREA_TYPE || type == LightscapeLight::TARGET_AREA_TYPE)
			{
				float width = phlight->GetWidth(t) * mMasterScale;
				float length = phlight->GetLength(t) * mMasterScale;
				float w2 = width * 0.5f;
				float h2 = length * 0.5f;

				Point3 points[4] = {
					Point3(-w2, -h2, 0.f),
					Point3(w2, -h2, 0.f),
					Point3(w2, h2, 0.f),
					Point3(-w2, h2, 0.f)
				};

				Point3 normals[2] = {
					Point3(0.f, 0.f, 1.f),
					Point3(0.f, 0.f, -1.f)
				};

				rpr_int indices[] = {
					2, 1, 0,
					3, 2, 0
				};

				rpr_int normal_indices[] = {
					1, 1, 1,
					1, 1, 1
				};

				rpr_int num_face_vertices[] = { 3, 3, 3, 3 };

				auto shape = context.CreateMesh(
					(rpr_float const*)&points[0], 4, sizeof(Point3),
					(rpr_float const*)&normals[0], 2, sizeof(Point3),
					nullptr, 0, 0,
					(rpr_int const*)indices, sizeof(rpr_int),
					(rpr_int const*)normal_indices, sizeof(rpr_int),
					nullptr, 0,
					num_face_vertices, 2);

				shape.SetTransform(tm);

				auto ms = mtlParser.materialSystem;
				frw::EmissiveShader material(ms);

				float lumens = phlight->GetResultingFlux(t);
				float watts = lumens / 683.f;

				float area = 2.f * width * length;
				watts /= area;

				Point3 color = phlight->GetRGBFilter(t) * phlight->GetRGBColor(t) * watts;
				material.SetColor(frw::Value(color.x, color.y, color.z));
				shape.SetShader(material);

				shape.SetShadowFlag(false);
				shape.SetPrimaryVisibility( bool_cast(visibleInRender) );

				auto ll = mLightShapes.find(node);
				if (ll != mLightShapes.end())
					mLightShapes.erase(ll);
				mLightShapes.insert(std::make_pair(node, new SShape(shape, node)));

				SetNameFromNode(node, shape);
				mScope.GetScene().Attach(shape);

				return;
			}
			else if (type == LightscapeLight::DISC_TYPE || type == LightscapeLight::TARGET_DISC_TYPE)
			{
				int numpoints = phlight->GetShape(0, 0);
				std::vector<Point3> points(numpoints + 1);
				phlight->GetShape(&points[0], numpoints);
				for (auto i = points.begin(); i < points.end(); i++)
					*i *= mMasterScale;
				points[numpoints] = Point3(0.f, 0.f, 0.f);

				std::vector<int> indices;
				std::vector<int> faces;
				for (int i = 0; i < numpoints - 1; i++)
				{
					indices.push_back(i);
					indices.push_back(numpoints);
					indices.push_back(i + 1);
					faces.push_back(3);
				}
				indices.push_back(numpoints - 1);
				indices.push_back(numpoints);
				indices.push_back(0);
				faces.push_back(3);

				Point3 normals[1] = {
					Point3(0.f, 0.f, -1.f)
				};

				std::vector<int> normal_indices;
				for (int i = 0; i <= indices.size(); i++)
				{
					normal_indices.push_back(0);
				}

				auto shape = context.CreateMesh(
					(rpr_float const*)&points[0], points.size(), sizeof(Point3),
					(rpr_float const*)&normals[0], 1, sizeof(Point3),
					nullptr, 0, 0,
					(rpr_int const*)&indices[0], sizeof(rpr_int),
					(rpr_int const*)&normal_indices[0], sizeof(rpr_int),
					nullptr, 0,
					&faces[0], faces.size());

				shape.SetTransform(tm);

				auto ms = mtlParser.materialSystem;
				frw::EmissiveShader material(ms);

				float lumens = phlight->GetResultingFlux(t);
				float watts = lumens / 683.f;

				float radius = phlight->GetRadius(t) * mMasterScale;
				float area = PI * radius * radius;
				watts /= area;

				Point3 color = phlight->GetRGBFilter(t) * phlight->GetRGBColor(t) * watts;
				material.SetColor(frw::Value(color.x, color.y, color.z));
				shape.SetShader(material);

				shape.SetShadowFlag(false);
				shape.SetPrimaryVisibility( bool_cast(visibleInRender) );

				auto ll = mLightShapes.find(node);
				if (ll != mLightShapes.end())
					mLightShapes.erase(ll);
				mLightShapes.insert(std::make_pair(node, new SShape(shape, node)));

				SetNameFromNode(node, shape);
				mScope.GetScene().Attach(shape);

				return;
			}
			else if (type == LightscapeLight::POINT_TYPE || type == LightscapeLight::TARGET_POINT_TYPE)
			{
				float radius = 0.01f;
				int nbLong = 8;
				int nbLat = 4;

				std::vector<Point3> vertices((nbLong + 1) * nbLat + 2);
				float _pi = PI;
				float _2pi = _pi * 2.f;

				Point3 vectorup(0.f, 1.f, 0.f);

				vertices[0] = vectorup * radius;
				for (int lat = 0; lat < nbLat; lat++)
				{
					float a1 = 0.5f * _pi * (float)(lat) / (nbLat);
					float sin1 = sin(a1);
					float cos1 = cos(a1);

					for (int lon = 0; lon <= nbLong; lon++)
					{
						float a2 = _2pi * (float)(lon == nbLong ? 0 : lon) / nbLong;
						float sin2 = sin(a2);
						float cos2 = cos(a2);
						vertices[lon + lat * (nbLong + 1) + 1] = Point3(sin1 * cos2, sin1 * sin2, -cos1) * radius;
					}
				}

				// normals
				size_t vsize = vertices.size();

				std::vector<Point3> normals(vsize);
				for (size_t n = 0; n < vsize; n++)
					normals[n] = Normalize(vertices[n]);

				// triangles
				int nbTriangles = nbLong + (nbLat - 1)*nbLong * 2;
				int nbIndexes = nbTriangles * 3;
				std::vector<int> triangles(nbIndexes);

				//vsize = nbLat * (nbLong + 1) + 2
				//nbTriangles = (nbLat * (nbLong + 1) + 2)*2 = nbLat * nbLong*2 + nbLat*2 + 2*2
				//tris_inited = nbLong + (nbLat - 1)*nbLong*2 + nbLong = nbLat*nbLong*2

				//Top Cap
				int i = 0;
				for (int lon = 0; lon < nbLong; lon++)
				{
					triangles[i++] = lon + 2;
					triangles[i++] = lon + 1;
					triangles[i++] = 0;
				}

				//Middle
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


				std::vector<int> normal_indices;
				for (int i = 0; i < nbIndexes; i++)
					normal_indices.push_back(triangles[i]);

				std::vector<int> faces(nbTriangles);
				for (size_t i = 0; i < nbTriangles; i++)
					faces[i] = 3;

				auto shape = context.CreateMesh(
					(rpr_float const*)&vertices[0], vsize, sizeof(Point3),
					(rpr_float const*)&normals[0], vsize, sizeof(Point3),
					nullptr, 0, 0,
					(rpr_int const*)&triangles[0], sizeof(rpr_int),
					(rpr_int const*)&normal_indices[0], sizeof(rpr_int),
					nullptr, 0,
					&faces[0], nbTriangles);

				shape.SetTransform(tm);

				auto ms = mtlParser.materialSystem;

				frw::EmissiveShader material(ms);

				float lumens = phlight->GetResultingFlux(t);
				float watts = lumens / 683.f;

				float area = 4 * PI * radius * radius * 0.5;
				watts /= area;

				Point3 color = phlight->GetRGBFilter(t) * phlight->GetRGBColor(t) * watts;
				material.SetColor(frw::Value(color.x, color.y, color.z));
				shape.SetShader(material);

				shape.SetShadowFlag(false);
				shape.SetPrimaryVisibility( bool_cast(visibleInRender) );

				auto ll = mLightShapes.find(node);
				if (ll != mLightShapes.end())
					mLightShapes.erase(ll);
				mLightShapes.insert(std::make_pair(node, new SShape(shape, node)));

				SetNameFromNode(node, shape);
				mScope.GetScene().Attach(shape);

				return;
			}
			else if (type == LightscapeLight::LINEAR_TYPE || type == LightscapeLight::TARGET_LINEAR_TYPE)
			{
				int segments = 16;
				float length = phlight->GetLength(t) * mMasterScale;
				float hlength = length * 0.5f;
				float radius = 0.01f;//tested wtih CPU  render, smaller radius crates grid patern(like 0.001 - very visible)
				Point3 centerTop(0.f, -hlength, 0.f);
				Point3 centerBottom(0.f, hlength, 0.f);
				float step = (PI) / float(segments);
				float angle = 0.f;
				std::vector<Point3> pshape;
				for (int i = 0; i <= segments; i++, angle += step)
					pshape.push_back(Point3(radius * cos(angle), 0.f, -radius * sin(angle)));

				int numPointsShape = int_cast(pshape.size());
				int numPoints = numPointsShape << 1;
				std::vector<Point3> points(numPoints);
				std::vector<Point3> normals(numPoints);
				for (int i = 0; i < numPointsShape; i++)
				{
					points[i] = Point3(pshape[i].x, centerTop.y, pshape[i].z);
					normals[i] = Normalize(points[i] - centerTop);
					points[i + numPointsShape] = Point3(pshape[i].x, centerBottom.y, pshape[i].z);
					normals[i + numPointsShape] = Normalize(points[i + numPointsShape] - centerBottom);
				}

				std::vector<int> indices;
				std::vector<int> normal_indices;
				std::vector<int> faces;

				// sides

				for (int i = 0; i < (numPointsShape - 1); i++)
				{
					indices.push_back(i + numPointsShape);
					indices.push_back(i);
					int next = i + 1;
					indices.push_back(next);
					faces.push_back(3);

					indices.push_back(next);
					indices.push_back(next + numPointsShape);
					indices.push_back(i + numPointsShape);
					faces.push_back(3);
				}

				for (auto ii = indices.begin(); ii != indices.end(); ii++)
					normal_indices.push_back(*ii);

				auto shape = context.CreateMesh(
					(rpr_float const*)&points[0], points.size(), sizeof(Point3),
					(rpr_float const*)&normals[0], normals.size(), sizeof(Point3),
					nullptr, 0, 0,
					(rpr_int const*)&indices[0], sizeof(rpr_int),
					(rpr_int const*)&normal_indices[0], sizeof(rpr_int),
					nullptr, 0,
					&faces[0], faces.size());

				shape.SetTransform(tm);


				auto ms = mtlParser.materialSystem;
				frw::EmissiveShader material(ms);

				float lumens = phlight->GetResultingFlux(t);
				float watts = lumens / 683.f;

				float area = 2 * PI * radius * length * 0.5;
				watts /= area;

				Point3 color = phlight->GetRGBFilter(t) * phlight->GetRGBColor(t) * watts;
				material.SetColor(frw::Value(color.x, color.y, color.z));
				shape.SetShader(material);

				shape.SetShadowFlag(false);
				shape.SetPrimaryVisibility( bool_cast(visibleInRender) );

				auto ll = mLightShapes.find(node);
				if (ll != mLightShapes.end())
					mLightShapes.erase(ll);
				mLightShapes.insert(std::make_pair(node, new SShape(shape, node)));

				SetNameFromNode(node, shape);
				mScope.GetScene().Attach(shape);

				return;
			}
			return; // currrently not supported
		}
		break;

		default:
			return; // unknown distribution
		}
	}
	else
	{
		mFlags.mUsesNonPMLights = true;

		// STANDARD LIGHTS
		
		Color color = light->GetRGBColor(t) * intensity;

		if (light->IsDir())
		{
			// A directional light (located in infinity)
			res = rprContextCreateDirectionalLight(context.Handle(), &fireLight);
			FCHECK_CONTEXT(res, context.Handle(), "rprContextCreateDirectionalLight");
			color *= PI; // it seems we need to multiply by pi to get same intensity in RPR and 3dsmax
			res = rprDirectionalLightSetRadiantPower3f(fireLight, color.r, color.g, color.b);
			FCHECK(res);
		}
		else if (light->IsSpot())
		{
			// Spotlight (point light with non-uniform directional distribution)
			res = rprContextCreateSpotLight(context.Handle(), &fireLight);
			FCHECK_CONTEXT(res, context.Handle(), "rprContextCreateSpotLight");

			const float iAngle = DEG_TO_RAD*light->GetHotspot(t) * 0.5f;
			const float oAngle = DEG_TO_RAD*light->GetFallsize(t) * 0.5f;
			color *= 683.f * PI; // should be 4PI (steradians in a sphere) doh
			res = rprSpotLightSetRadiantPower3f(fireLight, color.r, color.g, color.b);
			FCHECK(res);
			res = rprSpotLightSetConeShape(fireLight, iAngle, oAngle);
			FCHECK(res);
		}
		else if (evaluatedObject->ClassID() == Class_ID(0x7bf61478, 0x522e4705))
		{
			//standard Skylight object
			res = rprContextCreateSkyLight(context.Handle(), &fireLight);
			FCHECK_CONTEXT(res, context.Handle(), "rprContextCreateSkyLight");
			float intensity = light->GetIntensity(t);
			res = rprSkyLightSetScale(fireLight, (intensity * 0.2f));
			Point3 color = light->GetRGBColor(t);
			rprSkyLightSetAlbedo(fireLight, (color.x + color.y + color.z) * 1.f / 3.f);
			FCHECK(res);
		}
		else
		{
			// point light with uniform directional distribution
			res = rprContextCreatePointLight(context.Handle(), &fireLight);
			FCHECK_CONTEXT(res, context.Handle(), "rprContextCreatePointLight");
			color *= 683.f * PI; // should be 4PI (steradians in a sphere) doh
			res = rprPointLightSetRadiantPower3f(fireLight, color.r, color.g, color.b);
			FCHECK(res);
		}
	}

	if (fireLight)
	{
		SLightPtr light(new SLight(frw::Light(fireLight, context), node));
		light->Get().SetTransform(fxLightTm(tm));
		
		auto ll = mLights.find(node);
		if (ll != mLights.end())
			mLights.erase(ll);
		mLights.insert(std::make_pair(node, light));
		
		SetNameFromNode(node, light->Get());
		mScope.GetScene().Attach(light->Get());
	}

#endif
}

void Synchronizer::RebuildCoronaSun(INode *node, Object* evaluatedObject)
{
	auto context = mScope.GetContext();

	// Approximates the Corona sun with RPR directional light. Uses Corona function publishing interface to obtain the 
	// light parameters.
	Corona::IFireMaxSunInterface* interf = dynamic_cast<Corona::IFireMaxSunInterface*>(evaluatedObject->GetInterface(Corona::IFIREMAX_SUN_INTERFACE));
	if (interf)
	{
		Matrix3 tm;
		Color color;
		float size;
		interf->fmGetParams(mBridge->t(), node, tm, color, size);
		color /= 10000.f; // magic matching constant
		rpr_light light;
		rpr_int res = rprContextCreateDirectionalLight(context.Handle(), &light);
		FCHECK_CONTEXT(res, context.Handle(), "rprContextCreateDirectionalLight");
		res = rprDirectionalLightSetRadiantPower3f(light, color.r, color.g, color.b);
		FCHECK(res);

		float frTm[16];
		CreateFrMatrix(fxLightTm(tm), frTm);
		res = rprLightSetTransform(light, false, frTm);
		FCHECK(res);

		auto fireLight = frw::Light(light, context);
		
		std::wstring name = node->GetName();
		std::string name_s(name.begin(), name.end());
		fireLight.SetName(name_s.c_str());
		mScope.GetScene().Attach(fireLight);

		auto ll = mLights.find(node);
		if (ll != mLights.end())
			mLights.erase(ll);
		mLights.insert(std::make_pair(node, new SLight(fireLight, node)));
	}
	else
	{
		MessageBox(GetCOREInterface()->GetMAXHWnd(), _T("Too old Corona version."), _T("Radeon ProRender warning"), MB_OK);
	}
}

void Synchronizer::RebuildFRPortal(INode* node, Object* evaluatedObject)
{
	auto tm = node->GetObjTMAfterWSM(mBridge->t());
	tm.SetTrans(tm.GetTrans() * mMasterScale);
	BOOL parity = tm.Parity();
	auto context = mScope.GetContext();

	IFireRenderPortalLight *pPortal = dynamic_cast<IFireRenderPortalLight *>(evaluatedObject);
	FASSERT(pPortal);

	Point3 Vertices[4];
	pPortal->GetCorners(Vertices[0], Vertices[1], Vertices[2], Vertices[3]);

	Point3 points[8] = {
		Point3(Vertices[0].x * mMasterScale, Vertices[0].y * mMasterScale, Vertices[0].z * mMasterScale),
		Point3(Vertices[1].x * mMasterScale, Vertices[1].y * mMasterScale, Vertices[1].z * mMasterScale),
		Point3(Vertices[2].x * mMasterScale, Vertices[2].y * mMasterScale, Vertices[2].z * mMasterScale),
		Point3(Vertices[3].x * mMasterScale, Vertices[3].y * mMasterScale, Vertices[3].z * mMasterScale),

		Point3(Vertices[0].x * mMasterScale, Vertices[0].y * mMasterScale, Vertices[0].z * mMasterScale),
		Point3(Vertices[1].x * mMasterScale, Vertices[1].y * mMasterScale, Vertices[1].z * mMasterScale),
		Point3(Vertices[2].x * mMasterScale, Vertices[2].y * mMasterScale, Vertices[2].z * mMasterScale),
		Point3(Vertices[3].x * mMasterScale, Vertices[3].y * mMasterScale, Vertices[3].z * mMasterScale),
	};

	Point3 normals[2] = {
		Point3(0.f, 0.f, 1.f),
		Point3(0.f, 0.f, -1.f)
	};

	rpr_int indices[] = {
		0, 1, 2,
		0, 2, 3,
		6, 5, 4,
		7, 6, 4
	};

	rpr_int normal_indices[] = {
		0, 0, 0,
		0, 0, 0,
		1, 1, 1,
		1, 1, 1
	};

	rpr_int num_face_vertices[] = { 3, 3, 3, 3 };

	auto shape = context.CreateMesh(
		(rpr_float const*)&points[0], 8, sizeof(Point3),
		(rpr_float const*)&normals[0], 2, sizeof(Point3),
		nullptr, 0, 0,
		(rpr_int const*)indices, sizeof(rpr_int),
		(rpr_int const*)normal_indices, sizeof(rpr_int),
		nullptr, 0,
		num_face_vertices, 2 //using only 2 triangles to have our portal mesh one-sided and portal to work onl from one side therefore
	);

	shape.SetTransform(tm);

	auto ms = mtlParser.materialSystem;

	shape.SetShadowFlag(false);

	SetNameFromNode(node, shape);

	auto pp = mPortals.find(node);
	if (pp != mPortals.end())
		mPortals.erase(pp);
	mPortals.insert(std::make_pair(node, shape));
}

void Synchronizer::RemoveEnvironment()
{
	mScope.GetScene().SetBackgroundReflectionsOverride(frw::EnvironmentLight());
	mScope.GetScene().SetBackgroundRefractionsOverride(frw::EnvironmentLight());

	if (!mBgLight.empty())
		mScope.GetScene().Detach(mBgLight[0]);
	if (!mBgReflLight.empty())
		mScope.GetScene().Detach(mBgReflLight[0]);
	if (!mBgRefrLight.empty())
		mScope.GetScene().Detach(mBgRefrLight[0]);
	mBgLight.clear();
	mBgReflLight.clear();
	mBgRefrLight.clear();
	mBgIBLImage.clear();
	mBgReflImage.clear();
	mBgRefrlImage.clear();
}

frw::Image Synchronizer::CreateColorEnvironment(const Point3 &color)
{
	if (avg(color) > 0.f)
	{
		// we need a constant-value environment map. Since RPR does not currently support it directly, we do a work-around
		// by creating a single-pixel image of required color.

		rpr_image_desc imgDesc;
		imgDesc.image_width = 2;
		imgDesc.image_height = 2;
		imgDesc.image_depth = 1;
		imgDesc.image_row_pitch = imgDesc.image_width * sizeof(rpr_float) * 4;
		imgDesc.image_slice_pitch = imgDesc.image_row_pitch * imgDesc.image_height;

		rpr_float imgData[16];
		for (int i = 0; i < 16; i += 4)
		{
			imgData[i] = color.x;
			imgData[i + 1] = color.y;
			imgData[i + 2] = color.z;
			imgData[i + 3] = 1;
		}

		return frw::Image(mScope.GetContext(), { 4, RPR_COMPONENT_TYPE_FLOAT32 }, imgDesc, imgData);
	}

	return frw::Image();
}

bool Synchronizer::MAXEnvironmentNeedsUpdate()
{
	Texmap *bgtex = GetCOREInterface()->GetEnvironmentMap();
	BOOL bgtexuse = GetCOREInterface()->GetUseEnvironmentMap();
	Point3 bgColor = GetCOREInterface()->GetBackGround(mBridge->t(), Interval());
	HashValue bgHash;
	std::map<Animatable*, HashValue> hash_visited;
	DWORD syncTimestamp = 0;
	if (bgtex && bgtexuse)
		bgHash << mtlParser.GetHashValue(bgtex, mBridge->t(), hash_visited, syncTimestamp);

	bool rebuild = mMAXEnvironmentForceRebuild;
	mMAXEnvironmentForceRebuild = false;

	if (!rebuild)
	{
		if (bgtex != mMAXEnvironment || bgtexuse != mMAXEnvironmentUse)
		{
			rebuild = true;
		}
		else
		{
			if (bgtexuse)
			{
				if (bgHash != mMAXEnvironmentHash)
					rebuild = true;
			}
			else
			{
				if (abs(mEnvironmentColor.x - bgColor.x) > 0.001f || abs(mEnvironmentColor.y - bgColor.y) > 0.001f || abs(mEnvironmentColor.z - bgColor.z) > 0.001f)
					rebuild = true;
			}
		}
	}

	if (rebuild)
	{
		mMAXEnvironmentUse = bgtexuse;
		mMAXEnvironment = bgtex;
		mEnvironmentColor = bgColor;
		mMAXEnvironmentHash = bgHash;
	}

	return rebuild;
}

void Synchronizer::UpdateMAXEnvironment()
{
	if (mBridge->GetProgressCB())
		mBridge->GetProgressCB()->SetTitle(_T("Synchronizing: Environment"));

	RemoveEnvironment();
	UpdateEnvironmentImage();

	if (mBridge->GetProgressCB())
		mBridge->GetProgressCB()->SetTitle(_T(""));
}

void Synchronizer::UpdateEnvironmentImage()
{
	frw::Image enviroImage;

	if (mMAXEnvironmentUse && mMAXEnvironment)
		enviroImage = mtlParser.createImageFromMap(mMAXEnvironment, MAP_FLAG_WANTSHDR);
	else
		enviroImage = CreateColorEnvironment(mEnvironmentColor);

	if (!enviroImage)
	{
		return;
	}

	mBgIBLImage.push_back(enviroImage);
	auto enviroLight = mScope.GetContext().CreateEnvironmentLight();
	mBgLight.push_back(enviroLight);

	float scale = 1.f;

	if (mMAXEnvironment)
	{
		if (auto map = dynamic_cast<BitmapTex*>(mMAXEnvironment->GetInterface(BITMAPTEX_INTERFACE)))
		{
			if (auto output = map->GetTexout())
				scale *= output->GetOutputLevel(map->GetStartTime()); // this is in reality the RGB level
		}
	}

	enviroLight.SetLightIntensityScale(scale);
	enviroLight.SetImage(enviroImage);
	mScope.GetScene().Attach(enviroLight);

	Matrix3 tx;
	tx.IdentityMatrix();
	float angle = 0;
	// flip yz
	tx.PreRotateX(M_PI_2);
	tx.PreRotateY(M_PI);
	if (angle != 0)
		tx.PreRotateY(angle);
	enviroLight.SetTransform(tx);

	// PORTAL LIGHTS
	// Do not remove the comments below
	// std::vector<frw::Shape> &portals = parsed.frEnvironment.portals;
	// if (parsed.frEnvironment.enabled && !portals.empty())
	// {
	// 	for (auto shape = portals.begin(); shape != portals.end(); shape++) {
	// 		enviroLight.AttachPortal(*shape);
	// 	}
	// }
}

void Synchronizer::AddRPREnvironment()
{
	UpdateRPREnvironment();
}

void Synchronizer::RemoveRPREnvironment()
{
	RemoveEnvironment();
	mMAXEnvironmentUse = false;
	mMAXEnvironment = 0;
	mEnvironmentColor = Point3(0, 0, 0);
	mMAXEnvironmentHash = HashValue();
	mRPREnvironmentHash = HashValue();
	mMAXEnvironmentForceRebuild = true;
}

void Synchronizer::UpdateRPREnvironment()
{
	auto env = BgManagerMax::TheManager.GetEnvironmentNode();
	if (!env)
		return;

	RemoveEnvironment();

	if (mBridge->GetProgressCB())
		mBridge->GetProgressCB()->SetTitle(_T("Synchronizing: Environment"));

	auto t = mBridge->t();
	auto pblock = mBridge->GetPBlock();

	int bgType = 0;
	pblock->GetValue(TRPARAM_BG_TYPE, t, bgType, FOREVER);

	BOOL bgUsebgMap = FALSE;
	BOOL bgUsereflMap = FALSE;
	BOOL bgUserefrMap = FALSE;
	Texmap *bgMap = 0;
	Texmap *bgReflMap = 0;
	Texmap *bgRefrMap = 0;
	Color envColor;

	float bgIntensity = 1.f;
	if (bgType == FRBackgroundType_IBL)
	{
		pblock->GetValue(TRPARAM_BG_IBL_MAP_USE, t, bgUsebgMap, FOREVER);
		pblock->GetValue(TRPARAM_BG_IBL_REFLECTIONMAP_USE, t, bgUsereflMap, FOREVER);
		pblock->GetValue(TRPARAM_BG_IBL_REFRACTIONMAP_USE, t, bgUserefrMap, FOREVER);
		if (bgUsebgMap)
			pblock->GetValue(TRPARAM_BG_IBL_MAP, t, bgMap, FOREVER);
		if (bgUsereflMap)
			pblock->GetValue(TRPARAM_BG_IBL_REFLECTIONMAP, t, bgReflMap, FOREVER);
		if (bgUserefrMap)
			pblock->GetValue(TRPARAM_BG_IBL_REFRACTIONMAP, t, bgRefrMap, FOREVER);
		pblock->GetValue(TRPARAM_BG_IBL_INTENSITY, t, bgIntensity, FOREVER);
	}
	else
	{
		pblock->GetValue(TRPARAM_BG_SKY_REFLECTIONMAP_USE, t, bgUsereflMap, FOREVER);
		pblock->GetValue(TRPARAM_BG_SKY_REFRACTIONMAP_USE, t, bgUserefrMap, FOREVER);
		if (bgUsereflMap)
			pblock->GetValue(TRPARAM_BG_SKY_REFLECTIONMAP, t, bgReflMap, FOREVER);
		if (bgUserefrMap)
			pblock->GetValue(TRPARAM_BG_SKY_REFRACTIONMAP, t, bgRefrMap, FOREVER);
		pblock->GetValue(TRPARAM_BG_SKY_INTENSITY, t, bgIntensity, FOREVER);
	}

	const ObjectState& state = env->EvalWorldState(t);
	auto tm = env->GetObjTMAfterWSM(t);
	tm.SetTrans(tm.GetTrans() * mMasterScale);

	pblock->GetValue(TRPARAM_BG_IBL_SOLIDCOLOR, t, envColor, FOREVER);

	bool sky = (bgType == FRBackgroundType_SunSky);
		
	Texmap *bgBackplate = 0;
	BOOL bgUseBackplate = FALSE;

	if (bgType == FRBackgroundType_IBL)
	{
		pblock->GetValue(TRPARAM_BG_IBL_BACKPLATE_USE, t, bgUseBackplate, FOREVER);
		if (bgUseBackplate)
			pblock->GetValue(TRPARAM_BG_IBL_BACKPLATE, t, bgBackplate, FOREVER);
	}
	else
	{
		pblock->GetValue(TRPARAM_BG_SKY_BACKPLATE_USE, t, bgUseBackplate, FOREVER);
		if (bgUseBackplate)
			pblock->GetValue(TRPARAM_BG_SKY_BACKPLATE, t, bgBackplate, FOREVER);
	}
		
	float backgroundIntensity = 1.0f;

	//////////////////////////////////////////////////////////////////////
	//

	auto context = mScope.GetContext();

	frw::Image enviroImage;		// an image that if non-null will be used as scene environment light
	frw::Image enviroReflImage;	// an image that if non-null will be used as environment reflections override light
	frw::Image enviroRefrImage;	// an image that if non-null will be used as environment refractions override light

	Texmap *enviroReflMap = 0;
	Texmap *enviroRefrMap = 0;

	bool useEnvColor = false;

	float scale = 1.f;

	// use sun-sky
	if (sky)
	{
		enviroImage = BgManagerMax::TheManager.GenerateSky(mScope, pblock, t, bgIntensity);
	}
	else // use IBL
	{
		if (bgMap)
		{
			enviroImage = mtlParser.createImageFromMap(bgMap, MAP_FLAG_WANTSHDR);
		}
		else
			useEnvColor = true;
	}
	scale *= bgIntensity;

	if (bgReflMap)
	{
		enviroReflMap = bgReflMap;
		enviroReflImage = mtlParser.createImageFromMap(bgReflMap, MAP_FLAG_NOFLAGS);
	}
	if (bgRefrMap)
	{
		enviroRefrMap = bgRefrMap;
		enviroRefrImage = mtlParser.createImageFromMap(bgRefrMap, MAP_FLAG_NOFLAGS);
	}

	if (useEnvColor && avg(envColor) > 0.f)
	{
		// we need a constant-value environment map. Since RPR does not currently support it directly, we do a work-around
		// by creating a single-pixel image of required color.

		rpr_image_desc imgDesc;
		imgDesc.image_width = 2;
		imgDesc.image_height = 2;
		imgDesc.image_depth = 1;
		imgDesc.image_row_pitch = imgDesc.image_width * sizeof(rpr_float) * 4;
		imgDesc.image_slice_pitch = imgDesc.image_row_pitch * imgDesc.image_height;

		rpr_float imgData[16];
		for (int i = 0; i < 16; i += 4)
		{
			imgData[i] = envColor.r;
			imgData[i + 1] = envColor.g;
			imgData[i + 2] = envColor.b;
			imgData[i + 3] = 1;
		}

		enviroImage = frw::Image(context, { 4, RPR_COMPONENT_TYPE_FLOAT32 }, imgDesc, imgData);
	}

	frw::EnvironmentLight enviroLight;
	frw::EnvironmentLight enviroReflLight;
	frw::EnvironmentLight enviroRefrLight;
		
	// Create the environment light if there is any environment present
	if (enviroImage || enviroReflImage || enviroRefrImage)
	{
		if (enviroImage)
		{
			enviroLight = context.CreateEnvironmentLight();
			mBgLight.push_back(enviroLight);
			mBgIBLImage.push_back(enviroImage);
		}
		if (enviroReflImage)
		{
			enviroReflLight = context.CreateEnvironmentLight();
			mBgReflLight.push_back(enviroReflLight);
			mBgReflImage.push_back(enviroReflImage);
		}
		if (enviroRefrImage)
		{
			enviroRefrLight = context.CreateEnvironmentLight();
			mBgRefrLight.push_back(enviroRefrLight);
			mBgRefrlImage.push_back(enviroRefrImage);
		}

		if (bgMap)
		{
			if (auto map = dynamic_cast<BitmapTex*>(bgMap->GetInterface(BITMAPTEX_INTERFACE)))
			{
				if (auto output = map->GetTexout())
				{
					scale *= output->GetOutputLevel(map->GetStartTime()); // this is in reality the RGB level
				}
			}
		}

		if (enviroImage)
		{
			enviroLight.SetLightIntensityScale(scale);
			enviroLight.SetImage(enviroImage);
		}
		if (enviroReflImage)
			enviroReflLight.SetImage(enviroReflImage);
		if (enviroRefrImage)
			enviroRefrLight.SetImage(enviroRefrImage);

		{
			Matrix3 tx;
			tx = tm;
			float angle = 0;
			if (sky)
			{
				// analytical sky, should use sun's azimuth
				float sunAzimuth, sunAltitude;
				BgManagerMax::TheManager.GetSunPosition(pblock, t, sunAzimuth, sunAltitude);
				angle = -sunAzimuth / 180.0f * M_PI - M_PI * 0.5;
			}
			// flip yz
			tx.PreRotateX(M_PI_2);
			tx.PreRotateY(M_PI);
			if (angle != 0)
				tx.PreRotateY(angle);
			if (enviroImage)
				enviroLight.SetTransform(tx);
			if (enviroReflImage)
				enviroReflLight.SetTransform(tx);
			if (enviroRefrImage)
				enviroRefrLight.SetTransform(tx);
		}

		if (enviroImage)
		{
			mScope.GetScene().Attach(enviroLight);
		}
		if (enviroReflImage)
		{
			mScope.GetScene().SetBackgroundReflectionsOverride(enviroReflLight);
		}
		if (enviroRefrImage)
		{
			mScope.GetScene().SetBackgroundRefractionsOverride(enviroRefrLight);
		}

		// PORTAL LIGHTS
		// Do not remove the comments below
		//std::vector<frw::Shape> &portals = parsed.frEnvironment.portals;
		//if (parsed.frEnvironment.enabled && !portals.empty())
		//{
		//	for (auto shape = portals.begin(); shape != portals.end(); shape++) {
		//		enviroLight.AttachPortal(*shape);
		//	}
		//}
	}

	// Backplate
	if (bgBackplate)
	{	
		auto map = bgBackplate;
	
		Matrix3 matTm;
		matTm.IdentityMatrix();
	
		// explicitly support only Screen and Spherical mapping
		// Background(IBL) is used for spherical mapping
		// BackgroundImage(backplate) used for other mappings(same as Screen)

		bool useIBL = false;
		if (auto uvGen = map->GetTheUVGen())
		{
			uvGen->GetUVTransform(matTm);
	
			bool isMappingTypeEnvironment = (uvGen->GetSlotType() == MAPSLOT_ENVIRON);
	
			if (isMappingTypeEnvironment)
			{
				if (auto gen = dynamic_cast<StdUVGen*>(uvGen)) {
					useIBL = UVMAP_SPHERE_ENV == gen->GetCoordMapping(0);
				}
			}
		}
	
		auto image = mtlParser.createImageFromMap(map, MAP_FLAG_WANTSHDR);
	
		if (useIBL)
		{
			auto ibl = context.CreateEnvironmentLight();
	
			float scale = backgroundIntensity;
			if (auto bimaptex = dynamic_cast<BitmapTex*>(map->GetInterface(BITMAPTEX_INTERFACE)))
			{
				if (auto output = bimaptex->GetTexout())
				{
					scale *= output->GetOutputLevel(bimaptex->GetStartTime()); // this is in reality the RGB level
				}
			}
	
			ibl.SetLightIntensityScale(scale);
			ibl.SetImage(image);
	
			{
				Matrix3 tx = tm;
	
				// flip yz
				tx.PreRotateX(M_PI_2);
				tx.PreRotateY(M_PI);
	
				ibl.SetTransform(tx);
			}
	
			mScope.GetScene().SetBackground(ibl);
			mScope.GetScene().SetBackgroundImage(frw::Image());
		}
		else
		{
			mScope.GetScene().SetBackground(frw::EnvironmentLight());
			mScope.GetScene().SetBackgroundImage(image);
		}
	}
	else
	{
		mScope.GetScene().SetBackground(frw::EnvironmentLight());
		mScope.GetScene().SetBackgroundImage(frw::Image());
	}
}

void Synchronizer::UpdateEnvXForm(INode *pNode)
{
	Matrix3 tm;

	auto env = BgManagerMax::TheManager.GetEnvironmentNode();

	auto t = mBridge->t();
	auto pblock = mBridge->GetPBlock();

	if (pNode)
	{
		const ObjectState& state = pNode->EvalWorldState(t);
		auto tm = env->GetObjTMAfterWSM(t);
		tm.SetTrans(tm.GetTrans() * mMasterScale);
	}
	else
		tm.IdentityMatrix();

	int bgType = 0;
	pblock->GetValue(TRPARAM_BG_TYPE, t, bgType, FOREVER);

	Matrix3 tx;
	tx = tm;
	float angle = 0;
	if (bgType == FRBackgroundType_SunSky)
	{
		// analytical sky, should use sun's azimuth
		float sunAzimuth, sunAltitude;
		BgManagerMax::TheManager.GetSunPosition(pblock, t, sunAzimuth, sunAltitude);
		angle = -sunAzimuth / 180.0f * M_PI;
	}
	// flip yz
	tx.PreRotateX(M_PI_2);
	tx.PreRotateY(M_PI);
	if (angle != 0)
		tx.PreRotateY(angle);

	if (!mBgLight.empty())
		mBgLight[0].SetTransform(tx);
	if (!mBgReflLight.empty())
		mBgReflLight[0].SetTransform(tx);
	if (!mBgRefrLight.empty())
		mBgRefrLight[0].SetTransform(tx);
}

void Synchronizer::AddDefaultLights()
{
	int numLights = 0;
	auto lights = mBridge->GetDefaultLights(numLights);
	if (numLights && lights)
	{
		for (int i = 0; i < numLights; i++)
		{
			auto state = lights[i].ls;
			switch (state.type)
			{
				case DIRECT_LGT:
				{
					auto context = mScope.GetContext();

					auto light = context.CreateDirectionalLight();

					Color color = state.color;
					color *= PI * state.intens; // mapping intensity to RPR requires this
					light.SetRadiantPower(color.r, color.g, color.b);

					light.SetTransform(fxLightTm(lights[i].tm));

					mScope.GetScene().Attach(light);

					SLightPtr slight(new SLight(light, 0));
					mDefaultLights.push_back(slight);
				}
				break;
			}
			
		}
	}
}

void Synchronizer::RemoveDefaultLights()
{
	for (auto l : mDefaultLights)
		mScope.GetScene().Detach(l->Get());
	mDefaultLights.clear();
}

FIRERENDER_NAMESPACE_END;
