/*********************************************************************************************************************************
* Radeon ProRender for 3ds Max plugin
* Copyright (c) 2017 AMD
* All Rights Reserved
*
* Radeon ProRender dedicated custom material plugin base class
*********************************************************************************************************************************/
#pragma once
#include "frWrap.h"

#include <max.h>
#include "Common.h"
#include "ClassDescs.h"
#include <assetmanagement/IAssetAccessor.h>
#include "utils/Utils.h"
#include <fstream>
#include <vector>

#include <iparamm2.h>
#include <map>

FIRERENDER_NAMESPACE_BEGIN

/// IDs of different versions of the plugin. The ID is written in the saved files and can be read later during loading to
/// potentially run a legacy-porting script.
enum FireRenderMtlPbVersions {
	FIRERENDERMTLVER_DELIVERABLE3 = 4567,
	FIRERENDERMTLVER_LATEST = FIRERENDERMTLVER_DELIVERABLE3,
};

class MaterialParser;

#define FRMTLCLASSNAME(NAME) FireRender##NAME
#define FRMTLCLASSDESCNAME(NAME) FireRenderClassDesc##NAME
#define FRMTLCLASSBROWSERENTRYNAME(NAME) FireRenderBrowserEntry##NAME

class FireRenderMtlBase : public Mtl
{
protected:
	IParamBlock2* pblock;
	TimeValue currentTime = 0;

public:
	FireRenderMtlBase(IParamBlock2* block = nullptr);

	void SetCurrentTime(TimeValue t);
	void DeleteThis() override;
	RefResult NotifyRefChanged(NOTIFY_REF_CHANGED_PARAMETERS) override;
	RefTargetHandle GetReference(int i) override;
	void SetReference(int i, RefTargetHandle rtarg) override;
	int NumRefs() override;
	int SubNumToRefNum(int subNum) override;
	int NumSubs() override;
	Animatable* SubAnim(int i) override;
	TSTR SubAnimName(int i) override;
	float GetSelfIllum(int mtlNum, BOOL backFace) override;
	Color GetSelfIllumColor(int mtlNum, BOOL backFace) override;
	Color GetDiffuse(int mtlNum, BOOL backFace) override;
	Color GetSpecular(int mtlNum, BOOL backFace) override;
	float GetShininess(int mtlNum, BOOL backFace) override;
	float GetShinStr(int mtlNum, BOOL backFace) override;
	Color GetAmbient(int mtlNum, BOOL backFace) override;
	float GetXParency(int mtlNum, BOOL backFace) override;
	void SetAmbient(Color c, TimeValue t) override;
	void SetDiffuse(Color c, TimeValue t) override;
	void SetSpecular(Color c, TimeValue t) override;
	void SetShininess(float v, TimeValue t) override;
	void Shade(::ShadeContext& context) override;

	void Update(TimeValue t, Interval& valid)
	{
		for (int i = 0; i < NumSubTexmaps(); ++i)
		{
			Texmap* map = GetSubTexmap(i);

			if (map != NULL)
			{
				map->Update(t, valid);
			}
		}

		pblock->GetValidity(t, valid);
	}
};

template<typename Traits, typename Derived>
class FireRenderMtlBrowserEntry :
	public IMaterialBrowserEntryInfo
{
public:
	const MCHAR* GetEntryName() const override
	{
		return nullptr;
	}

	const MCHAR* GetEntryCategory() const override
	{
		return _T("MaterialsRadeon ProRender"); 
	}

	Bitmap* GetEntryThumbnail() const override
	{
		using CD = typename Derived::ClassDesc;
		return CD::Instance().GetEntryThumbnail();
	}

	bool HasEntryThumbnail() const override
	{
		using CD = typename Derived::ClassDesc;
		return CD::Instance().HasEntryThumbnail();
	}

	static FireRenderMtlBrowserEntry& Instance()
	{
		static FireRenderMtlBrowserEntry instance;
		return instance;
	}
};

