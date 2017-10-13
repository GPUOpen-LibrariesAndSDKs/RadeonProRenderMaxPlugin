#include <maxscript/maxscript.h>
#include <maxscript/foundation/numbers.h>
#include <maxscript/foundation/arrays.h>

static bool EnableGltfExport = false;

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

	EnableGltfExport = pEnable->to_bool();

	mputs(_M("GLTF Export functionality is turned "));
	EnableGltfExport ? mputs(_M("ON\r\n")) : mputs(_M("OFF\r\n"));

	one_typed_value_local(Integer* value);
	vl.value = new Integer(EnableGltfExport);

	return_value(vl.value);
}

extern "C" bool IsGltfExportEnabled()
{
	return EnableGltfExport;
}
