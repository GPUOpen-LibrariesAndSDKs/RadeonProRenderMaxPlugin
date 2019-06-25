/*********************************************************************************************************************************
* Radeon ProRender for 3ds Max plugin
* Copyright (C) 2017 AMD
* All Rights Reserved
*
* Abstraction of the Scope class (see ScopeManager.cpp)
*********************************************************************************************************************************/

#pragma once
#include <frwrap.h>
#include <memory>
#include <map>
#include <vector>

namespace frw 
{

enum
{
	CACHE_SHAPES = 1,
	CACHE_SHADERS = 2,
	CACHE_VALUES = 4,
	CACHE_IMAGES = 8,
	CACHE_SHAPESETS = 16,
	CACHE_ALL = CACHE_SHAPES | CACHE_SHADERS | CACHE_VALUES | CACHE_IMAGES | CACHE_SHAPESETS
};

template<class K, class T>
class Cache
{
	struct Entry
	{
		T value;
	};

	T defaultValue;

public:
	typedef std::map<K, Entry> EntryMap;
	EntryMap map;

	bool enabled = true;

public:		
	void Clear()
	{
		map.clear();
	}
	const T& Get(K k)
	{
		if (!enabled)
			return defaultValue;

		auto it = map.find(k);
		if (it == map.end())
			return defaultValue;
		return it->second.value;
	}

	void Set(K k, const T& v)
	{
		if (!enabled)
			return;

		if (v)
			map[k].value = v;
		else
			map.erase(k);
	}
};

// reference type
class Scope
{
	// should never be stored as address, always treat as value
	void operator&() const = delete;

	class Data;
	typedef std::shared_ptr<Data> DataPtr;

	class ShapeSet : public std::vector<Shape>
	{
	public:
		ShapeSet() {}
		ShapeSet(const std::vector<Shape>& p) : std::vector<Shape>(p) {}
		explicit operator bool() const { return !empty(); }
	};

	class Data
	{
	public:
		DataPtr parent;
		Context context;
		MaterialSystem materialSystem;

		rprx_context contextEx;
		bool bOwnContextEx;

		struct CacheStruct
		{
			Cache<size_t, Image> image;
			Cache<size_t, Shader> shader;
			Cache<size_t, Shape> shape;
			Cache<size_t, Value> value;
			Cache<size_t, ShapeSet> shapeSet;
		} cache;

		Scene scene;	// current scene

		int width = 0;
		int height = 0;
		std::map<int, FrameBuffer> frameBuffers;

		explicit Data(Context c, bool bCreateContextEx = false);

		Data(Context c, MaterialSystem ms, bool bCreateContextEx = false);

		~Data();
	};

	DataPtr m;

	Scope(DataPtr data) : m(data) {}

protected:
	int nextUnusedMaterialKey = 0;

public:
	Scope() {}
	explicit Scope(rpr_context p, bool destroyOnDelete);
	~Scope();

	void Reset() { m.reset(); }

	bool IsValid() const { return m != nullptr; }
	explicit operator bool() const { return IsValid(); }

	Context GetContext() const
	{
		return m ? m->context : Context();
	}

	rprx_context GetContextEx() const
	{
		return m ? m->contextEx : nullptr;
	}

	int GetNextUnusedMaterialKey()
	{
		return nextUnusedMaterialKey++;
	}

	MaterialSystem GetMaterialSystem() const
	{
		return m ? m->materialSystem : MaterialSystem();
	}

	int Width() const
	{
		return m ? m->width : 0;
	}

	int Height() const
	{
		return m ? m->height : 0;
	}

	FrameBuffer GetFrameBuffer(int width, int height, int id = 0);
	FrameBuffer GetFrameBuffer(int id);
	void DestroyFrameBuffer(int id = 0);
	void DestroyFrameBuffers();

	Scene GetScene() const;
	Scene GetScene(bool orCreate = true);
	void SetScene(Scene scene);

	Shader GetShader(size_t key) { return m->cache.shader.Get(key); }
	void SetShader(size_t key, Shader shader) { m->cache.shader.Set(key, shader); }
	Shader GetShadowCatcherShader()
	{
		for (auto shader : m->cache.shader.map)
		{
			if (shader.second.value.IsShadowCatcher()) 
				return shader.second.value;
		}

		return nullptr;
	}


	Shape GetShape(size_t key) { return m->cache.shape.Get(key); }
	void SetShape(size_t key, Shape shape) { m->cache.shape.Set(key, shape); }

	const std::vector<Shape>& GetShapeSet(size_t key) { return m->cache.shapeSet.Get(key); }
	void SetShapeSet(size_t key, const std::vector<Shape>& value) { m->cache.shapeSet.Set(key, value); }

	Image GetImage(size_t key) { return m->cache.image.Get(key); }
	void SetImage(size_t key, Image image) { m->cache.image.Set(key, image); }

	Value GetValue(size_t key) { return m->cache.value.Get(key); }
	void SetValue(size_t key, Value value) { m->cache.value.Set(key, value); }

	// just clear out stuff
	void Clear(int which = CACHE_ALL);

	// Garbage collect, delete objects in the cache not referenced elsewhere
	// TODO: Use 3ds Max validity intervals to determine cache entry lifespan
	void gc();

	// A child scope will destroy all its objects on close
	Scope CreateChildScope() const;

	// A local scope is useful for temp caching but objects are persisted in parent (cache entries are discarded on destruct)
	// eg if source objects are not going to change you can cache by simple pointer reference and avoid hashing issues
	Scope CreateLocalScope() const;

	Scope ParentScope(bool orSelf = true) const;

	operator Context() const { return GetContext(); }
	operator MaterialSystem() const { return GetMaterialSystem(); }
};

} // namespace frw