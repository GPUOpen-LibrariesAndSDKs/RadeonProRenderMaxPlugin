/*********************************************************************************************************************************
* Radeon ProRender for 3ds Max plugin
* Copyright (C) 2017 AMD
* All Rights Reserved
*
* Abstractions of Radeon ProRender core objects
*
* Smart pointers track usage and by default destroy objects when pointers are destroyed
*
* The smart pointers are protected to avoid unwanted dereferencing. This does complicate the code
* here (with the requirement for internal Data/Extra structs) yet it means that client code can use
* these objects as completely encapsulated reference types.
*********************************************************************************************************************************/

#pragma once

#include <RadeonProRender.h>
#include <RprSupport.h>

#define _USE_MATH_DEFINES
#include <math.h>

#include <Math/float3.h>

#include "utils/Utils.h"

#ifdef MAX_DEPRECATED
#define FRW_USE_MAX_TYPES	// allow use of max types in helpers
#endif

#include <list>
#include <map>
#include <vector>
#include <memory>
#include <set>


// AMDMAX-232 there seems to be some issue with the ID of nodes from DOT3
#if RPR_API_VERSION == 0x010000098 || RPR_API_VERSION == 0x010000096
#undef RPR_MATERIAL_NODE_OP_DOT3
#define RPR_MATERIAL_NODE_OP_DOT3 0xB
#endif

#ifndef RPR_MATERIAL_NODE_FRESNEL_SCHLICK
#define RPR_MATERIAL_NODE_FRESNEL_SCHLICK RPR_MATERIAL_NODE_FRESNEL
#endif

namespace frw
{
	class ImageNode;
	class Image;
	// forward declarations

	class Node;
	class Value;
	class Context;
	class MaterialSystem;
	class Shader;
	class FrameBuffer;
    class PostEffect;
	class Scope;

	enum Operator
	{
		OperatorAdd = RPR_MATERIAL_NODE_OP_ADD,
		OperatorSubtract = RPR_MATERIAL_NODE_OP_SUB,
		OperatorMultiply = RPR_MATERIAL_NODE_OP_MUL,
		OperatorDivide = RPR_MATERIAL_NODE_OP_DIV,
		OperatorSin = RPR_MATERIAL_NODE_OP_SIN,
		OperatorCos = RPR_MATERIAL_NODE_OP_COS,
		OperatorTan = RPR_MATERIAL_NODE_OP_TAN,
		OperatorSelectX = RPR_MATERIAL_NODE_OP_SELECT_X,
		OperatorSelectY = RPR_MATERIAL_NODE_OP_SELECT_Y,
		OperatorSelectZ = RPR_MATERIAL_NODE_OP_SELECT_Z,
		OperatorCombine = RPR_MATERIAL_NODE_OP_COMBINE,
		OperatorDot = RPR_MATERIAL_NODE_OP_DOT3,
		OperatorCross = RPR_MATERIAL_NODE_OP_CROSS3,
		OperatorLength = RPR_MATERIAL_NODE_OP_LENGTH3,
		OperatorNormalize = RPR_MATERIAL_NODE_OP_NORMALIZE3,
		OperatorPow = RPR_MATERIAL_NODE_OP_POW,
		OperatorArcCos = RPR_MATERIAL_NODE_OP_ACOS,
		OperatorArcSin = RPR_MATERIAL_NODE_OP_ASIN,
		OperatorArcTan = RPR_MATERIAL_NODE_OP_ATAN,
		OperatorComponentAverage = RPR_MATERIAL_NODE_OP_AVERAGE_XYZ,
		OperatorAverage = RPR_MATERIAL_NODE_OP_AVERAGE,
		OperatorMin = RPR_MATERIAL_NODE_OP_MIN,
		OperatorMax = RPR_MATERIAL_NODE_OP_MAX,
		OperatorFloor = RPR_MATERIAL_NODE_OP_FLOOR,
		OperatorMod = RPR_MATERIAL_NODE_OP_MOD,
		OperatorAbs = RPR_MATERIAL_NODE_OP_ABS,
	};

	enum LookupType
	{
		LookupTypeUV0 = RPR_MATERIAL_NODE_LOOKUP_UV,
		LookupTypeNormal = RPR_MATERIAL_NODE_LOOKUP_N,
		LookupTypePosition = RPR_MATERIAL_NODE_LOOKUP_P,
		LookupTypeIncident = RPR_MATERIAL_NODE_LOOKUP_INVEC,
		LookupTypeOutVector = RPR_MATERIAL_NODE_LOOKUP_OUTVEC,
		LookupTypeUV1 = RPR_MATERIAL_NODE_LOOKUP_UV1,
	};

	enum ValueType
	{
		ValueTypeArithmetic = RPR_MATERIAL_NODE_ARITHMETIC,
		ValueTypeFresnel = RPR_MATERIAL_NODE_FRESNEL,
		ValueTypeNormalMap = RPR_MATERIAL_NODE_NORMAL_MAP,
		ValueTypeBumpMap = RPR_MATERIAL_NODE_BUMP_MAP,
		ValueTypeImageMap = RPR_MATERIAL_NODE_IMAGE_TEXTURE,
		ValueTypeNoiseMap = RPR_MATERIAL_NODE_NOISE2D_TEXTURE,
		ValueTypeRadialMap = RPR_MATERIAL_NODE_DOT_TEXTURE,
		ValueTypeGradientMap = RPR_MATERIAL_NODE_GRADIENT_TEXTURE,
		ValueTypeCheckerMap = RPR_MATERIAL_NODE_CHECKER_TEXTURE,
		ValueTypeConstant = RPR_MATERIAL_NODE_CONSTANT_TEXTURE,
		ValueTypeLookup = RPR_MATERIAL_NODE_INPUT_LOOKUP,
		ValueTypeBlend = RPR_MATERIAL_NODE_BLEND_VALUE,
		ValueTypeFresnelSchlick = RPR_MATERIAL_NODE_FRESNEL_SCHLICK,
	};

	enum ShaderType
	{
		ShaderTypeInvalid = 0xffffffff,
		ShaderTypeRprx    = 0xfffffffe,

		ShaderTypeDiffuse = RPR_MATERIAL_NODE_DIFFUSE,
		ShaderTypeMicrofacet = RPR_MATERIAL_NODE_MICROFACET,
		ShaderTypeReflection = RPR_MATERIAL_NODE_REFLECTION,
		ShaderTypeRefraction = RPR_MATERIAL_NODE_REFRACTION,
		ShaderTypeMicrofacetRefraction = RPR_MATERIAL_NODE_MICROFACET_REFRACTION,
		ShaderTypeTransparent = RPR_MATERIAL_NODE_TRANSPARENT,
		ShaderTypeEmissive = RPR_MATERIAL_NODE_EMISSIVE,
		ShaderTypeWard = RPR_MATERIAL_NODE_WARD,
		ShaderTypeAdd = RPR_MATERIAL_NODE_ADD,
		ShaderTypeBlend = RPR_MATERIAL_NODE_BLEND,
		ShaderTypeStandard = RPR_MATERIAL_NODE_STANDARD,
		ShaderTypePassthrough = RPR_MATERIAL_NODE_PASSTHROUGH,
		ShaderTypeOrenNayer = RPR_MATERIAL_NODE_ORENNAYAR,
		ShaderTypeDiffuseRefraction = RPR_MATERIAL_NODE_DIFFUSE_REFRACTION,
		ShaderTypeUber = RPR_MATERIAL_NODE_STANDARD,
		ShaderTypeVolume = RPR_MATERIAL_NODE_VOLUME
	};

	enum ContextParameterType
	{
		ParameterTypeFloat = RPR_PARAMETER_TYPE_FLOAT,
		ParameterTypeFloat2 = RPR_PARAMETER_TYPE_FLOAT2,
		ParameterTypeFloat3 = RPR_PARAMETER_TYPE_FLOAT3,
		ParameterTypeFloat4 = RPR_PARAMETER_TYPE_FLOAT4,
		ParameterTypeImage = RPR_PARAMETER_TYPE_IMAGE,
		ParameterTypeString = RPR_PARAMETER_TYPE_STRING,
		ParameterTypeShader = RPR_PARAMETER_TYPE_SHADER,
		ParameterTypeUint = RPR_PARAMETER_TYPE_UINT,
	};

	enum ContextParameterInfo
	{
		ParameterInfoId = RPR_PARAMETER_NAME,
		ParameterInfoName = RPR_PARAMETER_NAME_STRING,
		ParameterInfoType = RPR_PARAMETER_TYPE,
		ParameterInfoDescription = RPR_PARAMETER_DESCRIPTION,
		ParameterInfoValue = RPR_PARAMETER_VALUE,
	};

	enum NodeInfo
	{
		NodeInfoType = RPR_MATERIAL_NODE_TYPE,
		NodeInfoInputCount = RPR_MATERIAL_NODE_INPUT_COUNT,
	};

	enum NodeInputType
	{
		NodeInputTypeFloat4 = RPR_MATERIAL_NODE_INPUT_TYPE_FLOAT4,
		NodeInputTypeUint = RPR_MATERIAL_NODE_INPUT_TYPE_UINT,
		NodeInputTypeNode = RPR_MATERIAL_NODE_INPUT_TYPE_NODE,
		NodeInputTypeImage = RPR_MATERIAL_NODE_INPUT_TYPE_IMAGE,
	};

	enum NodeInputInfo
	{
		NodeInputInfoName = RPR_MATERIAL_NODE_INPUT_NAME,
		NodeInputInfoDescription = RPR_MATERIAL_NODE_INPUT_DESCRIPTION,
		NodeInputInfoValue = RPR_MATERIAL_NODE_INPUT_VALUE,
		NodeInputInfoType = RPR_MATERIAL_NODE_INPUT_TYPE,
	};

	enum ToneMappingType
	{
		ToneMappingTypeNone = 0,
		ToneMappingTypeSimplified = 1,
		ToneMappingTypeLinear = 2,
		ToneMappingTypeNonLinear = 3,
	};

	enum NormalMapType
	{
		NormalMapTypeNormal = RPR_MATERIAL_NODE_NORMAL_MAP,
		NormalMapTypeBump = RPR_MATERIAL_NODE_BUMP_MAP,
	};

	enum ErrorCode
	{
		ErrorSuccess = RPR_SUCCESS,
		ErrorComputeApiNotSupported = RPR_ERROR_COMPUTE_API_NOT_SUPPORTED,
		ErrorOutOfSystemMemory = RPR_ERROR_OUT_OF_SYSTEM_MEMORY,
		ErrorOutOfVideoMemory = RPR_ERROR_OUT_OF_VIDEO_MEMORY,
		ErrorInvalidLightPathExpression = RPR_ERROR_INVALID_LIGHTPATH_EXPR,
		ErrorInvalidImage =	RPR_ERROR_INVALID_IMAGE,
		ErrorInvalidAAMethod = RPR_ERROR_INVALID_AA_METHOD,
		ErrorUnsupportedImageFormat = RPR_ERROR_UNSUPPORTED_IMAGE_FORMAT,
		ErrorInvalidGlTexture = RPR_ERROR_INVALID_GL_TEXTURE,
		ErrorInvalidClImage = RPR_ERROR_INVALID_CL_IMAGE,
		ErrorInvalidObject = RPR_ERROR_INVALID_OBJECT,
		ErrorInvalidParameter = RPR_ERROR_INVALID_PARAMETER,
		ErrorInvalidTag = RPR_ERROR_INVALID_TAG,
		ErrorInvalidLight = RPR_ERROR_INVALID_LIGHT,
		ErrorInvalidContext = RPR_ERROR_INVALID_CONTEXT,
		ErrorUnimplemented = RPR_ERROR_UNIMPLEMENTED,
		ErrorInvalidApiVersion = RPR_ERROR_INVALID_API_VERSION,
		ErrorInternalError = RPR_ERROR_INTERNAL_ERROR,
		ErrorIoError = RPR_ERROR_IO_ERROR,
		ErrorUnsupportedShaderParameterType = RPR_ERROR_UNSUPPORTED_SHADER_PARAMETER_TYPE,
		ErrorMaterialStackOverflow = RPR_ERROR_MATERIAL_STACK_OVERFLOW
	};

