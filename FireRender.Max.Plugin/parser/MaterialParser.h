/*********************************************************************************************************************************
* Radeon ProRender for 3ds Max plugin
* Copyright (c) 2017 AMD
* All Rights Reserved
*
* Converting all material types we support from 3ds Max to Radeon ProRender core format
*********************************************************************************************************************************/
#pragma once
#include "frWrap.h"

#include "Common.h"
#include <RadeonProRender.h>
#include "RenderParameters.h"
#include <max.h>
#include <stdmat.h>
#include <bitmap.h>
#include "3dsMaxDeclarations.h"
#include <iparamb2.h>
#include <map>
#include <vector>
#include <string>
#include "UvwContext.h"


#define NEW_FALLOFF_CLASS_ID 0x6ec3730c
#include <FrScope.h>
#include <icurvctl.h>

FIRERENDER_NAMESPACE_BEGIN;

enum
{
	MAP_FLAG_NOFLAGS = 0,
	MAP_FLAG_NOGAMMA = 1,
	MAP_FLAG_CLAMP = 2,
	MAP_FLAG_WANTSHDR = 4,
	MAP_FLAG_NORMALMAP = 8
};

//////////////////////////////////////////////////////////////////////////////
// MaterialParser converts 3ds Max texmaps and materials to RPR shaders/maps
//

class MaterialParser
{
	MaterialParser(const MaterialParser&) = delete;
	MaterialParser& operator=(const MaterialParser&) = delete;

protected:

	frw::Scope mScope; // associated context mScope
	TimeValue mT = 0; // current time
	IParamBlock2 *mPblock = 0; // renderer's parameter block

public:
	frw::MaterialSystem materialSystem;

	struct
	{
		INode* mNode = nullptr;
		bool mCacheable = true;
		Matrix3 mCameraTransform;
		int mNumEmissive = 0;
		int mNumVolume = 0;
		int mNumDisplacement = 0;
		bool mDisplacementDirectlyPlugged = false;
		bool mNormalDirectlyPlugged = false;
	} shaderData;

	static HashValue GetHashValue(Animatable *mat, TimeValue mT, std::map<Animatable*, HashValue> &hash_visited, DWORD syncTimestamp, bool bReloadMaterial = false);

	// Traverses a mtlbase (base class for both texmaps and materials) paramblock and computes an unique hash value for it
	HashValue getMaterialHash(MtlBase *mat, bool bReloadMaterial = false);

	HashValue getBitmapHash(Bitmap *bm);

	DWORD syncTimestamp;

	inline void SetTimeValue(const TimeValue &tt)
	{
		mT = tt;
	}

	inline void SetParamBlock(IParamBlock2 *pb)
	{
		mPblock = pb;
	}
	
protected:
	// Finds a leaf material given an input material (which may be a hierarchy of multi-materials. Returns the input material
	// itself if a leaf material
	// material - material for which we want to find the leaf. Can be NULL
	// submtlId - Which sub-material to choose, if the input material is a multi-material
	static Mtl* getLeaf(Mtl* material, const int submtlId)
	{
		while (material != NULL && material->IsMultiMtl())
		{
			const int index = std::min(material->NumSubMtls() - 1, submtlId);
			if (material->ClassID() == MULTI_MATERIAL_CLASS_ID)
			{
				IParamBlock2* pb = material->GetParamBlock(multi_params);
				// we cannot pick n-th submtl directly, we need to iterate all sub-materials because the IDs can be remapped
				for (int i = 0; i < pb->Count(multi_ons); ++i)
				{
					if (GetFromPb<int>(pb, multi_ids, 0, NULL, i) == index && !GetFromPb<bool>(pb, multi_ons, 0, NULL, i))
					{
						return NULL;
					}
				}
			}
			material = material->GetSubMtl(index);
		}
		return material;
	}

	inline frw::Value StdMat2_glossinessToRoughness(frw::Value glossy)
	{
		auto rough = materialSystem.ValueSub(1.0, glossy);
		return  materialSystem.ValuePow(rough, 3);
	}

public:
	// Helper methods that create RPR shaders
	
	frw::Value createMap(Texmap* input, const int flags = MAP_FLAG_NOFLAGS); // may return an empty value

