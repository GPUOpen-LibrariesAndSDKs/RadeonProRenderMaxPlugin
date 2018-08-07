/*********************************************************************************************************************************
* Radeon ProRender for 3ds Max plugin
* Copyright (c) 2017 AMD
* All Rights Reserved
*
* Implementation of Material-related reactions of the Synchronizer class
*********************************************************************************************************************************/

#include "Synchronizer.h"
#include "CoronaDeclarations.h"
#include "FireRenderMaterialMtl.h"
#include "FireRenderUberMtl.h"
#include "FireRenderDisplacementMtl.h"
#include "ScopeManager.h"

FIRERENDER_NAMESPACE_BEGIN;

void Synchronizer::RebuildUsersOfMaterial(Mtl *pMat, std::vector<INode*> &nodesToRebuild)
{
	auto users = mMaterialUsers.find(pMat);
	if (users != mMaterialUsers.end())
	{
		for (auto shape : users->second)
		{
			INode *pNode = shape->mParent;
			if (pNode)
				nodesToRebuild.push_back(pNode);
		}
	}
}

frw::Shader Synchronizer::CreateShader(Mtl *pMat, INode *node, bool force)
{
	auto res = mtlParser.createShader(pMat, node, true);
	if (mtlParser.shaderData.mNumEmissive > 0)
	{
		if (mEmissives.find(pMat) == mEmissives.end())
			mEmissives.insert(pMat);
	}
	return res;
}

