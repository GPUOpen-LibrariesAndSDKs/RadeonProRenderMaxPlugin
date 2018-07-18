/*********************************************************************************************************************************
* Radeon ProRender for 3ds Max plugin
* Copyright (C) 2017 AMD
* All Rights Reserved
*
* Exports a Radeon ProRender native material to a XML file
*********************************************************************************************************************************/

#define NOMINMAX
#include <maxscript\maxscript.h>
#include <maxscript\macros\define_instantiation_functions.h>
#include <bitmap.h>
#include <parser/MaterialParser.h>
#include <FrScope.h>
#include <vector>
#include <set>
#include <string>
#include <fstream>
#include <regex>

#include "parser/MaterialLoader.h"
#include "plugin/ParamBlock.h"
#include "plugin/FireRenderer.h"




std::vector<RPRX_DEFINE_PARAM_MATERIAL> g_rprxParamList;


//retur false if FAIL
bool ParseRPR(
	rpr_material_node material ,
	std::set<rpr_material_node>& nodeList ,
	std::set<rprx_material>& nodeListX ,
	const std::string& folder  
)
{

	//check if the node already stored
	if ( nodeList.find(material) != nodeList.end() )
	{
		return true;
	}

	nodeList.insert(material);

	rpr_int status = RPR_SUCCESS;

	size_t input_count = 0;
	status = frMaterialNodeGetInfo(material, RPR_MATERIAL_NODE_INPUT_COUNT, sizeof(size_t), &input_count, nullptr);
	for (int i = 0; i < input_count; ++i)
	{
		fr_int in_type;
		status = frMaterialNodeGetInputInfo(material, i, RPR_MATERIAL_NODE_INPUT_TYPE, sizeof(in_type), &in_type, nullptr);
		if (in_type == RPR_MATERIAL_NODE_INPUT_TYPE_NODE)
		{
			rpr_material_node ref_node = nullptr;
			status = frMaterialNodeGetInputInfo(material, i, RPR_MATERIAL_NODE_INPUT_VALUE, sizeof(ref_node), &ref_node, nullptr);	
			ParseRPR(ref_node,nodeList,nodeListX,folder);
		}
		else if (in_type == RPR_MATERIAL_NODE_INPUT_TYPE_IMAGE)
		{
			fr_image img = nullptr;
			status = frMaterialNodeGetInputInfo(material, i, RPR_MATERIAL_NODE_INPUT_VALUE, sizeof(img), &img, nullptr);
			size_t name_size = 0;
			status = frImageGetInfo(img, RPR_OBJECT_NAME, 0, nullptr, &name_size);
			//create file if filename is empty(name == "\0")
			if (name_size == 1)
			{
				//looking for a free filename
				int img_id = 0;
				std::string ext = ".png";
				std::string img_name = folder + "tex_" + std::to_string(img_id) + ext;
				while (std::ifstream(img_name).good())
				{
					img_name = folder + "tex_" + std::to_string(++img_id) + ext;
				}

				std::wstring wimg_name(img_name.begin(), img_name.end());

				fr_image_desc desc;
				status = frImageGetInfo(img, RPR_IMAGE_DESC, sizeof(desc), &desc, nullptr);
				BitmapInfo bi;
				bi.SetFlags(MAP_NOFLAGS);
				bi.SetName(wimg_name.c_str());
				bi.SetWidth(desc.image_width);
				bi.SetHeight(desc.image_height);
				bi.SetType(BMM_TRUE_32);

				Bitmap* map = TheManager->Create(&bi);

				//write image data
				fr_image_format format;
				status = frImageGetInfo(img, RPR_IMAGE_FORMAT, sizeof(format), &format, nullptr);

				if (format.type == RPR_COMPONENT_TYPE_UINT8)
				{
					size_t imgsize = 0;
					status = frImageGetInfo(img, RPR_IMAGE_DATA, 0, nullptr, &imgsize);
					static std::vector<unsigned char> img_data(imgsize);
					img_data.resize(imgsize);
					status = frImageGetInfo(img, RPR_IMAGE_DATA, imgsize, img_data.data(), nullptr);

					if (format.num_components == 1)
					{
#pragma omp parallel for
						for (int i = 0; i < bi.Height(); ++i)
							for (int j = 0; j < bi.Width(); ++j)
							{
								unsigned char* frcolor = &img_data[(i * bi.Width() + j)];
								float c = (float)frcolor[0] * 1.f / 255.f;
								BMM_Color_fl col(c, c, c, 1.f);
								map->PutPixels(j, i, 1, &col);
							}

					}
					else if (format.num_components == 3)
					{
#pragma omp parallel for
						for (int i = 0; i < bi.Height(); ++i)
							for (int j = 0; j < bi.Width(); ++j)
							{
								unsigned char* frcolor = &img_data[(i * bi.Width() + j) * 3];
								BMM_Color_fl col((float)frcolor[0] * 1.f / 255.f, (float)frcolor[1] * 1.f / 255.f, (float)frcolor[2] * 1.f / 255.f, 1.f);
								map->PutPixels(j, i, 1, &col);
							}
					}
				}
				else if (format.type == RPR_COMPONENT_TYPE_FLOAT32)
				{
					size_t imgsize = 0;
					status = frImageGetInfo(img, RPR_IMAGE_DATA, 0, nullptr, &imgsize);
					static std::vector<float> img_data(imgsize);
					img_data.resize(imgsize);
					status = frImageGetInfo(img, RPR_IMAGE_DATA, imgsize, img_data.data(), nullptr);

					if (format.num_components == 1)
					{
#pragma omp parallel for
						for (int i = 0; i < bi.Height(); ++i)
							for (int j = 0; j < bi.Width(); ++j)
							{
								float* frcolor = &img_data[(i * bi.Width() + j)];
								BMM_Color_fl col(frcolor[0], frcolor[0], frcolor[0], 1.f);
								map->PutPixels(j, i, 1, &col);
							}
					}
					else if (format.num_components == 3)
					{
#pragma omp parallel for
						for (int i = 0; i < bi.Height(); ++i)
							for (int j = 0; j < bi.Width(); ++j)
							{
								float* frcolor = &img_data[(i * bi.Width() + j) * 3];
								BMM_Color_fl col(frcolor[0], frcolor[1], frcolor[2], 1.f);
								map->PutPixels(j, i, 1, &col);
							}
					}
				}

				map->OpenOutput(&bi);
				map->Write(&bi);
				map->Close(&bi);
				map->DeleteThis();
				frObjectSetName(img, ("tex_" + std::to_string(img_id) + ext).c_str());
			}
		}
	}


	return true;
}



