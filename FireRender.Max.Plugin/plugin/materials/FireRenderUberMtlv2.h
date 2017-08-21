#pragma once

#include "FireRenderMtlBase.h"

FIRERENDER_NAMESPACE_BEGIN;

enum FRUberMtlV2_TexmapId
{
// Diffuse
	FRUBERMTLV2_MAP_DIFFUSE_COLOR = 0,
	FRUBERMTLV2_MAP_DIFFUSE_WEIGHT,
	FRUBERMTLV2_MAP_DIFFUSE_ROUGHNESS,

// Reflection
	FRUBERMTLV2_MAP_REFLECTION_COLOR,
	FRUBERMTLV2_MAP_REFLECTION_WEIGHT,
	FRUBERMTLV2_MAP_REFLECTION_ROUGHNESS,
	FRUBERMTLV2_MAP_REFLECTION_ANISO,
	FRUBERMTLV2_MAP_REFLECTION_ANISO_ROT,
	FRUBERMTLV2_MAP_REFLECTION_IOR,
	FRUBERMTLV2_MAP_REFLECTION_METALNESS,

// Refraction
	FRUBERMTLV2_MAP_REFRACTION_COLOR,
	FRUBERMTLV2_MAP_REFRACTION_WEIGHT,
	FRUBERMTLV2_MAP_REFRACTION_ROUGHNESS,
	FRUBERMTLV2_MAP_REFRACTION_IOR,

// Coat
	FRUBERMTLV2_MAP_COAT_COLOR,
	FRUBERMTLV2_MAP_COAT_WEIGHT,
	FRUBERMTLV2_MAP_COAT_ROUGHNESS,
	//FRUBERMTLV2_MAP_COAT_NORMAL,
	FRUBERMTLV2_MAP_COAT_IOR,
	FRUBERMTLV2_MAP_COAT_METALNESS,

// SSS
	FRUBERMTLV2_MAP_SSS_SUBSURFACE_COLOR,
	FRUBERMTLV2_MAP_SSS_WEIGHT,
	FRUBERMTLV2_MAP_SSS_SCATTER_COLOR,
	FRUBERMTLV2_MAP_SSS_SCATTER_AMOUNT,
	FRUBERMTLV2_MAP_SSS_ABSORPTION,
	FRUBERMTLV2_MAP_SSS_ABSORPTION_DIST,
	FRUBERMTLV2_MAP_SSS_DIRECTION,


// Emissive
	FRUBERMTLV2_MAP_EMISSIVE_COLOR,
	FRUBERMTLV2_MAP_EMISSIVE_WEIGHT,

// Material Settings
	FRUBERMTLV2_MAP_MAT_OPACITY,
	FRUBERMTLV2_MAP_MAT_NORMAL,
	FRUBERMTLV2_MAP_MAT_DISPLACE,
	FRUBERMTLV2_MAP_MAT_BUMP,
};

enum FRUberMtlV2_ParamID : ParamID
{
// Diffuse
	FRUBERMTLV2_DIFFUSE_COLOR_MUL = 100,
	FRUBERMTLV2_DIFFUSE_COLOR,
	FRUBERMTLV2_DIFFUSE_COLOR_MAP,
	FRUBERMTLV2_DIFFUSE_COLOR_USEMAP,

	FRUBERMTLV2_DIFFUSE_WEIGHT_MUL,
	FRUBERMTLV2_DIFFUSE_WEIGHT,
	FRUBERMTLV2_DIFFUSE_WEIGHT_MAP,
	FRUBERMTLV2_DIFFUSE_WEIGHT_USEMAP,

	FRUBERMTLV2_DIFFUSE_ROUGHNESS_MUL,
	FRUBERMTLV2_DIFFUSE_ROUGHNESS,
	FRUBERMTLV2_DIFFUSE_ROUGHNESS_MAP,
	FRUBERMTLV2_DIFFUSE_ROUGHNESS_USEMAP,

// Reflection
	FRUBERMTLV2_REFLECTION_COLOR_MUL,
	FRUBERMTLV2_REFLECTION_COLOR,
	FRUBERMTLV2_REFLECTION_COLOR_MAP,
	FRUBERMTLV2_REFLECTION_COLOR_USEMAP,
	
	FRUBERMTLV2_REFLECTION_WEIGHT_MUL,
	FRUBERMTLV2_REFLECTION_WEIGHT,
	FRUBERMTLV2_REFLECTION_WEIGHT_MAP,
	FRUBERMTLV2_REFLECTION_WEIGHT_USEMAP,

	FRUBERMTLV2_REFLECTION_ROUGHNESS_MUL,
	FRUBERMTLV2_REFLECTION_ROUGHNESS,
	FRUBERMTLV2_REFLECTION_ROUGHNESS_MAP,
	FRUBERMTLV2_REFLECTION_ROUGHNESS_USEMAP,

