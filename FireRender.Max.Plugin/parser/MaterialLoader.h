/*********************************************************************************************************************************
* Radeon ProRender for 3ds Max plugin
* Copyright (c) 2017 AMD
* All Rights Reserved
*
* Load and save Radeon ProRender XML material files
*********************************************************************************************************************************/

#ifndef MATERIAL_LOADER_H_
#define MATERIAL_LOADER_H_

#include "RadeonProRender.h"
#include "RprSupport.h"
#include <string>
#include <set>
#include <vector>

	
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

void ExportMaterials(
		const std::string& filename,
		const std::set<rpr_material_node>& nodeList,
		const std::set<rprx_material>& nodeListX ,
		const std::vector<RPRX_DEFINE_PARAM_MATERIAL>& rprxParamList,
		rprx_context contextX 
		);

bool ImportMaterials(const std::string& filename, rpr_context context, rpr_material_system sys, rpr_material_node** out_materials, int* out_mat_count);

#endif //MATERIAL_LOADER_H_