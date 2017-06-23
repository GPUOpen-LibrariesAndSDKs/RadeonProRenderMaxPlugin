/*********************************************************************************************************************************
* Radeon ProRender for 3ds Max plugin
* Copyright (C) 2017 AMD
* All Rights Reserved
* 
* 3ds Max ClassDesc2 declarations for all plugins we implement. These classes define the basic parameters of each plugin 
* (such as name, type, category, and method to instance them).
*********************************************************************************************************************************/
#pragma once
#include "Common.h"
#include <iparamb2.h>
#include <IMaterialBrowserEntryInfo.h>

FIRERENDER_NAMESPACE_BEGIN;

/// The main rendering plugin
class FireRenderClassDesc : public ClassDesc2 {
public:
    int IsPublic() override {
        return 1;
    }
    BOOL NeedsToSave() override {
        return FALSE;
    }
    HINSTANCE HInstance() override {
        FASSERT(fireRenderHInstance != NULL);
        return fireRenderHInstance;
    }
    const TCHAR* InternalName() override {
        return FIRE_MAX_NAME;
    }
    const TCHAR * ClassName() override {
        return InternalName();
    }
    SClass_ID SuperClassID() override {
        return RENDERER_CLASS_ID;
    }
    Class_ID ClassID() override {
        return FIRE_MAX_CID;
    }
    const TCHAR* Category() override {
        return _T("Standard");
    }
    void* Create(BOOL loading) override;

};

class FireRenderClassDescBase : public ClassDesc2
{
public:
	// If HasEntryThumbnail returns true, this function must return a bitmap object
	// with an icon that will be displayed in the material browser, rather than
	// letting 3dsmax rendering a default one
	virtual Bitmap *GetEntryThumbnail() {
		return NULL;
	}
	virtual bool HasEntryThumbnail() {
		return false;
	}
};

#define BEGIN_DECLARE_FRMTLCLASSDESC(NAME, MNEM_NAME, ID)\
\
class FRMTLCLASSBROWSERENTRYNAME(NAME) : public IMaterialBrowserEntryInfo\
{\
public:\
	static FRMTLCLASSBROWSERENTRYNAME(NAME) mInstance; \
public:\
	FRMTLCLASSBROWSERENTRYNAME(NAME)() {\
	}\
	~FRMTLCLASSBROWSERENTRYNAME(NAME)() {\
	}\
	virtual const MCHAR* GetEntryName() const override {\
	return NULL; \
	}\
	virtual const MCHAR *GetEntryCategory() const override {\
			return _T("Materials\\Radeon ProRender"); \
	}\
	virtual Bitmap *GetEntryThumbnail() const override; \
	virtual bool  HasEntryThumbnail() const;\
}; \
\
class FireRenderClassDesc##NAME : public FireRenderClassDescBase { \
public:\
	int IsPublic() override {\
		return 1;\
	}\
	BOOL NeedsToSave() override {\
		return FALSE;\
	}\
	HINSTANCE HInstance() override {\
		FASSERT(fireRenderHInstance != NULL);\
		return fireRenderHInstance;\
	}\
	const TCHAR* InternalName() override {\
		return MNEM_NAME;\
	}\
	const TCHAR * ClassName() override {\
		return InternalName();\
	}\
	SClass_ID SuperClassID() override {\
		return MATERIAL_CLASS_ID;\
	}\
	Class_ID ClassID() override {\
		return ID;\
	}\
	const TCHAR* Category() override {\
		return _T("Radeon ProRender");\
	}\
	void* Create(BOOL loading) override;\
	virtual FPInterface* GetInterface(Interface_ID id) override;

#define END_DECLARE_FRMTLCLASSDESC()\
};


#define BEGIN_DECLARE_FRTEXCLASSDESC(NAME, MNEM_NAME, ID)\
\
class FRMTLCLASSBROWSERENTRYNAME(NAME) : public IMaterialBrowserEntryInfo\
{\
public:\
	static FRMTLCLASSBROWSERENTRYNAME(NAME) mInstance; \
public:\
	FRMTLCLASSBROWSERENTRYNAME(NAME)() {\
	}\
	~FRMTLCLASSBROWSERENTRYNAME(NAME)() {\
	}\
	virtual const MCHAR* GetEntryName() const override {\
	return NULL; \
	}\
	virtual const MCHAR *GetEntryCategory() const override {\
	return _T("Maps\\Radeon ProRender"); \
	}\
	virtual Bitmap *GetEntryThumbnail() const override;\
	virtual bool  HasEntryThumbnail() const;\
}; \
\
class FireRenderClassDesc##NAME : public FireRenderClassDescBase { \
public:\
	int IsPublic() override {\
	return 1; \
	}\
	BOOL NeedsToSave() override {\
	return FALSE; \
	}\
	HINSTANCE HInstance() override {\
	FASSERT(fireRenderHInstance != NULL); \
	return fireRenderHInstance; \
	}\
	const TCHAR* InternalName() override {\
	return MNEM_NAME; \
	}\
	const TCHAR * ClassName() override {\
	return InternalName(); \
	}\
	SClass_ID SuperClassID() override {\
	return TEXMAP_CLASS_ID; \
	}\
	Class_ID ClassID() override {\
	return ID; \
	}\
	const TCHAR* Category() override {\
	return TEXMAP_CAT_COLMOD; \
	}\
	void* Create(BOOL loading) override; \
	virtual FPInterface* GetInterface(Interface_ID id) override;

