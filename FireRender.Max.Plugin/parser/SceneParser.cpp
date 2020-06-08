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

#pragma once

#include "SceneParser.h"
#include "RenderParameters.h"
#include "ParamBlock.h"
#include "CamManager.h"
#include "ScopeManager.h"
#include "utils/Utils.h"
#include "SceneCallbacks.h"
#include "CoronaDeclarations.h"
#include "FireRenderStandardMtl.h"
#include "FireRenderDisplacementMtl.h"
#include "FireRenderMaterialMtl.h"
#include "FireRenderUberMtl.h"
#include "FireRenderUberMtlv3.h"
#include "FireRenderEnvironment.h"
#include "FireRenderAnalyticalSun.h"
#include "BgManager.h"
#include "FireRenderPortalLight.h"
#include "light/FireRenderIESLight.h"
#include "light/physical/FireRenderPhysicalLight.h"

#include <iparamm2.h>
#include <lslights.h>
#include <ICustAttribContainer.h>
#include <MeshNormalSpec.h>
#include <scene/IPhysicalCamera.h>

#include <set>
#include <map>
#include <chrono>

#define USE_INSTANCES_ONLY false
#define DEFAULT_LIGHT_ID 0x100000000

#define ENVIRONMENT_GROUND_ID 0x200000000
#define ENVIRONMENT_DOME_ID 0x300000000
#define ENVIRONMENT_SKYLIGHT_ID 0x300000001

#define ENVIRONMENT_STANDARD_ID 0x400000000

#define BACKGROUND_IBL_ID 0x500000000

#define DISABLED_MATERIAL (Mtl*)1

#define PROFILING 0

#if PROFILING > 0

struct ProfilingData
{
	std::chrono::time_point<std::chrono::steady_clock> mSyncStart;
	std::chrono::time_point<std::chrono::steady_clock> mSyncEnd;

	std::int32_t shadersNum;

	void Reset()
	{
		mSyncStart = std::chrono::high_resolution_clock::now();
		mSyncEnd = std::chrono::high_resolution_clock::now();

		shadersNum = 0;
	}
};

ProfilingData profilingData;

#endif

FIRERENDER_NAMESPACE_BEGIN

// Returns true if the input node is hidden in rendering
inline bool isHidden(INode* node, const RenderParameters& params)
{
	return bool_cast( node->IsNodeHidden(TRUE) );

    // 3ds Max has 2 global switches we need to consider:
    // - hide frozen: causes frozen objects to behave as hidden objects during rendering (= invisible by default)
    // - render hidden: causes hidden objects to be rendered (they are ignored by default)
    //   const bool hideFrozen = (params.rendParams.extraFlags & RENDER_HIDE_FROZEN) != 0;
    //   const bool rendHidden = params.rendParams.rendHidden != FALSE;
    //   return !rendHidden && (node->IsHidden(0, TRUE) || (node->IsFrozen() && hideFrozen));
}

bool isNodeSelected(INode* node)
{
	if (node)
	{
		Interface* ip = GetCOREInterface();
		for (int i = 0; i < ip->GetSelNodeCount(); i++)
		{
			if (ip->GetSelNode(i) == node)
				return TRUE;
		}
	}
	return FALSE;
}

ParamID GetParamIDFromName(const wchar_t* paramName, IParamBlock2 *pb)
{
	if (pb)
	{
		ParamBlockDesc2 *pbDesc = pb->GetDesc();
		if (pbDesc)
		{
			int paramIdx = pbDesc->NameToIndex(paramName);
			if (paramIdx != -1)
				return pbDesc->IndextoID(paramIdx);
		}
	}

	debugPrint(std::wstring(L"Could not retreive ParamID for ") + std::wstring(paramName));

	return -1;
}

ParsedView SceneParser::ParseView()
{
	ParsedView output = {};
	output.cameraExposure = 1.f;

	// In the interactive mode we always refresh the currently selected viewport before parsing (user might have switched 
	// between viewports). In offline mode we stick with the initial viewport we got from 3ds Max.
	//if (GetFromPb<bool>(params.pblock, PARAM_INTERACTIVE_MODE) && !params.rendParams.inMtlEdit) {
	//    params.viewParams = viewExp2viewParams(getRenderViewExp(), params.viewNode);
	//}

	BOOL res;
	BOOL overWriteDOFSettings;
	BOOL useRPRMotionBlur;
	res = CamManagerMax::TheManager.GetProperty(PARAM_CAM_OVERWRITE_DOF_SETTINGS, overWriteDOFSettings);
	FASSERT(res);
	res = CamManagerMax::TheManager.GetProperty(PARAM_CAM_USE_MOTION_BLUR, useRPRMotionBlur);
	FASSERT(res);

	if (!overWriteDOFSettings) {
		//set default values:
		BOOL useDof;
		res = CamManagerMax::TheManager.GetProperty(PARAM_CAM_USE_DOF, useDof);
		FASSERT(res);
		output.useDof = useDof != FALSE;

		res = CamManagerMax::TheManager.GetProperty(PARAM_CAM_FOCUS_DIST, output.focusDistance);
		FASSERT(res);
		res = CamManagerMax::TheManager.GetProperty(PARAM_CAM_SENSOR_WIDTH, output.sensorWidth);
		FASSERT(res);
		res = CamManagerMax::TheManager.GetProperty(PARAM_CAM_F_STOP, output.fSTop);
		FASSERT(res);
		res = CamManagerMax::TheManager.GetProperty(PARAM_CAM_BLADES, output.bokehBlades);
		FASSERT(res);
		res = CamManagerMax::TheManager.GetProperty(PARAM_CAM_FOCAL_LENGTH, output.focalLength);
		FASSERT(res);
		res = CamManagerMax::TheManager.GetProperty(PARAM_CAM_FOV, output.perspectiveFov);
		FASSERT(res);
	}

	output.tilt.Set(0, 0);

	if (params.viewNode)
	{	// we are currently rendering a camera - eval the view node, cast to camera, parse its data

		std::wstring name = params.viewNode->GetName();
		output.cameraNodeName = std::string( name.begin(), name.end() );

		const ObjectState& os = params.viewNode->EvalWorldState(params.t);
		CameraObject* cam = dynamic_cast<CameraObject*>(os.obj);
		cam->UpdateTargDistance(params.t, params.viewNode); // necessary, otherwise we sometimes get incorrect target distance

		Matrix3 camTransform = params.viewNode->GetObjTMAfterWSM(params.t);
		camTransform.SetTrans(camTransform.GetTrans() * masterScale);
		output.tm = camTransform;

		Matrix3 camTransformMotion = params.viewNode->GetObjTMAfterWSM(params.t+1);
		camTransformMotion.SetTrans(camTransformMotion.GetTrans() * masterScale);
		output.tmMotion = camTransformMotion;


		output.perspectiveFov = cam->GetFOV(params.t);
		// Do not allow zero fov value
		if (output.perspectiveFov < 0.001f)
		{
			output.perspectiveFov = 0.001f;
		}

		output.projection = cam->IsOrtho() ? P_ORTHO : P_PERSPECTIVE;

		bool physicalCameraUsed = false;
#if MAX_PRODUCT_YEAR_NUMBER >= 2016
		MaxSDK::IPhysicalCamera* physicalCamera = dynamic_cast<MaxSDK::IPhysicalCamera*>(cam);
		if (physicalCamera) {
			physicalCameraUsed = true;
			// Physical camera, if present, overrides the DOF parameters
			output.useDof = physicalCamera->GetDOFEnabled(params.t, Interval());

			output.sensorWidth = physicalCamera->GetFilmWidth(params.t, Interval()) * getSystemUnitsToMillimeters();

			if (output.useDof) {
				output.focusDistance = physicalCamera->GetFocusDistance(params.t, Interval()) * getSystemUnitsToMillimeters() * 0.001f;
				output.fSTop = physicalCamera->GetLensApertureFNumber(params.t, Interval());
				output.bokehBlades = physicalCamera->GetBokehNumberOfBlades(params.t, Interval());
			}

			output.tilt = physicalCamera->GetTiltCorrection(params.t, Interval());

			if (!useRPRMotionBlur)
			{
				// Motion blur
				output.useMotionBlur = physicalCamera->GetMotionBlurEnabled(params.t, Interval());

				float shutterOpenDuration = physicalCamera->GetShutterDurationInFrames(params.t, Interval());
				output.motionBlurScale = shutterOpenDuration;
				output.cameraExposure = shutterOpenDuration;
			}
		}
#endif
		if (!physicalCameraUsed) {
			output.useDof = bool_cast( cam->GetMultiPassEffectEnabled(params.t, Interval()) );
			//sensor width is 36mm on default
			output.sensorWidth = 36;
			//output.focalLength = output.sensorWidth / (2.f * std::tan(output.perspectiveFov / 2.f));
			if (output.useDof) {
				//use dof possible
				IMultiPassCameraEffect *mpCE = cam->GetIMultiPassCameraEffect();
				//mpCE->IsAnimated

				for (int i = 0; i < mpCE->NumSubs(); i++)
				{
					Animatable *animatableMP = mpCE->SubAnim(i);
					MSTR animatableNameMP = mpCE->SubAnimName(i);

					MaxSDK::Util::MaxString dofpString;
					dofpString.SetUTF8("Depth of Field Parameters");

					if (animatableNameMP.ToMaxString().Compare(dofpString) == 0) {
						IParamBlock2 *pb = dynamic_cast<IParamBlock2*>(animatableMP);
						ParamBlockDesc2 *pbDesc = pb->GetDesc();
						int utdIdx = pbDesc->NameToIndex(L"useTargetDistance");
						if (utdIdx != -1) {
							BOOL useTargetDistance = true;
							float focalDepth = cam->GetTDist(params.t) * getSystemUnitsToMillimeters() * 0.001f;

#if MAX_PRODUCT_YEAR_NUMBER >= 2016
							pb->GetValueByName<BOOL>(L"useTargetDistance", params.t, useTargetDistance, Interval());
							pb->GetValueByName<float>(L"focalDepth", params.t, focalDepth, Interval());
#else
							ParamID pUseTD = GetParamIDFromName(L"useTargetDistance", pb);
							if (pUseTD != -1)
								pb->GetValue(pUseTD, params.t, useTargetDistance, Interval());

							ParamID pFocalDepth = GetParamIDFromName(L"focalDepth", pb);
							if (pFocalDepth != -1)
								pb->GetValue(pFocalDepth, params.t, focalDepth, Interval());
#endif
							if (useTargetDistance) {
								output.focusDistance = cam->GetTDist(params.t) * getSystemUnitsToMillimeters() * 0.001f;
							}
							else {
								output.focusDistance = focalDepth * getSystemUnitsToMillimeters() * 0.001f;
							}
						}
					}
				}
			}
		}

		output.orthoSize = output.focusDistance*std::tan(output.perspectiveFov * 0.5f) * 2.f;

		float focalLength = output.sensorWidth / (2.f * std::tan(output.perspectiveFov * 0.5f));
		FASSERT(!isnan(focalLength) && focalLength > 0 && focalLength < std::numeric_limits<float>::infinity());
		output.focalLength = focalLength;
    } 
	else 
	{
        // No camera present. Use ViewParams class
        output.perspectiveFov = params.viewParams.fov;
        output.orthoSize = params.viewParams.zoom * 400.f * masterScale;    //400 = magic constant necessary, from 3dsmax api doc
		//sensor width is 36mm on default
		output.sensorWidth = 36;
        output.projection = (params.viewParams.projType == PROJ_PERSPECTIVE) ? P_PERSPECTIVE : P_ORTHO;

		if (output.projection == P_PERSPECTIVE) 
		{
			float focalLength = output.sensorWidth / (2.f * std::tan(output.perspectiveFov / 2.f));
			FASSERT(!isnan(focalLength) && focalLength > 0 && focalLength < std::numeric_limits<float>::infinity());
			output.focalLength = focalLength;
		}

		output.tm = params.viewParams.affineTM;
		output.tm.SetTrans(output.tm.GetTrans() * masterScale);
		output.tm.Invert();

		output.tmMotion = output.tm;
	}

	if (overWriteDOFSettings) 
	{
		//overwrite active, ignore previously set values

		BOOL useDof;
		res = CamManagerMax::TheManager.GetProperty(PARAM_CAM_USE_DOF, useDof);
		FASSERT(res);
		output.useDof = useDof != FALSE;

		if (useDof) {
			res = CamManagerMax::TheManager.GetProperty(PARAM_CAM_FOCUS_DIST, output.focusDistance);
			FASSERT(res);
			res = CamManagerMax::TheManager.GetProperty(PARAM_CAM_SENSOR_WIDTH, output.sensorWidth);
			FASSERT(res);
			res = CamManagerMax::TheManager.GetProperty(PARAM_CAM_F_STOP, output.fSTop);
			FASSERT(res);
			res = CamManagerMax::TheManager.GetProperty(PARAM_CAM_BLADES, output.bokehBlades);
			FASSERT(res);
			res = CamManagerMax::TheManager.GetProperty(PARAM_CAM_FOCAL_LENGTH, output.focalLength);
			FASSERT(res);
			res = CamManagerMax::TheManager.GetProperty(PARAM_CAM_FOV, output.perspectiveFov);
			FASSERT(res);
			
			if (params.viewNode) {
				output.orthoSize = output.focusDistance*std::tan(output.perspectiveFov / 2.f) * 2.f;
			}
			else {
				output.orthoSize = params.viewParams.zoom * 400.f * masterScale;
			}
		}
	}

	res = CamManagerMax::TheManager.GetProperty(PARAM_CAM_TYPE, output.projectionOverride);
	FASSERT(res);

	if (useRPRMotionBlur)
	{
		output.useMotionBlur = true;

		res = CamManagerMax::TheManager.GetProperty(PARAM_CAM_MOTION_BLUR_SCALE, output.motionBlurScale);
		FASSERT(res);

		res = CamManagerMax::TheManager.GetProperty(PARAM_CAM_EXPOSURE, output.cameraExposure);
		FASSERT(res);
	}

	return output;
}

	/// Recursively traverses a node tree and outputs all sub-nodes (including scattered nodes and XRef nodes) to the output stack.