void Synchronizer::UpdateMaterial(Mtl *pMat, std::vector<INode*> &nodesToRebuild)
{
	if (pMat)
	{
		// remove from cache
		auto ii = mShaderCache.find(pMat);
		if (ii != mShaderCache.end())
			mShaderCache.erase(ii);

		auto vv = mVolumeShaderCache.find(pMat);
		if (vv != mVolumeShaderCache.end())
			mVolumeShaderCache.erase(vv);

		bool rebuildingObject = false;
		if (pMat->IsMultiMtl())
		{
			auto ii = mMultiMats.find(pMat);
			if (ii != mMultiMats.end())
			{
				// number of sub mtls has changed?
				if (ii->second.size() != pMat->NumSubMtls())
				{
					for (int i = 0; i < ii->second.size(); i++)
					{
						Mtl *prev = ii->second[i];
						RebuildUsersOfMaterial(prev, nodesToRebuild);
					}
					ii->second.clear();
					for (int i = 0; i < pMat->NumSubMtls(); i++)
						ii->second.push_back(pMat->GetSubMtl(i));
					rebuildingObject = true;
				}

				// same number of sub mtls
				for (int i = 0; (i < ii->second.size()) && !rebuildingObject; i++)
				{
					Mtl *prev = ii->second[i];
					if (i < pMat->NumSubMtls())
					{
						Mtl *cur = pMat->GetSubMtl(i);
						if (prev != cur)
						{
							ii->second[i] = cur;
							auto users = mMaterialUsers.find(prev);
							if (users != mMaterialUsers.end())
							{
								for (auto shape : users->second)
								{
									INode *pNode = shape->mParent;
									if (pNode)
										nodesToRebuild.push_back(pNode);
								}
							}
							rebuildingObject = true;
						}
					}
				}
			}
			else
			{
				for (int i = 0; i < pMat->NumSubMtls(); i++)
				{
					auto sub = pMat->GetSubMtl(i);
					mMultiMats[pMat].push_back(sub);
				}
				// maybe this material went from single mat to multi mat
				RebuildUsersOfMaterial(pMat, nodesToRebuild);
				rebuildingObject = true;
			}
		}
		else
		{
			// was it a multi mat before?
			auto ii = mMultiMats.find(pMat);
			if (ii != mMultiMats.end())
			{
				// yes. we need to rebuild its users
				for (int i = 0; i < ii->second.size(); i++)
				{
					Mtl *prev = ii->second[i];
					RebuildUsersOfMaterial(prev, nodesToRebuild);
				}
				mMultiMats.erase(ii);
				rebuildingObject = true;
			}
		}

		// Don't rebuild the material now if we've going to rebuild the whole object later
		if( rebuildingObject )
			return;

		auto users = mMaterialUsers.find(pMat);
		if (users != mMaterialUsers.end())
		{
			frw::Shader sharedShader;
			bool sharedShaderCreated = false;
			SShapePtr lastShape(nullptr);

			for (auto shape : users->second)
			{
				INode *pNode = shape->mParent;
				if (pNode)
				{
					if( (!sharedShader) || (!sharedShaderCreated) )
					{
						sharedShader = CreateShader(pMat, pNode, true);
						sharedShaderCreated = true;
						mShaderCache.insert(std::make_pair(pMat, sharedShader));
					}
					frw::Shader shader = sharedShader;

					if (shader)
					{
						TimeValue t = mBridge->t();

						float minHeight = 0.f;
						float maxHeight = 0.0f;
						float subdivision = 0.f;
						float creaseWeight = 0.f;
						int boundary = RPR_SUBDIV_BOUNDARY_INTERFOP_TYPE_EDGE_AND_CORNER;

						bool shadowCatcher = false;
						bool castsShadows = true;

						// Handling of some special flags for Corona Material is necessary here at geometry level
						if (pMat && pMat->ClassID() == Corona::MTL_CID)
						{
							if (ScopeManagerMax::CoronaOK)
							{
							IParamBlock2* pb = pMat->GetParamBlock(0);
							const bool useCaustics = GetFromPb<bool>(pb, Corona::MTLP_USE_CAUSTICS, t);
							const float lRefract = GetFromPb<float>(pb, Corona::MTLP_LEVEL_REFRACT, t);
							const float lOpacity = GetFromPb<float>(pb, Corona::MTLP_LEVEL_OPACITY, t);

							if ((lRefract > 0.f || lOpacity < 1.f) && !useCaustics)
								castsShadows = false;
						}
						}
						else if (pMat && pMat->ClassID() == FIRERENDER_MATERIALMTL_CID)
						{
							IParamBlock2* pb = pMat->GetParamBlock(0);
							castsShadows = bool_cast( GetFromPb<BOOL>(pb, FRMaterialMtl_CAUSTICS, t) );
							shadowCatcher = bool_cast( GetFromPb<BOOL>(pb, FRMaterialMtl_SHADOWCATCHER, t) );
						}
						else if (pMat && pMat->ClassID() == FIRERENDER_UBERMTL_CID)
						{
							IParamBlock2* pb = pMat->GetParamBlock(0);
							castsShadows = bool_cast( GetFromPb<BOOL>(pb, FRUBERMTL_FRUBERCAUSTICS, t) );
							shadowCatcher = bool_cast( GetFromPb<BOOL>(pb, FRUBERMTL_FRUBERSHADOWCATCHER, t) );
						}
						else if (pMat && pMat->ClassID() == Corona::SHADOW_CATCHER_MTL_CID)
						{
							if (ScopeManagerMax::CoronaOK)
							shadowCatcher = true;
						}
						else if (pMat && pMat->ClassID() == FIRERENDER_SCMTL_CID) // Shadow Catcher Material
						{
							shadowCatcher = true;
						}

						frw::Value displImageNode;
						bool notAccurate;
						displImageNode = FRMTLCLASSNAME(DisplacementMtl)::translateDisplacement(t, mtlParser, pMat,
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
						shape->Get().SetShadowCatcherFlag(shadowCatcher);

						// reassign the new shader

						// Set shader, but without a commit
						shape->Get().SetShader(shader,false);

						lastShape = shape;

						// reassign the new volume shader

						frw::Shader volumeShader;
						volumeShader = mtlParser.findVolumeMaterial(pMat);
						if (volumeShader)
						{
							shape->Get().SetVolumeShader(volumeShader);
							mVolumeShaderCache.insert(std::make_pair(pMat, volumeShader));
						}
						else
							shape->Get().SetVolumeShader(frw::Shader());
					}
				}
			}
			// Perform a commit as the last step
			if( sharedShaderCreated && (lastShape.get()!=nullptr) )
				sharedShader.Commit();
		}
	}
}

FIRERENDER_NAMESPACE_END;
