/*********************************************************************************************************************************
* Radeon ProRender for 3ds Max plugin
* Copyright (C) 2017 AMD
* All Rights Reserved
*
* Abstraction of the Scope class (see ScopeManager.cpp)
*********************************************************************************************************************************/

#include "FrScope.h"
#include "Common.h"

using namespace frw;

Scope::Scope(rpr_context p, bool destroyOnDelete)
{
	if (p)
		m = std::make_shared<Data>(Context(p, destroyOnDelete), true);
}

Scope::~Scope()
{

}

FrameBuffer Scope::GetFrameBuffer(int width, int height, int id)
{
	assert(m);

	if (width != m->width || height != m->height)
	{
		m->frameBuffers.clear();
		m->width = width;
		m->height = height;
	}

	if (!m->frameBuffers.count(id))
	{
		FrameBuffer fb(m->context, width, height);
		fb.Clear();
		m->frameBuffers[id] = fb;
	}

	return m->frameBuffers[id];
}

FrameBuffer Scope::GetFrameBuffer(int id)
{
	auto ff = m->frameBuffers.find(id);
	if (ff != m->frameBuffers.end())
		return ff->second;
	return FrameBuffer();
}

void Scope::DestroyFrameBuffer(int id)
{
	auto ff = m->frameBuffers.find(id);
	if (ff != m->frameBuffers.end())
	{
		m->frameBuffers.erase(ff);
	}
}

void Scope::DestroyFrameBuffers()
{
	m->frameBuffers.clear();
}

Scene Scope::GetScene() const
{
	assert(m);
	auto p = m;
	while (!p->scene && p->parent)
		p = p->parent;

	auto scene = p->scene;
	
	return scene;
}

Scene Scope::GetScene(bool orCreate)
{
	assert(m);
	auto p = m;
	while (!p->scene && p->parent)
		p = p->parent;

	auto scene = p->scene;

	if (!scene && orCreate)
	{
		scene = m->scene = GetContext().CreateScene();
		GetContext().SetScene(scene);
	}

	return scene;
}

void Scope::SetScene(Scene scene)
{
	assert(m);
	if (m->scene)
		m->scene.Clear();

	m->scene = scene;
	GetContext().SetScene(scene);
}

void Scope::Clear(int which)
{
	if (m)
	{
		if (which & CACHE_SHAPES)
			m->cache.shape.Clear();
		if (which & CACHE_SHADERS)
			m->cache.shader.Clear();
		if (which & CACHE_VALUES)
			m->cache.value.Clear();
		if (which & CACHE_IMAGES)
			m->cache.image.Clear();
		if (which & CACHE_SHAPESETS)
			m->cache.shapeSet.Clear();
	}
}

// Garbage collect, delete objects in the cache not referenced elsewhere
// TODO: Use 3ds Max validity intervals to determine cache entry lifespan
void Scope::gc()
{
	// Delete any shape objects in the cache not referenced elsewhere
	Cache<size_t, Shape>::EntryMap::iterator it_shape = m->cache.shape.map.begin();
	while( it_shape!= m->cache.shape.map.end() )
	{
		if( it_shape->second.value.use_count() == 1 )
			it_shape = m->cache.shape.map.erase(it_shape);
		else it_shape++;
	}

	// Delete any shader objects in the cache not referenced elsewhere
	Cache<size_t, Shader>::EntryMap::iterator it_shader = m->cache.shader.map.begin();
	while( it_shader != m->cache.shader.map.end()  )
	{
		if( it_shader->second.value.use_count() == 1 )
			it_shader = m->cache.shader.map.erase(it_shader);
		else it_shader++;
	}

	// Not supported for cache value objects, they don't derive from frw::Object

	// Delete any image objects in the cache not referenced elsewhere
	Cache<size_t, Image>::EntryMap::iterator it_image = m->cache.image.map.begin();
	while( it_image != m->cache.image.map.end() )
	{
		if( it_image->second.value.use_count() == 1 )
			it_image = m->cache.image.map.erase(it_image);
		else it_image++;
	}

	// Delete any shape objects in the cache not referenced elsewhere, within shapeSets
	// Delete entire shapeSet if no members remain
	Cache<size_t, ShapeSet>::EntryMap::iterator it_shapeSet = m->cache.shapeSet.map.begin();
	while( it_shapeSet != m->cache.shapeSet.map.end()  )
	{
		std::vector<Shape>::iterator it = it_shapeSet->second.value.begin();
		while( it != it_shapeSet->second.value.end() )
		{
			if( it->use_count() == 1 )
				it = it_shapeSet->second.value.erase(it);
			else it++;
		}
		if( it_shapeSet->second.value.size()==0 )
			it_shapeSet = m->cache.shapeSet.map.erase( it_shapeSet );
		else it_shapeSet++;
	}
}

Scope Scope::CreateChildScope() const
{
	assert(m);
	Scope child(m->context.Handle(), false);
	child.m->parent = m;
	return child;
}

Scope Scope::CreateLocalScope() const
{
	assert(m);
	Scope child(m->context.Handle(), false);
	child.m->materialSystem = m->materialSystem;
	child.m->parent = m;
	child.m->scene = m->scene;
	return child;
}

Scope Scope::ParentScope(bool orSelf) const
{
	if (m && m->parent)
		return m->parent;

	if (orSelf)
		return *this;

	return Scope();
}

Scope::Data::Data(Context c, bool bCreateContextEx /*= false*/) :
	context(c), materialSystem(c)
{
}

Scope::Data::Data(Context c, MaterialSystem ms, bool bCreateContextEx /*= false*/) :
	context(c), materialSystem(ms)
{
}

Scope::Data::~Data()
{
	cache.shapeSet.Clear();
	cache.shape.Clear();
	cache.shader.Clear();
	cache.value.Clear();
	cache.image.Clear();

	scene.Reset(); // destroy scene before deleting context
}