/// Nodes invisible to rendering (e.g. hidden ones) are not reported.
/// \param processXRef if true, then xref scenes will be also traversed when processing scene root node. We want to optionally 
///                    disable this for material editor rendering.
void SceneParser::traverseNode(INode* input, const bool processXRef, const RenderParameters& parameters, ParsedNodes& output)
{
    if (!input) 
        return;

    if (input->IsRootNode() && processXRef) { // Only scene root node holds XRefs
        for (int i = 0; i < input->GetXRefFileCount(); ++i) {
            traverseNode(input->GetXRefTree(i), processXRef, parameters, output);
        }
    }
    for (int i = 0; i < input->NumberOfChildren(); ++i) {
        traverseNode(input->GetChildNode(i), processXRef, parameters, output);
    }

    if (isHidden(input, parameters)) 
        return;

	if (parameters.rendParams.rendType == RENDTYPE_SELECT && !isNodeSelected(input))
		return;


	if (auto objRef = input->GetObjectRef())
	{
		auto id = Animatable::GetHandleByAnim(input);
		if (objRef->ClassID() == Corona::SCATTER_CID)
		{
			if (ScopeManagerMax::CoronaOK)
			{
			// If this is a scatter node, we will use its function publishing interface to get list of scattered nodes and report them 
			// all with their transformation matrices
			Corona::ScatterFpOps* ops = GetScatterOpsInterface(objRef->GetParamBlockByID(0)->GetDesc()->cd);
			const Point3 cameraPos = parameters.renderGlobalContext.worldToCam.GetTrans() * GetUnitScale();
			ops->update(objRef, cameraPos, parameters.t); // necessary so we use the correct time/config for the scatter

			const int numInstances = ops->getNumInstances(objRef);

			for (int i = 0; i < numInstances; i++)
			{
				auto node = ops->getInstanceObject(objRef, i);
				auto tm = ops->getInstanceTm(objRef, i);
				tm.SetTrans(tm.GetTrans() * masterScale);
				output.push_back(ParsedNode((id << 16) + i, node, tm));
			}
		}
		}
		else
		{
			auto tm = input->GetObjTMAfterWSM(parameters.t);
			tm.SetTrans(tm.GetTrans() * masterScale);
			output.push_back(ParsedNode(id, input, tm));
			
			Matrix3 ntmMotion = input->GetObjTMAfterWSM(parameters.t+1);
			ntmMotion.SetTrans(ntmMotion.GetTrans() * masterScale);
			output.back().tmMotion = ntmMotion;

		}
	}
}