template<typename Traits, typename Derived>
class FireRenderMtlClassDesc :
	public FireRenderClassDescBase
{
public:
	static FireRenderMtlClassDesc& Instance()
	{
		static FireRenderMtlClassDesc _instance;
		return _instance;
	}

	int IsPublic() override
	{
		return 1;
	}

	BOOL NeedsToSave() override
	{
		return FALSE;
	}

	HINSTANCE HInstance() override
	{
		FASSERT(fireRenderHInstance != nullptr);
		return fireRenderHInstance;
	}

	const TCHAR* InternalName() override
	{
		return Traits::InternalName();
	}

	const TCHAR * ClassName() override
	{
		return InternalName();
	}

	SClass_ID SuperClassID() override
	{
		return MATERIAL_CLASS_ID;
	}

	Class_ID ClassID() override
	{
		return Traits::ClassId();
	}

	const TCHAR* Category() override
	{
		return _T("Radeon ProRender");
	}

	void* Create(BOOL loading) override
	{
		return new Derived;
	}

	virtual FPInterface* GetInterface(Interface_ID id) override
	{
		if (id == IMATERIAL_BROWSER_ENTRY_INFO_INTERFACE)
		{
			return &FireRenderMtlBrowserEntry<Traits, Derived>::Instance();
		}

		return ClassDesc2::GetInterface(id);
	}
};

template<typename Traits, typename Derived>
class FireRenderMtl :
	public FireRenderMtlBase
{
public:
	using ClassDesc = FireRenderMtlClassDesc<Traits, Derived>;
	static ClassDesc& GetClassDesc()
	{
		return ClassDesc::Instance();
	}

	FireRenderMtl()
	{
		GetClassDesc().MakeAutoParamBlocks(this);
		FASSERT(pblock != nullptr);
	}

	IOResult Save(ISave *isave) override
	{
		IOResult res;
		isave->BeginChunk(BASECLASS_CHUNK);
		res = Mtl::Save(isave);
		if (res != IO_OK) return res;
		isave->EndChunk();
		return IO_OK;
	}

	IOResult Load(ILoad* iload) override
	{
		IOResult res;
		int id;

		while (IO_OK == (res = iload->OpenChunk()))
		{
			switch (id = iload->CurChunkID())
			{
				case BASECLASS_CHUNK:
					res = Mtl::Load(iload);
					break;
			}

			iload->CloseChunk();

			if (res != IO_OK)
			{
				return res;
			}
		}

		return IO_OK;
	}

	RefTargetHandle Clone(RemapDir &remap) override
	{
		Derived* newCopy = new Derived();
		*static_cast<Mtl*>(newCopy) = *static_cast<Mtl*>(this);

		for (int i = 0; i < NumRefs(); i++)
		{
			if (GetReference(i) && IsRealDependency(GetReference(i)))
			{
				newCopy->ReplaceReference(i, remap.CloneRef(GetReference(i)));
			}
		}

		BaseClone(this, newCopy, remap);

		return newCopy;
	}

	void Reset() override
	{
		GetClassDesc().Reset(this, TRUE);
	}

	ParamDlg* CreateParamDlg(HWND hwMtlEdit, IMtlParams* imp) override
	{
		IAutoMParamDlg* dlg = GetClassDesc().CreateParamDlgs(hwMtlEdit, imp, this);
		GetClassDesc().RestoreRolloutState();
		return dlg;
	}

	Class_ID ClassID() override
	{
		return GetClassDesc().ClassID();
	}

	void GetClassName(TSTR& s) override
	{
		s = GetClassDesc().ClassName();
	}

	int NumParamBlocks() override
	{
		return 1;
	}

	IParamBlock2* GetParamBlock(int i) override
	{
		FASSERT(i == 0);
		return pblock;
	}

	IParamBlock2* GetParamBlockByID(BlockID id) override
	{
		FASSERT(id == 0);
		return pblock;
	}

	Interval Validity(TimeValue t) override
	{
		Interval res = FOREVER;
		pblock->GetValidity(t, res);
		return res;
	}

	void BeginEditParams(IObjParam *ip, ULONG flags, Animatable *prev) override
	{
		GetClassDesc().BeginEditParams(ip, this, flags, prev);
	}

	void EndEditParams(IObjParam *ip, ULONG flags, Animatable *next) override
	{
		GetClassDesc().EndEditParams(ip, this, flags, next);
	}

	int NumSubTexmaps() override
	{
		return int(TEXMAP_MAPPING.size());
	}

	Texmap* GetSubTexmap(int i) override
	{
		FASSERT(TEXMAP_MAPPING.find(i) != TEXMAP_MAPPING.end());
		return GetFromPb<Texmap*>(pblock, TEXMAP_MAPPING[i].first);
	}

	void SetSubTexmap(int i, Texmap* m) override
	{
		FASSERT(TEXMAP_MAPPING.find(i) != TEXMAP_MAPPING.end());
		SetInPb(pblock, TEXMAP_MAPPING[i].first, m);
		pblock->GetDesc()->InvalidateUI();
	}

	MSTR GetSubTexmapSlotName(int i) override
	{
		FASSERT(TEXMAP_MAPPING.find(i) != TEXMAP_MAPPING.end());
		return TEXMAP_MAPPING[i].second;
	}

protected:
	static std::map<int, std::pair<ParamID, MCHAR*>> TEXMAP_MAPPING;
	static const int BASECLASS_CHUNK = 4000;

	FireRenderMtl& operator=(const FireRenderMtl&) = delete;
	FireRenderMtl(FireRenderMtl&) = delete;
};

