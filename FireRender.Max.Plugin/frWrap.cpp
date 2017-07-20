#include "frWrap.h"

namespace frw
{
	Shader::Shader(DataPtr p)
	{
		m = p;
	}

	Shader::Shader(const MaterialSystem& ms, ShaderType type, bool destroyOnDelete) :
		Node(ms, type, destroyOnDelete, new Data())
	{
		data().shaderType = type;
	}

	Shader::Shader(rpr_material_node h, const MaterialSystem& ms, bool destroyOnDelete) :
		Node(h, ms, destroyOnDelete, new Data())
	{
	}

	Shader::Shader(Context& context, rprx_context xContext, rprx_material_type type) :
		Node(context, new Data())
	{
		Data& d = data();
		d.context = xContext;
		
		rpr_int status = rprxCreateMaterial(xContext, type, &d.material);
		FASSERT(RPR_SUCCESS == status);
		
		d.shaderType = ShaderTypeRprx;
		DebugPrint(L"\tCreated RPRX material %08X\n", d.material);
	}

	ShaderType Shader::GetShaderType() const
	{
		if (!IsValid())
			return ShaderTypeInvalid;

		return data().shaderType;
	}

	bool Shader::IsRprxMaterial() const
	{
		return data().material != nullptr;
	}

	void Shader::SetDirty()
	{
		data().bDirty = true;
	}

	bool Shader::IsDirty() const
	{
		return data().bDirty;
	}

	void Shader::AttachToShape(Shape::Data& shape)
	{
		rpr_int res;
		Data& d = data();

		d.numAttachedShapes++;

		if (d.material)
		{
			DebugPrint(L"\tShape.AttachMaterial: shape=%08X x_material=%08X\n", shape.Handle(), d.material);
			res = rprxShapeAttachMaterial(d.context, shape.Handle(), d.material);
			FASSERT(RPR_SUCCESS == res);
			res = rprxMaterialCommit(d.context, d.material);
			FASSERT(RPR_SUCCESS == res);
		}
		else
		{
			DebugPrint(L"\tShape.AttachMaterial: shape=%08X material=%08X\n", shape.Handle(), d.Handle());
			res = rprShapeSetMaterial(shape.Handle(), d.Handle());
			FASSERT(RPR_SUCCESS == res);
		}
	}

	void Shader::DetachFromShape(Shape::Data& shape)
	{
		Data& d = data();

		d.numAttachedShapes--;

		DebugPrint(L"\tShape.DetachMaterial: shape=%08X\n", shape.Handle());
		
		if (d.material)
		{
			rpr_int res = rprxShapeDetachMaterial(d.context, shape.Handle(), d.material);
			FASSERT(RPR_SUCCESS == res);
		}

		{
			auto res = rprShapeSetMaterial(shape.Handle(), nullptr);
			FASSERT(RPR_SUCCESS == res);
		}
	}

	void Shader::AttachToMaterialInput(rpr_material_node node, const char* inputName) const
	{
		rpr_int res;
		const Data& d = data();

		if (d.material)
		{
			// attach rprx shader output to some material's input
			// note: there's no call to rprxShapeDetachMaterial
			res = rprxMaterialAttachMaterial(d.context, node, inputName, d.material);
			FASSERT(RPR_SUCCESS == res);
			res = rprxMaterialCommit(d.context, d.material);
			FASSERT(RPR_SUCCESS == res);
		}
		else
		{
			res = rprMaterialNodeSetInputN(node, inputName, d.Handle());
			FASSERT(RPR_SUCCESS == res);
		}
	}

	void Shader::xSetParameterN(rprx_parameter parameter, const frw::Node& node)
	{
		AddReference(node);

		const Data& d = data();
		auto res = rprxMaterialSetParameterN(d.context, d.material, parameter, node.Handle());
		FASSERT(RPR_SUCCESS == res);
	}

	void Shader::xSetParameterU(rprx_parameter parameter, rpr_uint value) const
	{
		const Data& d = data();
		auto res = rprxMaterialSetParameterU(d.context, d.material, parameter, value);
		FASSERT(RPR_SUCCESS == res);
	}

	void Shader::xSetParameterF(rprx_parameter parameter, rpr_float x, rpr_float y, rpr_float z, rpr_float w) const
	{
		const Data& d = data();
		auto res = rprxMaterialSetParameterF(d.context, d.material, parameter, x, y, z, w);
		FASSERT(RPR_SUCCESS == res);
	}

	bool Shader::xSetValue(rprx_parameter parameter, const Value& v)
	{
		switch (v.type)
		{
		case Value::FLOAT:
			xSetParameterF(parameter, v.x, v.y, v.z, v.w);
			return true;

		case Value::NODE:
			if (!v.node)	// in theory we should now allow this, as setting a NULL input is legal (as of FRSDK 1.87)
				return false;

			xSetParameterN(parameter, v.node);	// should be ok to set null here now

			return true;
		}

		assert(!"bad type");

		return false;
	}
}