	enum PostEffectType
	{
		PostEffectTypeToneMap = RPR_POST_EFFECT_TONE_MAP,
		PostEffectTypeWhiteBalance = RPR_POST_EFFECT_WHITE_BALANCE,
		PostEffectTypeSimpleTonemap = RPR_POST_EFFECT_SIMPLE_TONEMAP,
		PostEffectTypeNormalization = RPR_POST_EFFECT_NORMALIZATION,
		PostEffectTypeGammaCorrection = RPR_POST_EFFECT_GAMMA_CORRECTION,
	};

	enum ColorSpaceType
	{
        ColorSpaceTypeSRGB = RPR_COLOR_SPACE_SRGB,
        ColorSpaceTypeAdobeSRGB = RPR_COLOR_SPACE_ADOBE_RGB,
        ColorSpaceTypeREC2020 = RPR_COLOR_SPACE_REC2020,
        ColorSpaceTypeDCIP3 = RPR_COLOR_SPACE_DCIP3,
    };

	enum Constant
	{
		MaximumSubdividedFacecount = 100000, 
	};

    


	class Exception : public std::exception
	{
	public:
		ErrorCode code;
		explicit Exception(int code) : code(static_cast<ErrorCode>(code)) {}
	};

	// a self deleting shared object
	// base class for objects that need to manage their own lifespan
	class Object
	{
		// should never be stored as address, always treat as value
		void operator&() const = delete;

	protected:
		class Data;
		typedef std::shared_ptr<Data> DataPtr;

		// the internal wrapper of the handle, + references and context if applicable
		// note that this is protected so as to avoid the hideous problems
		// caused by accidentally duplicating shared pointers when given access
		// to their contents. 
		class Data
		{
			friend Object;
		private:
			void*					handle = nullptr;


			bool operator==(void* h) const = delete;

		protected:
			bool					destroyOnDelete = true;	// handle will be deleted?
			const wchar_t* typeNameMirror = L"UnknownType";

			void Init(void* h, const Context& context, bool destroyOnDelete);

		public:
			DataPtr					context;

			std::set<DataPtr>		references;	// list of references to objects used by this object
			size_t					userData = 0;	// simple lvalue for user reference

			Data() {}
			Data(void* h, const Context& context, bool destroyOnDelete = true)
			{
				Init(h, context, destroyOnDelete);
			}
			virtual ~Data();
			void Attach(void* h, bool destroyOnDelete = true);
			void* Handle() const { return handle; }

			virtual bool IsValid() const { return handle != nullptr; }
			virtual const TCHAR* GetTypeName() const { return _T("Object"); }
		};

		DataPtr m;	// never null

		Object(DataPtr p) : m(p)
		{}

		Object(void* h, const Context& context, bool destroyOnDelete = true, Data* data = nullptr)
		{
			if (!data)
				data = new Data(h, context, destroyOnDelete);
			else
				data->Init(h, context, destroyOnDelete);
			m.reset(data);
		}

		void AddReference(const Object & value)
		{
			if (value)	// shouldn't be necessary but lets keep things tidy
				m->references.insert(value.m);
		}

		void RemoveReference(const Object & value)
		{
			m->references.erase(value.m);
		}

		void RemoveReference(void* h)
		{
			for (auto it = m->references.begin(); it != m->references.end(); )
			{
				if ((*it)->Handle() == h)
					it = m->references.erase(it);
				else
					++it;
			}
		}

	public:
		template <class T>
		T FindRef(void* h) const
		{
			T ret;
			if (h)
			{
				for (auto ref : m->references)
				{
					if (ref->Handle() == h)
					{
						ret.m = ref;
						break;
					}
				}
			}
			return ret;
		}

	protected:
		void RemoveAllReferences()
		{
			m->references.clear();
		}

		int ReferenceCount() const { return (int)m->references.size(); }
		int UseCount() const { return m.use_count(); }

	public:
		Object(Data* data = nullptr)
		{
			if (!data) data = new Data();
			m.reset(data);
		}
 
		void Reset()
		{
			m = std::make_shared<Data>();
		}

		bool operator==(const Object& rhs) const { return m.get() == rhs.m.get(); }
		bool operator<(const Object& rhs) const { return (size_t)m.get() < (size_t)rhs.m.get(); };

		void* Handle() const { return m->Handle(); }

		Context GetContext() const;

		bool DestroyOnDelete() const
		{
			return m->destroyOnDelete;
		}

		void SetUserData(size_t v) { m->userData = v; }
		size_t GetUserData() const { return m->userData; }

		void SetName(const char* name)
		{
			if (Handle())
			{
				auto res = rprObjectSetName(Handle(), name);
				FASSERT(RPR_SUCCESS == res);
			}
		}

		const TCHAR* GetTypeName() const { return m->GetTypeName(); }
		bool IsValid() const { return m->IsValid(); }

		// for easier scope and creation flow
		explicit operator bool() const { return m->IsValid(); }

		template <class T>
		T As()
		{
			Data* data = m.get();
			if (!data || dynamic_cast<typename T::Data*>(data))
				return reinterpret_cast<T&>(*this);
			return T();	// return a null otherwise
		}