//return true if success
bool ParseRPRX(
	rprx_material materialX ,
	rprx_context contextX, 
	std::set<rpr_material_node>& nodeList ,
	std::set<rprx_material>& nodeListX ,
	const std::string& folder  
	)
{
	rpr_int status = RPR_SUCCESS;

	//check if the node already stored
	if ( nodeListX.find(materialX) != nodeListX.end() )
	{
		return true;
	}
	
	nodeListX.insert(materialX);

	for(int iParam=0; iParam<g_rprxParamList.size(); iParam++)
	{

		rpr_parameter_type type = 0;
		status = rprxMaterialGetParameterType(contextX,materialX,g_rprxParamList[iParam].param,&type); if (status != RPR_SUCCESS) { return false; };

		if ( type == RPRX_PARAMETER_TYPE_FLOAT4 )
		{
			float value4f[] = { 0.0f,0.0f,0.0f,0.0f };
			status = rprxMaterialGetParameterValue(contextX,materialX,g_rprxParamList[iParam].param,&value4f); if (status != RPR_SUCCESS) { return false; };
		}
		else if ( type == RPRX_PARAMETER_TYPE_UINT )
		{
			unsigned int value1u = 0;
			status = rprxMaterialGetParameterValue(contextX,materialX,g_rprxParamList[iParam].param,&value1u); if (status != RPR_SUCCESS) { return false; };
		}
		else if ( type == RPRX_PARAMETER_TYPE_NODE )
		{
			rprx_material valueN = 0;
			status = rprxMaterialGetParameterValue(contextX,materialX,g_rprxParamList[iParam].param,&valueN); if (status != RPR_SUCCESS) { return false; };

			if ( valueN )
			{
				//check if the node is RPR or RPRX
				rpr_bool materialIsRPRX = false;
				status = rprxIsMaterialRprx(contextX,valueN,nullptr,&materialIsRPRX); if (status != RPR_SUCCESS) { return false; };

				if ( materialIsRPRX )
				{
					bool success = ParseRPRX(valueN , contextX, nodeList , nodeListX , folder  );
					if  ( !success )
					{
						return false;
					}
				}
				else
				{
					bool success = ParseRPR(valueN , nodeList , nodeListX , folder  );
					if  ( !success )
					{
						return false;
					}

				}

			}

			int a=0;
		}
		else
		{
			return false;
		}

	}




	return true;
}

