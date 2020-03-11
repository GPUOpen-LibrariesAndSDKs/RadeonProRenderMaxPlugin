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

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// bmtex.h - 3ds Max Bitmap Texmap
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// IDs for ParamBlock2 blocks and parameters
// Parameter and ParamBlock IDs, bmtex_time
enum {
    bmtex_params, bmtex_time
};  // pblock ID

enum {
    bmtex_clipu, bmtex_clipv, bmtex_clipw, bmtex_cliph,
    bmtex_jitter, bmtex_usejitter,
    bmtex_apply, bmtex_crop_place,
    bmtex_filtering,
    bmtex_monooutput,
    bmtex_rgboutput,
    bmtex_alphasource,
    bmtex_premultalpha,
    bmtex_bitmap,
    bmtex_coords,    // access for UVW mapping
    bmtex_output,    //output window
    bmtex_filename   // bitmap filename virtual parameter
};

enum {
    bmtex_start, bmtex_playbackrate, bmtex_endcondition,
    bmtex_matidtime
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// mixmat.cpp - 3ds Max Blend Material
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

enum MixMtlParamblockId {
    mixmat_params
};  // pblock ID

// mixmat_params param IDs
enum MixMtlParamId {
    mixmat_mix, mixmat_curvea, mixmat_curveb, mixmat_usecurve, mixmat_usemat,
    mixmat_map1, mixmat_map2, mixmat_mask,
    mixmat_map1_on, mixmat_map2_on, mixmat_mask_on // main grad params
};


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// cmtl.cpp - 3ds Max Top/Bottom Material
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

enum {
    topbottom_params,
};  // pblock ID
// topbottom_params param IDs
enum {
    topbottom_map1, topbottom_map2,
    topbottom_map1_on, topbottom_map2_on, // main grad params 
    topbottom_blend,
    topbottom_position,
    topbottom_space,

};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// dblsided.cpp - 3ds Max Doublesided Material
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



enum {
    doublesided_params,
};  // pblock ID
// doublesided_params param IDs
enum {
    doublesided_map1, doublesided_map2,
    doublesided_map1_on, doublesided_map2_on, // main grad params 
    doublesided_transluency
};



//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// multi.cpp - 3ds Max MultiMtl
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Parameter and ParamBlock IDs

enum {
    multi_params,
};       // pblock ID

enum                    // multi_params param IDs
{
    multi_mtls,
    multi_ons,
    multi_names,
    multi_ids,
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// bakeShell.cpp - 3ds Max Shell mtl
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

enum {
    bakeShell_params
};  // mpBlock ID
// BakeShell_params param IDs


enum {
    bakeShell_vp_n_mtl, bakeShell_render_n_mtl,
    bakeShell_orig_mtl, bakeShell_baked_mtl
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// shaderMultiLayer.cpp - 3ds Max Multilayer shader
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
enum
{
	ml_ambient, ml_diffuse, ml_self_illum_color,
	ml_self_illum_amnt, ml_diffuse_level, ml_diffuse_rough,
	ml_specular1, ml_specular_level1, ml_glossiness1, ml_anisotropy1, ml_orientation1,
	ml_specular2, ml_specular_level2, ml_glossiness2, ml_anisotropy2, ml_orientation2,
	ml_map_channel, ml_ad_texlock, ml_ad_lock, ml_use_self_illum_color,
};


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// shaderTranslucent.cpp - 3ds Max Translucent shader
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
enum
{
	tl_ambient, tl_diffuse, tl_specular,
	tl_self_illum_color, tl_self_illum_amnt,
	tl_glossiness, tl_specular_level, tl_diffuse_level,
	tl_filter, tl_translucent_color,
	tl_backside_specular,
	tl_ad_texlock, tl_ad_lock, tl_ds_lock, tl_use_self_illum_color
};

/// Normal Bump
const Class_ID NORMALBUMP_CID(608051910, 1677107220);

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MAX 2017 Physical Material
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

const Class_ID PHYSICALMATERIAL_CID(0x3D6B1CEC, 0xDEADC001);

enum {
	/* TYPE_INT */ phm_material_mode = 0x0,
	/* TYPE_FLOAT */ phm_base_weight = 0x1,
	/* TYPE_FRGBA */ phm_base_color = 0x2,
	/* TYPE_FLOAT */ phm_reflectivity = 0x3,
	/* TYPE_FLOAT */ phm_roughness = 0x4,
	/* TYPE_BOOL */ phm_roughness_inv = 0x5,
	/* TYPE_FLOAT */ phm_metalness = 0x6,
	/* TYPE_FRGBA */ phm_refl_color = 0xA,
	/* TYPE_FLOAT */ phm_diff_roughness = 0xB,
	/* TYPE_BOOL */ phm_brdf_mode = 0xC,
	/* TYPE_FLOAT */ phm_brdf_low = 0xD,
	/* TYPE_FLOAT */ phm_brdf_high = 0xE,
	/* TYPE_FLOAT */ phm_brdf_curve = 0xF,
	/* TYPE_FLOAT */ phm_anisotropy = 0x14,
	/* TYPE_FLOAT */ phm_anisoangle = 0x15,
	/* TYPE_INT */ phm_aniso_mode = 0x16,
	/* TYPE_INT */ phm_aniso_channel = 0x17,
	/* TYPE_FLOAT */ phm_transparency = 0x1E,
	/* TYPE_FRGBA */ phm_trans_color = 0x1F,
	/* TYPE_FLOAT */ phm_trans_depth = 0x20,
	/* TYPE_FLOAT */ phm_trans_roughness = 0x23,
	/* TYPE_BOOL */ phm_trans_roughness_inv = 0x24,
	/* TYPE_BOOL */ phm_trans_roughness_lock = 0x22,
	/* TYPE_FLOAT */ phm_trans_ior = 0x21,
	/* TYPE_BOOL */ phm_thin_walled = 0x25,
	/* TYPE_FLOAT */ phm_scattering = 0x28,
	/* TYPE_FRGBA */ phm_sss_color = 0x29,
	/* TYPE_FLOAT */ phm_sss_depth = 0x2A,
	/* TYPE_FLOAT */ phm_sss_scale = 0x2B,
	/* TYPE_FRGBA */ phm_sss_scatter_color = 0x2C,
	/* TYPE_FLOAT */ phm_emission = 0x32,
	/* TYPE_FRGBA */ phm_emit_color = 0x33,
	/* TYPE_FLOAT */ phm_emit_luminance = 0x34,
	/* TYPE_FLOAT */ phm_emit_kelvin = 0x36,
	/* TYPE_FLOAT */ phm_coating = 0x3C,
	/* TYPE_FRGBA */ phm_coat_color = 0x3D,
	/* TYPE_FLOAT */ phm_coat_roughness = 0x3E,
	/* TYPE_BOOL */ phm_coat_roughness_inv = 0x3F,
	/* TYPE_FLOAT */ phm_coat_affect_color = 0x40,
	/* TYPE_FLOAT */ phm_coat_affect_roughness = 0x41,
	/* TYPE_FLOAT */ phm_coat_ior = 0x42,
	/* TYPE_TEXMAP */ phm_base_weight_map = 0x64,
	/* TYPE_TEXMAP */ phm_base_color_map = 0x65,
	/* TYPE_TEXMAP */ phm_reflectivity_map = 0x66,
	/* TYPE_TEXMAP */ phm_refl_color_map = 0x67,
	/* TYPE_TEXMAP */ phm_roughness_map = 0x68,
	/* TYPE_TEXMAP */ phm_metalness_map = 0x69,
	/* TYPE_TEXMAP */ phm_diff_rough_map = 0x6A,
	/* TYPE_TEXMAP */ phm_anisotropy_map = 0x6B,
	/* TYPE_TEXMAP */ phm_aniso_angle_map = 0x6C,
	/* TYPE_TEXMAP */ phm_transparency_map = 0x6D,
	/* TYPE_TEXMAP */ phm_trans_color_map = 0x6E,
	/* TYPE_TEXMAP */ phm_trans_rough_map = 0x6F,
	/* TYPE_TEXMAP */ phm_trans_ior_map = 0x70,
	/* TYPE_TEXMAP */ phm_scattering_map = 0x71,
	/* TYPE_TEXMAP */ phm_sss_color_map = 0x72,
	/* TYPE_TEXMAP */ phm_sss_scale_map = 0x73,
	/* TYPE_TEXMAP */ phm_emission_map = 0x74,
	/* TYPE_TEXMAP */ phm_emit_color_map = 0x75,
	/* TYPE_TEXMAP */ phm_coat_map = 0x76,
	/* TYPE_TEXMAP */ phm_coat_color_map = 0x77,
	/* TYPE_TEXMAP */ phm_coat_rough_map = 0x78,
	/* TYPE_TEXMAP */ phm_bump_map = 0x82,
	/* TYPE_TEXMAP */ phm_coat_bump_map = 0x83,
	/* TYPE_TEXMAP */ phm_displacement_map = 0x84,
	/* TYPE_TEXMAP */ phm_cutout_map = 0x85,
	/* TYPE_BOOL */ phm_base_weight_map_on = 0x96,
	/* TYPE_BOOL */ phm_base_color_map_on = 0x97,
	/* TYPE_BOOL */ phm_reflectivity_map_on = 0x98,
	/* TYPE_BOOL */ phm_refl_color_map_on = 0x99,
	/* TYPE_BOOL */ phm_roughness_map_on = 0x9A,
	/* TYPE_BOOL */ phm_metalness_map_on = 0x9B,
	/* TYPE_BOOL */ phm_diff_rough_map_on = 0x9C,
	/* TYPE_BOOL */ phm_anisotropy_map_on = 0x9D,
	/* TYPE_BOOL */ phm_aniso_angle_map_on = 0x9E,
	/* TYPE_BOOL */ phm_transparency_map_on = 0x9F,
	/* TYPE_BOOL */ phm_trans_color_map_on = 0xA0,
	/* TYPE_BOOL */ phm_trans_rough_map_on = 0xA1,
	/* TYPE_BOOL */ phm_trans_ior_map_on = 0xA2,
	/* TYPE_BOOL */ phm_scattering_map_on = 0xA3,
	/* TYPE_BOOL */ phm_sss_color_map_on = 0xA4,
	/* TYPE_BOOL */ phm_sss_scale_map_on = 0xA5,
	/* TYPE_BOOL */ phm_emission_map_on = 0xA6,
	/* TYPE_BOOL */ phm_emit_color_map_on = 0xA7,
	/* TYPE_BOOL */ phm_coat_map_on = 0xA8,
	/* TYPE_BOOL */ phm_coat_color_map_on = 0xA9,
	/* TYPE_BOOL */ phm_coat_rough_map_on = 0xAA,
	/* TYPE_BOOL */ phm_bump_map_on = 0xB4,
	/* TYPE_BOOL */ phm_coat_bump_map_on = 0xB5,
	/* TYPE_BOOL */ phm_displacement_map_on = 0xB6,
	/* TYPE_BOOL */ phm_cutout_map_on = 0xB7,
	/* TYPE_FLOAT */ phm_bump_map_amt = 0xE6,
	/* TYPE_FLOAT */ phm_clearcoat_bump_map_amt = 0xE7,
	/* TYPE_FLOAT */ phm_displacement_map_amt = 0xE8,
	/* TYPE_TEXMAP */ phm_mapM0 = 0xFA,
	/* TYPE_TEXMAP */ phm_mapM1 = 0xFB,
	/* TYPE_TEXMAP */ phm_mapM2 = 0xFC,
	/* TYPE_TEXMAP */ phm_mapM4 = 0xFE,
	/* TYPE_TEXMAP */ phm_mapM5 = 0xFF,
	/* TYPE_TEXMAP */ phm_mapM3 = 0xFD,
	/* TYPE_TEXMAP */ phm_mapM6 = 0x100,
	/* TYPE_TEXMAP */ phm_mapM7 = 0x101,
	/* TYPE_TEXMAP */ phm_mapM8 = 0x102,
	/* TYPE_TEXMAP */ phm_mapM9 = 0x103,
	/* TYPE_TEXMAP */ phm_mapM10 = 0x104,
	/* TYPE_TEXMAP */ phm_mapM11 = 0x105,
	/* TYPE_TEXMAP */ phm_mapM12 = 0x106,
	/* TYPE_TEXMAP */ phm_mapM13 = 0x107,
	/* TYPE_TEXMAP */ phm_mapM14 = 0x108,
	/* TYPE_TEXMAP */ phm_mapM15 = 0x109,
	/* TYPE_TEXMAP */ phm_mapM16 = 0x10A,
	/* TYPE_TEXMAP */ phm_mapM17 = 0x10B,
	/* TYPE_TEXMAP */ phm_mapM18 = 0x10C,
	/* TYPE_TEXMAP */ phm_mapM19 = 0x10D,
	/* TYPE_TEXMAP */ phm_mapM20 = 0x10E
};