#define END_DECLARE_FRTEXCLASSDESC()\
};

#define IMPLEMENT_FRMTLCLASSDESC(NAME)\
\
	FRMTLCLASSBROWSERENTRYNAME(NAME) FRMTLCLASSBROWSERENTRYNAME(NAME)::mInstance;\
\
	void* FireRenderClassDesc##NAME::Create(BOOL loading) {\
	return new FireRender##NAME;\
}\
FPInterface* FireRenderClassDesc##NAME::GetInterface(Interface_ID id) {\
	if (id == IMATERIAL_BROWSER_ENTRY_INFO_INTERFACE)\
		return &FRMTLCLASSBROWSERENTRYNAME(NAME)::mInstance;\
	return ClassDesc2::GetInterface(id);\
}\
Bitmap *FRMTLCLASSBROWSERENTRYNAME(NAME)::GetEntryThumbnail() const {\
\
	return FRMTLCLASSNAME(NAME)::ClassDescInstance.GetEntryThumbnail(); \
}\
bool  FRMTLCLASSBROWSERENTRYNAME(NAME)::HasEntryThumbnail() const {\
\
	return FRMTLCLASSNAME(NAME)::ClassDescInstance.HasEntryThumbnail(); \
}


/// The autotesting tool utility plugin
class AutoTestingClassDesc : public ClassDesc2 {
public:
    int IsPublic() override {
        return 1;
    }
    BOOL NeedsToSave() override {
        return FALSE;
    }
    HINSTANCE HInstance() override {
        FASSERT(fireRenderHInstance != NULL);
        return fireRenderHInstance;
    }
    const TCHAR* InternalName() override {
        return AUTOTESTING_NAME;
    }
    const TCHAR * ClassName() override {
        return InternalName();
    }
    SClass_ID SuperClassID() override {
        return UTILITY_CLASS_ID;
    }
    Class_ID ClassID() override {
        return AUTOTESTING_CID;
    }
    const TCHAR* Category() override {
        return _T("Radeon ProRender");
    }
    void* Create(BOOL loading) override;
};

class FireRenderClassDesc_MaterialImport : public ClassDesc2 {
public:
	int IsPublic() override {
		return 1;
	}
	BOOL NeedsToSave() override {
		return FALSE;
	}
	HINSTANCE HInstance() override {
		FASSERT(fireRenderHInstance != NULL);
		return fireRenderHInstance;
	}
	const TCHAR* InternalName() override {
		return _T("Radeon ProRender Material Import");
	}
	const TCHAR * ClassName() override {
		return InternalName();
	}
	SClass_ID SuperClassID() override {
		return SCENE_IMPORT_CLASS_ID;
	}
	Class_ID ClassID() override {
		return FIRERENDER_MTLIMPORTER_CID;
	}
	const TCHAR* Category() override {
		return _T("Standard");
	}
	void* Create(BOOL loading) override;
};

class FireRenderClassDesc_MaterialExport : public ClassDesc2 {
public:
	int IsPublic() override {
		return 1;
	}
	BOOL NeedsToSave() override {
		return FALSE;
	}
	HINSTANCE HInstance() override {
		FASSERT(fireRenderHInstance != NULL);
		return fireRenderHInstance;
	}
	const TCHAR* InternalName() override {
		return _T("Radeon ProRender Material Exporter");
	}
	const TCHAR * ClassName() override {
		return InternalName();
	}
	SClass_ID SuperClassID() override {
		return SCENE_EXPORT_CLASS_ID;
	}
	Class_ID ClassID() override {
		return FIRERENDER_MTLEXPORTER_CID;
	}
	const TCHAR* Category() override {
		return _T("Standard");
	}
	void* Create(BOOL loading) override;
};

extern FireRenderClassDesc fireRenderClassDesc;
extern AutoTestingClassDesc autoTestingClassDesc;
extern FireRenderClassDesc_MaterialImport fireRenderMaterialImporterClassDesc;

FIRERENDER_NAMESPACE_END;