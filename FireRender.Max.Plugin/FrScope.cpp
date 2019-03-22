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
	child.m->contextEx = m->contextEx;
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






Scope::Data::Data(Context c, bool bCreateContextEx /*= false*/)
:context(c), materialSystem(c)
{
	bOwnContextEx = bCreateContextEx && c.DestroyOnDelete();
	if (bOwnContextEx)
	{
		auto res = rprxCreateContext(materialSystem.Handle(), RPRX_FLAGS_ENABLE_LOGGING, &contextEx);
		FCHECK(res);
	}
}

Scope::Data::Data(Context c, MaterialSystem ms, bool bCreateContextEx /*= false*/)
:context(c), materialSystem(ms)
{
	bOwnContextEx = bCreateContextEx && c.DestroyOnDelete();
	if (bOwnContextEx)
	{
		auto res = rprxCreateContext(materialSystem.Handle(), RPRX_FLAGS_ENABLE_LOGGING, &contextEx);
		FCHECK(res);
	}
}

Scope::Data::~Data()
{
	cache.shapeSet.Clear();
	cache.shape.Clear();
	cache.shader.Clear();
	cache.value.Clear();
	cache.image.Clear();

	scene.Reset(); // destroy scene before deleting context

	if (contextEx && bOwnContextEx)
	{
		auto res = rprxDeleteContext(contextEx);
		FCHECK(res);
		DebugPrint(L"\tDeleted RPRX context %08X\n", contextEx);
	}	
}