class FireRenderTexBase : public Texmap
{
protected:
	IParamBlock2* pblock;
public:
	virtual void DeleteThis() override {
		delete this;
	}
	virtual RefResult NotifyRefChanged(NOTIFY_REF_CHANGED_PARAMETERS) override {
		return REF_SUCCEED;
	}
	virtual int NumRefs() override {
		return 1;
	}
	virtual RefTargetHandle GetReference(int i) override {
		FASSERT(unsigned(i) < unsigned(NumRefs()));
		return pblock;
	}
	virtual void SetReference(int i, RefTargetHandle rtarg) override {
		FASSERT(unsigned(i) < unsigned(NumRefs()));
		pblock = dynamic_cast<IParamBlock2*>(rtarg);
	}
	virtual int NumSubs() override {
		return NumRefs();
	}
	virtual Animatable* SubAnim(int i) override {
		return GetReference(i);
	}
	virtual TSTR SubAnimName(int i) override {
		FASSERT(unsigned(i) < unsigned(NumRefs()));
		return _T("Pblock");
	}
	virtual int SubNumToRefNum(int subNum) override {
		return subNum;
	}

	virtual AColor Texmap::EvalColor(ShadeContext &) override {
		return AColor(0.0, 0.0, 0.0);
	}

	virtual  Point3 EvalNormalPerturb(ShadeContext& sc) override {
		return Point3(0.0, 0.0, 0.0);
	}
};