	FRUBERMTLV2_REFLECTION_ANISO_MUL,
	FRUBERMTLV2_REFLECTION_ANISO_MAP,
	FRUBERMTLV2_REFLECTION_ANISO_USEMAP,

	FRUBERMTLV2_REFLECTION_ANISO_ROT_MUL,
	FRUBERMTLV2_REFLECTION_ANISO_ROT,
	FRUBERMTLV2_REFLECTION_ANISO_ROT_MAP,
	FRUBERMTLV2_REFLECTION_ANISO_ROT_USEMAP,

	FRUBERMTLV2_REFLECTION_MODE,

	FRUBERMTLV2_REFLECTION_IOR_MUL,
	FRUBERMTLV2_REFLECTION_IOR,
	FRUBERMTLV2_REFLECTION_IOR_MAP,
	FRUBERMTLV2_REFLECTION_IOR_USEMAP,

	FRUBERMTLV2_REFLECTION_METALNESS,
	FRUBERMTLV2_REFLECTION_METALNESS_MUL,
	FRUBERMTLV2_REFLECTION_METALNESS_MAP,
	FRUBERMTLV2_REFLECTION_METALNESS_USEMAP,

// Refraction
	FRUBERMTLV2_REFRACTION_COLOR_MUL,
	FRUBERMTLV2_REFRACTION_COLOR,
	FRUBERMTLV2_REFRACTION_COLOR_MAP,
	FRUBERMTLV2_REFRACTION_COLOR_USEMAP,

	FRUBERMTLV2_REFRACTION_WEIGHT_MUL,
	FRUBERMTLV2_REFRACTION_WEIGHT,
	FRUBERMTLV2_REFRACTION_WEIGHT_MAP,
	FRUBERMTLV2_REFRACTION_WEIGHT_USEMAP,

	FRUBERMTLV2_REFRACTION_ROUGHNESS_MUL,
	FRUBERMTLV2_REFRACTION_ROUGHNESS,
	FRUBERMTLV2_REFRACTION_ROUGHNESS_MAP,
	FRUBERMTLV2_REFRACTION_ROUGHNESS_USEMAP,

	FRUBERMTLV2_REFRACTION_IOR,
	FRUBERMTLV2_REFRACTION_IOR_MUL,
	FRUBERMTLV2_REFRACTION_IOR_MAP,
	FRUBERMTLV2_REFRACTION_IOR_USEMAP,

	FRUBERMTLV2_REFRACTION_THIN_SURFACE,
	FRUBERMTLV2_REFRACTION_LINK_TO_REFLECTION,

// Coat
	FRUBERMTLV2_COAT_COLOR_MUL,
	FRUBERMTLV2_COAT_COLOR,
	FRUBERMTLV2_COAT_COLOR_MAP,
	FRUBERMTLV2_COAT_COLOR_USEMAP,
	
	FRUBERMTLV2_COAT_WEIGHT_MUL,
	FRUBERMTLV2_COAT_WEIGHT,
	FRUBERMTLV2_COAT_WEIGHT_MAP,
	FRUBERMTLV2_COAT_WEIGHT_USEMAP,
	
	FRUBERMTLV2_COAT_ROUGHNESS_MUL,
	FRUBERMTLV2_COAT_ROUGHNESS,
	FRUBERMTLV2_COAT_ROUGHNESS_MAP,
	FRUBERMTLV2_COAT_ROUGHNESS_USEMAP,
	
	//FRUBERMTLV2_COAT_NORMAL_MUL,
	//FRUBERMTLV2_COAT_NORMAL_MAP,
	//FRUBERMTLV2_COAT_NORMAL_USEMAP,

	FRUBERMTLV2_COAT_MODE,

	FRUBERMTLV2_COAT_IOR,
	FRUBERMTLV2_COAT_IOR_MUL,
	FRUBERMTLV2_COAT_IOR_MAP,
	FRUBERMTLV2_COAT_IOR_USEMAP,

	FRUBERMTLV2_COAT_METALNESS,
	FRUBERMTLV2_COAT_METALNESS_MUL,
	FRUBERMTLV2_COAT_METALNESS_MAP,
	FRUBERMTLV2_COAT_METALNESS_USEMAP,

// SSS
	FRUBERMTLV2_USE_SSS_DIFFUSE_COLOR,

	FRUBERMTLV2_SSS_SUBSURFACE_COLOR_MUL,
	FRUBERMTLV2_SSS_SUBSURFACE_COLOR,
	FRUBERMTLV2_SSS_SUBSURFACE_COLOR_MAP,
	FRUBERMTLV2_SSS_SUBSURFACE_COLOR_USEMAP,

	FRUBERMTLV2_SSS_WEIGHT_MUL,
	FRUBERMTLV2_SSS_WEIGHT,
	FRUBERMTLV2_SSS_WEIGHT_MAP,
	FRUBERMTLV2_SSS_WEIGHT_USEMAP,