//return true if success
//
// max_mat : pointer to the material
// node : pointer to a node (mesh) with this material assigned to it
bool exportMat(Mtl *max_mat, INode* node,const std::wstring &path)
{
		
	if ( g_rprxParamList.size() == 0 ) // if it's the first time , fill the variable
	{
		g_rprxParamList.push_back(RPRX_DEFINE_PARAM_MATERIAL(RPRX_UBER_MATERIAL_DIFFUSE_COLOR,"diffuse.color"));
		g_rprxParamList.push_back(RPRX_DEFINE_PARAM_MATERIAL(RPRX_UBER_MATERIAL_DIFFUSE_WEIGHT,"diffuse.weight"));
		g_rprxParamList.push_back(RPRX_DEFINE_PARAM_MATERIAL(RPRX_UBER_MATERIAL_DIFFUSE_ROUGHNESS,"diffuse.roughness"));

		g_rprxParamList.push_back(RPRX_DEFINE_PARAM_MATERIAL(RPRX_UBER_MATERIAL_REFLECTION_COLOR,"reflection.color"));
		g_rprxParamList.push_back(RPRX_DEFINE_PARAM_MATERIAL(RPRX_UBER_MATERIAL_REFLECTION_WEIGHT,"reflection.weight"));
		g_rprxParamList.push_back(RPRX_DEFINE_PARAM_MATERIAL(RPRX_UBER_MATERIAL_REFLECTION_ROUGHNESS,"reflection.roughness"));
		g_rprxParamList.push_back(RPRX_DEFINE_PARAM_MATERIAL(RPRX_UBER_MATERIAL_REFLECTION_ANISOTROPY,"reflection.anisotropy"));
		g_rprxParamList.push_back(RPRX_DEFINE_PARAM_MATERIAL(RPRX_UBER_MATERIAL_REFLECTION_ANISOTROPY_ROTATION,"reflection.anistropyRotation"));
		g_rprxParamList.push_back(RPRX_DEFINE_PARAM_MATERIAL(RPRX_UBER_MATERIAL_REFLECTION_MODE,"reflection.mode"));
		g_rprxParamList.push_back(RPRX_DEFINE_PARAM_MATERIAL(RPRX_UBER_MATERIAL_REFLECTION_IOR,"reflection.ior")); 
		g_rprxParamList.push_back(RPRX_DEFINE_PARAM_MATERIAL(RPRX_UBER_MATERIAL_REFLECTION_METALNESS,"reflection.metalness"));

		g_rprxParamList.push_back(RPRX_DEFINE_PARAM_MATERIAL(RPRX_UBER_MATERIAL_REFRACTION_COLOR,"refraction.color"));
		g_rprxParamList.push_back(RPRX_DEFINE_PARAM_MATERIAL(RPRX_UBER_MATERIAL_REFRACTION_WEIGHT,"refraction.weight"));
		g_rprxParamList.push_back(RPRX_DEFINE_PARAM_MATERIAL(RPRX_UBER_MATERIAL_REFRACTION_ROUGHNESS,"refraction.roughness"));
		g_rprxParamList.push_back(RPRX_DEFINE_PARAM_MATERIAL(RPRX_UBER_MATERIAL_REFRACTION_IOR,"refraction.ior"));
		g_rprxParamList.push_back(RPRX_DEFINE_PARAM_MATERIAL(RPRX_UBER_MATERIAL_REFRACTION_IOR_MODE,"refraction.iorMode"));
		g_rprxParamList.push_back(RPRX_DEFINE_PARAM_MATERIAL(RPRX_UBER_MATERIAL_REFRACTION_THIN_SURFACE,"refraction.thinSurface"));

		g_rprxParamList.push_back(RPRX_DEFINE_PARAM_MATERIAL(RPRX_UBER_MATERIAL_COATING_COLOR,"coating.color"));
		g_rprxParamList.push_back(RPRX_DEFINE_PARAM_MATERIAL(RPRX_UBER_MATERIAL_COATING_WEIGHT,"coating.weight"));
		g_rprxParamList.push_back(RPRX_DEFINE_PARAM_MATERIAL(RPRX_UBER_MATERIAL_COATING_ROUGHNESS,"coating.roughness"));
		g_rprxParamList.push_back(RPRX_DEFINE_PARAM_MATERIAL(RPRX_UBER_MATERIAL_COATING_MODE,"coating.mode"));
		g_rprxParamList.push_back(RPRX_DEFINE_PARAM_MATERIAL(RPRX_UBER_MATERIAL_COATING_IOR,"coating.ior"));
		g_rprxParamList.push_back(RPRX_DEFINE_PARAM_MATERIAL(RPRX_UBER_MATERIAL_COATING_METALNESS,"coating.metalness"));

		g_rprxParamList.push_back(RPRX_DEFINE_PARAM_MATERIAL(RPRX_UBER_MATERIAL_EMISSION_COLOR,"emission.color"));
		g_rprxParamList.push_back(RPRX_DEFINE_PARAM_MATERIAL(RPRX_UBER_MATERIAL_EMISSION_WEIGHT,"emission.weight"));
		g_rprxParamList.push_back(RPRX_DEFINE_PARAM_MATERIAL(RPRX_UBER_MATERIAL_EMISSION_MODE,"emission.mode"));

		g_rprxParamList.push_back(RPRX_DEFINE_PARAM_MATERIAL(RPRX_UBER_MATERIAL_TRANSPARENCY,"transparency"));
		g_rprxParamList.push_back(RPRX_DEFINE_PARAM_MATERIAL(RPRX_UBER_MATERIAL_NORMAL,"normal"));
		g_rprxParamList.push_back(RPRX_DEFINE_PARAM_MATERIAL(RPRX_UBER_MATERIAL_BUMP,"bump"));
		g_rprxParamList.push_back(RPRX_DEFINE_PARAM_MATERIAL(RPRX_UBER_MATERIAL_DISPLACEMENT,"displacement"));

		g_rprxParamList.push_back(RPRX_DEFINE_PARAM_MATERIAL(RPRX_UBER_MATERIAL_SSS_ABSORPTION_COLOR,"sss.absorptionColor"));
		g_rprxParamList.push_back(RPRX_DEFINE_PARAM_MATERIAL(RPRX_UBER_MATERIAL_SSS_SCATTER_COLOR,"sss.scatterColor"));
		g_rprxParamList.push_back(RPRX_DEFINE_PARAM_MATERIAL(RPRX_UBER_MATERIAL_SSS_ABSORPTION_DISTANCE,"sss.absorptionDistance"));
		g_rprxParamList.push_back(RPRX_DEFINE_PARAM_MATERIAL(RPRX_UBER_MATERIAL_SSS_SCATTER_DISTANCE,"sss.scatterDistance"));
		g_rprxParamList.push_back(RPRX_DEFINE_PARAM_MATERIAL(RPRX_UBER_MATERIAL_SSS_SCATTER_DIRECTION,"sss.scatterDirection"));
		g_rprxParamList.push_back(RPRX_DEFINE_PARAM_MATERIAL(RPRX_UBER_MATERIAL_SSS_WEIGHT,"sss.weight"));
		g_rprxParamList.push_back(RPRX_DEFINE_PARAM_MATERIAL(RPRX_UBER_MATERIAL_SSS_SUBSURFACE_COLOR,"sss.subsurfaceColor"));
		g_rprxParamList.push_back(RPRX_DEFINE_PARAM_MATERIAL(RPRX_UBER_MATERIAL_SSS_MULTISCATTER,"sss.multiscatter"));
	}

	std::string filename(path.begin(), path.end());

	std::string folder = "";
	std::string::size_type pos = filename.find_last_of('\\');
	if (pos == std::string::npos)
		pos = filename.find_last_of('/');
	if (pos != std::string::npos)
		folder = filename.substr(0, pos + 1);

	//folder = std::regex_replace(filename, std::regex("[^\\/]*$"), ""); //cut name of file

	FireRender::RenderParameters params;
	// fetch an instance of production renderer so we can read its parameters block
	auto renderer = GetCOREInterface()->GetRenderer(RS_Production);
	params.pblock = renderer->GetParamBlock(0);
	params.t = GetCOREInterface()->GetTime();

	// compute the context creation flags based on hardware capabilities
	rpr_creation_flags createFlags = FireRender::ScopeManagerMax::TheManager.getContextCreationFlags();
	
	// create a temporary context
	fr_int status = RPR_SUCCESS;
	fr_int plugs = 0;
	const char pluginsPath[] = "Tahoe64.dll";
	static fr_int plug = frRegisterPlugin(pluginsPath);
	fr_context context = nullptr;
	status = rprCreateContext(RPR_API_VERSION, &plug, 1, createFlags, NULL, NULL, &context);
	FCHECK(status);

	if (status != RPR_SUCCESS)
	{
		return false;
	}

	frw::Scope scope(context, true);
	FireRender::MaterialParser parser(scope);
	parser.SetTimeValue(params.t);
	parser.SetParamBlock(params.pblock);
	frw::Shader sh = parser.createShader(max_mat,node);
	
	std::set<rpr_material_node> nodeList;
	std::set<rprx_material> nodeListX;

	rprx_context mat_rprx_context = nullptr;

	bool isiRprx = sh.IsRprxMaterial();
	if ( isiRprx )
	{
		rprx_material mat_rprx_material = sh.GetHandleRPRXmaterial();
		mat_rprx_context = sh.GetHandleRPRXcontext();

		bool success = ParseRPRX(
			mat_rprx_material ,
			mat_rprx_context,
			 nodeList ,
			 nodeListX ,
			 folder  );

		if ( !success )
		{
			return false;
		}

	}
	else
	{

		bool success = ParseRPR(
			sh.Handle() ,
			 nodeList ,
			 nodeListX ,
			 folder  );

		if ( !success )
		{
			return false;
		}

	}


	ExportMaterials(filename, 
		//nodes_to_save.data(), nodes_to_save.size()
		nodeList, nodeListX, 
		g_rprxParamList,
		mat_rprx_context
		);

	return true;
}


def_visible_primitive(exportFrMat, "exportFrMat");

Value* exportFrMat_cf(Value** arg_list, int count)
{
    check_arg_count(exportFrMat, 3, count);

    Mtl* max_mat = arg_list[0]->to_mtl(); // pointer to the material
	INode* nodeShape = arg_list[2]->to_node(); // pointer to a node (mesh) with this material assigned to it

	if (!max_mat)
		return &empty;

	std::wstring path = arg_list[1]->to_string();

	if (exportMat(max_mat, nodeShape, path))
		return &ok;
	else
		return &empty;
}
///