		DataPtr GetData(void)
		{
			return m;
		}
	};

/*
	Helper macros for defining Object child classes.

	Each class will have its 'Data' class derived from Parent::Data. It is pointed by Object::m, however
	'm' points to Object::Data. To automatically cast to ClassName::Data, use generated data() function.
	Use DECLARE_OBJECT to define a class with custom data, or DECLARE_OBJECT_NO_DATA to define a class
	with fully derived data set. Insert DECLARE_OBJECT_DATA into Data definition to add necessary data
	members. 'm' is assigned in Object's constructor, so you should pass allocated Data instance to
	parent's constructor. If your class is supposed to be used as parent for another class, its constructor
	should have 'Data* d = nullptr' parameter: child class will pass its Data structore there, but when
	'this' class created itself, 'd' will be null - and in this case we should allocate data.

	Typical constructor for class with children:

	inline Node::Node(const MaterialSystem& ms, int type, bool destroyOnDelete, Node::Data* _data)
		: Object(ms.CreateNode(type)
		, ms.GetContext()
		, destroyOnDelete
		, _data ? _data : new Node::Data()) // here we're either creating Node::Data, or passing child's data class to Object's constructor
	{
		data().materialSystem = ms; // we're accessing local Data class here
	}

	It is possible to create constructor which will accept RPR object's handle and context (and 'Data* d')
	as input, however some classes has different set of parameters.
*/

#define DECLARE_OBJECT(ClassName, Parent)	\
		friend Object;						\
	protected:								\
		typedef ClassName ThisClass;		\
		typedef Parent Super;				\
		class Data;							\
		Data& data() const { return *static_cast<Data*>(m.get()); } \
	public:									\
		ClassName(Data* data = nullptr)		\
		: Parent(data ? data : new Data())	\
		{}									\
	protected:								\
		static const TCHAR* GetTypeName()	\
		{									\
			static_assert(sizeof(ThisClass) == sizeof(std::shared_ptr<frw::Object::Data>), "Error in class " #ClassName "! It should not contain any data fields"); \
			return _T(#ClassName);			\
		}

#define DECLARE_OBJECT_NO_DATA(ClassName, Parent) \
	DECLARE_OBJECT(ClassName, Parent)		\
		class Data : public Parent::Data	\
		{									\
			DECLARE_OBJECT_DATA				\
		};

#define DECLARE_OBJECT_DATA					\
	public:									\
		virtual const TCHAR* GetTypeName() const	{ return ThisClass::GetTypeName(); } \
	private:


	class Node : public Object
	{
		DECLARE_OBJECT(Node, Object);
		class Data : public Object::Data
		{
			DECLARE_OBJECT_DATA
		public:
			Object materialSystem;
			int type;
		};

	protected:
		// This constructor is used by Shader, when RPRX material created
		Node(Context& context, Data* data)
		: Object(nullptr, context, true, data)
		{}

	public:
		Node(const MaterialSystem& ms, int type, bool destroyOnDelete = true, Data* data = nullptr);	// not typesafe
		Node(rpr_material_node h, const MaterialSystem& ms, bool destroyOnDelete = true, Data* data = nullptr); // this constructor is used only once (for Shader)

		void SetValue(const char* key, const Value& v);
		void SetValue(const char* key, int);

		MaterialSystem GetMaterialSystem() const;

		/// do not use if you can avoid it (unsafe)
		void _SetInputNode(const char* key, const Shader& shader);

		int GetType() const
		{
			return data().type;
		}

		int GetParameterCount() const
		{
			size_t n = 0;
			auto res = rprMaterialNodeGetInfo(Handle(), NodeInfoInputCount, sizeof(n), &n, nullptr);
			FASSERT(res == RPR_SUCCESS);
			return n;
		}

		class ParameterInfo
		{
		public:
			std::string name;
			std::string description;
			NodeInputType type;
		};

		ParameterInfo GetParameterInfo(int i) const
		{
			ParameterInfo info = {};
			size_t size = 0;

			// Get name
			auto res = rprMaterialNodeGetInputInfo(Handle(), i, NodeInputInfoName, 0, nullptr, &size);
			FASSERT(res == RPR_SUCCESS);
			info.name.resize(size);
			res = rprMaterialNodeGetInputInfo(Handle(), i, NodeInputInfoName, size, const_cast<char*>(info.name.data()), nullptr);
			FASSERT(res == RPR_SUCCESS);

			// Get description
			res = rprMaterialNodeGetInputInfo(Handle(), i, NodeInputInfoDescription, 0, nullptr, &size);
			FASSERT(res == RPR_SUCCESS);
			info.description.resize(size);
			res = rprMaterialNodeGetInputInfo(Handle(), i, NodeInputInfoDescription, size, const_cast<char*>(info.description.data()), nullptr);
			FASSERT(res == RPR_SUCCESS);

				// get type
			res = rprMaterialNodeGetInputInfo(Handle(), i, NodeInputInfoType, sizeof(info.type), &info.type, nullptr);
			FASSERT(res == RPR_SUCCESS);

			return info;
		}
	};

	class ValueNode : public Node
	{
		DECLARE_OBJECT_NO_DATA(ValueNode, Node);
	public:
		ValueNode(const MaterialSystem& ms, ValueType type) : Node(ms, type, true, new Data()) {}
	};


	class Value
	{
		ValueNode node;
#pragma warning(push)
#pragma warning(disable:4201)

		union 
		{
			struct 
			{
				rpr_float x, y, z, w;
			};
		};
#pragma warning(pop)

		enum
		{
			FLOAT,
			NODE,
		} type;

	public:

		Value() : type(NODE)
		{ }

		// single scalar value gets copied to all components
		Value(double s) : type(FLOAT)
		{
			x = y = z = w = (float)s;
		}

		// otherwise it's just zeros 
		Value(double _x, double _y, double _z = 0, double _w = 0) : type(FLOAT)
		{
			x = rpr_float(_x);
			y = rpr_float(_y);
			z = rpr_float(_z);
			w = rpr_float(_w);
		}

#ifdef FRW_USE_MAX_TYPES
		Value(const Color& v) : type(FLOAT)
		{
			x = v.r;
			y = v.g;
			z = v.b;
			w = 0.0;
		}
		Value(const Point3& v) : type(FLOAT)
		{
			x = v.x;
			y = v.y;
			z = v.z;
			w = 0.0;
		}
#endif

		Value(ValueNode v) : node(v), type(NODE) {}

			// this basically means "has value that can be assigned as input"
		bool IsNull() const { return type == NODE && !node; }
		bool IsFloat() const { return type == FLOAT; }
		bool IsNode() const { return type == NODE && node; }

		const Node& GetNode() const { return node; }

		// returns true unless null
		explicit operator bool() const { return type != NODE || node; }

		// either a map or a float > 0
		bool NonZero() const
		{
			return type == FLOAT ? (x*x + y*y + z*z + w*w) > 0 : (type == NODE && node); 
		}

		bool NonZeroXYZ() const
		{
			return type == FLOAT ? (x*x + y*y + z*z) > 0 : (type == NODE && node);
		}

		bool operator==(const Value& rhs) const
		{
			return type == rhs.type
				&& (type == NODE
					? (node == rhs.node)
					: (x == rhs.x && y == rhs.y && z == rhs.z && w == rhs.w));
		}

			// quick best value check
		Value operator|(const Value &rhs) const
		{
			return *this ? *this : rhs;
		}

		// casting
		explicit operator ValueNode() const { return ValueNode(type == NODE ? node : ValueNode()); }

		friend class Node;
		friend class Shader;
		friend class MaterialSystem;

		MaterialSystem GetMaterialSystem() const;
	};


	class Shape : public Object
	{
		friend class Shader;

		DECLARE_OBJECT(Shape, Object);
		class Data : public Object::Data
		{
			DECLARE_OBJECT_DATA
		public:
			Object shader;
			Object volumeShader;
			Object displacementShader;
			virtual ~Data();

			bool isAreaLight = false;
		};

	public:
		Shape(rpr_shape h, const Context &context, bool destroyOnDelete = true) : Object(h, context, destroyOnDelete, new Data()) {}

		void SetShader(Shader& shader);
		Shader GetShader() const;

		void SetVolumeShader(const Shader& shader);
		Shader GetVolumeShader() const;

		void SetVisibility(bool visible)
		{
			auto res = rprShapeSetVisibility(Handle(), visible);
			FASSERT(RPR_SUCCESS == res);
		}
		void SetPrimaryVisibility(bool visible)
		{
#if RPR_API_VERSION != 0x010000110	
			auto res = rprShapeSetVisibilityPrimaryOnly(Handle(), visible);
			FASSERT(RPR_SUCCESS == res);
#endif
		}
		void SetReflectionVisibility(bool visible)
		{
#if RPR_API_VERSION != 0x010000110	// AMDMAX-617
			auto res = rprShapeSetVisibilityInSpecular(Handle(), visible);
			FASSERT(RPR_SUCCESS == res);
#endif
		}
		Shape CreateInstance(Context context) const;
		void SetTransform(const float* tm, bool transpose = false)
		{
			auto res = rprShapeSetTransform(Handle(), transpose, tm);
			FASSERT(RPR_SUCCESS == res);
		}

		void SetLinearMotion(float x, float y, float z)
		{
			auto res = rprShapeSetLinearMotion(Handle(), x, y, z);
			FASSERT(RPR_SUCCESS == res);
		}

		void SetAngularMotion(float x, float y, float z, float w)
		{
			auto res = rprShapeSetAngularMotion(Handle(), x, y, z, w);
			FASSERT(RPR_SUCCESS == res);
		}

#ifdef FRW_USE_MAX_TYPES
		void SetTransform(const Matrix3& tm)
		{
			float m44[16] = {};
			float* p = m44;
			for (int x = 0; x < 4; ++x) 
			{
				for (int y = 0; y < 3; ++y) 
					*p++ = tm.GetRow(x)[y];

				*p++ = (x == 3) ? 1.f : 0.f;
			}
			SetTransform(m44);
		}
#endif
		void SetShadowFlag(bool castsShadows)
		{
			auto res = rprShapeSetShadow(Handle(), castsShadows);
			FASSERT(RPR_SUCCESS == res);
		}

		void SetShadowCatcherFlag(bool shadowCatcher)
		{
#if RPR_API_VERSION >= 0x010000109
			auto res = rprShapeSetShadowCatcher(Handle(), shadowCatcher);
			FASSERT(RPR_SUCCESS == res);
#endif
		}

		void SetDisplacement(Value v, float minscale = 0, float maxscale = 1);
		void RemoveDisplacement();
		void SetSubdivisionFactor(int sub);
		void SetSubdivisionBoundaryInterop(rpr_subdiv_boundary_interfop_type type);
		void SetSubdivisionCreaseWeight(float weight);

		bool IsInstance() const
		{
			rpr_shape_type type = RPR_SHAPE_TYPE_INSTANCE;
			auto res = rprShapeGetInfo(Handle(), RPR_SHAPE_TYPE, sizeof(type), &type, nullptr);
			FASSERT(RPR_SUCCESS == res);
			return type == RPR_SHAPE_TYPE_INSTANCE;
		}

		int GetFaceCount() const
		{
			rpr_shape shape;
			if (IsInstance())
			{
				auto res = rprInstanceGetBaseShape(Handle(), &shape);
				FASSERT(RPR_SUCCESS == res);
			}
			else
			{
				shape = Handle();
			}

			size_t n = 0;
			auto res = rprMeshGetInfo(shape, RPR_MESH_POLYGON_COUNT, sizeof(n), &n, nullptr);
			FASSERT(RPR_SUCCESS == res);
			return n;
		}
		
		// to allow use with hash structures
		inline bool operator < (const Shape &left) const
		{
			return this->m < left.m;
		}
	};

	class Light : public Object
	{
		DECLARE_OBJECT_NO_DATA(Light, Object);
	public:
		Light(rpr_light h, const Context &context, Data* data = nullptr) : Object(h, context, true, data ? data : new Data()) {}
		void SetTransform(const float* tm, bool transpose = false)
		{
			auto res = rprLightSetTransform(Handle(), transpose, tm);
			FASSERT(RPR_SUCCESS == res);
		}
#ifdef FRW_USE_MAX_TYPES
		void SetTransform(const Matrix3& tm)
		{
			float m44[16] = {};
			float* p = m44;
			for (int x = 0; x < 4; ++x)
			{
				for (int y = 0; y < 3; ++y)
					*p++ = tm.GetRow(x)[y];

				*p++ = (x == 3) ? 1.f : 0.f;
			}
			SetTransform(m44);
		}
#endif
	};

	class Image : public Object
	{
		DECLARE_OBJECT_NO_DATA(Image, Object);
	public:
		explicit Image(rpr_image h, const Context& context) : Object(h, context, true, new Data()) {}

		Image(Context context, const rpr_image_format& format, const rpr_image_desc& image_desc, const void* data);

		bool IsGrayScale();
	};

	class PointLight : public Light
	{
		DECLARE_OBJECT_NO_DATA(PointLight, Light);
	public:
		PointLight(rpr_light h, const Context &context) : Light(h, context, new Data()) {}
		void SetRadiantPower(float r, float g, float b)
		{
			auto res = rprPointLightSetRadiantPower3f(Handle(), r, g, b);
			FASSERT(RPR_SUCCESS == res);
		}
	};

	class DirectionalLight : public Light
	{
		DECLARE_OBJECT_NO_DATA(DirectionalLight, Light);
	public:
		DirectionalLight(rpr_light h, const Context &context) : Light(h, context, new Data()) {}

		void SetRadiantPower(float r, float g, float b)
		{
			auto res = rprDirectionalLightSetRadiantPower3f(Handle(), r, g, b);
			FASSERT(RPR_SUCCESS == res);
		}
	};

	class EnvironmentLight : public Light
	{
		DECLARE_OBJECT(EnvironmentLight, Light);
		class Data : public Light::Data
		{
			DECLARE_OBJECT_DATA
		public:
			Image image;
			std::vector<Shape> portals;
		};
	public:
		EnvironmentLight(rpr_light h, const Context &context) : Light(h, context, new Data()) {}
		void SetLightIntensityScale(float k)
		{
			auto res = rprEnvironmentLightSetIntensityScale(Handle(), k);
			FASSERT(RPR_SUCCESS == res);
		}
		void SetImage(Image img);
		void AttachPortal(Shape shape);
	};

	class IESLight : public Light
	{
		DECLARE_OBJECT_NO_DATA(IESLight, Light);

	public:
		IESLight(rpr_light h, const Context& context) :
			Light(h, context, new Data())
		{}

#ifdef FRW_USE_MAX_TYPES
		void SetRadiantPower(Color color)
		{
			SetRadiantPower(color.r, color.g, color.b);
		}
#endif

		void SetIESFile(const rpr_char* path, int nx, int ny)
		{
			rpr_int res = rprIESLightSetImageFromFile(Handle(), path, nx, ny);
			FASSERT(RPR_SUCCESS == res);
		}

		void SetIESData(const char* data, int nx, int ny)
		{
			rpr_int res = rprIESLightSetImageFromIESdata(Handle(), data, nx, ny);
			FASSERT(RPR_SUCCESS == res);
		}

		void SetRadiantPower(float r, float g, float b)
		{
			rpr_int res = rprIESLightSetRadiantPower3f(Handle(), r, g, b);
			FASSERT(RPR_SUCCESS == res);
		}
	};
	
	class Camera : public Object
	{
		DECLARE_OBJECT_NO_DATA(Camera, Object);
	public:
		Camera(rpr_camera h, const Context &context) : Object(h, context, true, new Data()) {}
	};

	class Scene : public Object
	{
		DECLARE_OBJECT(Scene, Object);
		class Data : public Object::Data
		{
			DECLARE_OBJECT_DATA;
		public:
			Camera camera;
			Light background;
			Light backgroundReflOverride;
			Light backgroundRefrOverride;
			Image backgroundImage;
		};

	public:
		Scene(rpr_scene h, const Context &context) : Object(h, context, true, new Data()) {}

		void Attach(Shape v)
		{
			AddReference(v);
			auto res = rprSceneAttachShape(Handle(), v.Handle()); FASSERT(RPR_SUCCESS == res);
		}
		void Attach(Light v)
		{
			AddReference(v);
			auto res = rprSceneAttachLight(Handle(), v.Handle()); FASSERT(RPR_SUCCESS == res);
		}
		void Detach(Shape v)
		{
			RemoveReference(v);
			auto res = rprSceneDetachShape(Handle(), v.Handle()); FASSERT(RPR_SUCCESS == res);
		}
		void Detach(Light v)
		{
			RemoveReference(v);
			auto res = rprSceneDetachLight(Handle(), v.Handle()); FASSERT(RPR_SUCCESS == res);
		}

		void SetCamera(Camera v)
		{
			auto res = rprSceneSetCamera(Handle(), v.Handle()); FASSERT(RPR_SUCCESS == res);
			data().camera = v;
		}
		Camera GetCamera()
		{
			return data().camera;
		}

		void Clear()
		{
			SetCamera(Camera());
			auto res = rprSceneClear(Handle()); // this only detaches objects, doesn't delete them
			FASSERT(RPR_SUCCESS == res);
			RemoveAllReferences();
			// clear local data
			Data& d = data();
			d.camera = Camera();
			d.background = Light();
			d.backgroundReflOverride = Light();
			d.backgroundRefrOverride = Light();
			d.backgroundImage = Image();
		}

		int ShapeCount() const
		{
			size_t count = 0;
			auto res = rprSceneGetInfo(Handle(), RPR_SCENE_SHAPE_COUNT, sizeof(count), &count, nullptr); FASSERT(RPR_SUCCESS == res);
			return count;
		}

		void DetachShapes()
		{
			auto count = ShapeCount();
			std::vector<rpr_shape> items(count);
			auto res = rprSceneGetInfo(Handle(), RPR_SCENE_SHAPE_LIST, count * sizeof(rpr_shape), items.data(), nullptr); FASSERT(RPR_SUCCESS == res);
			for (auto& it : items)
			{
				res = rprSceneDetachShape(Handle(), it); FASSERT(RPR_SUCCESS == res);
				RemoveReference(it);
			}
		}

		std::list<Shape> GetShapes()
		{
			auto count = ShapeCount();
			std::vector<rpr_shape> items(count);
			auto res = rprSceneGetInfo(Handle(), RPR_SCENE_SHAPE_LIST, count * sizeof(rpr_shape), items.data(), nullptr); FASSERT(RPR_SUCCESS == res);
			std::list<Shape> ret;
			for (auto& it : items)
			{
				if (auto shape = FindRef<Shape>(it))
					ret.push_back(shape);
			}
			return ret;
		}

		int LightCount() const
		{
			size_t count = 0;
			auto res = rprSceneGetInfo(Handle(), RPR_SCENE_LIGHT_COUNT, sizeof(count), &count, nullptr); FASSERT(RPR_SUCCESS == res);
			return count;
		}

		std::list<Light> GetLights()
		{
			auto count = LightCount();
			std::vector<rpr_light> items(count);
			auto res = rprSceneGetInfo(Handle(), RPR_SCENE_LIGHT_LIST, count * sizeof(rpr_shape), items.data(), nullptr); FASSERT(RPR_SUCCESS == res);
			std::list<Light> ret;
			for (auto& it : items)
			{
				if (auto light = FindRef<Light>(it))
					ret.push_back(light);
			}
			return ret;
		}

		void DetachLights()
		{
			auto count = LightCount();
			std::vector<rpr_light> items(count);
			auto res = rprSceneGetInfo(Handle(), RPR_SCENE_LIGHT_LIST, count * sizeof(rpr_shape), items.data(), nullptr); FASSERT(RPR_SUCCESS == res);
			for (auto& it : items)
			{
				res = rprSceneDetachLight(Handle(), it); FASSERT(RPR_SUCCESS == res);
				RemoveReference(it);
			}
		}

		void SetBackgroundImage(Image v)
		{
			auto res = rprSceneSetBackgroundImage(Handle(), v.Handle()); FASSERT(RPR_SUCCESS==res);
			data().backgroundImage = v;
		}

		void SetBackground(EnvironmentLight v)
		{
			auto res = rprSceneSetEnvironmentOverride(Handle(), RPR_SCENE_ENVIRONMENT_OVERRIDE_BACKGROUND, v.Handle()); FASSERT(RPR_SUCCESS==res);
			data().background = v;
		}

		void SetBackgroundReflectionsOverride(EnvironmentLight v)
		{
			auto res = rprSceneSetEnvironmentOverride(Handle(), RPR_SCENE_ENVIRONMENT_OVERRIDE_REFLECTION, v.Handle()); FASSERT(RPR_SUCCESS == res);
			data().backgroundReflOverride = v;
		}

		void SetBackgroundRefractionsOverride(EnvironmentLight v)
		{
			auto res = rprSceneSetEnvironmentOverride(Handle(), RPR_SCENE_ENVIRONMENT_OVERRIDE_REFRACTION, v.Handle()); FASSERT(RPR_SUCCESS == res);
			data().backgroundRefrOverride = v;
		}

	};

	class Context : public Object
	{
		DECLARE_OBJECT(Context, Object);
		class Data : public Object::Data
		{
			DECLARE_OBJECT_DATA;
		public:
			Data(): scene(nullptr){}

			rpr_scene scene;
		};

	public:
		explicit Context(DataPtr p)
		{
			m = p;
		}

		explicit Context(rpr_context h, bool destroyOnDelete = true) : Object(h, *this, destroyOnDelete, new Data())
		{
		}

		void SetParameter(const char * sz, rpr_uint v)
		{
			auto res = rprContextSetParameter1u(Handle(), sz, v);
			FASSERT(RPR_SUCCESS == res);
		}
		void SetParameter(const char * sz, int v)
		{
			auto res = rprContextSetParameter1u(Handle(), sz, v);
			FASSERT(RPR_SUCCESS == res);
		}
		void SetParameter(const char * sz, double v)
		{
			auto res = rprContextSetParameter1f(Handle(), sz, (rpr_float)v);
			FASSERT(RPR_SUCCESS == res);
		}
		void SetParameter(const char * sz, double x, double y, double z)
		{
			auto res = rprContextSetParameter3f(Handle(), sz, (rpr_float)x, (rpr_float)y, (rpr_float)z);
			FASSERT(RPR_SUCCESS == res);
		}
		void SetParameter(const char * sz, double x, double y, double z, double w)
		{
			auto res = rprContextSetParameter4f(Handle(), sz, (rpr_float)x, (rpr_float)y, (rpr_float)z, (rpr_float)w);
			FASSERT(RPR_SUCCESS == res);
		}

		Scene CreateScene()
		{
			DebugPrint(L"CreateScene()\n");
			rpr_scene v;
			auto status = rprContextCreateScene(Handle(), &v);
			FASSERT(status == RPR_SUCCESS);

			data().scene = v;
			return Scene(v, *this);
		}

		Camera CreateCamera()
		{
			DebugPrint(L"CreateCamera()\n");
			rpr_camera v;
			auto status = rprContextCreateCamera(Handle(), &v);
			FASSERT(status == RPR_SUCCESS);
			return Camera(v, *this);
		}

		Shape CreateMesh(const rpr_float* vertices, size_t num_vertices, rpr_int vertex_stride,
			const rpr_float* normals, size_t num_normals, rpr_int normal_stride,
			const rpr_float* texcoords, size_t num_texcoords, rpr_int texcoord_stride,
			const rpr_int*   vertex_indices, rpr_int vidx_stride,
			const rpr_int*   normal_indices, rpr_int nidx_stride,
			const rpr_int*   texcoord_indices, rpr_int tidx_stride,
			const rpr_int*   num_face_vertices, size_t num_faces);

		Shape CreateMeshEx(const rpr_float* vertices, size_t num_vertices, rpr_int vertex_stride, 
			const rpr_float* normals, size_t num_normals, rpr_int normal_stride,
			const rpr_int* perVertexFlag, size_t num_perVertexFlags, rpr_int perVertexFlag_stride,
			rpr_int numberOfTexCoordLayers, const rpr_float** texcoords, const size_t* num_texcoords, const rpr_int* texcoord_stride, 
			const rpr_int* vertex_indices, rpr_int vidx_stride,
			const rpr_int* normal_indices, rpr_int nidx_stride, const rpr_int** texcoord_indices,
			const rpr_int* tidx_stride, const rpr_int * num_face_vertices, size_t num_faces);

		PointLight CreatePointLight()
		{
			DebugPrint(L"CreatePointLight()\n");
			rpr_light h;
			auto res = rprContextCreatePointLight(Handle(), &h);
			FASSERT(RPR_SUCCESS == res);
			return PointLight(h,*this);
		}

		EnvironmentLight CreateEnvironmentLight()
		{
			DebugPrint(L"CreateEnvironmentLight()\n");
			rpr_light h;
			auto res = rprContextCreateEnvironmentLight(Handle(), &h);
			FASSERT(RPR_SUCCESS == res);
			return EnvironmentLight(h,*this);
		}

		DirectionalLight CreateDirectionalLight()
		{
			DebugPrint(L"CreateDirectionalLight()\n");
			rpr_light h;
			auto res = rprContextCreateDirectionalLight(Handle(), &h);
			FASSERT(RPR_SUCCESS == res);
			return DirectionalLight(h, *this);
		}

		IESLight CreateIESLight()
		{
			rpr_light light = 0;
			rpr_int res = rprContextCreateIESLight(Handle(), &light);
			FASSERT(res == RPR_SUCCESS);

			return IESLight(light, *this);
		}

		// context state
		void SetScene(Scene scene)
		{
			FASSERT(!scene || scene.GetContext() == *this);

			auto res = rprContextSetScene(Handle(), scene.Handle());
			FASSERT(RPR_SUCCESS == res);
		}

        void Attach(PostEffect post_effect);

        void Detach(PostEffect post_effect);

		/// set the target buffer for render calls
		rpr_int SetAOV(FrameBuffer frameBuffer, rpr_aov aov = RPR_AOV_COLOR);

		bool Render()
		{
			auto res = rprContextRender(Handle());
			return (RPR_SUCCESS == res);
		}

		bool RenderTile(int rxmin, int rxmax, int rymin, int rymax)
		{
			auto res = rprContextRenderTile(Handle(), rxmin, rxmax, rymin, rymax);
			return (RPR_SUCCESS == res);
		}

		int GetMemoryUsage() const
		{
			rpr_render_statistics statistics = {};
			auto res = rprContextGetInfo(Handle(), RPR_CONTEXT_RENDER_STATISTICS, sizeof(statistics), &statistics, nullptr);
			FASSERT(res == RPR_SUCCESS);
			return statistics.gpumem_usage;
		}
		int GetMaterialStackSize() const
		{
			size_t info = 0;
			auto res = rprContextGetInfo(Handle(), RPR_CONTEXT_MATERIAL_STACK_SIZE, sizeof(info), &info, nullptr);
			FASSERT(res == RPR_SUCCESS);
			return info;
		}

		int GetParameterCount() const
		{
			size_t info = 0;
			auto res = rprContextGetInfo(Handle(), RPR_CONTEXT_PARAMETER_COUNT, sizeof(info), &info, nullptr);
			FASSERT(res == RPR_SUCCESS);
			return info;
		}

		rpr_scene GetScene() const
		{
			return data().scene;
		}

		class ParameterInfo
		{
		public:
			std::string name;
			std::string description;
			ContextParameterType type;
		};

		ParameterInfo GetParameterInfo(int i) const
		{
			ParameterInfo info = {};
			size_t size = 0;

			// get name
			auto res = rprContextGetParameterInfo(Handle(), i, ParameterInfoName, 0, nullptr, &size);
			FASSERT(res == RPR_SUCCESS);
			info.name.resize(size);
			res = rprContextGetParameterInfo(Handle(), i, ParameterInfoName, size, const_cast<char*>(info.name.data()), nullptr);
			FASSERT(res == RPR_SUCCESS);

			// get description
			res = rprContextGetParameterInfo(Handle(), i, ParameterInfoDescription, 0, nullptr, &size);
			FASSERT(res == RPR_SUCCESS);
			info.description.resize(size);
			res = rprContextGetParameterInfo(Handle(), i, ParameterInfoDescription, size, const_cast<char*>(info.description.data()), nullptr);
			FASSERT(res == RPR_SUCCESS);

			// get type
			res = rprContextGetParameterInfo(Handle(), i, ParameterInfoType, sizeof(info.type), &info.type, nullptr);
			FASSERT(res == RPR_SUCCESS);

			return info;
		}
	};



	class ImageNode : public ValueNode
	{
	public:
		explicit ImageNode(const MaterialSystem& h) : ValueNode(h, ValueTypeImageMap) {}
		rpr_int SetMap(Image v)
		{
			AddReference(v);
			return rprMaterialNodeSetInputImageData(Handle(), "data", v.Handle());
		}
	};

	class LookupNode : public ValueNode
	{
	public:
		LookupNode(const MaterialSystem& h, LookupType v) : ValueNode(h, ValueTypeLookup)
		{
			SetValue("value", static_cast<int>(v));
		}
	};

	class ArithmeticNode : public ValueNode
	{
	public:
		ArithmeticNode(const MaterialSystem& h, Operator op) : ValueNode(h, ValueTypeArithmetic)
		{
			SetValue("op", static_cast<int>(op));
		}
		ArithmeticNode(const MaterialSystem& h, Operator op, const Value& a) : ValueNode(h, ValueTypeArithmetic)
		{
			SetValue("op", static_cast<int>(op));
			SetValue("color0", a);
		}
		ArithmeticNode(const MaterialSystem& h, Operator op, const Value& a, const Value& b) : ValueNode(h, ValueTypeArithmetic)
		{
			SetValue("op", static_cast<int>(op));
			SetValue("color0", a);
			SetValue("color1", b);
		}
		ArithmeticNode(const MaterialSystem& h, Operator op, const Value& a, const Value& b, const Value& c) : ValueNode(h, ValueTypeArithmetic)
		{
			SetValue("op", static_cast<int>(op));
			SetValue("color0", a);
			SetValue("color1", b);
			SetValue("color2", c);
		}

		void SetArgument(int i, const Value& v)
		{
			switch (i)
			{
			case 0: SetValue("color0", v); break;
			case 1: SetValue("color1", v); break;
			case 2: SetValue("color2", v); break;
			}
		}
	};

	class NormalMapNode : public ValueNode
	{
	public:
		explicit NormalMapNode(MaterialSystem& h) : ValueNode(h, ValueTypeNormalMap) {}

		void SetMap(Value v)
		{
			if (v.IsNode())
			{
				Node n = v.GetNode();
				AddReference(n);
				auto res = rprMaterialNodeSetInputN(Handle(), "color", n.Handle());
				FASSERT(RPR_SUCCESS == res);
			}
		}
	};

	
	class BumpMapNode : public ValueNode
	{
	public:
		explicit BumpMapNode(MaterialSystem& h) : ValueNode(h, ValueTypeBumpMap) {}

		void SetMap(Value v)
		{
			if (v.IsNode())
			{
				Node n = v.GetNode();
				AddReference(n);
				auto res = rprMaterialNodeSetInputN(Handle(), "color", n.Handle());
				FASSERT(RPR_SUCCESS == res);
			}
		}
	};

	class MaterialSystem : public Object
	{
		static const bool allowShortcuts = true;
		DECLARE_OBJECT_NO_DATA(MaterialSystem, Object);

	protected:
		friend class Node;
		rpr_material_node CreateNode(rpr_material_node_type type) const
		{
			DebugPrint(L"CreateNode(%d)\n", type);
			rpr_material_node node = nullptr;
			auto res = rprMaterialSystemCreateNode(Handle(), type, &node);
			FASSERT(res == RPR_SUCCESS);
			return node;
		}

	public:
		explicit MaterialSystem(DataPtr p)
		{
			m = p;
		}

		explicit MaterialSystem(const Context& c, rpr_material_system h = nullptr, bool destroyOnDelete = true) : Object(h, c, destroyOnDelete, new Data())
		{
			if (!h)
			{
				DebugPrint(L"CreateMaterialSystem()\n");
				rpr_material_system h = nullptr;
				auto res = rprContextCreateMaterialSystem(c.Handle(), 0, &h);
				FASSERT(res == RPR_SUCCESS);
				m->Attach(h);
			}
		}

		Value ValueBlend(const Value& a, const Value& b, const Value& t) const
		{
				// shortcuts
			if (t.IsFloat())
			{
				if (t.x <= 0)
					return a;
				if (t.x >= 1)
					return b;
			}

			if (allowShortcuts && a.IsFloat() && b.IsFloat() && t.IsFloat())
			{
				auto k = std::min(std::max(t.x, 0.0f), 1.0f);

				return Value(
					a.x * (1 - k) + b.x * k,
					a.y * (1 - k) + b.y * k,
					a.z * (1 - k) + b.z * k,
					a.w * (1 - k) + b.w * k
					);
			}

			ValueNode node(*this, ValueTypeBlend);
			node.SetValue("color0", a);
			node.SetValue("color1", b);
			node.SetValue("weight", t);
			return node;
		}

		// unclamped, multichannel version of ValueBlend
		Value ValueMix(const Value& a, const Value& b, const Value& t) const
		{
			return ValueAdd(
				ValueMul(a, ValueSub(1, t)),
				ValueMul(b, t)				
				);
		}

		Value ValueAdd(const Value& a, const Value& b) const
		{
			if (a.IsNull())
				return b;

			if (b.IsNull())
				return a;

			if (!a.NonZero())
				return b;

			if (!b.NonZero())
				return a;

			if (allowShortcuts && a.IsFloat() && b.IsFloat())
				return Value(a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w);

			return ArithmeticNode(*this, OperatorAdd, a, b);
		}
		Value ValueAdd(const Value& a, const Value& b, const Value& c) const
		{
			return ValueAdd(ValueAdd(a, b), c);
		}
		Value ValueAdd(const Value& a, const Value& b, const Value& c, const Value& d) const
		{
			return ValueAdd(ValueAdd(a, b), ValueAdd(c,d));
		}

		Value ValueSub(Value a, const Value& b) const
		{
			if (a.IsNull())
				a = 0;

			if (!b.NonZero())
				return a;

			if (allowShortcuts && a.IsFloat() && b.IsFloat())
				return Value(a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w);

			return ArithmeticNode(*this, OperatorSubtract, a, b); 
		}

		Value ValueMul(const Value& a, const Value& b) const
		{
			if (!a || !b)
				return Value();

			if (!a.NonZero() || !b.NonZero())
				return 0;

			if (b == 1)
				return a;

			if (a == 1)
				return b;

			if (allowShortcuts && a.IsFloat() && b.IsFloat())
				return Value(a.x * b.x, a.y * b.y, a.z * b.z, a.w * b.w);

			return ArithmeticNode(*this, OperatorMultiply, a, b); 
		}

		static float safeDiv(float a, float b) 
		{ return b == 0 ? 0 : a / b; }

		Value ValueDiv(const Value& a, const Value& b) const
		{
			if (!a || !b)
				return Value();

			if (!a.NonZero())
				return 0;

			if (b == 1)
				return a;

			if (allowShortcuts && a.IsFloat() && b.IsFloat())
				return Value(
					safeDiv(a.x, b.x),
					safeDiv(a.y, b.y),
					safeDiv(a.z, b.z),
					safeDiv(a.w, b.w)
				);

			return ArithmeticNode(*this, OperatorDivide, a, b);
		}

		static float safeMod(float a, float b)
		{
			return b == 0 ? 0 : fmod(a, b);
		}
		
		Value ValueMod(const Value& a, const Value& b) const
		{
			if (!a || !b)
				return Value();

			if (!a.NonZero() || !b.NonZero())
				return 0;

			if (allowShortcuts && a.IsFloat() && b.IsFloat())
				return Value(
					safeMod(a.x, b.x),
					safeMod(a.y, b.y),
					safeMod(a.z, b.z),
					safeMod(a.w, b.w)
				);

			return ArithmeticNode(*this, OperatorMod, a, b);
		}

		Value ValueFloor(const Value& a) const
		{
			if (!a)
				return a;

			if (allowShortcuts && a.IsFloat())
				return Value(
					floor(a.x),
					floor(a.y),
					floor(a.z),
					floor(a.w)
				);

			return ArithmeticNode(*this, OperatorFloor, a);
		}

		Value ValueComponentAverage(const Value& a) const
		{
			if (!a)
				return a;

			if (allowShortcuts && a.IsFloat())
				return Value((a.x + a.y + a.z) / 3);

			return ArithmeticNode(*this, OperatorComponentAverage, a);
		}

		Value ValueAverage(const Value& a, const Value& b) const
		{
			if (!a || !b)
				return Value();

			if (allowShortcuts && a.IsFloat() && b.IsFloat())
				return Value(
					(a.x + b.x) * 0.5,
					(a.y + b.y) * 0.5,
					(a.z + b.z) * 0.5,
					(a.w + b.w) * 0.5
				);

			return ArithmeticNode(*this, OperatorAverage, a, b);
		}

		Value ValueNegate(const Value& a) const
		{
			return ValueSub(0, a);
		}

		Value ValueDot(Value a, Value b) const
		{
			if (!a.NonZeroXYZ() || !b.NonZeroXYZ())
				return 0;

			if (allowShortcuts && a.IsFloat() && b.IsFloat())
				return Value(a.x * b.x + a.y * b.y + a.z * b.z);

			return ArithmeticNode(*this, OperatorDot, a, b); 
		}

		Value ValueCombine(const Value& a, const Value& b) const
		{
			if (allowShortcuts && a.IsFloat() && b.IsFloat())
				return Value(a.x, b.x);

			return ArithmeticNode(*this, OperatorCombine, a, b);
		}
		Value ValueCombine(const Value& a, const Value& b, const Value& c) const
		{
			if (allowShortcuts && a.IsFloat() && b.IsFloat() && c.IsFloat())
				return Value(a.x, b.x, c.x);

			// return ArithmeticNode(*this, OperatorCombine, a, b, c);
			// since RPR 212 3-args combine was broken
			return ValueAdd(
				ValueMul(a, frw::Value(1, 0, 0)), 
				ValueMul(b, frw::Value(0, 1, 0)), 
				ValueMul(c, frw::Value(0, 0, 1)) 
				);
		}

		Value ValuePow(const Value& a, const Value& b) const
		{
			if (a == 0 || !a)
				return a;

			if (b == 1)
				return a;

			if (b == 0)
				return 1;

			if (allowShortcuts && a.IsFloat() && b.IsFloat())
				return Value(pow(a.x,b.x), pow(a.y,b.y), pow(a.z,b.z), pow(a.w,b.w));

			return ArithmeticNode(*this, OperatorPow, a, b); 
		}


		// unary

		Value ValueMagnitude(const Value& a) const
		{
			if (!a)
				return 0;

			if (allowShortcuts && a.IsFloat())
				return sqrt(a.x*a.x + a.y*a.y + a.z*a.z);

			return ArithmeticNode(*this, OperatorLength, a);
		}

		Value ValueAbs(const Value& a) const
		{
			if (!a)
				return 0;

			if (allowShortcuts && a.IsFloat())
				return Value(abs(a.x), abs(a.y), abs(a.z), abs(a.w));

			return ArithmeticNode(*this, OperatorAbs, a);
		}

		Value ValueNormalize(const Value& a) const
		{
			if (!a)
				return 0;

			if (allowShortcuts && a.IsFloat())
			{
				auto m = sqrt(a.x*a.x + a.y*a.y + a.z*a.z);
				if (m > 0)
					m = 1.0 / m;
				return Value(a.x * m, a.y * m, a.z * m, a.w * m);
			}

			return ArithmeticNode(*this, OperatorNormalize, a);
		}

		Value ValueSin(const Value& a) const
		{
			if (allowShortcuts && a.IsFloat())
				return Value(sin(a.x), sin(a.y), sin(a.z), sin(a.w));

			return ArithmeticNode(*this, OperatorSin, a); 
		}

		Value ValueCos(const Value& a) const
		{
			if (allowShortcuts && a.IsFloat())
				return Value(cos(a.x), cos(a.y), cos(a.z), cos(a.w));

			return ArithmeticNode(*this, OperatorCos, a);
		}

		Value ValueTan(const Value& a) const
		{
			if (allowShortcuts && a.IsFloat())
				return Value(tan(a.x), tan(a.y), tan(a.z), tan(a.w));

			return ArithmeticNode(*this, OperatorTan, a); 
		}

		Value ValueArcSin(const Value& a) const
		{
			if (allowShortcuts && a.IsFloat())
				return Value(asin(a.x), asin(a.y), asin(a.z), asin(a.w));

			return ArithmeticNode(*this, OperatorArcSin, a); 
		}


		Value ValueArcCos(const Value& a) const
		{
			if (allowShortcuts && a.IsFloat())
				return Value(acos(a.x), acos(a.y), acos(a.z), acos(a.w));

			return ArithmeticNode(*this, OperatorArcCos, a);
		}


		Value ValueArcTan(const Value& a, const Value& b) const
		{
			if (allowShortcuts && a.IsFloat() && b.IsFloat())
				return Value(atan2(a.x, b.x), atan2(a.y, b.y), atan2(a.z, b.z), atan2(a.w, b.w));
			return ArithmeticNode(*this, OperatorArcTan, a, b);
		}

		Value ValueSelectX(const Value& a) const
		{
			if (allowShortcuts && a.IsFloat())
				return Value(a.x);

			return ArithmeticNode(*this, OperatorSelectX, a); 
		}
		Value ValueSelectY(const Value& a) const
		{
			if (allowShortcuts && a.IsFloat())
				return Value(a.y);

			return ArithmeticNode(*this, OperatorSelectY, a);
		}
		Value ValueSelectZ(const Value& a) const
		{
			if (allowShortcuts && a.IsFloat())
				return Value(a.z);

			return ArithmeticNode(*this, OperatorSelectZ, a);
		}

			// special lookup values 
		Value ValueLookupN() const
		{
			return LookupNode(*this, LookupTypeNormal);
		}

		Value ValueLookupUV(int idx) const
		{
			switch (idx)
			{
			case 0:
				return LookupNode(*this, LookupTypeUV0);
			
			case 1:
				return LookupNode(*this, LookupTypeUV1);
			}

			FASSERT(0);
			return LookupNode(*this, LookupTypeUV0);
		}

		Value ValueLookupP() const
		{
			return LookupNode(*this, LookupTypePosition);
		}
		Value ValueLookupINVEC() const
		{
			return LookupNode(*this, LookupTypeIncident);
		}
		Value ValueLookupOUTVEC() const
		{
			return LookupNode(*this, LookupTypeOutVector);
		}

		Value ValueFresnel(const Value& ior, Value n = Value(), Value v = Value()) const
		{
			ValueNode node(*this, ValueTypeFresnel);
				
			node.SetValue("ior", ior);
			node.SetValue("invec", v);
			node.SetValue("normal", n);
			return node;
		}

		Value ValueFresnelSchlick(const Value& reflectance, Value n = Value(), Value v = Value()) const
		{
			ValueNode node(*this, ValueTypeFresnelSchlick);

			node.SetValue("reflectance", reflectance);
			node.SetValue("invec", v);
			node.SetValue("normal", n);
			return node;
		}

			// this performs a 3x3 matrix transform on a 3 component vector (EXPENSIVE!?!)
		Value ValueTransform(const Value& v, const Value& mx, const Value& my, const Value& mz)
		{
			auto x = mx.NonZero() ? ValueMul(ValueSelectX(v), mx) : 0;
			auto y = my.NonZero() ? ValueMul(ValueSelectY(v), my) : 0;
			auto z = mz.NonZero() ? ValueMul(ValueSelectZ(v), mz) : 0;

			return ValueAdd(x, y, z);
		}

		Value ValueTransform(const Value& v, const Value& mx, const Value& my, const Value& mz, const Value& w)
		{
			auto x = mx.NonZero() ? ValueMul(ValueSelectX(v), mx) : 0;
			auto y = my.NonZero() ? ValueMul(ValueSelectY(v), my) : 0;
			auto z = mz.NonZero() ? ValueMul(ValueSelectZ(v), mz) : 0;

			return ValueAdd(x, y, z, w);
		}

		Value ValueCross(const Value& a, const Value& b) const
		{
			if (!a.NonZero() || !b.NonZero())
				return 0;

			return ArithmeticNode(*this, OperatorCross, a, b);
		}



#ifdef FRW_USE_MAX_TYPES
		Value ValueTransform(Value v, const Matrix3& tm)
		{
			if (tm.IsIdentity())
				return v;

				// shortcut scale
			if (tm[0][0] == tm[1][1] && tm[0][0] == tm[2][2] && tm[0][0] != 0)
				return ValueAdd(ValueMul(v, tm[0][0]), tm.GetRow(3));
			else
				return ValueTransform(v, tm.GetRow(0), tm.GetRow(1), tm.GetRow(2), tm.GetRow(3));
		}

#endif
		Value ValueMin(const Value& a, const Value& b)
		{
			if (allowShortcuts && a.IsFloat() && b.IsFloat())
				return Value(std::min(a.x, b.x), std::min(a.y, b.y), std::min(a.z, b.z), std::min(a.w, b.w));

			return ArithmeticNode(*this, OperatorMin, a, b);
		}

		Value ValueMax(const Value& a, const Value& b)
		{
			if (allowShortcuts && a.IsFloat() && b.IsFloat())
				return Value(std::max(a.x, b.x), std::max(a.y, b.y), std::max(a.z, b.z), std::max(a.w, b.w));

			return ArithmeticNode(*this, OperatorMax, a, b);
		}

			// clamp components between 0 and 1
		Value ValueClamp(const Value& v)
		{
			return ValueMin(ValueMax(v, 0), 1);
		}

		Shader ShaderBlend(const Shader& a, const Shader& b, const Value& t);
		Shader ShaderAdd(const Shader& a, const Shader& b);
	};

	class PostEffect : public Object
	{
		DECLARE_OBJECT(PostEffect, Object)

		class Data : public Object::Data
		{
			DECLARE_OBJECT_DATA

			bool m_attached;

		public:
			Data() :
				m_attached(false)
			{}

			// Detach post effect before delete it
			~Data() override
			{
				if (!m_attached)
				{
					return;
				}

				DebugPrint(L"PostEfect::Data::~Data()\n");

				void* _context = context ? context->Handle() : nullptr;
				void* _posteffect = Handle();

				if (_context == nullptr || _posteffect == nullptr)
				{
					return;
				}

				auto res = rprContextDetachPostEffect(_context, _posteffect);
				FASSERT(RPR_SUCCESS == res);
			}

			void SetAttached(bool value)
			{
				m_attached = value;
			}
		};

	public:
		PostEffect(rpr_post_effect h, const Context& context, bool destroyOndelete = true) : Object(h, context, destroyOndelete, new Data()) {}

		PostEffect(const Context& context, PostEffectType type, bool destroyOndelete = true) : Object(nullptr, context, destroyOndelete, new Data())
		{
			DebugPrint(L"PostEfect()\n");
			rpr_post_effect h = nullptr;
			auto res = rprContextCreatePostEffect(context.Handle(), type, &h);
			FASSERT(RPR_SUCCESS == res);
			m->Attach(h);
		}

		void SetParameter(const char * name, int value)
		{
			auto res = rprPostEffectSetParameter1u(Handle(), name, value);
			FASSERT(res == RPR_SUCCESS);
		}

		void SetParameter(const char * name, float value)
		{
			auto res = rprPostEffectSetParameter1f(Handle(), name, value);
			FASSERT(res == RPR_SUCCESS);
		}

		void SetAttached(bool value)
		{
			FASSERT(m != nullptr);
			data().SetAttached(value);
		}
	};

	class FrameBuffer : public Object
	{
		DECLARE_OBJECT_NO_DATA(FrameBuffer, Object);
	public:
		FrameBuffer(rpr_framebuffer h, const Context& context) : Object(h, context, true, new Data()) {}

		FrameBuffer(const Context& context, int width, int height, rpr_framebuffer_format format = { 4, RPR_COMPONENT_TYPE_FLOAT32 })
			: Object(nullptr, context, true, new Data())
		{
			DebugPrint(L"CreateFrameBuffer()\n");
			rpr_framebuffer h = nullptr;
			rpr_framebuffer_desc desc = { rpr_uint(width), rpr_uint(height) };
			auto res = rprContextCreateFrameBuffer(context.Handle(), format, &desc, &h);
			FASSERT(RPR_SUCCESS == res);
			m->Attach(h);
		}

		void Resolve(FrameBuffer dest)
		{
			auto res = rprContextResolveFrameBuffer(GetContext().Handle(), Handle(), dest.Handle());
			FASSERT(res == RPR_SUCCESS);
		}

		void Clear()
		{
			auto res = rprFrameBufferClear(Handle());
			FASSERT(RPR_SUCCESS == res);
		}

		bool SaveToFile(const std::string& path)
		{
			rpr_int res = rprFrameBufferSaveToFile(Handle(), path.c_str());
			return res == RPR_SUCCESS;
		}

		std::vector<float> GetPixelData()
		{
			std::vector<float> result;
			
			size_t fbSize;
			int res = rprFrameBufferGetInfo(Handle(), RPR_FRAMEBUFFER_DATA, 0, NULL, &fbSize);
			FASSERT(res == RPR_SUCCESS);
			
			// Get all image data from RPR in single call, then use multiple threads to copy them over to 3ds Max
			result.resize(int(fbSize / sizeof(float)));
			res = rprFrameBufferGetInfo(Handle(), RPR_FRAMEBUFFER_DATA, result.size() * sizeof(float), result.data(), NULL);
			FASSERT(res == RPR_SUCCESS);
			
			// Just to be sure, move it
			return std::move(result);
		}
	};

	// this cannot be used for standard inputs!
	class Shader : public Node
	{
		DECLARE_OBJECT(Shader, Node);
		class Data : public Node::Data
		{
			DECLARE_OBJECT_DATA

			struct ShadowCatcherParams
			{
				float mShadowR = 0.0f;
				float mShadowG = 0.0f;
				float mShadowB = 0.0f;
				float mShadowA = 0.0f;
				float mShadowWeight = 1.0f;
				bool mBgIsEnv = false;
			};
		public:
			virtual ~Data()
			{
				if (material)
				{
					auto res = rprxMaterialDelete(context, material);
					FASSERT(RPR_SUCCESS == res);
					DebugPrint(L"\tDeleted RPRX material %08X\n", material);
				}
			}

			virtual bool IsValid() const
			{
				if (Handle() != nullptr) return true;
				if (material != nullptr) return true;
				return false;
			}

			bool bDirty = false;
			int numAttachedShapes = 0;
			ShaderType shaderType = ShaderTypeInvalid;
			rprx_context context = nullptr;
			rprx_material material = nullptr; // RPRX material
			bool isShadowCatcher = false;
			ShadowCatcherParams mShadowCatcherParams;
		};

	public:
		void SetShadowCatcher(bool isShadowCatcher) { data().isShadowCatcher = isShadowCatcher; }
		bool IsShadowCatcher() const { return data().isShadowCatcher; }
		void SetShadowColor(float r, float g, float b, float a)
		{
			data().mShadowCatcherParams.mShadowR = r;
			data().mShadowCatcherParams.mShadowG = g;
			data().mShadowCatcherParams.mShadowB = b;
			data().mShadowCatcherParams.mShadowA = a;
		}
		void GetShadowColor(float *r, float *g, float *b, float *a) const
		{
			*r = data().mShadowCatcherParams.mShadowR;
			*g = data().mShadowCatcherParams.mShadowG;
			*b = data().mShadowCatcherParams.mShadowB;
			*a = data().mShadowCatcherParams.mShadowA;
		}

		void SetShadowWeight(float w) { data().mShadowCatcherParams.mShadowWeight = w; }
		float GetShadowWeight() const { return data().mShadowCatcherParams.mShadowWeight; }

		void SetBackgroundIsEnvironment(bool bgIsEnv) { data().mShadowCatcherParams.mBgIsEnv = bgIsEnv; }
		bool BgIsEnv() const { return data().mShadowCatcherParams.mBgIsEnv; }

		Shader(DataPtr p)
		{ m = p; }
		explicit Shader(const MaterialSystem& ms, ShaderType type, bool destroyOnDelete = true)
		: Node(ms, type, destroyOnDelete, new Data())
		{
			data().shaderType = type;
		}

		Shader(rpr_material_node h, const MaterialSystem& ms, bool destroyOnDelete = true) // this constructor is used only once
		: Node(h, ms, destroyOnDelete, new Data())
		{}

		explicit Shader(Context& context, rprx_context xContext, rprx_material_type type)
		: Node(context, new Data())
		{
			Data& d = data();
			d.context = xContext;
			auto status = rprxCreateMaterial(xContext, type, &d.material);
			FASSERT(RPR_SUCCESS == status);
			d.shaderType = ShaderTypeRprx;
			DebugPrint(L"\tCreated RPRX material %08X\n", d.material);
		}

		ShaderType GetShaderType() const
		{
			if (!IsValid())
				return ShaderTypeInvalid;
			return data().shaderType;
		}

		bool IsRprxMaterial() const
		{
			return data().material != nullptr;
		}

		void SetDirty()
		{
			data().bDirty = true;
		}

		bool IsDirty() const
		{
			return data().bDirty;
 		}

		void AttachToShape(Shape::Data& shape)
		{
			Data& d = data();
			d.numAttachedShapes++;
			rpr_int res;
			if (d.material)
			{
				DebugPrint(L"\tShape.AttachMaterial: shape=%08X x_material=%08X\n", shape.Handle(), d.material);
				res = rprxShapeAttachMaterial(d.context, shape.Handle(), d.material);
				FASSERT(RPR_SUCCESS == res);
				res = rprxMaterialCommit(d.context, d.material);
				FASSERT(RPR_SUCCESS == res);

				if (d.isShadowCatcher)
				{
					res = rprShapeSetShadowCatcher(shape.Handle(), true);
					FASSERT(RPR_SUCCESS == res);
				}
			}
			else
			{
				DebugPrint(L"\tShape.AttachMaterial: shape=%08X material=%08X\n", shape.Handle(), d.Handle());
				res = rprShapeSetMaterial(shape.Handle(), d.Handle());
				FASSERT(RPR_SUCCESS == res);
			}
		}
		void DetachFromShape(Shape::Data& shape)
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
		void AttachToMaterialInput(rpr_material_node node, const char* inputName) const
		{
			const Data& d = data();
			rpr_int res;
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
		void xSetParameterN(rprx_parameter parameter, frw::Node node)
		{
			AddReference(node);

			const Data& d = data();
			auto res = rprxMaterialSetParameterN(d.context, d.material, parameter, node.Handle());
			FASSERT(RPR_SUCCESS == res);
		}
		void xSetParameterU(rprx_parameter parameter, rpr_uint value)
		{
			const Data& d = data();
			auto res = rprxMaterialSetParameterU(d.context, d.material, parameter, value);
			FASSERT(RPR_SUCCESS == res);
		}
		void xSetParameterF(rprx_parameter parameter, rpr_float x, rpr_float y, rpr_float z, rpr_float w)
		{
			const Data& d = data();
			auto res = rprxMaterialSetParameterF(d.context, d.material, parameter, x, y, z, w);
			FASSERT(RPR_SUCCESS == res);
		}

		bool xSetValue(rprx_parameter parameter, const Value& v)
		{
			switch (v.type)
			{
				case Value::FLOAT:
					xSetParameterF(parameter, v.x, v.y, v.z, v.w);
					return true;

				case Value::NODE:
				{
					if (!v.node)	// in theory we should now allow this, as setting a NULL input is legal (as of FRSDK 1.87)
						return false;
					
					xSetParameterN(parameter, v.node);	// should be ok to set null here now
					return true;
				}
			}
			assert(!"bad type");
			return false;
		}
	};

	class DiffuseShader : public Shader
	{
	public:
		explicit DiffuseShader(const MaterialSystem& h) : Shader(h, ShaderTypeDiffuse) {}

		void SetColor(Value value) { SetValue("color", value); }
		void SetNormal(Value value) { SetValue("normal", value); }
		void SetRoughness(Value value) { SetValue("roughness", value); }
	};

	class EmissiveShader : public Shader
	{
	public:
		explicit EmissiveShader(MaterialSystem& h) : Shader(h, ShaderTypeEmissive) {}

		void SetColor(Value value) { SetValue("color", value); }
	};


	// inline definitions
	static int allocatedObjects = 0;

	inline void Object::Data::Init(void* h, const Context& c, bool destroy)
	{
		handle = h;
		context = c.m;
		destroyOnDelete = destroy;
		if (h)
		{
			allocatedObjects++;

			typeNameMirror = GetTypeName();

			DebugPrint(L"\tFR+ %s %08X%s (%d total)\n", GetTypeName(), h, destroyOnDelete ? L"" : L"(unmanaged)", allocatedObjects);
		}
	}

	inline Object::Data::~Data()
	{
		if (handle)
		{
			// Clear references before destroying the object itself. For example, Scene has referenced Shapes,
			// and these shapes better to be destroyed before the scene itself.
			references.clear();

			if (destroyOnDelete)
			{
				auto res = rprObjectDelete(handle);
				FASSERT(res == RPR_SUCCESS);
			}

			allocatedObjects--;

			// Can't use virtual GetTypeName() in destructor, so using "mirrored" type name
			DebugPrint(L"\tFR- %s %08X%s (%d total)\n", typeNameMirror, handle, destroyOnDelete ? L"" : L" (unmanaged)", allocatedObjects);
			handle = nullptr;
		}
	}

	inline void Object::Data::Attach(void* h, bool destroy)
	{
		if (handle)
		{
			if (destroyOnDelete)
			{
				auto res = rprObjectDelete(handle);
				FASSERT(res == RPR_SUCCESS);
			}
			allocatedObjects--;
			DebugPrint(L"\tFR- %s %08X%s (%d total)\n", GetTypeName(), handle, destroyOnDelete ? L"" : L" (unmanaged)", allocatedObjects);
		}

		if (h)
		{
			destroyOnDelete = destroy;
			handle = h;
			allocatedObjects++;

			typeNameMirror = GetTypeName();

			DebugPrint(L"\tFR+ %s %08X%s (%d total)\n", GetTypeName(), h, destroyOnDelete ? L"" : L" (unmanaged)", allocatedObjects);
		}
	}

	inline Context Object::GetContext() const
	{
		return Context(m->context);
	}

	inline Node::Node(const MaterialSystem& ms, int type, bool destroyOnDelete, Node::Data* _data)
		: Object(ms.CreateNode(type), ms.GetContext(), destroyOnDelete, _data ? _data : new Node::Data())
	{
		data().materialSystem = ms;
		data().type = type;
	}

	inline Node::Node(rpr_material_node h, const MaterialSystem& ms, bool destroyOnDelete, Data* _data)
		: Object(h, ms.GetContext(), destroyOnDelete, _data ? _data : new Node::Data())
	{
		data().materialSystem = ms;
		data().type = 0; // unknown
	}

	inline void Node::_SetInputNode(const char* key, const Shader& shader)
	{
		AddReference(shader);
		shader.AttachToMaterialInput(Handle(), key);
	}

	inline void Node::SetValue(const char* key, const Value& v)
	{
		auto res = RPR_SUCCESS;
		switch (v.type)
		{
			case Value::FLOAT:
				res = rprMaterialNodeSetInputF(Handle(), key, v.x, v.y, v.z, v.w);
				FASSERT(res == RPR_SUCCESS);
				break;
			case Value::NODE:
			{
				void* handle = nullptr;
				if (v.node){
					handle = v.node.Handle();
					AddReference(v.node);
				}
				res = rprMaterialNodeSetInputN(Handle(), key, handle);	// should be ok to set null here now
				FASSERT(res == RPR_SUCCESS);
			}
			break;
			default:
				FASSERT(!"bad type");
		}
	}

	inline void Node::SetValue(const char* key, int v)
	{
		auto res = rprMaterialNodeSetInputU(Handle(), key, v);
		FASSERT(res == RPR_SUCCESS);
	}

	inline MaterialSystem Node::GetMaterialSystem() const
	{
		return data().materialSystem.As<MaterialSystem>();
	}

	inline Shader MaterialSystem::ShaderBlend(const Shader& a, const Shader& b, const Value& t)
	{
			// in theory a null could be considered a transparent material, but would create unexpected node
		if (!a)
			return b;
		if (!b)
			return a;

		// shortcuts
		if (t.IsFloat())
		{
			if (t.x <= 0)
				return a;
			if (t.x >= 1)
				return b;
		}

		Shader node(*this, ShaderTypeBlend);
		node._SetInputNode("color0", a);
		node._SetInputNode("color1", b);
		node.SetValue("weight", t);
		return node;
	}

	inline Shader MaterialSystem::ShaderAdd(const Shader& a, const Shader& b)
	{
		if (!a)
			return b;
		if (!b)
			return a;

		Shader node(*this, ShaderTypeAdd);
		node._SetInputNode("color0", a);
		node._SetInputNode("color1", b);
		return node;
	}

	inline MaterialSystem Value::GetMaterialSystem() const
	{
		if (IsNode())
			return node.GetMaterialSystem();
		return MaterialSystem();
	}

	inline Shape::Data::~Data()
	{
		Shader sh = shader.As<Shader>();
		if (sh.IsValid())
		{
			sh.DetachFromShape(*this);
		}
	}

	inline void Shape::SetShader(Shader& shader)
	{
		if (Shader old = GetShader())
		{
			if (old == shader)	// no change?
				return;

			RemoveReference(old);
			old.DetachFromShape(data());
		}

		AddReference(shader);
		data().shader = shader;
		shader.AttachToShape(data());
	}

	inline Shader Shape::GetShader() const
	{
		return data().shader.As<Shader>();
	}

	inline void Shape::SetVolumeShader( const frw::Shader& shader )
	{
		if( auto old = GetVolumeShader() )
		{
			if( old == shader )	// no change?
				return;
			RemoveReference( old );
		}

		AddReference(shader);
		data().volumeShader = shader;

		if (!shader.IsRprxMaterial())
		{
			auto res = rprShapeSetVolumeMaterial( Handle(), shader.Handle() );
			FASSERT(RPR_SUCCESS == res);
		}
	}

	inline Shader Shape::GetVolumeShader() const
	{
		return data().volumeShader.As<Shader>();
	}

	inline Shape Shape::CreateInstance(Context context) const
	{
		DebugPrint(L"CreateInstance()\n");
		rpr_shape h = nullptr;
		auto res = rprContextCreateInstance(context.Handle(), Handle(), &h);
		FASSERT(RPR_SUCCESS == res);
		Shape shape(h, context);
		shape.AddReference(*this);
		return shape;
	}

	inline void Shape::SetDisplacement(Value v, float minscale, float maxscale)
	{
		RemoveDisplacement();
		if (v.IsNode())
		{
			Node n = v.GetNode();
			data().displacementShader = n;
			auto res = rprShapeSetDisplacementMaterial(Handle(), n.Handle());
			FASSERT(RPR_SUCCESS == res);
			res = rprShapeSetDisplacementScale(Handle(), minscale, maxscale);
			FASSERT(RPR_SUCCESS == res);
		}
	}

	inline void Shape::RemoveDisplacement()
	{
		if (data().displacementShader)
		{
			auto res = rprShapeSetDisplacementMaterial(Handle(), nullptr);
			FASSERT(RPR_SUCCESS == res);
			res = rprShapeSetSubdivisionFactor(Handle(), 0);
			FASSERT(RPR_SUCCESS == res);
			data().displacementShader = nullptr;
		}
	}

	inline void Shape::SetSubdivisionFactor(int sub)
	{
		size_t faceCount = this->GetFaceCount();

		while (sub > 1 && (faceCount * sub * sub) > MaximumSubdividedFacecount)
			sub--;

		auto res = rprShapeSetSubdivisionFactor(Handle(), sub);
		FASSERT(RPR_SUCCESS == res);
	}

	inline void Shape::SetSubdivisionCreaseWeight(float weight)
	{
		auto res = rprShapeSetSubdivisionCreaseWeight(Handle(), weight);
		FASSERT(RPR_SUCCESS == res);
	}

	inline void Shape::SetSubdivisionBoundaryInterop(rpr_subdiv_boundary_interfop_type type)
	{
		auto res = rprShapeSetSubdivisionBoundaryInterop(Handle(), type);
		FASSERT(RPR_SUCCESS == res);
	}

	inline Image::Image(Context context, const rpr_image_format& format, const rpr_image_desc& image_desc, const void* data)
		: Object(nullptr, context, true, new Data())
	{
		DebugPrint(L"CreateImage()\n");
		rpr_image h = nullptr;
		auto res = rprContextCreateImage(context.Handle(), format, &image_desc, data, &h);
		FASSERT(RPR_SUCCESS == res);
		m->Attach(h);
	}

	inline bool Image::IsGrayScale()
	{
		rpr_image_desc desc;
		rpr_image img = Handle();
		if (!img)
			return false;
		rpr_int status = rprImageGetInfo(img, RPR_IMAGE_DESC, sizeof(desc), &desc, nullptr); FASSERT(status == RPR_SUCCESS);
		fr_image_format format;
		status = frImageGetInfo(img, RPR_IMAGE_FORMAT, sizeof(format), &format, nullptr); FASSERT(status == RPR_SUCCESS);
		if (format.num_components == 1)
		{
			return true;
		}

		unsigned int rasters[4] = {
			desc.image_height - 1,
			desc.image_height >> 1,
			desc.image_height >> 2,
			desc.image_height >> 4
		};

		bool gray[4] = { true, true, true, true };

		int rasterw = desc.image_width - 3;

		if (format.type == RPR_COMPONENT_TYPE_UINT8)
		{
			size_t imgsize = 0;
			status = frImageGetInfo(img, RPR_IMAGE_DATA, 0, nullptr, &imgsize); FASSERT(status == RPR_SUCCESS);
			static std::vector<unsigned char> img_data(imgsize);
			img_data.resize(imgsize);
			status = frImageGetInfo(img, RPR_IMAGE_DATA, imgsize, img_data.data(), nullptr); FASSERT(status == RPR_SUCCESS);
			if (format.num_components == 3)
			{
#pragma omp parallel for
				for (int i = 0; i < 4; i++)
				{
					unsigned char* raster = &img_data[desc.image_width * 3 * rasters[i]];
					for (int j = 0; j < rasterw; j += 3)
					{
						if ((raster[j] != raster[j + 1]) || (raster[j] != raster[2]))
						{
							gray[i] = false;
							break;
						}
					}
				}
			}
		}
		else if (format.type == RPR_COMPONENT_TYPE_FLOAT32)
		{
			size_t imgsize = 0;
			status = frImageGetInfo(img, RPR_IMAGE_DATA, 0, nullptr, &imgsize); FASSERT(status == RPR_SUCCESS);
			static std::vector<float> img_data(imgsize);
			img_data.resize(imgsize);
			status = frImageGetInfo(img, RPR_IMAGE_DATA, imgsize, img_data.data(), nullptr); FASSERT(status == RPR_SUCCESS);
			if (format.num_components == 3)
			{
#pragma omp parallel for
				for (int i = 0; i < 4; i++)
				{
					float* raster = &img_data[desc.image_width * 3 * rasters[i]];
					for (int j = 0; j < rasterw; j += 3)
					{
						if ((abs(raster[j] - raster[j + 1]) > 0.0000001f) || (abs(raster[j] - raster[2]) > 0.0000001f))
						{
							gray[i] = false;
							break;
						}
					}
				}
			}
		}
		return gray[0] && gray[1] && gray[2] && gray[3];
	}

	inline void EnvironmentLight::SetImage(Image img)
	{
		auto res = rprEnvironmentLightSetImage(Handle(), img.Handle());
		FASSERT(RPR_SUCCESS == res);
		data().image = img;
	}

	inline void EnvironmentLight::AttachPortal(Shape shape)
	{
		auto res = rprEnvironmentLightAttachPortal(GetContext().GetScene(), Handle(), shape.Handle());
		FASSERT(RPR_SUCCESS == res);
		data().portals.push_back(shape);
	}

    inline void Context::Attach(PostEffect post_effect){
        auto res = rprContextAttachPostEffect(Handle(), post_effect.Handle());
        FASSERT(RPR_SUCCESS == res);
		post_effect.SetAttached(true);
    }

    inline void Context::Detach(PostEffect post_effect){
        auto res = rprContextDetachPostEffect(Handle(), post_effect.Handle());
		FASSERT(RPR_SUCCESS == res);
		post_effect.SetAttached(false);
    }

	inline Shape Context::CreateMesh(const rpr_float* vertices, size_t num_vertices, rpr_int vertex_stride,
		const rpr_float* normals, size_t num_normals, rpr_int normal_stride,
		const rpr_float* texcoords, size_t num_texcoords, rpr_int texcoord_stride,
		const rpr_int*   vertex_indices, rpr_int vidx_stride,
		const rpr_int*   normal_indices, rpr_int nidx_stride,
		const rpr_int*   texcoord_indices, rpr_int tidx_stride,
		const rpr_int*   num_face_vertices, size_t num_faces)
	{
		DebugPrint(L"CreateMesh() - %d faces\n", num_faces);
		rpr_shape shape = nullptr;

		auto res = rprContextCreateMesh(Handle(),
			vertices, num_vertices, vertex_stride,
			normals, num_normals, normal_stride,
			texcoords, num_texcoords, texcoord_stride,
			vertex_indices, vidx_stride,
			normal_indices, nidx_stride,
			texcoord_indices, tidx_stride,
			num_face_vertices, num_faces, 
			&shape);
		
		FASSERT(RPR_SUCCESS == res);

		return Shape(shape, *this);
	}

	inline Shape Context::CreateMeshEx(const rpr_float* vertices, size_t num_vertices, rpr_int vertex_stride, 
			const rpr_float* normals, size_t num_normals, rpr_int normal_stride,
			const rpr_int* perVertexFlag, size_t num_perVertexFlags, rpr_int perVertexFlag_stride,
			rpr_int numberOfTexCoordLayers, const rpr_float** texcoords, const size_t* num_texcoords, const rpr_int* texcoord_stride, 
			const rpr_int* vertex_indices, rpr_int vidx_stride,
			const rpr_int* normal_indices, rpr_int nidx_stride, const rpr_int** texcoord_indices,
			const rpr_int* tidx_stride, const rpr_int * num_face_vertices, size_t num_faces)
	{
		DebugPrint(L"CreateMesh() - %d faces\n", num_faces);
		rpr_shape shape = nullptr;

		rpr_int res = -18;
		try
		{
			res = rprContextCreateMeshEx(Handle(),
				vertices, num_vertices, vertex_stride,
				normals, num_normals, normal_stride,
				perVertexFlag, num_perVertexFlags, perVertexFlag_stride,
				numberOfTexCoordLayers, texcoords, num_texcoords, texcoord_stride,
				vertex_indices, vidx_stride,
				normal_indices, nidx_stride, texcoord_indices,
				tidx_stride, num_face_vertices, num_faces,
				&shape);
		}
		catch (...)
		{
		}
		
		FASSERT(RPR_SUCCESS == res);

		return Shape(shape, *this);
	}

	inline rpr_int Context::SetAOV(FrameBuffer frameBuffer, rpr_aov aov)
	{
		rpr_int res = rprContextSetAOV(Handle(), aov, frameBuffer.Handle());
		FASSERT(RPR_SUCCESS == res);
		return res;
	}

	// convenience operators
	inline Value operator+(const Value& a, const Value& b)
	{
		auto ms = a.GetMaterialSystem();
		if (!ms)
			ms = b.GetMaterialSystem();
		return ms.ValueAdd(a, b);
	}
	inline Value operator*(const Value& a, const Value& b)
	{
		auto ms = a.GetMaterialSystem();
		if (!ms)
			ms = b.GetMaterialSystem();
		return ms.ValueMul(a, b);
	}
	inline Value operator/(const Value& a, const Value& b)
	{
		auto ms = a.GetMaterialSystem();
		if (!ms)
			ms = b.GetMaterialSystem();
		return ms.ValueDiv(a, b);
	}
	inline Value operator-(const Value& a, const Value& b)
	{
		auto ms = a.GetMaterialSystem();
		if (!ms)
			ms = b.GetMaterialSystem();
		return ms.ValueSub(a, b);
	}
	inline Value operator-(const Value& a)
	{
		auto ms = a.GetMaterialSystem();
		return ms.ValueNegate(a);
	}
	inline Value operator^(const Value& a, const Value& b)
	{
		auto ms = a.GetMaterialSystem();
		if (!ms)
			ms = b.GetMaterialSystem();
		return ms.ValuePow(a, b);
	}
	inline Value operator&(const Value& a, const Value& b)
	{
		auto ms = a.GetMaterialSystem();
		if (!ms)
			ms = b.GetMaterialSystem();
		return ms.ValueDot(a, b);
	}

}
