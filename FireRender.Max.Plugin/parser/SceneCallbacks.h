/*********************************************************************************************************************************
* Radeon ProRender for 3ds Max plugin
* Copyright (C) 2017 AMD
* All Rights Reserved
* 
* Code ensuring that all necessary methods to prepare each scene object for rendering/restore it after rendering are called
*********************************************************************************************************************************/
#pragma once
#include <set>
FIRERENDER_NAMESPACE_BEGIN;

//////////////////////////////////////////////////////////////////////////////
// SceneCallbacks
//
// This class holds pointers to all root reference targets we encounter during
// scene parsing, and it is responsible that the correct sequence of
// before/after-rendering methods is invoked on them. This is necessary because
// many objects in 3ds Max have an internal state that can be switched between
// "viewport" and "rendering" modes. Switching to rendering mode for example can 
// cause subdivided mesh to return a more finely tesselated geometry. Since the
// documentation by Autodesk is lacking, we reverse-engineered this sequence:
// 
// Before render:
// a) Animatable::RenderBegin() needs to be called on all ReferenceTargets in
//    each hierarchy (not only top level ones)
// b) MtlBase::Update() needs to be called on all top-level MtlBases (all
//    materials assigned to nodes and also standalone maps, such as environment
//    map). Reference implementations of Update() call Update() on all children,
//    so recursion is not necessary
// c) Texmap::LoadMapFiles() needs to be called on all Texmaps in each hierarchy
//    (e.g. not only top level ones)
// 
// After render:
// a) Animatable::RenderEnd() needs to be called on all ReferenceTargets in each
//    hierarchy (not only top level ones)
//

class SceneCallbacks
{
protected:

    // Safety wrapper around 3ds Max RefEnumProc. This class allows iterating every
	// ReferenceTarget in a hierarchy of provided root. Since visited nodes are
	// marked, duplicate items in any hierarchy between begin() and end() calls are
	// visited only once
	
    class MyRefEnum : public RefEnumProc
	{
    protected:
        TimeValue t = INT_MIN;

    public:
        void begin(const TimeValue t)
		{
            this->t = t;
            BeginEnumeration();
        }
        
		void end()
		{
            this->t = INT_MIN;
            EndEnumeration();
        }

        // For debugging only
        bool isStarted() const
		{
            return this->t > INT_MIN;
        }
    };

    // Goes through the hierarchy and calls RenderBegin and LoadMapFiles on its nodes
    class BeginEnum : public MyRefEnum 
	{
    public:
        int proc(ReferenceMaker *rm) override 
		{
            FASSERT(this->t != INT_MIN); // assert we are between begin() and end()
            rm->RenderBegin(this->t);
            if (auto tm = dynamic_cast<Texmap*>(rm))
                tm->LoadMapFiles(this->t);

            return REF_ENUM_CONTINUE;
        }
    };

    // Goes through the hierarchy and calls RenderEnd on its nodes
    class EndEnum : public MyRefEnum 
	{
    public:
        int proc(ReferenceMaker *rm) override 
		{
            FASSERT(this->t != INT_MIN); // assert we are between begin() and end()
            rm->RenderEnd(this->t);
            return REF_ENUM_CONTINUE;
        }
    };

    // All already processed top level objects
    std::set<RefTargetHandle> items;

    BeginEnum beginEnum;

    /// Currently parsed frame time
    TimeValue t;

public:

    // Prepares the object for adding items. It has to be called before any call to addItem()
    void beforeParsing(const TimeValue t)
	{
        this->t = t;
        FASSERT(this->items.size() == 0);
        this->beginEnum.begin(t);
    }

    // Must be called after beforeParsing() and after adding all items and parsing them.
	// Reverts all objects back from the rendering mode to viewport mode
    void afterParsing()
	{
        this->beginEnum.end();
        FASSERT(!this->beginEnum.isStarted());
        EndEnum endEnum;
        endEnum.begin(this->t);
        for (auto& i : this->items)
            i->EnumRefHierarchy(endEnum); // call RenderEnd on everything on which we called RenderBegin before
        endEnum.end();
        this->items.clear();
    }

    // Handles a single ReferenceTarget (of any sub-class) by calling the before-parsing callbacks. Can only be called between 
    // beforeParsing() and afterParsing() calls.
	void addItem(RefTargetHandle item)
	{
        FASSERT(this->beginEnum.isStarted());
        if (!item || this->items.find(item) != this->items.end())
            return;

        this->items.insert(item);

        item->EnumRefHierarchy(this->beginEnum);

        if (auto mtlBase = dynamic_cast<MtlBase*>(item))
            mtlBase->Update(this->t, Interval()); // call update only on top-level entitites

        if (auto node = dynamic_cast<INode*>(item))
            addItem(node->GetMtl()); // node material must be handled separately as a top-level entity
    }
};


FIRERENDER_NAMESPACE_END;
