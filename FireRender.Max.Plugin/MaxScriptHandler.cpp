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


#include <maxscript/maxscript.h>
#include <maxscript/foundation/numbers.h>
#include <maxscript/foundation/arrays.h>

static bool enableGltfExport = true;

// Declare C++ function and register it with MAXScript
#include <maxscript\macros\define_instantiation_functions.h>
def_visible_primitive(rprEnableGltfExport, "rprEnableGltfExport");

Value* rprEnableGltfExport_cf(Value **arg_list, int count)
{
	//--------------------------------------------------------
	//Maxscript usage:
	//--------------------------------------------------------
	// <array> IntervalArray <Start:Number> <End:Number> <steps:Integer>
	check_arg_count(rprEnableGltfExport, 1, count);

	Value* pEnable = arg_list[0];

	enableGltfExport = pEnable->to_bool() == TRUE;

	mputs(_M("GLTF Export functionality is turned "));
	enableGltfExport ? mputs(_M("ON\r\n")) : mputs(_M("OFF\r\n"));

	one_typed_value_local(Integer* value);
	vl.value = new Integer(enableGltfExport);

	return_value(vl.value);
}

extern "C" bool IsGltfExportEnabled()
{
	return enableGltfExport;
}

extern "C" void EnableGltfExport()
{
	enableGltfExport = true;
}

extern "C" void DisableGltfExport()
{
	enableGltfExport = false;
}