#define BEGIN_DECLARE_FRTEX(NAME)\
class FireRender##NAME : public FireRenderTexBase { \
protected:\
	void operator=(const FireRender##NAME&) = delete; \
	FireRender##NAME(FireRender##NAME&) = delete; \
	static std::map<int, std::pair<ParamID, MCHAR*>> TEXMAP_MAPPING; \
	static const int BASECLASS_CHUNK = 4000; \
public:\
	static FireRenderClassDesc##NAME ClassDescInstance; \
	FireRender##NAME() {\
	pblock = NULL; \
	ClassDescInstance.MakeAutoParamBlocks(this); \
	FASSERT(pblock != NULL); \
	}\
	~FireRender##NAME(); \
	\
	frw::Value getShader(const TimeValue t, class MaterialParser& mtlParser); \
	\
	virtual IOResult Save(ISave *isave) override {\
		IOResult res; \
		isave->BeginChunk(BASECLASS_CHUNK); \
		res = Texmap::Save(isave); \
		if (res != IO_OK) return res; \
			isave->EndChunk(); \
		return IO_OK; \
	}\
	\
	virtual IOResult Load(ILoad* iload) override {\
		IOResult res; \
		int id; \
		while (IO_OK == (res = iload->OpenChunk())) {\
			switch (id = iload->CurChunkID()) {\
			case BASECLASS_CHUNK:\
			res = Texmap::Load(iload); \
			break; \
		}\
		iload->CloseChunk(); \
		if (res != IO_OK) {\
			return res; \
		}\
	}\
	return IO_OK; \
	}\
	\
	virtual RefTargetHandle Clone(RemapDir &remap) override {\
	FireRender##NAME* newCopy = new FireRender##NAME(); \
	*((Mtl*)newCopy) = *((Mtl*)this); \
			for (int i = 0; i < NumRefs(); i++) {\
			if (GetReference(i) && IsRealDependency(GetReference(i))) {\
				newCopy->ReplaceReference(i, remap.CloneRef(GetReference(i))); \
			}\
			}\
			BaseClone(this, newCopy, remap); \
			return newCopy; \
	}\
	\
	virtual void Reset() override {\
		ClassDescInstance.Reset(this, TRUE); \
	}\
	\
	virtual ParamDlg* CreateParamDlg(HWND hwMtlEdit, IMtlParams* imp) override {\
		IAutoMParamDlg* dlg = ClassDescInstance.CreateParamDlgs(hwMtlEdit, imp, this); \
		ClassDescInstance.RestoreRolloutState(); \
		return dlg; \
	}\
	\
	virtual Class_ID ClassID() override {\
		return ClassDescInstance.ClassID(); \
	}\
	\
	virtual void GetClassName(TSTR& s) override {\
		s = ClassDescInstance.ClassName(); \
	}\
	\
	virtual int NumParamBlocks() override {\
		return 1; \
	}\
	\
	virtual IParamBlock2* GetParamBlock(int i) override {\
		FASSERT(i == 0); \
		return pblock; \
	}\
	\
	virtual IParamBlock2* GetParamBlockByID(BlockID id) override {\
		FASSERT(id == 0); \
		return pblock; \
	}\
	\
	virtual Interval Validity(TimeValue t) override {\
		Interval res = FOREVER; \
		pblock->GetValidity(t, res); \
		return res; \
	}\
	\
	void BeginEditParams(IObjParam *ip, ULONG flags, Animatable *prev) override {\
		ClassDescInstance.BeginEditParams(ip, this, flags, prev); \
	}\
	\
	void EndEditParams(IObjParam *ip, ULONG flags, Animatable *next) override {\
		ClassDescInstance.EndEditParams(ip, this, flags, next); \
	}\
	\
	virtual void Update(TimeValue t, Interval& valid) override; \
	\
	virtual int NumSubTexmaps() override {\
		return int(TEXMAP_MAPPING.size()); \
	}\
	\
	virtual Texmap* GetSubTexmap(int i) override {\
		FASSERT(TEXMAP_MAPPING.find(i) != TEXMAP_MAPPING.end()); \
		return GetFromPb<Texmap*>(pblock, TEXMAP_MAPPING[i].first); \
	}\
	\
	virtual void SetSubTexmap(int i, Texmap* m) override {\
		FASSERT(TEXMAP_MAPPING.find(i) != TEXMAP_MAPPING.end()); \
		SetInPb(pblock, TEXMAP_MAPPING[i].first, m); \
		pblock->GetDesc()->InvalidateUI(); \
	}\
	\
	virtual MSTR GetSubTexmapSlotName(int i) override {\
		FASSERT(TEXMAP_MAPPING.find(i) != TEXMAP_MAPPING.end()); \
		return TEXMAP_MAPPING[i].second; \
	}