	frw::Value getValue(Texmap* input, frw::Value orValue = frw::Value())
	{
		return createMap(input, 0) | orValue;
	}

	static Texmap *getTexture(Mtl* mtl, int id)
	{
		Texmap *tex = 0;
		if (id < mtl->NumSubTexmaps())
			tex = mtl->GetSubTexmap(id);
		return tex;
	}

	frw::Scope GetScope() { return mScope; }

	Texmap *MaterialParser::findDisplacementMap(MtlBase *mat);
	frw::Value getTexmapUV(Texmap *tm, bool noDefault = false);

protected:
	
	// Helper methods that create RPR shaders out of  MAX materials

	frw::Shader parseStdMat2(StdMat2* mtl);
	frw::Shader parsePhysicalMaterial(Mtl* mtl);
	frw::Shader parseBlendMtl(Mtl* mtl);
	frw::Shader parseCoronaRaySwitchMtl(Mtl* mtl);
	frw::Shader parseCoronaMtl(Mtl* mtl);
	frw::Shader parseCoronaPortalMtl(Mtl* mtl);
	frw::Shader parseCoronaVolumeMtl(Mtl* mtl);
	frw::Shader parseCoronaLightObject(INode* node);
	frw::Shader parseCoronaShadowCatcherMtl(Mtl* mtl);
	frw::Shader parseCoronaLightMtl(Mtl* mtl);	
	frw::Shader parseCoronaLayeredMtl(Mtl* mtl, INode* node);
	
	// utilities to retrieve uv modifiers
	static Matrix3 getOutputTm(ReferenceTarget *p);
	frw::Value transformValue(ICurveCtl* curveCtrl, frw::Value v);

	// Helper methods that create RPR shaders out of  MAX maps
	frw::Value createFalloffMap(Texmap *tm);
	frw::Value createCheckerMap(Texmap *tm);
	frw::Value createGradientMap(Texmap *tm);
	frw::Value createNoiseMap(Texmap *tm);
	frw::Value createColorCorrectionMap(Texmap* tm);
	frw::Value createMixMap(Texmap* tm);
	frw::Value createOutputMap(Texmap* tm);
	frw::Value createRgbMultiplyMap(Texmap* texmap);
	frw::Value createTintMap(Texmap* texmap);
	frw::Value createMaskMap(Texmap* texmap);
	frw::Value createCompositeMap(Texmap* texmap);
	frw::Value createNormalBumpMap(Texmap* texmap);
	frw::Value createCoronaMixMap(Texmap* texmap);
	frw::Value createCoronaColorMap(Texmap* texmap);

public:

	MaterialParser(frw::Scope mScope)
		: mScope(mScope), materialSystem(mScope.GetMaterialSystem())
	{
	}

	frw::Value createTextureMap(Texmap *tm, int flags);

	// Converts a single Texmap to rpr_image by rasterizing it. Automatically uses caching to prevent duplicities.
	// input - The input map. Must not be NULL.
	// clamp - If true, then this map is used as an albedo (diffuse map, reflection map, opacity map, ...), and so it 
	//         will be clamped in the 0..1 interval
	frw::Image createImageFromMap(Texmap* input, const int flags, bool force = false);

	frw::Image createImage(Bitmap* bm, const HashValue &key, const int flags, std::wstring name = L"");


	// Creates a RPR volume shader from a given MAX material, eventually considering the node it is assigned to
	// mtl - MAX material to convert. Can be NULL
	// node - A node to which the material is assigned. Must not be NULL.
	frw::Shader createVolumeShader(Mtl* mtl, INode* node = nullptr);

	frw::Shader findVolumeMaterial(Mtl* mtl);

	// Creates a RPR shader from given MAX material, eventually considering the node it is assigned to
	// mtl - MAX material to convert. Can be NULL
	// node - A node to which the material is assigned. Must not be NULL.
	frw::Shader createShader(Mtl* mtl, INode* node = nullptr, bool bReloadMaterial = false);

	static Color Kelvin2Color(float kelvin);

	frw::Scope &getScope()
	{
		return mScope;
	}

	inline IParamBlock2 *GetPBlock()
	{
		return mPblock;
	}
};


FIRERENDER_NAMESPACE_END;
