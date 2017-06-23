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
#include <string>

void ExportMaterials(const std::string& filename, rpr_material_node* materials, int mat_count);
bool ImportMaterials(const std::string& filename, rpr_context context, rpr_material_system sys, rpr_material_node** out_materials, int* out_mat_count);

#endif //MATERIAL_LOADER_H_