#define END_DECLARE_FRTEX(NAME)\
};


#define GETSHADERCOLOR_USE_AMOUNT(NAME, DIRECT, USEMAP, TEXMAP, MAPAMOPUNT)\
	const Color NAME##_direct = FireRender::GetFromPb<Color>(pb, DIRECT, this->mT); \
	Texmap* NAME##_texmap = FireRender::GetFromPb<bool>(pb, USEMAP, this->mT) ? FireRender::GetFromPb<Texmap*>(pb, TEXMAP) : NULL; \
	frw::Value NAME(NAME##_direct.r, NAME##_direct.g, NAME##_direct.b); \
if (NAME##_texmap)\
	NAME = materialSystem.ValueMul(GetFromPb<float>(pb, MAPAMOPUNT, this->mT), createMap(NAME##_texmap, 0));

#define GETSHADERMAP_USE_AMOUNT(NAME, USEMAP, TEXMAP, MAPAMOPUNT)\
	Texmap* NAME##_texmap = GetFromPb<bool>(pb, USEMAP, this->mT) ? GetFromPb<Texmap*>(pb, TEXMAP) : NULL; \
	frw::Value NAME;\
	frw::Value NAME##_mapAmount;\
	if (NAME##_texmap)\
	{\
		NAME##_mapAmount = FireRender::GetFromPb<float>(pb, MAPAMOPUNT, this->mT);\
		NAME = materialSystem.ValueMul(NAME##_mapAmount, createMap(NAME##_texmap, 0));\
	}

#define GETSHADERFLOAT_USE_AMOUNT(NAME, DIRECT, USEMAP, TEXMAP, MAPAMOPUNT)\
	const float NAME##_direct = FireRender::GetFromPb<float>(pb, DIRECT, this->mT); \
	Texmap* NAME##_texmap = FireRender::GetFromPb<bool>(pb, USEMAP, this->mT) ? GetFromPb<Texmap*>(pb, TEXMAP) : NULL; \
	frw::Value NAME(NAME##_direct, NAME##_direct, NAME##_direct); \
if (NAME##_texmap)\
	NAME = materialSystem.ValueMul(GetFromPb<float>(pb, MAPAMOPUNT, this->mT), createMap(NAME##_texmap, 0));

#define GETSHADERCOLOR(MP, NAME, DIRECT, TEXMAP, T)\
	const Color NAME##_direct = FireRender::GetFromPb<Color>(pb, DIRECT, T); \
	Texmap* NAME##_texmap = FireRender::GetFromPb<Texmap*>(pb, TEXMAP); \
	frw::Value NAME(NAME##_direct.r, NAME##_direct.g, NAME##_direct.b); \
if (NAME##_texmap)\
	NAME = MP.createMap(NAME##_texmap, 0);

#define GETSHADERFLOAT(MP, NAME, DIRECT, TEXMAP, T)\
	const float NAME##_direct = FireRender::GetFromPb<float>(pb, DIRECT, T); \
	Texmap* NAME##_texmap = GetFromPb<Texmap*>(pb, TEXMAP); \
	frw::Value NAME(NAME##_direct, NAME##_direct, NAME##_direct); \
if (NAME##_texmap)\
	NAME = MP.createMap(NAME##_texmap, MAP_FLAG_NOGAMMA);

#define GETSHADERCOLOR_NOMAP(NAME, DIRECT, T)\
	const Color NAME##_direct = FireRender::GetFromPb<Color>(pb, DIRECT, T); \
	frw::Value NAME(NAME##_direct.r, NAME##_direct.g, NAME##_direct.b);

#define GETSHADERFLOAT_NOMAP(NAME, DIRECT, T)\
	const float NAME##_direct = FireRender::GetFromPb<float>(pb, DIRECT, T); \
	frw::Value NAME(NAME##_direct, NAME##_direct, NAME##_direct);

FIRERENDER_NAMESPACE_END;