	FRUBERMTLV2_SSS_SCATTER_COLOR_MUL,
	FRUBERMTLV2_SSS_SCATTER_COLOR,
	FRUBERMTLV2_SSS_SCATTER_COLOR_MAP,
	FRUBERMTLV2_SSS_SCATTER_COLOR_USEMAP,

	FRUBERMTLV2_SSS_SCATTER_AMOUNT_MUL,
	FRUBERMTLV2_SSS_SCATTER_AMOUNT_MAP,
	FRUBERMTLV2_SSS_SCATTER_AMOUNT_USEMAP,

	FRUBERMTLV2_SSS_DIRECTION_MUL,
	FRUBERMTLV2_SSS_DIRECTION_MAP,
	FRUBERMTLV2_SSS_DIRECTION_USEMAP,

	FRUBERMTLV2_SSS_SINGLESCATTERING,

	FRUBERMTLV2_SSS_ABSORPTION_MUL,
	FRUBERMTLV2_SSS_ABSORPTION,
	FRUBERMTLV2_SSS_ABSORPTION_MAP,
	FRUBERMTLV2_SSS_ABSORPTION_USEMAP,

	FRUBERMTLV2_SSS_ABSORPTION_DIST_MUL,
	FRUBERMTLV2_SSS_ABSORPTION_DIST_MAP,
	FRUBERMTLV2_SSS_ABSORPTION_DIST_USEMAP,
	
// Emissive
	FRUBERMTLV2_EMISSIVE_COLOR_MUL,
	FRUBERMTLV2_EMISSIVE_COLOR,
	FRUBERMTLV2_EMISSIVE_COLOR_MAP,
	FRUBERMTLV2_EMISSIVE_COLOR_USEMAP,
	
	FRUBERMTLV2_EMISSIVE_WEIGHT_MUL,
	FRUBERMTLV2_EMISSIVE_WEIGHT,
	FRUBERMTLV2_EMISSIVE_WEIGHT_MAP,
	FRUBERMTLV2_EMISSIVE_WEIGHT_USEMAP,

	FRUBERMTLV2_EMISSIVE_DOUBLESIDED,

// Material Settings
	FRUBERMTLV2_MAT_OPACITY_MUL,
	FRUBERMTLV2_MAT_OPACITY_MAP,
	FRUBERMTLV2_MAT_OPACITY_USEMAP,

	FRUBERMTLV2_MAT_NORMAL_MUL,
	FRUBERMTLV2_MAT_NORMAL,
	FRUBERMTLV2_MAT_NORMAL_MAP,
	FRUBERMTLV2_MAT_NORMAL_USEMAP,

	FRUBERMTLV2_MAT_DISPLACE_MUL,
	FRUBERMTLV2_MAT_DISPLACE_MAP,
	FRUBERMTLV2_MAT_DISPLACE_USEMAP,

	FRUBERMTLV2_MAT_BUMP_MUL,
	FRUBERMTLV2_MAT_BUMP_MAP,
	FRUBERMTLV2_MAT_BUMP_USEMAP,
};

BEGIN_DECLARE_FRMTLCLASSDESC(UberMtlv2, L"RPR Uber Material V2", FIRERENDER_UBERMTLV2_CID)
END_DECLARE_FRMTLCLASSDESC()

BEGIN_DECLARE_FRMTL(UberMtlv2)

public:
	virtual Color GetDiffuse(int mtlNum, BOOL backFace) override
	{
		return GetFromPb<Color>(pblock, FRUBERMTLV2_DIFFUSE_COLOR);
	}

	frw::Shader getVolumeShader(const TimeValue t, MaterialParser& mtlParser, INode* node);

private:
	std::tuple<bool, Texmap*, Color, float> GetParameters(FRUberMtlV2_ParamID, FRUberMtlV2_ParamID, FRUberMtlV2_ParamID, FRUberMtlV2_ParamID);
	std::tuple<bool, Texmap*, Color, float> GetParametersNoColor(FRUberMtlV2_ParamID, FRUberMtlV2_ParamID, FRUberMtlV2_ParamID);

	frw::Value SetupShaderOrdinary(MaterialParser& mtlParser, std::tuple<bool, Texmap*, Color, float> parameters, int mapFlags);

	void SetupDiffuse(MaterialParser& mtlParser, frw::Shader& shader);
	void SetupReflection(MaterialParser& mtlParser, frw::Shader& shader);
	void SetupRefraction(MaterialParser& mtlParser, frw::Shader& shader);
	void SetupCoating(MaterialParser& mtlParser, frw::Shader& shader);
	void SetupSSS(MaterialParser& mtlParser, frw::Shader& shader);
	void SetupEmissive(MaterialParser& mtlParser, frw::Shader& shader);
	void SetupMaterial(MaterialParser& mtlParser, frw::Shader& shader);

END_DECLARE_FRMTL(UberMtlv2)

FIRERENDER_NAMESPACE_END;