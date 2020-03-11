/**********************************************************************
Copyright 2020 Advanced Micro Devices, Inc
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at
    http://www.apache.org/licenses/LICENSE-2.0
Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
********************************************************************/

#include "Synchronizer.h"
#include "CoronaDeclarations.h"
#include <MeshNormalSpec.h>
#include "FireRenderMaterialMtl.h"
#include "FireRenderUberMtl.h"
#include "FireRenderDisplacementMtl.h"
#include "ScopeManager.h"
#include <list>

FIRERENDER_NAMESPACE_BEGIN;

#define GEOM_DISABLED_MATERIAL (Mtl*)1

namespace
{
	std::vector<Mtl*> GetAllMaterials(INode *node, const TimeValue &t)
	{
		auto mtl = node->GetMtl();

		std::vector<Mtl*> result;
		if (!mtl || !mtl->IsMultiMtl())
			result.push_back(mtl);	// note: we allow nullptr
		else
		{
			result.resize(mtl->NumSubMtls());
			const Class_ID cid = mtl->ClassID();
			IParamBlock2 *pb = 0;
			if (cid == MULTI_MATERIAL_CLASS_ID)
				pb = mtl->GetParamBlock(0);
			for (int i = 0; i < mtl->NumSubMtls(); i++)
			{
				BOOL enabled = TRUE;
				if (pb)
					pb->GetValue(0x01, t, enabled, FOREVER, i);
				if (enabled)
					result[i] = mtl->GetSubMtl(i);
				else
					result[i] = GEOM_DISABLED_MATERIAL;
			}
		}
		return result;
	}
};


