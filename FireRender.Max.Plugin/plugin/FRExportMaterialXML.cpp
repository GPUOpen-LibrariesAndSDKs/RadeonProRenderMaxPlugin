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

bool exportMat(Mtl *max_mat, const std::wstring &path)
{
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
	int renderDevice;
	params.pblock->GetValue(FireRender::PARAM_RENDER_DEVICE, params.t, renderDevice, FOREVER);
	bool useGpu = (renderDevice == RPR_RENDERDEVICE_GPUONLY) || (renderDevice == RPR_RENDERDEVICE_CPUGPU);
	bool useCpu = (renderDevice == RPR_RENDERDEVICE_CPUONLY) || (renderDevice == RPR_RENDERDEVICE_CPUGPU);
	int createFlags = FireRender::ScopeManagerMax::TheManager.getContextCreationFlags(useGpu, useCpu);
	
	// create a temporary context
	fr_int status = RPR_SUCCESS;
	fr_int plugs = 0;
	const char pluginsPath[512] = { "Tahoe64.dll" };
	static fr_int plug = frRegisterPlugin(pluginsPath);
	fr_context context = nullptr;
	status = rprCreateContext(RPR_API_VERSION, &plug, 1, createFlags, NULL, NULL, &context);

	frw::Scope scope(context, true);
	FireRender::MaterialParser parser(scope);
	parser.SetTimeValue(params.t);
	parser.SetParamBlock(params.pblock);
	frw::Shader sh = parser.createShader(max_mat);
	std::set<fr_material_node> nodes_to_check;
	std::vector<fr_material_node> nodes_to_save;

	nodes_to_check.insert(sh.Handle());
	//look for all required by sh material nodes and create files for backed textures
	while (!nodes_to_check.empty())
	{
		fr_material_node mat = *nodes_to_check.begin(); nodes_to_check.erase(mat);
		nodes_to_save.push_back(mat);

		size_t input_count = 0;
		status = frMaterialNodeGetInfo(mat, RPR_MATERIAL_NODE_INPUT_COUNT, sizeof(size_t), &input_count, nullptr);
		for (int i = 0; i < input_count; ++i)
		{
			fr_int in_type;
			status = frMaterialNodeGetInputInfo(mat, i, RPR_MATERIAL_NODE_INPUT_TYPE, sizeof(in_type), &in_type, nullptr);
			if (in_type == RPR_MATERIAL_NODE_INPUT_TYPE_NODE)
			{
				fr_material_node ref_node = nullptr;
				status = frMaterialNodeGetInputInfo(mat, i, RPR_MATERIAL_NODE_INPUT_VALUE, sizeof(ref_node), &ref_node, nullptr);
				//if node is not null and it's not duplicated
				if (ref_node && std::find(nodes_to_save.begin(), nodes_to_save.end(), ref_node) == nodes_to_save.end())
				{
					nodes_to_check.insert(ref_node);
				}
			}
			else if (in_type == RPR_MATERIAL_NODE_INPUT_TYPE_IMAGE)
			{
				fr_image img = nullptr;
				status = frMaterialNodeGetInputInfo(mat, i, RPR_MATERIAL_NODE_INPUT_VALUE, sizeof(img), &img, nullptr);
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
	}

	ExportMaterials(filename, nodes_to_save.data(), nodes_to_save.size());
}


def_visible_primitive(exportFrMat, "exportFrMat");

Value* exportFrMat_cf(Value** arg_list, int count)
{
    check_arg_count(exportFrMat, 2, count);

    Mtl* max_mat = arg_list[0]->to_mtl();

	if (!max_mat)
		return &empty;

	std::wstring path = arg_list[1]->to_string();

	if (exportMat(max_mat, path))
		return &ok;
	else
		return &empty;
}
///