bool createMesh(std::vector<frw::Shape>& result, bool& directlyVisible,
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
	if (evaluatedObject->ClassID() == Corona::LIGHT_CID) {
		if (ScopeManagerMax::CoronaOK)
		{
		// Corona lights require some special processing - we will get its mesh via function publishing interface.
		// But when we have it, we can parse the mesh as any other geometry object, because we also handle their the lights 
		// material specially in MaterialParser.
		directlyVisible &= GetFromPb<bool>(evaluatedObject->GetParamBlockByID(0), Corona::L_DIRECTLY_VISIBLE);
		BaseInterface* bi = evaluatedObject->GetInterface(Corona::IFIREMAX_LIGHT_INTERFACE);
		Corona::IFireMaxLightInterface* lightInterface = dynamic_cast<Corona::IFireMaxLightInterface*>(bi);
		if (lightInterface) {
			lightInterface->fmGetRenderMesh(timeValue, mesh, needsDelete);
		}
		else {
			MessageBox(GetCOREInterface()->GetMAXHWnd(), _T("Too old Corona version."), _T("Radeon ProRender warning"), MB_OK);
		}
		}
	}

	if (!mesh)  //nothing to do here
		return false;

	meshFaces = mesh->getNumFaces();
	
	// Next we try to get normals associated with the mesh
	MeshNormalSpec* normals = nullptr;

	// Handle normals: in case we need to delete the mesh, we are free to modify it, so we just do by calling SpecifyNormals()
	if (needsDelete) {
		mesh->SpecifyNormals();
		normals = mesh->GetSpecifiedNormals();
	}
	else {
		// Otherwise we will try to get the normals without modifying the mesh. We try it...
		normals = mesh->GetSpecifiedNormals();
		if (normals == NULL || normals->GetNumNormals() == 0) {
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
	int numChannels = mesh->tVerts == NULL ? 0 : 1;

	vi.resize(numSubmtls);
	ni.resize(numSubmtls);
	ti.resize(numChannels);
	t.resize(numChannels);

	for (int i = 0; i < numChannels; ++i)
	{
		ti[i].resize(numSubmtls);
	}

	v.resize(mesh->getNumVerts());
	for (int i = 0; i < mesh->getNumVerts(); ++i)
	{
		v[i] = mesh->verts[i] * masterScale;
	}

	n.resize(normals->GetNumNormals());
	for (int i = 0; i < normals->GetNumNormals(); ++i)
	{
		Point3 normal = normals->Normal(i);
		n[i] = normal.Normalize();
	}

	const bool textured = mesh->tVerts != NULL;
	for (int i = 0; i < numChannels; i++)
	{
		MeshMap& mapChannel = mesh->maps[i + 1]; // maps[0] is vertex color, leave it

		if (textured) {
			t[i].resize(mapChannel.vnum);

			for (int j = 0; j < mapChannel.vnum; ++j) {
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
			{
				ti[j][i].reserve(faceCount * 3 * 3);
			}
		}
	}

    for (int i = 0; i < mesh->getNumFaces(); ++i) {
        Face& face = mesh->faces[i];
        const int mtlId = face.getMatID() % numSubmtls;
        for (int j = 0; j < 3; ++j) {
#ifdef SWITCH_AXES
            int index = 0; // we swap 2 indices because we change parity via TM switch
            if (j == 1) {
                index = 2;
            } else if (j == 2) {
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
				
				if (textured) {
					ti[k][mtlId].push(mapChannel.tf[i].t[index]);
				}
			}
		}
    }

    // Create a dummy array holding numbers of vertices for each face (which is always "3" in our case)
    int maxFaceCount = 0;
    for (auto& i : vi) {
        maxFaceCount = std::max(maxFaceCount, i.size() / 3);
    }
    Stack<int> vertNums;
    vertNums.resize(maxFaceCount);
    for (auto& i : vertNums) {
        i = 3;
    }

    // Finally, create RPR rpr_shape objects.

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
			
			size_t texcoordsNum[2] = {0, 0};
			rpr_int texcoordStride[2] = {0, 0};
			rpr_int texcoordIdxStride[2] = {0, 0};

			const rpr_float* texcoords[2] = {nullptr, nullptr};
			const rpr_int* texcoordIndices[2] = {nullptr, nullptr};

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

			const rpr_int* niFloats = &ni[i][0];
			const float* normalFloats = n[0];
			if (ni[i][0] == -1)
			{
				niFloats = nullptr;
				normalFloats = nullptr;
			}

			auto shape = context.CreateMeshEx(
				v[0], v.size(), sizeof(v[0]),
				normalFloats, normalFloats != nullptr ? n.size() : 0, sizeof(n[0]),
				nullptr, 0, 0,
				numChannels, texcoords, texcoordsNum, texcoordStride,
				&vi[i][0], sizeof(vi[i][0]),
				niFloats, sizeof(ni[i][0]),
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


/// Parses a single mesh object. Outputs a list of shapes because the mesh can have multiple material IDs, which RPR cannot 
/// handle in single shape
/// \param inode Scene node where the mesh was encountered
/// \param evaluatedObject result of calling EvalWorldState on inode
/// \param numSubmtls Number of submaterials that the node material holds. If it is more than 1 (it uses multi-material) and the 
///                   mesh has multiple material IDs, we will have to create multiple shapes.
std::vector<frw::Shape> SceneParser::parseMesh(INode* inode, Object* evaluatedObject, const int numSubmtls, size_t& meshFaces, bool flipFaces)
{
	meshFaces = 0;
	std::vector<frw::Shape> result;
	if (!evaluatedObject)
		return result;

	auto context = scope.GetContext();
    FASSERT(evaluatedObject != NULL && inode != NULL);

	HashValue key;

	key << params.t << inode << flipFaces << numSubmtls << Animatable::GetHandleByAnim(evaluatedObject);

	Box3 bbox = {};
	evaluatedObject->GetDeformBBox(params.t, bbox);
	key << bbox;

		// hopefully we can get this info
	int numFaces = 0, numVerts = 0;
	if (evaluatedObject->PolygonCount(params.t, numFaces, numVerts))
	{
		meshFaces = numFaces;
		key << numFaces << numVerts;
	}
	else
	{
		DebugPrint(L"no face count\n");
	}

	result = scope.GetShapeSet(key);
	if (!result.empty())
		return result;

	bool directlyVisible ;
	if(!createMesh(result, directlyVisible,
		context, masterScale,
		params.t, const_cast<FireRenderView&>(params.view), inode, evaluatedObject, numSubmtls, meshFaces, flipFaces))
		return std::vector<frw::Shape>();

	for(auto& shape: result){
		if (!directlyVisible)
			shape.SetVisibility(false);
	}

	scope.SetShapeSet(key, result);
    return result;
}

ParsedNode::ParsedNode(size_t id, INode* node, const Matrix3& tm): node(node), id(id), tm(tm)
{
	if (node)
	{
		if (bool_cast(material = node->GetMtl()))
		{
			// if the material is a Shell mtl, we pick its correct sub-material for rendering
			while (material && material->ClassID() == Class_ID(BAKE_SHELL_CLASS_ID, 0))
				material = material->GetSubMtl(material->GetParamBlock(bakeShell_params)->GetInt(bakeShell_render_n_mtl, 0));

			// special case for multi material with only one sub
			while (material && material->IsMultiMtl() && material->NumSubMtls() == 1)
				material = material->GetSubMtl(0);

			if (material)
				matId = Animatable::GetHandleByAnim(material);
		}

		if (bool_cast(object = node->GetObjectRef()))
		{
			objectId = Animatable::GetHandleByAnim(object);
		}
	}
}

std::vector<Mtl*> ParsedNode::GetAllMaterials(const TimeValue &t) const
{
	auto mtl = material;

	std::vector<Mtl*> result;
	if (!mtl || !mtl->IsMultiMtl())
		result.push_back(mtl);	// note: we allow nullptr
	else
	{
		result.resize(mtl->NumSubMtls());
		const Class_ID cid = mtl->ClassID();
		IParamBlock2 *pb = 0;
		if (cid == MULTI_MATERIAL_CLASS_ID)
		{
			pb = mtl->GetParamBlock(0);
			//GET_PARAM_N(14, materialList, 0x0, n);
			//GET_PARAM_N(04, mapEnabled, 0x1, n);
			//GET_PARAM_N(08, names, 0x2, n);
			//GET_PARAM_N(01, materialIDList, 0x3, n);
		}
		for (int i = 0; i < mtl->NumSubMtls(); i++)
		{
			BOOL enabled = TRUE;
			if (pb)
				pb->GetValue(0x01, t, enabled, FOREVER, i);
			if (enabled)
			result[i] = mtl->GetSubMtl(i);
			else
				result[i] = DISABLED_MATERIAL;
		}
	}
	return result;
}

SceneParser::SceneParser(RenderParameters& params, SceneCallbacks& callbacks, frw::Scope scope)
	: scope(scope), params(params), callbacks(callbacks), mtlParser(scope), hasDirectDisplacements(false)
{
	scene = scope.GetScene();
	masterScale = GetUnitScale();
	mtlParser.SetTimeValue(params.t);
	mtlParser.SetParamBlock(params.pblock);
}

void SceneParser::traverseMaterialUpdate(Mtl* material)
{
	if(!material)
		return;

	callbacks.addItem(material);

	for(int i = 0; i < material->NumSubMtls(); ++i)
		traverseMaterialUpdate(material->GetSubMtl(i));
}

void SceneParser::ParseScene()
{
	parsed = ParsedData();	// reset
	parsed.t = params.t;
	traverseNode(params.sceneNode, params.rendParams.inMtlEdit == FALSE, params, parsed.nodes);

	for (auto& it : parsed.nodes)
	{
		callbacks.addItem(it.node); // call all the necessary before-render callbacks

		// traverseNode done, but 3ds max is only giving us all the tree's nodes if we are rendering material preview.. 
		// So here we traverse manually and update materials
		if (it.material)
		{
			for(int i = 0; i < it.material->NumSubMtls(); ++i)
				traverseMaterialUpdate(it.material->GetSubMtl(i));
		}
	}
		

	if ((parsed.environment.texmap = GetCOREInterface()->GetUseEnvironmentMap() ? GetCOREInterface()->GetEnvironmentMap() : nullptr))
	{
		parsed.environment.id = Animatable::GetHandleByAnim(parsed.environment.texmap);
	}

	parseFRGround();
}

struct Motion{
	Point3 velocity;

	float momentumAngle;
	Point3 momentumAxis;
};

void resetMotion(frw::Shape& shape){
	shape.SetLinearMotion(0, 0, 0);
	shape.SetAngularMotion(1.0f, 0.0f, 0.0f, 0.0f);
}

void applyMotion(frw::Shape& shape, Motion motion){
	shape.SetLinearMotion(motion.velocity.x, motion.velocity.y, motion.velocity.z);

	// This check is for situation when our momentum is actually zero, but
	// RPR doesn't work well with zeroes for angular motion axes
	if(motion.momentumAxis.LengthSquared()>0.5f)
		shape.SetAngularMotion(motion.momentumAxis.x, motion.momentumAxis.y, motion.momentumAxis.z, motion.momentumAngle);
	else
		shape.SetAngularMotion(1.0f, 0.0f, 0.0f, 0.0f);
}

Motion getMotion(const ParsedView& view, const ParsedNode& parsedNode){
	//motion blur
	float dtInverse = SecToTicks(1.0f);
	//compute camera-space transforms for current frame and next one
	//taking into account camera motion too
	//then compute delta rotation and translatio9n in CAMERA space
	//so that linear and angular motion set for each object will be
	//relative to camera motion(e.g. still objects for moving camera 
	//would be motion-blurred)
	Matrix3 toView = InverseHighPrecision(view.tm);
	Matrix3 toViewAndBack = view.tm*toView;
	Matrix3 tmCamSpace = parsedNode.tm*toView;
	Quat rotationCamSpace(tmCamSpace);

	Matrix3 toViewMotion = InverseHighPrecision(view.tmMotion);
	Matrix3 tmCamSpaceMotion = parsedNode.tmMotion*toViewMotion;
	Quat rotationCamSpaceMotion(tmCamSpaceMotion);

	//extract motion in cameraspace
	Point3 axisDeltaCamSpace;
	float angleDeltaCamSpace = QangAxis(rotationCamSpace, rotationCamSpaceMotion, axisDeltaCamSpace);
	Point3 velocityCamSpace = (tmCamSpaceMotion.GetTrans()-tmCamSpace.GetTrans());

	//transform these back to worls space, where FR expects it 
	Point3 velocity = view.tm.VectorTransform(velocityCamSpace)*dtInverse;
	float momentumAngle = angleDeltaCamSpace*dtInverse;
	Point3 momentumAxis = view.tm.VectorTransform(axisDeltaCamSpace);

	float motionBlurScale = view.motionBlurScale;
	momentumAngle *= motionBlurScale*0.01f;
	velocity *= motionBlurScale*0.01f;

	Motion result;
	result.velocity = velocity;
	result.momentumAngle = momentumAngle;
	result.momentumAxis = momentumAxis;

	return result;
}

void SetNameFromNode(INode* node, frw::Object& obj){
	std::wstring name = node->GetName();
	std::string name_s( name.begin(), name.end() );
	obj.SetName(name_s.c_str());
}

void SceneParser::AddParsedNodes(const ParsedNodes& parsedNodes)
{
    // We sort all the geometry objects according to their Animatable handle, not by raw pointers. This is because while object 
    // addresses may get reused (the result of EvalWorldState may be a temporary object that is deleted as soon as another 
    // object in the scene is evaluated), AnimHandle is always unique. This way we dont get any false "instancing" of two 
    // different objects caused by random evaluated object address being aliased with previous address of already deleted object

    std::map<AnimHandle, ParsedNodes> instances;

	for (auto& actual : parsedNodes) 
	{
        FASSERT(actual.node);
        Object* objRef = actual.node->GetObjectRef();
        if (!objRef || !actual.node->Renderable()) {
            continue;
        }
        const Class_ID objRefCid = objRef->ClassID();
        // We specifically ignore some objects that either render garbage, or crash when we try to render them:
        // CAT parent: Class ID 0x56ae72e5, 0x389b6659
        // Delegate helper: Class ID 0x40c07baa, 0x245c7fe6
        if (objRefCid == Class_ID(0x56ae72e5, 0x389b6659) || objRefCid == Class_ID(0x40c07baa, 0x245c7fe6)) {
            continue;
        }
        const ObjectState& state = actual.node->EvalWorldState(params.t);
        const SClass_ID sClassId = state.obj->SuperClassID();
        const Class_ID classId = state.obj->ClassID();
		
		Matrix3 ntm = actual.node->GetObjTMAfterWSM(params.t);
		ntm.SetTrans(ntm.GetTrans() * masterScale);
		// incapsulate matrix retrieval with masterScale multiply?
		Matrix3 ntmMotion = actual.node->GetObjTMAfterWSM(params.t+1);
		ntmMotion.SetTrans(ntmMotion.GetTrans() * masterScale);

        const auto& tm = actual.tm;

		Matrix3 tmMotion = ntmMotion;

		if (!actual.node->GetVisibility(params.t))
		{
			// ignore invisible here (don't add it)
        } 
		else if (classId == Corona::SUN_OBJECT_CID)
		{
			if (ScopeManagerMax::CoronaOK)
			parseCoronaSun(actual, state.obj);
		}
		else if (classId == FIRERENDER_ENVIRONMENT_CLASS_ID)
		{
			parseFREnvironment(actual, state.obj);
		}
		else if (classId == FIRERENDER_PORTALLIGHT_CLASS_ID)
		{
			parseFRPortal(actual, state.obj);
		}
		else if ((classId == FireRenderIESLight::GetClassId()) || 
				(classId == FIRERENDER_PHYSICALLIGHT_CLASS_ID))
		{
			FireRenderLight* light = dynamic_cast<FireRenderLight*>(state.obj);
			if (light && light->GetUseLight())
			{
				light->CreateSceneLight(params.t, actual, scope);
			}
		}
		else if ((sClassId == LIGHT_CLASS_ID) && (classId != Corona::LIGHT_CID) && (classId != FIRERENDER_ENVIRONMENT_CLASS_ID) && (classId != FIRERENDER_ANALYTICALSUN_CLASS_ID) && (classId != FIRERENDER_PORTALLIGHT_CLASS_ID))
		{
			// A general light object, but not Corona Light (which we parse as a geometry)
			parseMaxLight(actual, state.obj);
		}
		else if (classId == Corona::LIGHT_CID || sClassId == GEOMOBJECT_CLASS_ID || state.obj->CanConvertToType(triObjectClassID) != FALSE)
		{
            if (!state.obj->IsRenderable() || GetDeterminant(tm) == 0.f) {
                continue; // ignore zero-scale/degenerated TM objects (which will have determinant 0)
            }
            auto handle = Animatable::GetHandleByAnim(state.obj);

			const_cast<FireRender::ParsedNode*>(&actual)->tmMotion = tmMotion;
            instances[handle].push_back(actual);
        }
    }

	int numInstances = int_cast(instances.size());
	wchar_t tempStr[1024];
	int i = 1;

    // Finally, we process the geometry
    for (auto& instance : instances)  // iterate over groups of all different objects. All nodes withing each group will be instanced
	{
		wsprintf(tempStr, L"Synchronizing: Rebuilding Object %d of %d (%s)", i++, numInstances, (instance.second.begin())->node->GetName());
		if (params.progress)
			params.progress->SetTitle(tempStr);

		const auto& nodes = instance.second;
		
        // determine the maximal number of sub-materials

        int numMtls = 0; 
        for (auto& parsedNode : nodes) 
            numMtls = std::max(numMtls, int_cast(parsedNode.GetAllMaterials(params.t).size()) );

        // Evaluate the mesh of first node in the group

        auto firstNode = &*nodes.begin();
        const ObjectState& state = firstNode->node->EvalWorldState(params.t);
		size_t currMeshFaces = 0;
		BOOL parity = firstNode->tm.Parity();

        auto originalShapes = parseMesh(firstNode->node, state.obj, numMtls, currMeshFaces, !!parity);
		if (originalShapes.size() == 0)
			continue; // evaluating the object yielded no faces - e.g. the object was empty

		if (USE_INSTANCES_ONLY)
		{
			for (auto it = originalShapes.begin(); it != originalShapes.end(); ++it)
			{
				scene.Attach(*it);
				(*it).SetVisibility(false);
			}
		}

		FASSERT(originalShapes.size() == numMtls);

        for (auto& parsedNode : nodes) 
		{ // iterate over all instances inside the group
            std::vector<frw::Shape> shapes;
            // For the first instance we directly reuse the parsed shapes, for subsequent ones we use instances
            if (USE_INSTANCES_ONLY || &parsedNode != firstNode)
			{
                for (int i = 0; i < numMtls; ++i) 
				{
                    if (originalShapes[i]) 
					{
						auto shape = originalShapes[i].CreateInstance(scope);
						shapes.push_back(shape);
                    } 
					else 
					{
						shapes.push_back(frw::Shape());
                    }
                }
            } 
			else 
			{
                shapes = originalShapes;
            }


			//motion blur
			Motion motion = getMotion(view, parsedNode);

            const auto& nodeMtls = parsedNode.GetAllMaterials(params.t);
            FASSERT(numMtls == shapes.size());

            // now go over all mtl IDs, set transforms and materials for shapes and handle special cases
			for (size_t i = 0; i < numMtls; ++i)
			{
				if (auto shape = shapes[i])
				{
					shape.SetTransform(parsedNode.tm);
					shape.SetUserData(parsedNode.id);

					Mtl* currentMtl = nodeMtls[std::min(i, nodeMtls.size() - 1)];

					float minHeight = 0.f;
					float maxHeight = 0.0f;
					float subdivision = 0.f;
					float creaseWeight = 0.f;
					int boundary = RPR_SUBDIV_BOUNDARY_INTERFOP_TYPE_EDGE_AND_CORNER;
                
					bool shadowCatcher = false;
					bool castsShadows = true;
					
					// Handling of some special flags for Corona Material is necessary here at geometry level
					if (currentMtl == DISABLED_MATERIAL)
					{
					}
					else if (currentMtl && currentMtl->ClassID() == Corona::MTL_CID) 
					{
						if (ScopeManagerMax::CoronaOK)
						{
							IParamBlock2* pb = currentMtl->GetParamBlock(0);
							const bool useCaustics = GetFromPb<bool>(pb, Corona::MTLP_USE_CAUSTICS, this->params.t);
							const float lRefract = GetFromPb<float>(pb, Corona::MTLP_LEVEL_REFRACT, this->params.t);
							const float lOpacity = GetFromPb<float>(pb, Corona::MTLP_LEVEL_OPACITY, this->params.t);

							//~mc hide for now as we have a flag for shadows and caustics
							if ((lRefract > 0.f || lOpacity < 1.f) && !useCaustics)
							{
								castsShadows = false;
							}
					}
					}
					else if (currentMtl && currentMtl->ClassID() == FIRERENDER_MATERIALMTL_CID) 
					{
						IParamBlock2* pb = currentMtl->GetParamBlock(0);
						castsShadows = bool_cast( GetFromPb<BOOL>(pb, FRMaterialMtl_CAUSTICS, this->params.t) );
						shadowCatcher = bool_cast( GetFromPb<BOOL>(pb, FRMaterialMtl_SHADOWCATCHER, this->params.t) );
					}
					else if (currentMtl && currentMtl->ClassID() == FIRERENDER_UBERMTL_CID)
					{
						IParamBlock2* pb = currentMtl->GetParamBlock(0);
						castsShadows = bool_cast( GetFromPb<BOOL>(pb, FRUBERMTL_FRUBERCAUSTICS, this->params.t) );
						shadowCatcher = bool_cast( GetFromPb<BOOL>(pb, FRUBERMTL_FRUBERSHADOWCATCHER, this->params.t) );
					}
					else if (currentMtl && currentMtl->ClassID() == FIRERENDER_UBERMTLV3_CID)
					{
						FireRenderUberMtlv3* mtl = dynamic_cast<FireRenderUberMtlv3*>(currentMtl);
						bool isEnabled = false;
						RprDisplacementParams displacementParams;
						
						mtl->GetDisplacement(isEnabled, displacementParams);

						if (isEnabled)
						{
							rpr_int res = rprShapeSetDisplacementScale(shape.Handle(), displacementParams.min, displacementParams.max);
							FCHECK(res);

							if (displacementParams.subdivType == Adaptive)
							{
								frw::Context ctx = scope.GetContext();
								frw::Scene scn = scope.GetScene();
								frw::Camera cam = scn.GetCamera();
								frw::FrameBuffer fbuf = scope.GetFrameBuffer(RPR_AOV_COLOR);
								shape.SetAdaptiveSubdivisionFactor(displacementParams.adaptiveSubDivFactor, cam.Handle(), fbuf.Handle());
							}
							else
							{
								shape.SetSubdivisionFactor(displacementParams.factor);
							}
							shape.SetSubdivisionCreaseWeight(displacementParams.creaseWeight);
							shape.SetSubdivisionBoundaryInterop(displacementParams.boundaryInteropType);
						}
					}
					else if (currentMtl && currentMtl->ClassID() == Corona::SHADOW_CATCHER_MTL_CID)
					{
						if (ScopeManagerMax::CoronaOK)
							shadowCatcher = true;
					}
					else if (currentMtl && currentMtl->ClassID() == FIRERENDER_SCMTL_CID) // Shadow Catcher Material
					{
						shadowCatcher = true;
					}

					frw::Value displImageNode;
					bool notAccurate = false;

					if (currentMtl != DISABLED_MATERIAL)
					{
						displImageNode = FRMTLCLASSNAME(DisplacementMtl)::translateDisplacement(this->params.t, mtlParser, currentMtl,
							minHeight, maxHeight, subdivision, creaseWeight, boundary, notAccurate);
					}

					if (displImageNode && shape.IsUVCoordinatesSet())
					{
						if (notAccurate)
						{
							hasDirectDisplacements = true;
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
					shape.SetShadowCatcherFlag(shadowCatcher);

					frw::Shader volumeShader;

					if (currentMtl != DISABLED_MATERIAL)
						volumeShader = mtlParser.findVolumeMaterial(currentMtl);

					if (volumeShader)
						shape.SetVolumeShader(volumeShader);

					if (currentMtl == DISABLED_MATERIAL)
					{
						auto diffuse = frw::DiffuseShader(mtlParser.materialSystem);
						diffuse.SetValue(RPR_MATERIAL_INPUT_COLOR, frw::Value(0.f));
						shape.SetShader(diffuse);
					}
					else if (auto shader = mtlParser.createShader(currentMtl, parsedNode.node, parsedNode.invalidationTimestamp != 0))
					{
						shape.SetShader(shader);

#if PROFILING > 0
						profilingData.shadersNum++;
#endif
					}

					SetNameFromNode(parsedNode.node, shape);

					scene.Attach(shape);
				}
			}
        }
    }
}

HashValue SceneParser::GetMaxSceneHash()
{
	MaxSceneState state = {};	// avoid uninitialized bytes causing hash problems

	// these values don't send ref messages when changed!
	state.timeValue = params.t;

	Texmap* environmentMap = nullptr;
	Texmap* bgIblMap = nullptr;
	Texmap* bgIblBackplate = nullptr;
	Texmap* bgIblReflections = nullptr;
	Texmap* bgIblRefractions = nullptr;
	Texmap* bgSkyReflections = nullptr;
	Texmap* bgSkyRefractions = nullptr;
	Texmap* bgSkyBackplate = nullptr;
	environmentMap = GetCOREInterface()->GetUseEnvironmentMap() ? GetCOREInterface()->GetEnvironmentMap() : nullptr;
	params.pblock->GetValue(TRPARAM_BG_IBL_MAP, params.t, bgIblMap, FOREVER);
	params.pblock->GetValue(TRPARAM_BG_IBL_REFLECTIONMAP, params.t, bgIblReflections, FOREVER);
	params.pblock->GetValue(TRPARAM_BG_IBL_REFRACTIONMAP, params.t, bgIblRefractions, FOREVER);
	params.pblock->GetValue(TRPARAM_BG_IBL_BACKPLATE, params.t, bgIblBackplate, FOREVER);
	params.pblock->GetValue(TRPARAM_BG_SKY_REFLECTIONMAP, params.t, bgSkyReflections, FOREVER);
	params.pblock->GetValue(TRPARAM_BG_SKY_REFRACTIONMAP, params.t, bgSkyRefractions, FOREVER);
	params.pblock->GetValue(TRPARAM_BG_SKY_BACKPLATE, params.t, bgSkyBackplate, FOREVER);
	
	HashValue hash;

	std::map<Animatable*, HashValue> hash_visited;
	DWORD syncTimestamp = 0;
	hash << mtlParser.GetHashValue(environmentMap, params.t, hash_visited, syncTimestamp);
	hash << mtlParser.GetHashValue(bgIblMap, params.t, hash_visited, syncTimestamp);
	hash << mtlParser.GetHashValue(bgIblReflections, params.t, hash_visited, syncTimestamp);
	hash << mtlParser.GetHashValue(bgIblRefractions, params.t, hash_visited, syncTimestamp);
	hash << mtlParser.GetHashValue(bgIblBackplate, params.t, hash_visited, syncTimestamp);
	hash << mtlParser.GetHashValue(bgSkyReflections, params.t, hash_visited, syncTimestamp);
	hash << mtlParser.GetHashValue(bgSkyRefractions, params.t, hash_visited, syncTimestamp);
	hash << mtlParser.GetHashValue(bgSkyBackplate, params.t, hash_visited, syncTimestamp);
	
	state.bgColor = GetCOREInterface()->GetBackGround(this->params.t, Interval());
	// environment changes

	params.pblock->GetValue(TRPARAM_BG_TYPE, params.t, state.envotype, FOREVER);
	params.pblock->GetValue(TRPARAM_BG_IBL_SOLIDCOLOR, params.t, state.envoColor, FOREVER);
	params.pblock->GetValue(TRPARAM_BG_IBL_MAP_USE, params.t, state.useIBLMap, FOREVER);
	params.pblock->GetValue(TRPARAM_BG_IBL_REFLECTIONMAP_USE, params.t, state.useReflectionMap, FOREVER);
	params.pblock->GetValue(TRPARAM_BG_IBL_REFRACTIONMAP_USE, params.t, state.useRefractionMap, FOREVER);
	params.pblock->GetValue(TRPARAM_BG_IBL_BACKPLATE_USE, params.t, state.useBackplate, FOREVER);
	params.pblock->GetValue(TRPARAM_BG_IBL_INTENSITY, params.t, state.intensity, FOREVER);
	// ground changes
	params.pblock->GetValue(TRPARAM_GROUND_ACTIVE, params.t, state.groundUse, FOREVER);
	params.pblock->GetValue(TRPARAM_GROUND_RADIUS, params.t, state.groundRadius, FOREVER);
	params.pblock->GetValue(TRPARAM_GROUND_GROUND_HEIGHT, params.t, state.groundHeight, FOREVER);
	params.pblock->GetValue(TRPARAM_GROUND_SHADOWS, params.t, state.groundShadows, FOREVER);
	params.pblock->GetValue(TRPARAM_GROUND_REFLECTIONS_STRENGTH, params.t, state.groundReflStrength, FOREVER);
	params.pblock->GetValue(TRPARAM_GROUND_REFLECTIONS, params.t, state.groundRefl, FOREVER);
	params.pblock->GetValue(TRPARAM_GROUND_REFLECTIONS_COLOR, params.t, state.groundReflColor, FOREVER);
	params.pblock->GetValue(TRPARAM_GROUND_REFLECTIONS_ROUGHNESS, params.t, state.groundReflRough, FOREVER);

	// sky changes
	params.pblock->GetValue(TRPARAM_BG_SKY_TYPE, params.t, state.skyType, FOREVER);
	params.pblock->GetValue(TRPARAM_BG_SKY_AZIMUTH, params.t, state.skyAzimuth, FOREVER);
	params.pblock->GetValue(TRPARAM_BG_SKY_ALTITUDE, params.t, state.skyAltitude, FOREVER);
	params.pblock->GetValue(TRPARAM_BG_SKY_HOURS, params.t, state.skyHours, FOREVER);
	params.pblock->GetValue(TRPARAM_BG_SKY_MINUTES, params.t, state.skyMinutes, FOREVER);
	params.pblock->GetValue(TRPARAM_BG_SKY_SECONDS, params.t, state.skySeconds, FOREVER);
	params.pblock->GetValue(TRPARAM_BG_SKY_DAY, params.t, state.skyDay, FOREVER);
	params.pblock->GetValue(TRPARAM_BG_SKY_MONTH, params.t, state.skyMonth, FOREVER);
	params.pblock->GetValue(TRPARAM_BG_SKY_YEAR, params.t, state.skyYear, FOREVER);
	params.pblock->GetValue(TRPARAM_BG_SKY_TIMEZONE, params.t, state.skyTimezone, FOREVER);
	params.pblock->GetValue(TRPARAM_BG_SKY_LATITUDE, params.t, state.skyLatitude, FOREVER);
	params.pblock->GetValue(TRPARAM_BG_SKY_LONGITUDE, params.t, state.skyLongitude, FOREVER);
	params.pblock->GetValue(TRPARAM_BG_SKY_DAYLIGHTSAVING, params.t, state.skyDaysaving, FOREVER);
	params.pblock->GetValue(TRPARAM_BG_SKY_HAZE, params.t, state.skyHaze, FOREVER);
	params.pblock->GetValue(TRPARAM_BG_SKY_DISCSIZE, params.t, state.skySunDiscSize, FOREVER);
	params.pblock->GetValue(TRPARAM_BG_SKY_INTENSITY, params.t, state.skyIntensity, FOREVER);
	params.pblock->GetValue(TRPARAM_BG_SKY_FILTER_COLOR, params.t, state.skyFilterColor, FOREVER);
	params.pblock->GetValue(TRPARAM_BG_SKY_SATURATION, params.t, state.skySaturation, FOREVER);
	params.pblock->GetValue(TRPARAM_BG_SKY_GROUND_COLOR, params.t, state.skyGroundColor, FOREVER);
	params.pblock->GetValue(TRPARAM_BG_SKY_GROUND_ALBEDO, params.t, state.skyGroundAlbedo, FOREVER);

	params.pblock->GetValue(TRPARAM_BG_SKY_REFLECTIONMAP_USE, params.t, state.useSkyReflectionMap, FOREVER);
	params.pblock->GetValue(TRPARAM_BG_SKY_REFRACTIONMAP_USE, params.t, state.useSkyRefractionMap, FOREVER);
	params.pblock->GetValue(TRPARAM_BG_SKY_BACKPLATE_USE, params.t, state.useSkyBackplate, FOREVER);

	// write these values for later access
	sceneState = state;
	
	hash << state;
	return hash;
}

bool SceneParser::NeedsUpdate()
{
	return !scene || scene.GetUserData() != GetMaxSceneHash();
}

bool SceneParser::SynchronizeView(bool forceUpdate)
{
	this->view = ParseView();

	mtlParser.shaderData.mCameraTransform = view.tm;

	return true;
}

bool SceneParser::Synchronize(bool forceUpdate)
{
#if PROFILING > 0
	profilingData.Reset();
#endif

	syncTimestamp = GetTickCount();

	mtlParser.syncTimestamp = syncTimestamp;
	hasDirectDisplacements = false;

	auto hash = GetMaxSceneHash();

	if (!forceUpdate && hash == scene.GetUserData())
		return false;

	callbacks.beforeParsing(params.t);

	auto old = parsed;
	ParseScene();

	// do geometry 

	std::map<AnimHandle, ParsedNode> oldMap, newMap, invalidatedMaterialMap;

	for (const auto& it : old.nodes)
	{
		if(!ParsedNode::isValidId(it.matId))
		{
			invalidatedMaterialMap[it.id] = it;
		}
		else if(!ParsedNode::isValidId(it.id))
		{
		}
		else if(!ParsedNode::isValidId(it.objectId))
		{
		}
		else if(old.t == params.t)
		{
			oldMap[it.id] = it;
		}	
	}

	// parsed.nodes modified in the loop
	for (auto& it : parsed.nodes)
	{
		// After the upper ParseScene(); we lose invalidationTimestamp data, so copy it from previous parsed scene data into parsed.nodes
		const auto& invalidatedIt = invalidatedMaterialMap.find(it.id);
		if(invalidatedIt != invalidatedMaterialMap.end())
		{
			it.invalidationTimestamp = invalidatedIt->second.invalidationTimestamp;
		}

		newMap[it.id] = it;
	}

	// delete or move existing shapes
	// NOTE: Because of multi-sub materials a single node may correspond to multiple FR shapes
	for (auto it : scene.GetShapes())
	{
		if (auto id = it.GetUserData())
		{
			auto newIt = newMap.find(id);
			auto oldIt = oldMap.find(id);
			if (newIt == newMap.end() || oldIt == oldMap.end())	
			{
				scene.Detach(it); // if not in new, delete, and if not in old, delete anyway because may need to be rebuilt
			}
			else
			{
				if (newIt->second.matId != oldIt->second.matId)
				{
					oldMap.erase(oldIt);	// remove from current to be readded
					scene.Detach(it);		// and detach
				}
				else 
				{
					if((newIt->second.tm == oldIt->second.tm) == 0)
					{
						it.SetTransform(newIt->second.tm);	// update transform
					}
				}
			}
		}
	}

	// delete or move existing lights
	for (auto it : scene.GetLights())
	{
		if (auto id = it.GetUserData())
		{
			auto newIt = newMap.find(id);
			auto oldIt = oldMap.find(id);
			if (newIt == newMap.end() || oldIt == oldMap.end())
			{
				scene.Detach(it); // if not in new, delete, and if not in old, delete anyway because may need to be rebuilt
			}
			else
			{
				if ((newIt->second.tm == oldIt->second.tm) == 0)
					it.SetTransform(newIt->second.tm);	// update transform
			}
		}
	}
		
	ParsedNodes toAdd;

	// and now add new ones
	for (auto it : newMap)
	{
			
		// if a new node doesn't exist in the old map
		if (!oldMap.count(it.first))
			toAdd.push_back(it.second);
	}
		
	AddParsedNodes(toAdd);
		
	for (auto shape : scene.GetShapes())
	{
		if (auto id = shape.GetUserData())
		{
			if(view.useMotionBlur && newMap.count(id))
			{
				applyMotion(shape, getMotion(view, newMap[id])); // update motion
			}
			else
			{
				resetMotion(shape);
			}
		}
	}
		
	parseEnvironment(parsed.environment.texmap);

	useFREnvironment();

	useFRGround();

	// switch on or off default lights?
	const auto& lights = scene.GetLights();
	bool hasDefaultLights = false;

	for (const auto& it : lights)
	{
		if (it.GetUserData() == DEFAULT_LIGHT_ID)
		{
			hasDefaultLights = true;
			break;
		}
	}

	int envLightCount = 0;

	if (parsed.environment.texmap)
		envLightCount++;

	if (parsed.frEnvironment.sky)
		envLightCount++;

	if ( (envLightCount + scene.LightObjectCount()) > 0)	// we have lights, so throw away oldies
	{
		if (hasDefaultLights)	// throw away any defaults that were created in last cycle
		{
			for (const auto& it : lights)
			{
				if (it.GetUserData() == DEFAULT_LIGHT_ID)
					scene.Detach(it);
			}
		}
	}
	else if (!hasDefaultLights)
	{
		parseDefaultLights(params.defaultLights);	// add some new ones!
	}
	
	callbacks.afterParsing();

	scene.SetUserData(hash);

#if PROFILING > 0
	profilingData.mSyncEnd = std::chrono::high_resolution_clock::now();
	auto delta = std::chrono::duration_cast<std::chrono::milliseconds>(profilingData.mSyncEnd - profilingData.mSyncStart).count();

	std::wstring sceneName(GetCOREInterface()->GetCurFilePath());
	uint32_t crc = GetMaxSceneHash();

	if ( sceneName.empty() )
	{
		sceneName = L"New scene";
	}

	std::string sceneNameStr = "Scene name : " + ws2s(sceneName);
	std::string crcStr = "CRC : " + std::to_string(crc);

	debugPrint("-------- Profiling Data --------");
	debugPrint(sceneNameStr);
	debugPrint(crcStr);
	debugPrint( "SceneParser::Synchronize: " + std::to_string(delta) + "ms" );
	debugPrint( "Shaders created: " + std::to_string(profilingData.shadersNum) );
	debugPrint("-------- Profiling Data --------");
#endif

	return true;
}

Mtl* SceneParser::parseMaterialPreview(frw::Shader& surfaceShader, frw::Shader& volumeShader, bool &castShadows)
{

	ParsedNodes toParse;
	traverseNode(params.sceneNode, params.rendParams.inMtlEdit == FALSE, params, toParse);
	for (auto& actual : toParse) 
	{
		callbacks.addItem(actual.node); // call all the necessary before-render callbacks
		FASSERT(actual.node);
		Object* objRef = actual.node->GetObjectRef();
		if (!objRef || !actual.node->Renderable()) {
			continue;
		}
		const Class_ID objRefCid = objRef->ClassID();

		// We specifically ignore some objects that either render garbage, or crash when we try to render them:
		// CAT parent: Class ID 0x56ae72e5, 0x389b6659
		// Delegate helper: Class ID 0x40c07baa, 0x245c7fe6
		if (objRefCid == Class_ID(0x56ae72e5, 0x389b6659) || objRefCid == Class_ID(0x40c07baa, 0x245c7fe6)) 
			continue;

		const ObjectState& state = actual.node->EvalWorldState(params.t);
		const SClass_ID sClassId = state.obj->SuperClassID();
		const Class_ID classId = state.obj->ClassID();

		const auto& tm = actual.tm;

		castShadows = true;

		if (sClassId == GEOMOBJECT_CLASS_ID || state.obj->CanConvertToType(triObjectClassID) != FALSE) 
		{
			if (!state.obj->IsRenderable() || GetDeterminant(tm) == 0.f) 
				continue; // ignore zero-scale/degenerated TM objects (which will have determinant 0)

			const auto& nodeMtls = actual.GetAllMaterials(params.t);
			if (!nodeMtls.empty())
			{
				Mtl *root = *nodeMtls.begin();
				if (root == DISABLED_MATERIAL) {
				}
				else if (root && root->ClassID() == FIRERENDER_MATERIALMTL_CID) {
					IParamBlock2* pb = root->GetParamBlock(0);
					castShadows = bool_cast( GetFromPb<BOOL>(pb, FRMaterialMtl_CAUSTICS, this->params.t) );
				}
				else if (root && root->ClassID() == FIRERENDER_UBERMTL_CID)
				{
					IParamBlock2* pb = root->GetParamBlock(0);
					castShadows = bool_cast( GetFromPb<BOOL>(pb, FRUBERMTL_FRUBERCAUSTICS, this->params.t) );
				}
				else if (root && root->ClassID() == PHYSICALMATERIAL_CID)
				{
					IParamBlock2* pb = root->GetParamBlock(0);
				}

				if (root != DISABLED_MATERIAL)
				volumeShader = mtlParser.findVolumeMaterial(root);
				//volumeShader = mtlParser.createVolumeShader(root, actual.node);

				if (root == DISABLED_MATERIAL)
				{
					surfaceShader = frw::DiffuseShader(mtlParser.materialSystem);
					surfaceShader.SetValue(RPR_MATERIAL_INPUT_COLOR, frw::Value(0.f));
				}
				else
				surfaceShader = mtlParser.createShader(root, actual.node);

				if(!surfaceShader && !volumeShader) {
					surfaceShader = frw::DiffuseShader(scope);
					surfaceShader.SetValue(RPR_MATERIAL_INPUT_COLOR, frw::Value(1, 0, 0, 1));
				}

				if(!surfaceShader && volumeShader) {
					surfaceShader = frw::Shader(mtlParser.materialSystem, frw::ShaderTypeTransparent);
				}

				return (root == DISABLED_MATERIAL) ? 0 : root;
			}
		}
	}
	
	// a default material(if not set)
	surfaceShader = frw::DiffuseShader(scope);
	return 0;
}

void SceneParser::parseDefaultLights(const Stack<DefaultLight>& defaultLights)
{
	if (defaultLights.isEmpty())
		return;

	flags.usesNonPMLights = true;

	auto context = scope.GetContext();

	for (auto& i : defaultLights)
	{
		auto light = context.CreateDirectionalLight();

		Color color = i.ls.color;
		color *= PI; // it seems we need to multiply by pi to get same intensity in RPR and 3dsmax
		light.SetRadiantPower(color.r, color.g, color.b);

		light.SetTransform(fxLightTm(i.tm));
		light.SetUserData(DEFAULT_LIGHT_ID);

		scene.Attach(light);
		parsed.defaultLightCount++;
	}
}

void ApplyFRGround(frw::Scope& scope, frw::MaterialSystem& materialSystem, MaterialParser& materialParser, frw::Scene& scene, SceneParser::ParsedGround parsedGround)
{
	auto context = scope.GetContext();
	auto ms = materialSystem;

	float width = parsedGround.radius*2.0f;
	float length = parsedGround.radius*2.0f;
	float w2 = width * 0.5f;
	float h2 = length * 0.5f;

	float z = parsedGround.groundHeight;

	Point3 points[4] = {
		Point3(-w2, -h2, z),
		Point3(w2, -h2, z),
		Point3(w2, h2, z),
		Point3(-w2, h2, z)
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
		0, 0, 0,
		0, 0, 0
	};

	rpr_int num_face_vertices[] = { 3, 3, 3, 3 };

	if (parsedGround.hasShadows)
	{
		auto shape = context.CreateMesh(
			(rpr_float const*)&points[0], 4, sizeof(Point3),
			(rpr_float const*)&normals[0], 2, sizeof(Point3),
			nullptr, 0, 0,
			(rpr_int const*)indices, sizeof(rpr_int),
			(rpr_int const*)normal_indices, sizeof(rpr_int),
			nullptr, 0,
			num_face_vertices, 2);

		frw::DiffuseShader diffuse(ms);
		diffuse.SetColor(frw::Value(0.0f));

		shape.SetShader(diffuse);

		shape.SetShadowCatcherFlag(true);

		shape.SetUserData(ENVIRONMENT_GROUND_ID);
		scene.Attach(shape);
	}

	if (parsedGround.hasReflections)
	{
		auto shape = context.CreateMesh(
			(rpr_float const*)&points[0], 4, sizeof(Point3),
			(rpr_float const*)&normals[0], 2, sizeof(Point3),
			nullptr, 0, 0,
			(rpr_int const*)indices, sizeof(rpr_int),
			(rpr_int const*)normal_indices, sizeof(rpr_int),
			nullptr, 0,
			num_face_vertices, 2);

		Matrix3 tm;
		tm.IdentityMatrix();
		tm.Translate(parsedGround.tm.GetTrans());
		shape.SetTransform(tm);

		frw::Shader reflection(ms, frw::ShaderTypeMicrofacet);

		reflection.SetValue(RPR_MATERIAL_INPUT_ROUGHNESS, materialSystem.ValuePow(frw::Value(parsedGround.reflectionsRoughness), 0.5));

		reflection.SetValue(RPR_MATERIAL_INPUT_COLOR, frw::Value(
			parsedGround.reflectionsColor.r,
			parsedGround.reflectionsColor.g,
			parsedGround.reflectionsColor.b));

		frw::Shader transparency(ms, frw::ShaderTypeTransparent);
		transparency.SetValue(RPR_MATERIAL_INPUT_COLOR, frw::Value(1.0f));

		Point3 p = parsedGround.tm.GetTrans();

		auto distanceToCenterNormalized = ms.ValueDiv(ms.ValueMagnitude(ms.ValueSub(ms.ValueLookupP(),
			frw::Value(p.x, p.y, p.z))), parsedGround.radius);

		//fade-out reflection at the edge of the ground disc
		auto distAlpha = ms.ValuePow(
			ms.ValueClamp(ms.ValueSub(1.0, distanceToCenterNormalized)), 2.0f);

		shape.SetShader(ms.ShaderBlend(transparency, reflection, distAlpha));
		shape.SetShadowFlag(false);
		shape.SetShadowCatcherFlag(false);
		shape.SetUserData(ENVIRONMENT_GROUND_ID + 1);

		frw::Shader test(ms, frw::ShaderTypeEmissive);

		test.SetValue(RPR_MATERIAL_INPUT_COLOR, distAlpha);

		scene.Attach(shape);
	}
}

void ApplyFREnvironment(frw::Scope& scope, frw::MaterialSystem& materialSystem, MaterialParser& materialParser, frw::Scene& scene, SceneParser::ParsedEnvironment parsedEnv)
{
	auto context = scope.GetContext();
	auto ms = materialSystem;

	if(parsedEnv.backgroundMap)
	{
		Matrix3 tm = parsedEnv.tm;

		auto map = parsedEnv.backgroundMap;

		Matrix3 matTm;
		matTm.IdentityMatrix();

		//explicitly support only Screen and Spherical mapping
		//Background(IBL) is used for spherical mapping
		//BackgroundImage(backplate) used for other mappings(same as Screen)
		bool useIBL = false;
		if (auto uvGen = map->GetTheUVGen())
		{
			uvGen->GetUVTransform(matTm);

			bool isMappingTypeEnvironment = (uvGen->GetSlotType() == MAPSLOT_ENVIRON);

			if(isMappingTypeEnvironment)
			{
				if (auto gen = dynamic_cast<StdUVGen*>(uvGen)) {
					useIBL = UVMAP_SPHERE_ENV == gen->GetCoordMapping(0);
				}
			}
		}

		auto image = materialParser.createImageFromMap(map, MAP_FLAG_WANTSHDR);

		if(useIBL)
		{
			auto ibl = context.CreateEnvironmentLight();

			float scale = parsedEnv.backgroundIntensity;
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
				tx.PreRotateX((float)M_PI_2);
				tx.PreRotateY((float)M_PI);

				ibl.SetTransform(tx);
			}

			ibl.SetUserData(BACKGROUND_IBL_ID);
			scene.SetBackground(ibl);
			scene.SetBackgroundImage(frw::Image());
		}
		else
		{
			scene.SetBackground(frw::EnvironmentLight());
			scene.SetBackgroundImage(image);
		}
	}
	else
	{
		Color envColor = parsedEnv.envColor;
		if (avg(envColor) > 0.0f)
		{
			scene.SetBackground(frw::EnvironmentLight());
			scene.SetBackgroundImage(frw::Image());
		}
	}
}

void SceneParser::useFRGround()
{
	if (parsed.frGround.enabled)
	{
		ApplyFRGround(scope, scope.GetMaterialSystem(), mtlParser, scope.GetScene(), parsed.frGround);
	}
}

bool SceneParser::parseFRGround()
{
	BOOL res;
	res = params.pblock->GetValue(TRPARAM_GROUND_ACTIVE, params.t, parsed.frGround.enabled, FOREVER); FASSERT(res);
	res = params.pblock->GetValue(TRPARAM_GROUND_RADIUS, params.t, parsed.frGround.radius, FOREVER); FASSERT(res);
	res = params.pblock->GetValue(TRPARAM_GROUND_GROUND_HEIGHT, params.t, parsed.frGround.groundHeight, FOREVER); FASSERT(res);
	res = params.pblock->GetValue(TRPARAM_GROUND_SHADOWS, params.t, parsed.frGround.hasShadows, FOREVER); FASSERT(res);
	res = params.pblock->GetValue(TRPARAM_GROUND_REFLECTIONS, params.t, parsed.frGround.hasReflections, FOREVER); FASSERT(res);
	res = params.pblock->GetValue(TRPARAM_GROUND_REFLECTIONS_COLOR, params.t, parsed.frGround.reflectionsColor, FOREVER); FASSERT(res);
	res = params.pblock->GetValue(TRPARAM_GROUND_REFLECTIONS_ROUGHNESS, params.t, parsed.frGround.reflectionsRoughness, FOREVER); FASSERT(res);
	return true;
}

void SceneParser::useFREnvironment()
{
	if(parsed.frEnvironment.enabled)
	{
		ApplyFREnvironment(scope, scope.GetMaterialSystem(), mtlParser, scope.GetScene(), parsed.frEnvironment);
	}
}

bool SceneParser::parseFREnvironment(const ParsedNode& parsedNode, Object* object)
{
	if (params.progress)
		params.progress->SetTitle(_T("Synchronizing: Environment"));

	// the presence of the object in the scene tells us that we need to use our own environment
	int bgType = 0;
	params.pblock->GetValue(TRPARAM_BG_TYPE, params.t, bgType, FOREVER);

	BOOL bgUsebgMap = FALSE;
	BOOL bgUsereflMap = FALSE;
	BOOL bgUserefrMap = FALSE;
	Texmap *bgMap = 0;
	Texmap *bgReflMap = 0;
	Texmap *bgRefrMap = 0;

	float bgIntensity = 1.f;
	if (bgType == FRBackgroundType_IBL)
	{
		params.pblock->GetValue(TRPARAM_BG_IBL_MAP_USE, params.t, bgUsebgMap, FOREVER);
		params.pblock->GetValue(TRPARAM_BG_IBL_REFLECTIONMAP_USE, params.t, bgUsereflMap, FOREVER);
		params.pblock->GetValue(TRPARAM_BG_IBL_REFRACTIONMAP_USE, params.t, bgUserefrMap, FOREVER);
		if (bgUsebgMap)
			params.pblock->GetValue(TRPARAM_BG_IBL_MAP, params.t, bgMap, FOREVER);
		if (bgUsereflMap)
			params.pblock->GetValue(TRPARAM_BG_IBL_REFLECTIONMAP, params.t, bgReflMap, FOREVER);
		if (bgUserefrMap)
			params.pblock->GetValue(TRPARAM_BG_IBL_REFRACTIONMAP, params.t, bgRefrMap, FOREVER);
		params.pblock->GetValue(TRPARAM_BG_IBL_INTENSITY, params.t, bgIntensity, FOREVER);
	}
	else
	{
		params.pblock->GetValue(TRPARAM_BG_SKY_REFLECTIONMAP_USE, params.t, bgUsereflMap, FOREVER);
		params.pblock->GetValue(TRPARAM_BG_SKY_REFRACTIONMAP_USE, params.t, bgUserefrMap, FOREVER);
		if (bgUsereflMap)
			params.pblock->GetValue(TRPARAM_BG_SKY_REFLECTIONMAP, params.t, bgReflMap, FOREVER);
		if (bgUserefrMap)
			params.pblock->GetValue(TRPARAM_BG_SKY_REFRACTIONMAP, params.t, bgRefrMap, FOREVER);
		params.pblock->GetValue(TRPARAM_BG_SKY_INTENSITY, params.t, bgIntensity, FOREVER);
	}

	
	parsed.frEnvironment.enabled = true;
	parsed.frEnvironment.tm = parsedNode.tm;
	parsed.frEnvironment.envMap = bgMap;
	parsed.frEnvironment.envReflMap = bgReflMap;
	parsed.frEnvironment.envRefrMap = bgRefrMap;
	params.pblock->GetValue(TRPARAM_BG_IBL_SOLIDCOLOR, params.t, parsed.frEnvironment.envColor, FOREVER);
	parsed.frEnvironment.intensity = bgIntensity;
	
	parsed.frEnvironment.sky = (bgType == FRBackgroundType_SunSky);
	
	Texmap *bgBackplate = 0;
	BOOL bgUseBackplate = FALSE;

	if (bgType == FRBackgroundType_IBL)
	{
		params.pblock->GetValue(TRPARAM_BG_IBL_BACKPLATE_USE, params.t, bgUseBackplate, FOREVER);
		if (bgUseBackplate)
			params.pblock->GetValue(TRPARAM_BG_IBL_BACKPLATE, params.t, bgBackplate, FOREVER);
	}
	else
	{
		params.pblock->GetValue(TRPARAM_BG_SKY_BACKPLATE_USE, params.t, bgUseBackplate, FOREVER);
		if (bgUseBackplate)
			params.pblock->GetValue(TRPARAM_BG_SKY_BACKPLATE, params.t, bgBackplate, FOREVER);
	}
	
	
	parsed.frEnvironment.backgroundIntensity = 1.0f;
	parsed.frEnvironment.backgroundMap = bgBackplate;

	if(bgUsebgMap && parsed.frEnvironment.envMap)
		callbacks.addItem(parsed.frEnvironment.envMap);
	if (bgUseBackplate && parsed.frEnvironment.backgroundMap)
		callbacks.addItem(parsed.frEnvironment.backgroundMap);
	if (bgUsereflMap && parsed.frEnvironment.envReflMap)
		callbacks.addItem(parsed.frEnvironment.envReflMap);
	if (bgUserefrMap && parsed.frEnvironment.envRefrMap)
		callbacks.addItem(parsed.frEnvironment.envRefrMap);

	return true;
}

bool SceneParser::parseEnvironment(Texmap* enviroMap) 
{
	bool ret = false;

	auto context = scope.GetContext();

	frw::Image enviroImage;		// an image that if non-null will be used as scene environment light
	frw::Image enviroReflImage;	// an image that if non-null will be used as environment reflections override light
	frw::Image enviroRefrImage;	// an image that if non-null will be used as environment refractions override light

	Texmap *enviroReflMap = 0;
	Texmap *enviroRefrMap = 0;

	bool useEnvColor = false;
    Color envColor = this->params.rendParams.inMtlEdit ? Color(0.25f, 0.25f, 0.25f) : sceneState.bgColor;
    
	float scale = 1.f;

	Matrix3 tm;
	tm.IdentityMatrix();

	// use our own render settings' environment
	if (parsed.frEnvironment.enabled && !this->params.rendParams.inMtlEdit)
	{
		tm = parsed.frEnvironment.tm;

		// use sun-sky
		if (parsed.frEnvironment.sky)
		{
			enviroImage = BgManagerMax::TheManager.GenerateSky(scope, params.pblock, params.t, parsed.frEnvironment.intensity);
		}
		else // use IBL
		{
			envColor = parsed.frEnvironment.envColor;
			if (parsed.frEnvironment.envMap)
			{
				enviroMap = parsed.frEnvironment.envMap;
				callbacks.addItem(enviroMap);
				enviroImage = mtlParser.createImageFromMap(enviroMap, MAP_FLAG_WANTSHDR);
			}
			else
				useEnvColor = true;
		}
		scale *= parsed.frEnvironment.intensity;

		if (parsed.frEnvironment.envReflMap)
		{
			enviroReflMap = parsed.frEnvironment.envReflMap;
			callbacks.addItem(enviroReflMap);
			enviroReflImage = mtlParser.createImageFromMap(parsed.frEnvironment.envReflMap, MAP_FLAG_NOFLAGS);
		}
		if (parsed.frEnvironment.envRefrMap)
		{
			enviroRefrMap = parsed.frEnvironment.envRefrMap;
			callbacks.addItem(enviroRefrMap);
			enviroRefrImage = mtlParser.createImageFromMap(parsed.frEnvironment.envRefrMap, MAP_FLAG_NOFLAGS);
		}
	}
	else
	{
		if (enviroMap && !this->params.rendParams.inMtlEdit)
		{ // if outside mtl editor, we may parse an environment texture
			callbacks.addItem(enviroMap);
			enviroImage = mtlParser.createImageFromMap(enviroMap, MAP_FLAG_WANTSHDR);
		}
		else
			useEnvColor = true;
	}

	if (useEnvColor && avg(envColor) > 0.f)
	{ 
        // we need a constant-value environment map. Since RPR does not currently support it directly, we do a work-around
        // by creating a single-pixel image of required color.
		
		// Note: This image is set to 2x2 as there is a bug in RPR core
		// v1.251 which causes IBL with portals to emit the wrong color.
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
	

    // Create the environment light if there is any environment present
    if (enviroImage || enviroReflImage || enviroRefrImage)
	{
		if (enviroImage)
			enviroLight = context.CreateEnvironmentLight();
		if (enviroReflImage)
			enviroReflLight = context.CreateEnvironmentLight();
		if (enviroRefrImage)
			enviroRefrLight = context.CreateEnvironmentLight();

		if(enviroMap)
		{
			if (auto map = dynamic_cast<BitmapTex*>(enviroMap->GetInterface(BITMAPTEX_INTERFACE)))
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
			if (parsed.frEnvironment.sky)
			{
				// analytical sky, should use sun's azimuth
				float sunAzimuth, sunAltitude;
				BgManagerMax::TheManager.GetSunPosition(params.pblock, params.t, sunAzimuth, sunAltitude);
				angle = -sunAzimuth / 180.0f * M_PI - M_PI * 0.5;
			}
			// flip yz
			tx.PreRotateX((float)M_PI_2);
			tx.PreRotateY((float)M_PI);
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
			enviroLight.SetUserData(ENVIRONMENT_STANDARD_ID);
			scene.Attach(enviroLight);
		}
		if (enviroReflImage)
		{
			enviroReflLight.SetUserData(ENVIRONMENT_STANDARD_ID);
			scene.SetBackgroundReflectionsOverride(enviroReflLight);
		}
		if (enviroRefrImage)
		{
			enviroRefrLight.SetUserData(ENVIRONMENT_STANDARD_ID);
			scene.SetBackgroundRefractionsOverride(enviroRefrLight);
		}
		
		// PORTAL LIGHTS
		std::vector<frw::Shape> &portals = parsed.frEnvironment.portals;
		if(parsed.frEnvironment.enabled && !portals.empty())
		{
			for (auto shape = portals.begin(); shape != portals.end(); shape++){
				enviroLight.AttachPortal(*shape);
			}
		}

		ret = true;
    }
	else
	{
		//scene.SetEnvironment(frw::EnvironmentLight());
	}

	if (!enviroReflImage)
	{
		scene.SetBackgroundReflectionsOverride(frw::EnvironmentLight());
	}
	if (!enviroRefrImage)
	{
		scene.SetBackgroundRefractionsOverride(frw::EnvironmentLight());
	}

	return ret;
}

void SceneParser::parseFRPortal(const ParsedNode& parsedNode, Object* evaluatedObject)
{
	auto node = parsedNode.node;
	auto tm = parsedNode.tm;
	BOOL parity = tm.Parity();
	auto context = scope.GetContext();

	IFireRenderPortalLight *pPortal = dynamic_cast<IFireRenderPortalLight *>(evaluatedObject);
	FASSERT(pPortal);

	Point3 Vertices[4];
	pPortal->GetCorners(Vertices[0], Vertices[1], Vertices[2], Vertices[3]);

	Point3 points[8] = {
		Point3(Vertices[0].x * masterScale, Vertices[0].y * masterScale, Vertices[0].z * masterScale),
		Point3(Vertices[1].x * masterScale, Vertices[1].y * masterScale, Vertices[1].z * masterScale),
		Point3(Vertices[2].x * masterScale, Vertices[2].y * masterScale, Vertices[2].z * masterScale),
		Point3(Vertices[3].x * masterScale, Vertices[3].y * masterScale, Vertices[3].z * masterScale),

		Point3(Vertices[0].x * masterScale, Vertices[0].y * masterScale, Vertices[0].z * masterScale),
		Point3(Vertices[1].x * masterScale, Vertices[1].y * masterScale, Vertices[1].z * masterScale),
		Point3(Vertices[2].x * masterScale, Vertices[2].y * masterScale, Vertices[2].z * masterScale),
		Point3(Vertices[3].x * masterScale, Vertices[3].y * masterScale, Vertices[3].z * masterScale),
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
	shape.SetUserData(parsedNode.id);

	auto ms = mtlParser.materialSystem;

	shape.SetShadowFlag(false);

	SetNameFromNode(node, shape);

	parsed.frEnvironment.portals.push_back(shape);
}

void SceneParser::parseMaxLight(const ParsedNode& parsedNode, Object* evaluatedObject) 
{
	if (params.progress)
		params.progress->SetTitle(_T("Synchronizing: Rebuilding Light"));

	auto node = parsedNode.node;
	auto tm = parsedNode.tm;
	BOOL parity = tm.Parity();

	auto context = scope.GetContext();

    //Try to match a RPR point light object to 3ds Max native light shader. Corona area lights are handled separately.
#ifndef DISABLE_MAX_LIGHTS

    GenLight* light = dynamic_cast<GenLight*>(evaluatedObject);
    if (!light || (light && !light->GetUseLight())) {
        return;
    }
    
	float intensity = light->GetIntensity(this->params.t);

	rpr_int res = 0;
    rpr_light fireLight = 0; // output light. A specific type used is determined by the input light type

	// PHOTOMETRIC LIGHTS
	//
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
							visibleInRender = aca->IsLightShapeRenderingEnabled(this->params.t);
						break;
					}
				}
			}
		}

		int renderable = phlight->IsRenderable();

		intensity = phlight->GetResultingIntensity(this->params.t); // intensity  in Cd

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
					Point3 color = phlight->GetRGBFilter(this->params.t) * phlight->GetRGBColor(this->params.t) * watts;

					res = rprPointLightSetRadiantPower3f(fireLight, color.x, color.y, color.z);
					FCHECK(res);
				}
				else if (type == LightscapeLight::AREA_TYPE || type == LightscapeLight::TARGET_AREA_TYPE)
				{
					float width = phlight->GetWidth(this->params.t) * masterScale;
					float length = phlight->GetLength(this->params.t) * masterScale;
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
					shape.SetUserData(parsedNode.id);

					auto ms = mtlParser.materialSystem;
					frw::EmissiveShader material(ms);

					float lumens = phlight->GetResultingFlux(this->params.t);
					float watts = lumens / 683.f;

					float area = 2.f * width * length;
					watts /= area;

					Point3 color = phlight->GetRGBFilter(this->params.t) * phlight->GetRGBColor(this->params.t) * watts;
					material.SetColor(frw::Value(color.x, color.y, color.z));
					shape.SetShader(material);

					shape.SetShadowFlag(false);
					shape.SetPrimaryVisibility( bool_cast(visibleInRender) );

					SetNameFromNode(node, shape);
					scene.Attach(shape);

					return;
				}
				else if (type == LightscapeLight::DISC_TYPE || type == LightscapeLight::TARGET_DISC_TYPE)
				{
					int numpoints = phlight->GetShape(0, 0);
					std::vector<Point3> points((numpoints << 1) + 2);
					phlight->GetShape(&points[0], numpoints);
					for (int i = 0; i < numpoints; i++)
					{
						points[i] *= masterScale;
						points[numpoints + i] = points[i];
						points[i].z += 0.0005f;
						points[numpoints + i].z -= 0.0005f;
					}

					int centerTop = int_cast(points.size() - 1);
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
					shape.SetUserData(parsedNode.id);

					auto ms = mtlParser.materialSystem;
					frw::EmissiveShader material(ms);

					float lumens = phlight->GetResultingFlux(this->params.t);
					float watts = lumens / 683.f;

					float radius = phlight->GetRadius(this->params.t) * masterScale;;
					float area = 2.f * PI * radius * radius; // both sides so we do not divide by two
					watts /= area;

					Point3 color = phlight->GetRGBFilter(this->params.t) * phlight->GetRGBColor(this->params.t) * watts;
					material.SetColor(frw::Value(color.x, color.y, color.z));
					shape.SetShader(material);

					shape.SetShadowFlag(false);
					shape.SetPrimaryVisibility( bool_cast(visibleInRender) );

					SetNameFromNode(node, shape);
					scene.Attach(shape);

					return;
				}
				else if (type == LightscapeLight::SPHERE_TYPE || type == LightscapeLight::TARGET_SPHERE_TYPE)
				{
					float radius = phlight->GetRadius(this->params.t) * masterScale;
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
					int nbTriangles = nbLong*2+(nbLat - 1)*nbLong*2;//2 caps and one middle
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
						// TODO: triangles is array of int32, but vsize is int64, should vsize be int32? 
						triangles[i++] = int_cast( vsize - 1 );
						triangles[i++] = int_cast( vsize - (lon + 2) - 1 );
						triangles[i++] = int_cast( vsize - (lon + 1) - 1 );
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
					shape.SetUserData(parsedNode.id);

					auto ms = mtlParser.materialSystem;

					frw::EmissiveShader material(ms);

					float lumens = phlight->GetResultingFlux(this->params.t);
					float watts = lumens / 683.f;

					float area = 4 * PI * radius * radius;
					watts /= area;

					Point3 color = phlight->GetRGBFilter(this->params.t) * phlight->GetRGBColor(this->params.t) * watts;
					material.SetColor(frw::Value(color.x, color.y, color.z));
					shape.SetShader(material);

					shape.SetShadowFlag(false);
					shape.SetPrimaryVisibility( bool_cast(visibleInRender) );

					SetNameFromNode(node, shape);
					scene.Attach(shape);

					return;
				}
				else if (type == LightscapeLight::CYLINDER_TYPE || type == LightscapeLight::TARGET_CYLINDER_TYPE)
				{
					int segments = 16;
					float length = phlight->GetLength(this->params.t) * masterScale;
					float hlength = length * 0.5f;
					float radius = phlight->GetRadius(this->params.t) * masterScale;
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
					shape.SetUserData(parsedNode.id);

					auto ms = mtlParser.materialSystem;
					frw::EmissiveShader material(ms);

					float lumens = phlight->GetResultingFlux(this->params.t);
					float watts = lumens / 683.f;

					float area = 2 * PI * radius * length;
					watts /= area;
				
					//Point3 color = Normalize(phlight->GetRGBFilter(this->params.t) * phlight->GetRGBColor(this->params.t)) * watts;
					Point3 color = phlight->GetRGBFilter(this->params.t) * phlight->GetRGBColor(this->params.t) * watts;
					material.SetColor(frw::Value(color.x, color.y, color.z));
					shape.SetShader(material);

					shape.SetShadowFlag(false);
					shape.SetPrimaryVisibility( bool_cast(visibleInRender) );

					SetNameFromNode(node, shape);
					scene.Attach(shape);

					return;
				}
				else if (type == LightscapeLight::LINEAR_TYPE || type == LightscapeLight::TARGET_LINEAR_TYPE)
				{
					int segments = 16;
					float length = phlight->GetLength(this->params.t) * masterScale;
					float hlength = length * 0.5f;
					float radius = 0.003f;//tested wtih CPU  render, smaller radius crates grid patern(like 0.001 - very visible)
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
					shape.SetUserData(parsedNode.id);


					auto ms = mtlParser.materialSystem;
					frw::EmissiveShader material(ms);

					float lumens = phlight->GetResultingFlux(this->params.t);
					float watts = lumens / 683.f;
					
					float area = 2 * PI * radius * length;
					watts /= area;

					//Point3 color = Normalize(phlight->GetRGBFilter(this->params.t) * phlight->GetRGBColor(this->params.t)) * watts;
					Point3 color = phlight->GetRGBFilter(this->params.t) * phlight->GetRGBColor(this->params.t) * watts;
					material.SetColor(frw::Value(color.x, color.y, color.z));
					shape.SetShader(material);

					shape.SetShadowFlag(false);
					shape.SetPrimaryVisibility( bool_cast(visibleInRender) );

					SetNameFromNode(node, shape);
					scene.Attach(shape);

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
					if (intensity > 0)
					{
						res = rprContextCreateSpotLight(context.Handle(), &fireLight);
						FCHECK_CONTEXT(res, context.Handle(), "rprContextCreateSpotLight");
						const float iAngle = DEG_TO_RAD*light->GetHotspot(this->params.t) * 0.5f;
						const float oAngle = DEG_TO_RAD*light->GetFallsize(this->params.t) * 0.5f;

						float lumens = phlight->GetResultingFlux(this->params.t);
						float steradians = lumens / intensity;
						float watts = lumens / ((1700.f * steradians) / (4.f * PI)); // scotopic
						Point3 color = phlight->GetRGBFilter(this->params.t) * phlight->GetRGBColor(this->params.t) * watts;

						res = rprSpotLightSetRadiantPower3f(fireLight, color.x, color.y, color.z);
						FCHECK(res);
						res = rprSpotLightSetConeShape(fireLight, iAngle, oAngle);
						FCHECK(res);
					}
				}
				// other types are currently not supported by FR
				else
					return;
			}
			break;

			case LightscapeLight::WEB_DIST:
			{
				if (type == LightscapeLight::TARGET_POINT_TYPE || type == LightscapeLight::POINT_TYPE)
				{
					try{
						std::string iesData((std::istreambuf_iterator<char>(std::ifstream(phlight->GetFullWebFileName()))),
							std::istreambuf_iterator<char>());

						// we don't need to be brutal, web lights can have an empty filename especially while being created and active shade is on
						if (!iesData.empty())
						{
							res = rprContextCreateIESLight(context.Handle(), &fireLight);
							FCHECK_CONTEXT(res, context.Handle(), "rprContextCreateIESLight");

							res = rprIESLightSetImageFromIESdata(fireLight, iesData.c_str(), 256, 256);
						if(RPR_SUCCESS!=res){
							throw std::runtime_error("rprIESLightSetImageFromIESdata failed with code:"+std::to_string(res));
						}
						
						// we are passing a simple scale factor (1 Lm to 1 W) however we are missing geometrical
						// information for an accurate conversion. Specifically we would need the light's Apex angle.
						// Perhaps the rendering engine could return it as a light query option. Or perhaps it could
						// handle the conversion altogether, so the rprIESLightSetRadiantPower3f would actually be used
						// to pass an optional dimming/scaling factor and coloration.
						Point3 originalColor = phlight->GetRGBFilter(this->params.t);
						Point3 color = originalColor * ((intensity / 682.069f) / 683.f) / 2.5f;

						res = rprIESLightSetRadiantPower3f(fireLight, color.x, color.y, color.z);

						FCHECK(res);

						Matrix3 r;
						r.IdentityMatrix();
						r.PreRotateX(-DegToRad(phlight->GetWebRotateX()));
						r.PreRotateY(DegToRad(phlight->GetWebRotateY()));
						r.PreRotateZ(DegToRad(phlight->GetWebRotateZ()));
						
						//rotation doesn't affect ies lights in fr - see AMDMAX-641
						// - use GetWebRotateX/Y/Z 
						// - fixup rotation from max's to FR's

						//copied transform fixup from Environment
						Matrix3 tx;
						tx.IdentityMatrix();
						// flip yz
						tx.PreRotateX((float)M_PI_2);
						tx.PreRotateY((float)M_PI);

						tm = tx*r*tm;
						}
					}
					catch(std::exception& error)
					{
						if(fireLight)
							fireLight = 0;
						std::string errorMsg(error.what());
						
						MessageBox(0, 
							(std::wstring(node->GetName())+L" - failed to create IES light:\n"+std::wstring(errorMsg.begin(), errorMsg.end())).c_str(), L"Radeon ProRender Error", MB_ICONERROR | MB_OK);
					}

				} else
				{
					MessageBox(0, 
						(std::wstring(node->GetName())+L" - Web Distribution is supported with Point light shape only").c_str(), L"Radeon ProRender Error", MB_ICONERROR | MB_OK);
					fireLight = 0;
				}
			}
			break;

			case LightscapeLight::DIFFUSE_DIST:
			{
				if (type == LightscapeLight::AREA_TYPE || type == LightscapeLight::TARGET_AREA_TYPE)
				{
					float width = phlight->GetWidth(this->params.t) * masterScale;
					float length = phlight->GetLength(this->params.t) * masterScale;
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
					shape.SetUserData(parsedNode.id);

					auto ms = mtlParser.materialSystem;
					frw::EmissiveShader material(ms);

					float lumens = phlight->GetResultingFlux(this->params.t);
					float watts = lumens / 683.f;

					float area = 2.f * width * length;
					watts /= area;

					Point3 color = phlight->GetRGBFilter(this->params.t) * phlight->GetRGBColor(this->params.t) * watts;
					material.SetColor(frw::Value(color.x, color.y, color.z));
					shape.SetShader(material);

					shape.SetShadowFlag(false);
					shape.SetPrimaryVisibility( bool_cast(visibleInRender) );

					SetNameFromNode(node, shape);
					scene.Attach(shape);

					return;
				}
				else if (type == LightscapeLight::DISC_TYPE || type == LightscapeLight::TARGET_DISC_TYPE)
				{
					int numpoints = phlight->GetShape(0, 0);
					std::vector<Point3> points(numpoints + 1);
					phlight->GetShape(&points[0], numpoints);
					for (auto i = points.begin(); i < points.end(); i++)
						*i *= masterScale;
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
					shape.SetUserData(parsedNode.id);

					auto ms = mtlParser.materialSystem;
					frw::EmissiveShader material(ms);

					float lumens = phlight->GetResultingFlux(this->params.t);
					float watts = lumens / 683.f;

					float radius = phlight->GetRadius(this->params.t) * masterScale;
					float area = PI * radius * radius;
					watts /= area;

					Point3 color = phlight->GetRGBFilter(this->params.t) * phlight->GetRGBColor(this->params.t) * watts;
					material.SetColor(frw::Value(color.x, color.y, color.z));
					shape.SetShader(material);

					shape.SetShadowFlag(false);
					shape.SetPrimaryVisibility( bool_cast(visibleInRender) );

					SetNameFromNode(node, shape);
					scene.Attach(shape);

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
					int nbTriangles = nbLong + (nbLat - 1)*nbLong*2;
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
					shape.SetUserData(parsedNode.id);

					auto ms = mtlParser.materialSystem;

					frw::EmissiveShader material(ms);

					float lumens = phlight->GetResultingFlux(this->params.t);
					float watts = lumens / 683.f;

					float area = 4 * PI * radius * radius * 0.5;
					watts /= area;

					Point3 color = phlight->GetRGBFilter(this->params.t) * phlight->GetRGBColor(this->params.t) * watts;
					material.SetColor(frw::Value(color.x, color.y, color.z));
					shape.SetShader(material);

					shape.SetShadowFlag(false);
					shape.SetPrimaryVisibility( bool_cast(visibleInRender) );

					SetNameFromNode(node, shape);
					scene.Attach(shape);

					return;
				}
				else if (type == LightscapeLight::LINEAR_TYPE || type == LightscapeLight::TARGET_LINEAR_TYPE)
				{
					int segments = 16;
					float length = phlight->GetLength(this->params.t) * masterScale;
					float hlength = length * 0.5f;
					float radius = 0.01f;//tested wtih CPU  render, smaller radius crates grid patern(like 0.001 - very visible)
					Point3 centerTop(0.f, -hlength, 0.f);
					Point3 centerBottom(0.f, hlength, 0.f);
					float step = (PI) / float(segments);
					float angle = 0.f;
					std::vector<Point3> pshape;
					for (int i = 0; i <=segments; i++, angle += step)
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

					for (int i = 0; i < (numPointsShape-1); i++)
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
					shape.SetUserData(parsedNode.id);


					auto ms = mtlParser.materialSystem;
					frw::EmissiveShader material(ms);

					float lumens = phlight->GetResultingFlux(this->params.t);
					float watts = lumens / 683.f;
					
					float area = 2 * PI * radius * length * 0.5;
					watts /= area;

					Point3 color = phlight->GetRGBFilter(this->params.t) * phlight->GetRGBColor(this->params.t) * watts;
					material.SetColor(frw::Value(color.x, color.y, color.z));
					shape.SetShader(material);

					shape.SetShadowFlag(false);
					shape.SetPrimaryVisibility( bool_cast(visibleInRender) );

					SetNameFromNode(node, shape);
					scene.Attach(shape);

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
		flags.usesNonPMLights = true;

		// STANDARD LIGHTS
		//
		Color color = light->GetRGBColor(this->params.t) * intensity;

		if (light->IsDir())
		{ // A directional light (located in infinity)
			res = rprContextCreateDirectionalLight(context.Handle(), &fireLight);
			FCHECK_CONTEXT(res, context.Handle(), "rprContextCreateDirectionalLight");
			color *= PI; // it seems we need to multiply by pi to get same intensity in RPR and 3dsmax
			res = rprDirectionalLightSetRadiantPower3f(fireLight, color.r, color.g, color.b);
			FCHECK(res);
		}
		else if (light->IsSpot()) { // Spotlight (point light with non-uniform directional distribution)
			res = rprContextCreateSpotLight(context.Handle(), &fireLight);
			FCHECK_CONTEXT(res, context.Handle(), "rprContextCreateSpotLight");

			const float iAngle = DEG_TO_RAD*light->GetHotspot(this->params.t) * 0.5f;
			const float oAngle = DEG_TO_RAD*light->GetFallsize(this->params.t) * 0.5f;
			color *= 683.f * PI; // should be 4PI (steradians in a sphere) doh
			res = rprSpotLightSetRadiantPower3f(fireLight, color.r, color.g, color.b);
			FCHECK(res);
			res = rprSpotLightSetConeShape(fireLight, iAngle, oAngle);
			FCHECK(res);
		}
		else if (evaluatedObject->ClassID() == Class_ID(0x7bf61478, 0x522e4705)) { //standard Skylight object
			res = rprContextCreateSkyLight(context.Handle(), &fireLight);		
			FCHECK_CONTEXT(res, context.Handle(), "rprContextCreateSkyLight");
			float intensity = light->GetIntensity(this->params.t);
			res = rprSkyLightSetScale(fireLight, (intensity * 0.2f));
			FCHECK(res);
			Point3 color = light->GetRGBColor(this->params.t);
			res = rprSkyLightSetAlbedo(fireLight, (color.x + color.y + color.z) * 1.f / 3.f);
			FCHECK(res);
		}
		else { // point light with uniform directional distribution
			res = rprContextCreatePointLight(context.Handle(), &fireLight);
			FCHECK_CONTEXT(res, context.Handle(), "rprContextCreatePointLight");
			color *= 683.f * PI; // should be 4PI (steradians in a sphere) doh
			res = rprPointLightSetRadiantPower3f(fireLight, color.r, color.g, color.b);
			FCHECK(res);
		}
	}

	if (fireLight)
	{
		frw::Light light(fireLight, context);
		light.SetTransform(fxLightTm(tm));
		light.SetUserData(parsedNode.id);
		SetNameFromNode(node, light);
		scene.Attach(light);
	}

#endif
}

void SceneParser::parseCoronaSun(const ParsedNode& parsedNode, Object* object)
{
	auto node = parsedNode.node;

	auto context = scope.GetContext();

    // Approximates the Corona sun with RPR directional light. Uses Corona function publishing interface to obtain the 
    // light parameters.
#ifndef DISABLE_MAX_LIGHTS
    Corona::IFireMaxSunInterface* interf = dynamic_cast<Corona::IFireMaxSunInterface*>(object->GetInterface(Corona::IFIREMAX_SUN_INTERFACE));
    if (interf) {
        Matrix3 tm;
        Color color;
        float size;
        interf->fmGetParams(this->params.t, node, tm, color, size);
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
		SetNameFromNode(node, fireLight);
		scene.Attach(fireLight);

	} else {
        MessageBox(GetCOREInterface()->GetMAXHWnd(), _T("Too old Corona version."), _T("Radeon ProRender warning"), MB_OK);
    }
#endif
}

FIRERENDER_NAMESPACE_END
