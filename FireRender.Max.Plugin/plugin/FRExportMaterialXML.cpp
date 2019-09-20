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
#include <unordered_map>

#include "parser/MaterialLoader.h"
#include "plugin/ParamBlock.h"
#include "plugin/FireRenderer.h"




std::vector<RPRX_DEFINE_PARAM_MATERIAL> g_rprxParamList;


//retur false if FAIL
bool ParseRPR(
	rpr_material_node material ,
	std::set<rpr_material_node>& nodeList ,
	std::set<rprx_material>& nodeListX ,
	const std::string& folder  ,
	bool originIsUberColor, // set to TRUE if this 'material' is connected to a COLOR input of the UBER material
	std::unordered_map<rpr_image , RPR_MATERIAL_XML_EXPORT_TEXTURE_PARAM>& textureParameter,
	const std::string& material_name
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
	status = rprMaterialNodeGetInfo(material, RPR_MATERIAL_NODE_INPUT_COUNT, sizeof(size_t), &input_count, nullptr);
	for (int i = 0; i < input_count; ++i)
	{
		rpr_int in_type;
		status = rprMaterialNodeGetInputInfo(material, i, RPR_MATERIAL_NODE_INPUT_TYPE, sizeof(in_type), &in_type, nullptr);
		if (in_type == RPR_MATERIAL_NODE_INPUT_TYPE_NODE)
		{
			rpr_material_node ref_node = nullptr;
			status = rprMaterialNodeGetInputInfo(material, i, RPR_MATERIAL_NODE_INPUT_VALUE, sizeof(ref_node), &ref_node, nullptr);	
			if ( ref_node )
			{
				ParseRPR(ref_node,nodeList,nodeListX,folder,originIsUberColor,textureParameter,material_name);
			}
		}
		else if (in_type == RPR_MATERIAL_NODE_INPUT_TYPE_IMAGE)
		{
			rpr_image img = nullptr;
			status = rprMaterialNodeGetInputInfo(material, i, RPR_MATERIAL_NODE_INPUT_VALUE, sizeof(img), &img, nullptr);
			size_t name_size = 0;
			status = rprImageGetInfo(img, RPR_OBJECT_NAME, 0, nullptr, &name_size);
			//create file if filename is empty(name == "\0")
			if (name_size == 1)
			{
				//looking for a free filename
				int img_id = 0;
				std::string ext = ".png";
				
				std::string prefix = material_name + "_" + "tex_";

				//std::string img_name = folder + "tex_" + std::to_string(img_id) + ext;
				std::string img_name_short = prefix + std::to_string(img_id) + ext;
				
				
				while (std::ifstream(img_name_short).good())
				{
					//img_name = folder + "tex_" + std::to_string(++img_id) + ext;
					img_name_short = prefix + std::to_string(++img_id) + ext;
				}

				std::string img_name = folder + img_name_short;

				std::wstring wimg_name(img_name.begin(), img_name.end());

				rpr_image_desc desc;
				status = rprImageGetInfo(img, RPR_IMAGE_DESC, sizeof(desc), &desc, nullptr);
				BitmapInfo bi;
				bi.SetFlags(MAP_NOFLAGS);
				bi.SetName(wimg_name.c_str());
				bi.SetWidth(desc.image_width);
				bi.SetHeight(desc.image_height);
				bi.SetType(BMM_TRUE_32);

				Bitmap* map = TheManager->Create(&bi);

				//write image data
				rpr_image_format format;
				status = rprImageGetInfo(img, RPR_IMAGE_FORMAT, sizeof(format), &format, nullptr);

				if (format.type == RPR_COMPONENT_TYPE_UINT8)
				{
					size_t imgsize = 0;
					status = rprImageGetInfo(img, RPR_IMAGE_DATA, 0, nullptr, &imgsize);
					static std::vector<unsigned char> img_data(imgsize);
					img_data.resize(imgsize);
					status = rprImageGetInfo(img, RPR_IMAGE_DATA, imgsize, img_data.data(), nullptr);

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
					status = rprImageGetInfo(img, RPR_IMAGE_DATA, 0, nullptr, &imgsize);
					static std::vector<float> img_data(imgsize);
					img_data.resize(imgsize);
					status = rprImageGetInfo(img, RPR_IMAGE_DATA, imgsize, img_data.data(), nullptr);

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
				//std::string nameTexture = ("tex_" + std::to_string(img_id) + ext);
				std::string nameTexture = "RadeonProRMaps/" + img_name_short;
				rprObjectSetName(img, nameTexture.c_str());
			}
			else
			{
				std::string mat_name;
				mat_name.resize(name_size - 1);
				rprImageGetInfo(img, RPR_OBJECT_NAME, name_size, &mat_name[0], nullptr);
				int a=0;
			}

			textureParameter[img].useGamma = originIsUberColor ? true : false;
			int a=0;
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
	const std::string& folder ,
	std::unordered_map<rpr_image , RPR_MATERIAL_XML_EXPORT_TEXTURE_PARAM>& textureParameter,
	const std::string& material_name
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
			rpr_material_node valueN = 0;
			status = rprxMaterialGetParameterValue(contextX,materialX,g_rprxParamList[iParam].param,&valueN); if (status != RPR_SUCCESS) { return false; };

			if ( valueN )
			{
				//check if the node is RPR or RPRX
				rpr_bool materialIsRPRX = false;

#if (RPR_API_COMPAT < 0x010034000)
				status = rprxIsMaterialRprx(contextX,valueN,nullptr,&materialIsRPRX); if (status != RPR_SUCCESS) { return false; };
#endif
				if ( materialIsRPRX )
				{
					//having a Uber material linked as an input of another Uber Material doesn't make sense and is not managed by
					//the RPRX library. only a rpr_material_node can be input in a rprx_material
				}
				else
				{
					bool originIsUberColor = false;
					if (   g_rprxParamList[iParam].param == RPRX_UBER_MATERIAL_DIFFUSE_COLOR
						|| g_rprxParamList[iParam].param == RPRX_UBER_MATERIAL_REFLECTION_COLOR
						|| g_rprxParamList[iParam].param == RPRX_UBER_MATERIAL_REFRACTION_COLOR
						|| g_rprxParamList[iParam].param == RPRX_UBER_MATERIAL_REFRACTION_ABSORPTION_COLOR
						|| g_rprxParamList[iParam].param == RPRX_UBER_MATERIAL_COATING_COLOR
						|| g_rprxParamList[iParam].param == RPRX_UBER_MATERIAL_COATING_TRANSMISSION_COLOR
						|| g_rprxParamList[iParam].param == RPRX_UBER_MATERIAL_EMISSION_COLOR
						|| g_rprxParamList[iParam].param == RPRX_UBER_MATERIAL_SSS_SCATTER_COLOR
						|| g_rprxParamList[iParam].param == RPRX_UBER_MATERIAL_BACKSCATTER_COLOR
						)
					{
						originIsUberColor = true;
					}

					bool success = ParseRPR(valueN , nodeList , nodeListX , folder , originIsUberColor, textureParameter, material_name );
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
#if (RPR_API_COMPAT >= 0x010031000)
		g_rprxParamList.push_back(RPRX_DEFINE_PARAM_MATERIAL(RPRX_UBER_MATERIAL_DIFFUSE_NORMAL,"diffuse.normal"));
#endif


		g_rprxParamList.push_back(RPRX_DEFINE_PARAM_MATERIAL(RPRX_UBER_MATERIAL_REFLECTION_COLOR,"reflection.color"));
		g_rprxParamList.push_back(RPRX_DEFINE_PARAM_MATERIAL(RPRX_UBER_MATERIAL_REFLECTION_WEIGHT,"reflection.weight"));
		g_rprxParamList.push_back(RPRX_DEFINE_PARAM_MATERIAL(RPRX_UBER_MATERIAL_REFLECTION_ROUGHNESS,"reflection.roughness"));
		g_rprxParamList.push_back(RPRX_DEFINE_PARAM_MATERIAL(RPRX_UBER_MATERIAL_REFLECTION_ANISOTROPY,"reflection.anisotropy"));
		g_rprxParamList.push_back(RPRX_DEFINE_PARAM_MATERIAL(RPRX_UBER_MATERIAL_REFLECTION_ANISOTROPY_ROTATION,"reflection.anistropyRotation"));
		g_rprxParamList.push_back(RPRX_DEFINE_PARAM_MATERIAL(RPRX_UBER_MATERIAL_REFLECTION_MODE,"reflection.mode"));
		g_rprxParamList.push_back(RPRX_DEFINE_PARAM_MATERIAL(RPRX_UBER_MATERIAL_REFLECTION_IOR,"reflection.ior")); 
		//g_rprxParamList.push_back(RPRX_DEFINE_PARAM_MATERIAL(RPRX_UBER_MATERIAL_REFLECTION_METALNESS,"reflection.metalness")); RPRX_UBER_MATERIAL_REFLECTION_IOR = RPRX_UBER_MATERIAL_REFLECTION_METALNESS
#if (RPR_API_COMPAT >= 0x010031000)
		g_rprxParamList.push_back(RPRX_DEFINE_PARAM_MATERIAL(RPRX_UBER_MATERIAL_REFLECTION_NORMAL,"reflection.normal"));
#endif

		g_rprxParamList.push_back(RPRX_DEFINE_PARAM_MATERIAL(RPRX_UBER_MATERIAL_REFRACTION_COLOR,"refraction.color"));
		g_rprxParamList.push_back(RPRX_DEFINE_PARAM_MATERIAL(RPRX_UBER_MATERIAL_REFRACTION_WEIGHT,"refraction.weight"));
		g_rprxParamList.push_back(RPRX_DEFINE_PARAM_MATERIAL(RPRX_UBER_MATERIAL_REFRACTION_ROUGHNESS,"refraction.roughness"));
		g_rprxParamList.push_back(RPRX_DEFINE_PARAM_MATERIAL(RPRX_UBER_MATERIAL_REFRACTION_IOR,"refraction.ior"));
#if (RPR_API_COMPAT < 0x010031000)
		g_rprxParamList.push_back(RPRX_DEFINE_PARAM_MATERIAL(RPRX_UBER_MATERIAL_REFRACTION_IOR_MODE,"refraction.iorMode"));
#endif
		g_rprxParamList.push_back(RPRX_DEFINE_PARAM_MATERIAL(RPRX_UBER_MATERIAL_REFRACTION_THIN_SURFACE,"refraction.thinSurface"));

#if (RPR_API_COMPAT >= 0x010031000)
		g_rprxParamList.push_back(RPRX_DEFINE_PARAM_MATERIAL(RPRX_UBER_MATERIAL_REFRACTION_ABSORPTION_COLOR,"refraction.absorptionColor"));
		g_rprxParamList.push_back(RPRX_DEFINE_PARAM_MATERIAL(RPRX_UBER_MATERIAL_REFRACTION_ABSORPTION_DISTANCE,"refraction.absorptionDistance"));
		g_rprxParamList.push_back(RPRX_DEFINE_PARAM_MATERIAL(RPRX_UBER_MATERIAL_REFRACTION_CAUSTICS,"refraction.caustics"));
#endif

		g_rprxParamList.push_back(RPRX_DEFINE_PARAM_MATERIAL(RPRX_UBER_MATERIAL_COATING_COLOR,"coating.color"));
		g_rprxParamList.push_back(RPRX_DEFINE_PARAM_MATERIAL(RPRX_UBER_MATERIAL_COATING_WEIGHT,"coating.weight"));
		g_rprxParamList.push_back(RPRX_DEFINE_PARAM_MATERIAL(RPRX_UBER_MATERIAL_COATING_ROUGHNESS,"coating.roughness"));
		g_rprxParamList.push_back(RPRX_DEFINE_PARAM_MATERIAL(RPRX_UBER_MATERIAL_COATING_MODE,"coating.mode"));
		g_rprxParamList.push_back(RPRX_DEFINE_PARAM_MATERIAL(RPRX_UBER_MATERIAL_COATING_IOR,"coating.ior"));
		g_rprxParamList.push_back(RPRX_DEFINE_PARAM_MATERIAL(RPRX_UBER_MATERIAL_COATING_METALNESS,"coating.metalness"));
#if (RPR_API_COMPAT >= 0x010031000)
		g_rprxParamList.push_back(RPRX_DEFINE_PARAM_MATERIAL(RPRX_UBER_MATERIAL_COATING_NORMAL,"coating.normal"));
		g_rprxParamList.push_back(RPRX_DEFINE_PARAM_MATERIAL(RPRX_UBER_MATERIAL_COATING_TRANSMISSION_COLOR,"coating.transmissionColor"));
		g_rprxParamList.push_back(RPRX_DEFINE_PARAM_MATERIAL(RPRX_UBER_MATERIAL_COATING_THICKNESS,"coating.thickness"));
#endif

#if (RPR_API_COMPAT >= 0x010031000)
		g_rprxParamList.push_back(RPRX_DEFINE_PARAM_MATERIAL(RPRX_UBER_MATERIAL_SHEEN,"sheen"));
		g_rprxParamList.push_back(RPRX_DEFINE_PARAM_MATERIAL(RPRX_UBER_MATERIAL_SHEEN_TINT,"sheen.tint"));
		g_rprxParamList.push_back(RPRX_DEFINE_PARAM_MATERIAL(RPRX_UBER_MATERIAL_SHEEN_WEIGHT,"sheen.weight"));
#endif

		g_rprxParamList.push_back(RPRX_DEFINE_PARAM_MATERIAL(RPRX_UBER_MATERIAL_EMISSION_COLOR,"emission.color"));
		g_rprxParamList.push_back(RPRX_DEFINE_PARAM_MATERIAL(RPRX_UBER_MATERIAL_EMISSION_WEIGHT,"emission.weight"));
		g_rprxParamList.push_back(RPRX_DEFINE_PARAM_MATERIAL(RPRX_UBER_MATERIAL_EMISSION_MODE,"emission.mode"));

		g_rprxParamList.push_back(RPRX_DEFINE_PARAM_MATERIAL(RPRX_UBER_MATERIAL_TRANSPARENCY,"transparency"));
#if (RPR_API_COMPAT < 0x010031000)
		g_rprxParamList.push_back(RPRX_DEFINE_PARAM_MATERIAL(RPRX_UBER_MATERIAL_NORMAL,"normal"));
#endif
#if (RPR_API_COMPAT < 0x010031000)
		g_rprxParamList.push_back(RPRX_DEFINE_PARAM_MATERIAL(RPRX_UBER_MATERIAL_BUMP,"bump"));
		g_rprxParamList.push_back(RPRX_DEFINE_PARAM_MATERIAL(RPRX_UBER_MATERIAL_DISPLACEMENT, "displacement"));
#endif

#if (RPR_API_COMPAT < 0x010031000)
		g_rprxParamList.push_back(RPRX_DEFINE_PARAM_MATERIAL(RPRX_UBER_MATERIAL_SSS_ABSORPTION_COLOR,"sss.absorptionColor"));
#endif
		g_rprxParamList.push_back(RPRX_DEFINE_PARAM_MATERIAL(RPRX_UBER_MATERIAL_SSS_SCATTER_COLOR,"sss.scatterColor"));
#if (RPR_API_COMPAT < 0x010031000)
		g_rprxParamList.push_back(RPRX_DEFINE_PARAM_MATERIAL(RPRX_UBER_MATERIAL_SSS_ABSORPTION_DISTANCE,"sss.absorptionDistance"));
#endif
		g_rprxParamList.push_back(RPRX_DEFINE_PARAM_MATERIAL(RPRX_UBER_MATERIAL_SSS_SCATTER_DISTANCE,"sss.scatterDistance"));
		g_rprxParamList.push_back(RPRX_DEFINE_PARAM_MATERIAL(RPRX_UBER_MATERIAL_SSS_SCATTER_DIRECTION,"sss.scatterDirection"));
		g_rprxParamList.push_back(RPRX_DEFINE_PARAM_MATERIAL(RPRX_UBER_MATERIAL_SSS_WEIGHT,"sss.weight"));
#if (RPR_API_COMPAT < 0x010031000)
		g_rprxParamList.push_back(RPRX_DEFINE_PARAM_MATERIAL(RPRX_UBER_MATERIAL_SSS_SUBSURFACE_COLOR,"sss.subsurfaceColor"));
#endif
		g_rprxParamList.push_back(RPRX_DEFINE_PARAM_MATERIAL(RPRX_UBER_MATERIAL_SSS_MULTISCATTER,"sss.multiscatter"));

#if (RPR_API_COMPAT >= 0x010031000)
		g_rprxParamList.push_back(RPRX_DEFINE_PARAM_MATERIAL(RPRX_UBER_MATERIAL_BACKSCATTER_WEIGHT,"backscatter.weight"));
		g_rprxParamList.push_back(RPRX_DEFINE_PARAM_MATERIAL(RPRX_UBER_MATERIAL_BACKSCATTER_COLOR,"backscatter.color"));
#endif

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
	rpr_int status = RPR_SUCCESS;
	rpr_int plugs = 0;
	const char pluginsPath[] = "Tahoe64.dll";
	static rpr_int plug = rprRegisterPlugin(pluginsPath);
	rpr_context context = nullptr;
#ifdef RPR_VERSION_MAJOR_MINOR_REVISION
	status = rprCreateContext(RPR_VERSION_MAJOR_MINOR_REVISION, &plug, 1, createFlags, NULL, NULL, &context);
#else
	status = rprCreateContext(RPR_API_VERSION, &plug, 1, createFlags, NULL, NULL, &context);
#endif
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
	std::unordered_map<rpr_image , RPR_MATERIAL_XML_EXPORT_TEXTURE_PARAM> textureParameter;

	rprx_context mat_rprx_context = nullptr;

	void* closureNode = nullptr;

	std::string material_name = std::regex_replace(filename, std::regex("^.*[\\\\/]"), ""); // cut path
	material_name = std::regex_replace(material_name, std::regex("[.].*"), ""); // cut extension

	bool isiRprx = sh.IsRprxMaterial();
	if ( isiRprx )
	{
		rprx_material mat_rprx_material = sh.GetHandleRPRXmaterial();
		mat_rprx_context = sh.GetHandleRPRXcontext();

		closureNode = mat_rprx_material;

		bool success = ParseRPRX(
			mat_rprx_material ,
			mat_rprx_context,
			nodeList ,
			nodeListX ,
			folder ,
			textureParameter,
			material_name);

		if ( !success )
		{
			return false;
		}

	}
	else
	{
		closureNode = sh.Handle();

		bool success = ParseRPR(
			sh.Handle() ,
			nodeList ,
			nodeListX ,
			folder ,
			false , 
			textureParameter,
			material_name);

		if ( !success )
		{
			return false;
		}

	}


	ExportMaterials(filename, 
		//nodes_to_save.data(), nodes_to_save.size()
		nodeList, nodeListX, 
		g_rprxParamList,
		mat_rprx_context,
		textureParameter,
		closureNode,
		material_name
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