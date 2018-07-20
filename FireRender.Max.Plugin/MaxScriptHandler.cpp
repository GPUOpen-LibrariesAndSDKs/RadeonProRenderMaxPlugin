#include <maxscript/maxscript.h>
#include <maxscript/foundation/numbers.h>
#include <maxscript/foundation/arrays.h>

inline bool bool_cast( int x ) { return (x? true:false); }

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

	enableGltfExport = bool_cast( pEnable->to_bool() );

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
