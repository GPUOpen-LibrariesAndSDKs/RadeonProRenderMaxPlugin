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


#if 0

#ifndef MATERIAL_LOADER_H_
#define MATERIAL_LOADER_H_

#include "RadeonProRender.h"
#include "RprSupport.h"
#include <string>
#include <set>
#include <vector>
#include <unordered_map>

	
struct RPRX_DEFINE_PARAM_MATERIAL
{
	RPRX_DEFINE_PARAM_MATERIAL(rprx_parameter param_ , const std::string& name_)
	{
		param = param_;
		nameInXML = name_;
	}
	rprx_parameter param;
	std::string nameInXML;
};

struct RPR_MATERIAL_XML_EXPORT_TEXTURE_PARAM
{
	RPR_MATERIAL_XML_EXPORT_TEXTURE_PARAM()
	{
		useGamma = false;
	}
	bool useGamma;
};

void ExportMaterials(
		const std::string& filename,
		const std::set<rpr_material_node>& nodeList,
		const std::set<rprx_material>& nodeListX ,
		const std::vector<RPRX_DEFINE_PARAM_MATERIAL>& rprxParamList,
		rprx_context contextX ,
		std::unordered_map<rpr_image , RPR_MATERIAL_XML_EXPORT_TEXTURE_PARAM>& textureParameter,
		void* closureNode,
		const std::string& material_name 
		);

bool ImportMaterials(const std::string& filename, rpr_context context, rpr_material_system sys, rpr_material_node** out_materials, int* out_mat_count);

#endif //MATERIAL_LOADER_H_

#endif
