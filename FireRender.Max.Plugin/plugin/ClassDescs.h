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