bool Synchronizer::CreateMesh(std::vector<frw::Shape>& result, bool& directlyVisible,
	frw::Context& context,
	float masterScale,
	TimeValue timeValue, View& view,
	INode* inode, Object* evaluatedObject, const int numSubmtls, size_t& meshFaces, bool flipFaces)
{
	Mesh* mesh = NULL;
	BOOL needsDelete = FALSE; // If GetRenderMesh sets this to true, we will need to delete the mesh when we are done
	Mesh dummyMeshCopy; // Used only if we need to copy the mesh when getting normals

	// First, we try to obtain render mesh from the object
	if (auto geomObject = dynamic_cast<GeomObject*>(evaluatedObject))
	{
		// expensive!
		mesh = geomObject->GetRenderMesh(timeValue, inode, view, needsDelete);
	}

	directlyVisible = inode->GetPrimaryVisibility() != FALSE;
	if (evaluatedObject->ClassID() == Corona::LIGHT_CID)
	{
		if (ScopeManagerMax::CoronaOK)
		{
		// Corona lights require some special processing - we will get its mesh via function publishing interface.
		// But when we have it, we can parse the mesh as any other geometry object, because we also handle their the lights 
		// material specially in MaterialParser.
		directlyVisible &= GetFromPb<bool>(evaluatedObject->GetParamBlockByID(0), Corona::L_DIRECTLY_VISIBLE);
		BaseInterface* bi = evaluatedObject->GetInterface(Corona::IFIREMAX_LIGHT_INTERFACE);
		Corona::IFireMaxLightInterface* lightInterface = dynamic_cast<Corona::IFireMaxLightInterface*>(bi);
		if (lightInterface)
			lightInterface->fmGetRenderMesh(timeValue, mesh, needsDelete);
		else
			MessageBox(GetCOREInterface()->GetMAXHWnd(), _T("Too old Corona version."), _T("Radeon ProRender warning"), MB_OK);
	}
	}

	if (!mesh)  //nothing to do here
		return false;

	meshFaces = mesh->getNumFaces();

	// Next we try to get normals associated with the mesh
	MeshNormalSpec* normals = nullptr;

	// Handle normals: in case we need to delete the mesh, we are free to modify it, so we just do by calling SpecifyNormals()
	if (needsDelete)
	{
		mesh->SpecifyNormals();
		normals = mesh->GetSpecifiedNormals();
	}
	else
	{
		// Otherwise we will try to get the normals without modifying the mesh. We try it...
		normals = mesh->GetSpecifiedNormals();
		if (normals == NULL || normals->GetNumNormals() == 0)
		{
			// ... and if it fails, we will have to copy the mesh, modify it by calling SpecifyNormals(), and get the normals 
			// from the mesh copy
			dummyMeshCopy = *mesh;
			dummyMeshCopy.SpecifyNormals();
			normals = dummyMeshCopy.GetSpecifiedNormals();
		}
	}
	FASSERT(normals);
	normals->CheckNormals();

	// Copy all vertices, normals, and texture coordinates from 3ds Max to our buffers
	Stack<Point3> v, n;				// all vertices, normals of the mesh
	Stack<Stack<int>> vi, ni;		// materialID[vertex/normals[]]

	Stack<Stack<Point3>> t;			// mapChannel[all texture coords of the mesh]
	Stack<Stack<Stack<int>>> ti;	// mapChannel[materialID[texcoords[]]]

	// Swap comment to enable SECONDARY UV CHANNEL
	//int numChannels = mesh->tVerts == NULL ? 0 : mesh->getNumMaps() - 1;
	int numChannels = mesh->tVerts == NULL ? 0 : 1;

	vi.resize(numSubmtls);
	ni.resize(numSubmtls);
	ti.resize(numChannels);
	t.resize(numChannels);

	for (int i = 0; i < numChannels; ++i)
		ti[i].resize(numSubmtls);

	v.resize(mesh->getNumVerts());
	for (int i = 0; i < mesh->getNumVerts(); ++i)
		v[i] = mesh->verts[i] * masterScale;

	n.resize(normals->GetNumNormals());
	for (int i = 0; i < normals->GetNumNormals(); ++i)
		n[i] = normals->Normal(i).Normalize();

	const bool textured = mesh->tVerts != NULL;
	for (int i = 0; i < numChannels; i++)
	{
		MeshMap& mapChannel = mesh->maps[i + 1]; // maps[0] is vertex color, leave it

		if (textured)
		{
			t[i].resize(mapChannel.vnum);
			for (int j = 0; j < mapChannel.vnum; ++j)
			{
				UVVert tv = mapChannel.tv[j];
				if (flipFaces)
					tv.x = 1.f - tv.x;
				t[i][j] = tv;
			}
		}
	}

	// Now create triangle index arrays. We push each triangle into appropriate "bin" based on its material ID
	int faceCount = mesh->getNumFaces();
	if (faceCount != 0)
	{
		for (int i = 0; i < numSubmtls; ++i)
		{
			vi[i].reserve(faceCount * 3);
			ni[i].reserve(faceCount * 3);
			for (int j = 0; j < numChannels; ++j)
				ti[j][i].reserve(faceCount * 3 * 3);
		}
	}

	for (int i = 0; i < mesh->getNumFaces(); ++i)
	{
		Face& face = mesh->faces[i];
		const int mtlId = face.getMatID() % numSubmtls;
		for (int j = 0; j < 3; ++j) {
#ifdef SWITCH_AXES
			int index = 0; // we swap 2 indices because we change parity via TM switch
			if (j == 1) {
				index = 2;
			}
			else if (j == 2) {
				index = 1;
			}
#else
			const int index = j;
#endif

			vi[mtlId].push(face.getVert(index));
			ni[mtlId].push(normals->GetNormalIndex(i, index));

			for (int k = 0; k < numChannels; ++k)
			{
				MeshMap& mapChannel = mesh->maps[k + 1]; // maps[0] is vertex color, leave it
				if (textured)
					ti[k][mtlId].push(mapChannel.tf[i].t[index]);
			}
		}
	}

	// Create a dummy array holding numbers of vertices for each face (which is always "3" in our case)
	int maxFaceCount = 0;
	for (auto& i : vi)
		maxFaceCount = std::max(maxFaceCount, i.size() / 3);
	Stack<int> vertNums;
	vertNums.resize(maxFaceCount);
	for (auto& i : vertNums)
		i = 3;

	// Finally, create RPR shape objects.

	for (int i = 0; i < numSubmtls; ++i)
	{
		if (vi[i].size() == 0)
		{
			// we still need to push something so the array of materials created elsewhere matches the array of shapes
			result.push_back(frw::Shape());
		}
		else
		{
			auto currMeshFaces = vi[i].size() / 3;

			size_t texcoordsNum[2] = { 0, 0 };
			rpr_int texcoordStride[2] = { 0, 0 };
			rpr_int texcoordIdxStride[2] = { 0, 0 };

			const rpr_float* texcoords[2] = { nullptr, nullptr };
			const rpr_int* texcoordIndices[2] = { nullptr, nullptr };

			if (mesh->numTVerts > 0)
			{
				for (int j = 0; j < numChannels; ++j)
				{
					const bool bPaddingVertices = true;

					// Padding texture vertices
					if (bPaddingVertices)
					{
						t[j].resize(mesh->numTVerts);
						texcoordsNum[j] = mesh->numTVerts;
					}
					else
					{
						texcoordsNum[j] = mesh->maps[j + 1].vnum;
					}

					texcoords[j] = t[j][0];
					texcoordIndices[j] = &ti[j][i][0];
					texcoordStride[j] = sizeof(Point3);
					texcoordIdxStride[j] = sizeof(rpr_int);
				}
			}

			auto shape = context.CreateMeshEx(
				v[0], v.size(), sizeof(v[0]),
				n[0], n.size(), sizeof(n[0]),
				nullptr, 0, 0,
				numChannels, texcoords, texcoordsNum, texcoordStride,
				&vi[i][0], sizeof(vi[i][0]),
				&ni[i][0], sizeof(ni[i][0]),
				texcoordIndices, texcoordIdxStride,
				&vertNums[0],
				currMeshFaces);

			result.push_back(shape);
		}
	}

	if (needsDelete) {
		mesh->DeleteThis();
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////////
// Parses a single mesh object. Outputs a list of shapes because the mesh can
// have multiple material IDs, which RPR cannot handle in single shape.
// parameters:
// inode - Scene node where the mesh was encountered
// evaluatedObject - result of calling EvalWorldState on inode
// numSubmtls - Number of submaterials that the node material holds. If it is
//              more than 1 (it uses multi-material) and the mesh has multiple
//              material IDs, we will have to create multiple shapes.
//

std::vector<frw::Shape> Synchronizer::parseMesh(INode* inode, Object* evaluatedObject, const int numSubmtls, size_t& meshFaces, bool flipFaces)
{
	meshFaces = 0;
	std::vector<frw::Shape> result;
	if (!evaluatedObject)
		return result;

	auto context = mScope.GetContext();
	FASSERT(evaluatedObject != NULL && inode != NULL);

	auto t = mBridge->t();
		
	// hopefully we can get this info
	int numFaces = 0, numVerts = 0;
	if (evaluatedObject->PolygonCount(t, numFaces, numVerts))
		meshFaces = numFaces;
	else
		DebugPrint(L"no face count\n");

	FireRenderView view;
	
	bool directlyVisible;
	if (!CreateMesh(result, directlyVisible,
	context, mMasterScale,
	t, view, inode, evaluatedObject, numSubmtls, meshFaces, flipFaces))
		return std::vector<frw::Shape>();

	for (auto& shape : result)
	{
		if (!directlyVisible)
			shape.SetVisibility(false);
	}

	return result;
}

void Synchronizer::RebuildGeometry(const std::list<INode *> &nodes)
{
	int numMtls = 0;

	auto t = mBridge->t();

	for (auto& parsedNode : nodes)
	{
		RemoveMaterialsFromNode(parsedNode);
		numMtls = std::max(numMtls, int_cast(GetAllMaterials(parsedNode, t).size()) );
	}

	// Evaluate the mesh of first node in the group
	auto firstNode = *nodes.begin();
	const ObjectState& state = firstNode->EvalWorldState(t);
	size_t currMeshFaces = 0;
	
	auto tm = firstNode->GetObjTMAfterWSM(mBridge->t());
	tm.SetTrans(tm.GetTrans() * mMasterScale);
	BOOL parity = tm.Parity();

	auto originalShapes = parseMesh(firstNode, state.obj, numMtls, currMeshFaces, !!parity);
	if (originalShapes.size() == 0)
		return; // evaluating the object yielded no faces - e.g. the object was empty

	FASSERT(originalShapes.size() == numMtls);

	// iterate over all instances inside the group
	for (auto& parsedNode : nodes)
	{
		std::vector<frw::Shape> shapes;
		// For the first instance we directly reuse the parsed shapes, for subsequent ones we use instances
		if (parsedNode != firstNode)
		{
			for (int i = 0; i < numMtls; ++i)
			{
				if (originalShapes[i])
				{
					auto shape = originalShapes[i].CreateInstance(mScope);
					shapes.push_back(shape);
				}
				else
					shapes.push_back(frw::Shape());
			}
		}
		else
			shapes = originalShapes;

		// associate this node to the RPR shapes
		auto mm = mShapes.find(parsedNode);
		if (mm != mShapes.end())
		{
			for (auto ss : mm->second)
				mScope.GetScene().Detach(ss->Get());
			mShapes.erase(mm);
		}
						
		//motion blur
		// wip: do not remove.
		//Motion motion = getMotion(view, parsedNode);

		const auto& nodeMtls = GetAllMaterials(parsedNode, t);
		FASSERT(numMtls == shapes.size());

		// now go over all mtl IDs, set transforms and materials for shapes and handle special cases
		for (size_t i = 0; i < numMtls; ++i)
		{
			if (auto shape = shapes[i])
			{
				auto tm = parsedNode->GetObjTMAfterWSM(t);
				tm.SetTrans(tm.GetTrans() * mMasterScale);
				shape.SetTransform(tm);

				Mtl* currentMtl = nodeMtls[std::min(i, nodeMtls.size() - 1)];

				float minHeight = 0.f;
				float maxHeight = 0.0f;
				float subdivision = 0.f;
				float creaseWeight = 0.f;
				int boundary = RPR_SUBDIV_BOUNDARY_INTERFOP_TYPE_EDGE_AND_CORNER;

				bool shadowCatcher = false;
				bool castsShadows = true;

				// Handling of some special flags is necessary here at geometry level
				if (!currentMtl)
				{
					// intentionally void
				}
				else if (currentMtl == GEOM_DISABLED_MATERIAL)
				{
					// intentionally void
				}
				else if (currentMtl && currentMtl->ClassID() == Corona::MTL_CID)
				{
					if (ScopeManagerMax::CoronaOK)
					{
					IParamBlock2* pb = currentMtl->GetParamBlock(0);
					const bool useCaustics = GetFromPb<bool>(pb, Corona::MTLP_USE_CAUSTICS, t);
					const float lRefract = GetFromPb<float>(pb, Corona::MTLP_LEVEL_REFRACT, t);
					const float lOpacity = GetFromPb<float>(pb, Corona::MTLP_LEVEL_OPACITY, t);
					
					if ((lRefract > 0.f || lOpacity < 1.f) && !useCaustics)
						castsShadows = false;
				}
				}
				else if (currentMtl && currentMtl->ClassID() == FIRERENDER_MATERIALMTL_CID)
				{
					IParamBlock2* pb = currentMtl->GetParamBlock(0);
					castsShadows = bool_cast( GetFromPb<BOOL>(pb, FRMaterialMtl_CAUSTICS, t) );
					shadowCatcher = bool_cast( GetFromPb<BOOL>(pb, FRMaterialMtl_SHADOWCATCHER, t) );
				}
				else if (currentMtl && currentMtl->ClassID() == FIRERENDER_UBERMTL_CID)
				{
					IParamBlock2* pb = currentMtl->GetParamBlock(0);
					castsShadows = bool_cast( GetFromPb<BOOL>(pb, FRUBERMTL_FRUBERCAUSTICS, t) );
					shadowCatcher = bool_cast( GetFromPb<BOOL>(pb, FRUBERMTL_FRUBERSHADOWCATCHER, t) );
				}
				else if (currentMtl && currentMtl->ClassID() == Corona::SHADOW_CATCHER_MTL_CID)
				{
					if (ScopeManagerMax::CoronaOK)
					shadowCatcher = true;
				}

				frw::Value displImageNode;
				bool notAccurate = false;
				if (currentMtl && currentMtl != GEOM_DISABLED_MATERIAL)
					displImageNode = FRMTLCLASSNAME(DisplacementMtl)::translateDisplacement(t, mtlParser, currentMtl,
						minHeight, maxHeight, subdivision, creaseWeight, boundary, notAccurate);

				if (displImageNode && shape.IsUVCoordinatesSet())
				{
					if (notAccurate)
					{
						mFlags.mHasDirectDisplacements = true;
					}

					shape.SetDisplacement(displImageNode, minHeight, maxHeight);
					shape.SetSubdivisionFactor(subdivision);
					shape.SetSubdivisionCreaseWeight(creaseWeight);
					shape.SetSubdivisionBoundaryInterop(boundary);
				}
				else
				{
					shape.RemoveDisplacement();
				}

				shape.SetShadowFlag(castsShadows);
				shape.SetShadowCatcherFlag(false);

				frw::Shader volumeShader;
				if (currentMtl && currentMtl != GEOM_DISABLED_MATERIAL)
				{
					auto ii = mVolumeShaderCache.find(currentMtl);
					if (ii != mVolumeShaderCache.end())
						volumeShader = ii->second;
					else
					{
					volumeShader = mtlParser.findVolumeMaterial(currentMtl);
				if (volumeShader)
							mVolumeShaderCache.insert(std::make_pair(currentMtl, volumeShader));
					}
					if (volumeShader)
					shape.SetVolumeShader(volumeShader);
					else
						shape.SetVolumeShader(frw::Shader());
				}

				SShapePtr sshape(new SShape(shape, parsedNode, currentMtl));

				frw::Shader shader;
				if (currentMtl)
				{
					auto ii = mShaderCache.find(currentMtl);
					if (ii != mShaderCache.end())
						shader = ii->second;
				}

				if (!currentMtl)
				{
					if (!shader)
					{
						// This geometry is using its wireframe color
						shader = frw::DiffuseShader(mtlParser.materialSystem);
						COLORREF color = parsedNode->GetWireColor();
						// COLORREF format = 0x00BBGGRR
						float r = float(color & 0x000000ff) * 1.f / 255.f;
						float g = float((color & 0x0000ff00) >> 8) * 1.f / 255.f;
						float b = float((color & 0x00ff0000) >> 16) * 1.f / 255.f;
						shader.SetValue(RPR_MATERIAL_INPUT_COLOR, frw::Value(r, g, b));
					}
					shape.SetShader(shader);
				}
				else if (currentMtl == GEOM_DISABLED_MATERIAL)
				{
					if (!shader)
					{
						// disabled materials appear black
						shader = frw::DiffuseShader(mtlParser.materialSystem);
						shader.SetValue(RPR_MATERIAL_INPUT_COLOR, frw::Value(0.f));
						mShaderCache.insert(std::make_pair(currentMtl, shader));
					}
					shape.SetShader(shader);
				}
				else
				{
					if (!shader)
					{
						if (shader = CreateShader(currentMtl, parsedNode, false))
						{
							mShaderCache.insert(std::make_pair(currentMtl, shader));
						}
					}
					
					if (!shader)
					{
						shader = frw::DiffuseShader(mtlParser.materialSystem);
						shader.SetValue(RPR_MATERIAL_INPUT_COLOR, frw::Value(0., 0., 0.)); // signal something's wrong
						mShaderCache.insert(std::make_pair(currentMtl, shader));
					}

					// assign the shader to the RPR shape
					shape.SetShader(shader);

					// add to the material users list
					if (currentMtl)
					{
						auto mm = mMaterialUsers.find(currentMtl);
						if (mm != mMaterialUsers.end())
						{
							if (mm->second.find(sshape) == mm->second.end())
								mm->second.insert(sshape);
						}
						else
						{
							std::set<SShapePtr> uu;
							uu.insert(sshape);
							mMaterialUsers.insert(std::make_pair(currentMtl, uu));
						}
					}
				}

				// associated this shape to the parent node
				auto nn = mShapes.find(parsedNode);
				if (nn != mShapes.end())
				{
					nn->second.push_back(sshape);
				}
				else
				{
					std::vector<SShapePtr> vv;
					vv.push_back(sshape);
					mShapes.insert(std::make_pair(parsedNode, vv));
				}

				std::wstring name = parsedNode->GetName();
				std::string name_s(name.begin(), name.end());
				shape.SetName(name_s.c_str());
				
				mScope.GetScene().Attach(shape);

				// placeholder code: do not remove
				// Set any shape with portal material as RPR portal
				//if (currentMtl && currentMtl->ClassID() == Corona::PORTAL_MTL_CID && enviroLight)
				//	if (ScopeManagerMax::CoronaOK)
				//	enviroLight.SetPortal(shape);
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////////
// this utility is called when an object is about to be deleted or rebuilt
//

void Synchronizer::RemoveMaterialsFromNode(INode *node, bool *wasMulti)
{
	if (wasMulti)
		*wasMulti = false;
	auto ss = mShapes.find(node);
	if (ss != mShapes.end())
	{
		// remove node's shapes from previous users lists
		std::vector<std::pair <Mtl*, SShapePtr>> toRemove;
		for (auto shape : ss->second)
		{
			for (auto mm : mMaterialUsers)
			{
				auto mu = mm.second.find(shape);
				if (mu != mm.second.end())
					toRemove.push_back(std::make_pair(mm.first, shape));
			}
		}
		for (auto rr : toRemove)
		{
			auto mm = mMaterialUsers.find(rr.first);
			if (mm != mMaterialUsers.end())
			{
				auto ss = mm->second.find(rr.second);
				if (ss != mm->second.end())
				{
					mm->second.erase(ss);
					if (mm->second.empty())
					{
						auto ee = mEmissives.find(mm->first);
						if (ee != mEmissives.end())
							mEmissives.erase(ee);
						auto sc = mShaderCache.find(mm->first);
						if (sc != mShaderCache.end())
							mShaderCache.erase(sc);
						auto vc = mVolumeShaderCache.find(mm->first);
						if (vc != mVolumeShaderCache.end())
							mVolumeShaderCache.erase(vc);
						mMaterialUsers.erase(mm);
					}
				}
			}
		}
		if (wasMulti)
			*wasMulti = toRemove.size() > 1;
	}
}

//////////////////////////////////////////////////////////////////////////////
// This utility is called when a material has been modified, hence we want
// to re-create its RPR shader(s) for a given node.
//

bool Synchronizer::ResetMaterial(INode *node)
{
	bool wasMultiple;
	RemoveMaterialsFromNode(node, &wasMultiple);

	auto ss = mShapes.find(node);
	if (ss != mShapes.end())
	{
		Mtl *currentMtl = node->GetMtl();
		bool isMultiple = bool_cast( currentMtl->IsMultiMtl() );

		if (isMultiple || wasMultiple)
		{
			if (isMultiple)
			{
				if (mMultiMats.find(currentMtl) == mMultiMats.end())
				{
					for (int i = 0; i < currentMtl->NumSubMtls(); i++)
					{
						auto sub = currentMtl->GetSubMtl(i);
						mMultiMats[currentMtl].push_back(sub);
					}
				}
			}
			
			// this object needs to be rebuilt: returning false will cause
			// synchronizer to add a rebuild command for the given node.
			return false;
		}

		// now we know it's only one user
		auto shape = *ss->second.begin();

		shape->mMtl = currentMtl;

		TimeValue t = mBridge->t();

		float minHeight = 0.f;
		float maxHeight = 0.0f;
		float subdivision = 0.f;
		float creaseWeight = 0.f;
		int boundary = RPR_SUBDIV_BOUNDARY_INTERFOP_TYPE_EDGE_AND_CORNER;

		bool shadowCatcher = false;
		bool castsShadows = true;

		// Handling of some special flags for Corona Material is necessary here at geometry level
		if (currentMtl == GEOM_DISABLED_MATERIAL)
		{
			// intentionally void
		}
		else if (currentMtl && currentMtl->ClassID() == Corona::MTL_CID)
		{
			if (ScopeManagerMax::CoronaOK)
			{
			IParamBlock2* pb = currentMtl->GetParamBlock(0);
			const bool useCaustics = GetFromPb<bool>(pb, Corona::MTLP_USE_CAUSTICS, t);
			const float lRefract = GetFromPb<float>(pb, Corona::MTLP_LEVEL_REFRACT, t);
			const float lOpacity = GetFromPb<float>(pb, Corona::MTLP_LEVEL_OPACITY, t);

			//~mc hide for now as we have a flag for shadows and caustics
			if ((lRefract > 0.f || lOpacity < 1.f) && !useCaustics) {
				castsShadows = false;
			}
		}
		}
		else if (currentMtl && currentMtl->ClassID() == FIRERENDER_MATERIALMTL_CID)
		{
			IParamBlock2* pb = currentMtl->GetParamBlock(0);
			castsShadows = bool_cast( GetFromPb<BOOL>(pb, FRMaterialMtl_CAUSTICS, t) );
			shadowCatcher = bool_cast( GetFromPb<BOOL>(pb, FRMaterialMtl_SHADOWCATCHER, t) );
		}
		else if (currentMtl && currentMtl->ClassID() == FIRERENDER_UBERMTL_CID)
		{
			IParamBlock2* pb = currentMtl->GetParamBlock(0);
			castsShadows = bool_cast( GetFromPb<BOOL>(pb, FRUBERMTL_FRUBERCAUSTICS, t) );
			shadowCatcher = bool_cast( GetFromPb<BOOL>(pb, FRUBERMTL_FRUBERSHADOWCATCHER, t) );
		}
		else if (currentMtl && currentMtl->ClassID() == Corona::SHADOW_CATCHER_MTL_CID)
		{
			if (ScopeManagerMax::CoronaOK)
			shadowCatcher = true;
		}

		frw::Value displImageNode;
		bool notAccurate = false;
		if (currentMtl != GEOM_DISABLED_MATERIAL)
			displImageNode = FRMTLCLASSNAME(DisplacementMtl)::translateDisplacement(t, mtlParser, currentMtl,
				minHeight, maxHeight, subdivision, creaseWeight, boundary, notAccurate);

		if (displImageNode && shape->Get().IsUVCoordinatesSet())
		{
			if (notAccurate)
			{
				mFlags.mHasDirectDisplacements = true;
			}

			shape->Get().SetDisplacement(displImageNode, minHeight, maxHeight);
			shape->Get().SetSubdivisionFactor(subdivision);
			shape->Get().SetSubdivisionCreaseWeight(creaseWeight);
			shape->Get().SetSubdivisionBoundaryInterop(boundary);
		}
		else
		{
			shape->Get().RemoveDisplacement();
		}

		shape->Get().SetShadowFlag(castsShadows);
		shape->Get().SetShadowCatcherFlag(false);

		frw::Shader volumeShader;
		if (currentMtl != GEOM_DISABLED_MATERIAL)
		{
			auto ii = mVolumeShaderCache.find(currentMtl);
			if (ii != mVolumeShaderCache.end())
				volumeShader = ii->second;
			else
			{
			volumeShader = mtlParser.findVolumeMaterial(currentMtl);
		if (volumeShader)
					mVolumeShaderCache.insert(std::make_pair(currentMtl, volumeShader));
			}
			if (volumeShader)
			shape->Get().SetVolumeShader(volumeShader);
			else
				shape->Get().SetVolumeShader(frw::Shader());
		}

		frw::Shader shader;
		auto ii = mShaderCache.find(currentMtl);
		if (ii != mShaderCache.end())
			shader = ii->second;
		// note: when a shader is picked from the cache, it might need
		// to be re-inserted in the emissives list

		if (!currentMtl)
		{
			if (!shader)
			{
				// this object is using its wireframe color
				shader = frw::DiffuseShader(mtlParser.materialSystem);
				COLORREF color = node->GetWireColor();
				// COLORREF format = 0x00BBGGRR
				float r = float(color & 0x000000ff) * 1.f / 255.f;
				float g = float((color & 0x0000ff00) >> 8) * 1.f / 255.f;
				float b = float((color & 0x00ff0000) >> 16) * 1.f / 255.f;
				shader.SetValue(RPR_MATERIAL_INPUT_COLOR, frw::Value(r, g, b));
			}
			shape->Get().SetShader(shader);
		}
		else if (currentMtl == GEOM_DISABLED_MATERIAL)
		{
			if (!shader)
			{
				shader = frw::DiffuseShader(mtlParser.materialSystem);
				shader.SetValue(RPR_MATERIAL_INPUT_COLOR, frw::Value(0.f));
				mShaderCache.insert(std::make_pair(currentMtl, shader));
			}
			shape->Get().SetShader(shader);
		}
		else
		{
			if (!shader)
			{
				if (shader = CreateShader(currentMtl, node, true))
				{
					mShaderCache.insert(std::make_pair(currentMtl, shader));
				}
			}

			if (!shader)
			{
				shader = frw::DiffuseShader(mtlParser.materialSystem);
				shader.SetValue(RPR_MATERIAL_INPUT_COLOR, frw::Value(0., 0., 0.));
				mShaderCache.insert(std::make_pair(currentMtl, shader));
			}

			// assign the shader to teh RPR shape
			shape->Get().SetShader(shader);

			// add to the material users list
			if (currentMtl)
			{
				auto mm = mMaterialUsers.find(currentMtl);
				if (mm != mMaterialUsers.end())
				{
					if (mm->second.find(shape) == mm->second.end())
						mm->second.insert(shape);
				}
				else
				{
					std::set<SShapePtr> uu;
					uu.insert(shape);
					mMaterialUsers.insert(std::make_pair(currentMtl, uu));
				}
			}
		}
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////
// Called when a node is being unhidden
// If the object was shown, returns true
// If the object doesn't exist yet, returns false. This will prompt the
// Synchronizer to schedule the rebuilding of that object.
// For this reason light objects return true always, because hiding/unhiding
// a light does not affect the light itself in rendering. Since Hiding a
// light's geometry does not mean to switch the light off.
//

bool Synchronizer::Show(INode *node)
{
	bool res = false;

	auto ss = mShapes.find(node);
	if (ss != mShapes.end())
	{
	for (auto &shape : ss->second)
		shape->Get().SetVisibility(true);
		res = true;
	}
	else
	{
		// maybe a light?
		auto ll = mLights.find(node);
		if (ll != mLights.end())
			res = true;
	}

	return res;
}

//////////////////////////////////////////////////////////////////////////////
// Called when a node is being hidden
//

void Synchronizer::Hide(INode *node)
{
	auto ss = mShapes.find(node);
	if (ss != mShapes.end())
	{
		for (auto &shape : ss->second)
			shape->Get().SetVisibility(false);
	}
}

//////////////////////////////////////////////////////////////////////////////
// Called when a node is being deleted
//

void Synchronizer::DeleteGeometry(INode *instance)
{
	// materials
	RemoveMaterialsFromNode(instance);

	auto ss = mShapes.find(instance);
	if (ss != mShapes.end())
	{
		// actual geometry
		for (auto shape : ss->second)
			mScope.GetScene().Detach(shape->Get());
		
		mShapes.erase(ss);
	}
}

FIRERENDER_NAMESPACE_END;
