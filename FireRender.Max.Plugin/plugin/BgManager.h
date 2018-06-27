/*********************************************************************************************************************************
* Radeon ProRender for 3ds Max plugin
* Copyright (c) 2017 AMD
* All Rights Reserved
*
* Property Manager specialized in handling Background related properties (IBL, Sun & Sky, etc..)
*
*********************************************************************************************************************************/

#pragma once

#include <max.h>
#include <guplib.h>
#include <notify.h>
#include <iparamb2.h>
#include "Common.h"
#include "frWrap.h"
#include "frScope.h"
#include <string>
#include <map>
#include <set>
#include <algorithm>
#include <string> 
#include "plugin/ManagerBase.h"

// Forward declarations
struct SkyRgbFloat32;

FIRERENDER_NAMESPACE_BEGIN

#define BGMANAGER_VERSION 1

#define CUSTOMLP_WC _T("CustomLocationPickerControl")
#define XXS_DOUBLEBUFFER (0x0001)
#define WM_LATLONGSET WM_USER + 100
typedef std::pair<Point2, float> LATLONGSETDATA;
#define WM_LOCSEARCHRESULT WM_USER + 200

#define PARAM_BG_FIRST 509

#define PROP_SENDER_SUNWIDGET 100

enum FRBackgroundType
{
	FRBackgroundType_IBL,
	FRBackgroundType_SunSky
};

enum FRBackgroundSkyType
{
	FRBackgroundSkyType_Analytical,
	FRBackgroundSkyType_DateTimeLocation
};

enum BgParameter : ParamID {
	PARAM_BG_USE = PARAM_BG_FIRST,
	PARAM_BG_TYPE, // FRBackgroundType_IBL, FRBackgroundType_SunSky
	PARAM_BG_IBL_SOLIDCOLOR,
	PARAM_BG_IBL_INTENSITY,
	PARAM_BG_IBL_MAP,
	PARAM_BG_IBL_BACKPLATE,
	PARAM_BG_IBL_REFLECTIONMAP,
	PARAM_BG_IBL_REFRACTIONMAP,
	PARAM_BG_IBL_MAP_USE,
	PARAM_BG_IBL_BACKPLATE_USE,
	PARAM_BG_IBL_REFLECTIONMAP_USE,
	PARAM_BG_IBL_REFRACTIONMAP_USE,

	PARAM_BG_SKY_TYPE, // FRBackgroundSkyType_Analytical, FRBackgroundSkyType_DateTimeLocation
	PARAM_BG_SKY_AZIMUTH,
	PARAM_BG_SKY_ALTITUDE,
	PARAM_BG_SKY_ALBEDO,
	PARAM_BG_SKY_TURBIDITY,
	PARAM_BG_SKY_HOURS,
	PARAM_BG_SKY_MINUTES,
	PARAM_BG_SKY_SECONDS,
	PARAM_BG_SKY_DAY,
	PARAM_BG_SKY_MONTH,
	PARAM_BG_SKY_YEAR,
	PARAM_BG_SKY_TIMEZONE,
	PARAM_BG_SKY_LATITUDE,
	PARAM_BG_SKY_LONGITUDE,
	PARAM_BG_SKY_DAYLIGHTSAVING,

	PARAM_BG_SKY_HAZE,
	PARAM_BG_SKY_GLOW,
	PARAM_BG_SKY_REDBLUE_SHIFT,
	PARAM_BG_SKY_GROUND_COLOR,
	PARAM_BG_SKY_DISCSIZE,
	PARAM_BG_SKY_HORIZON_HEIGHT,
	PARAM_BG_SKY_HORIZON_BLUR,
	PARAM_BG_SKY_INTENSITY,

	// GROUND
	PARAM_GROUND_ACTIVE,
	PARAM_GROUND_RADIUS,
	PARAM_GROUND_GROUND_HEIGHT,
	PARAM_GROUND_SHADOWS,
	PARAM_GROUND_REFLECTIONS_STRENGTH,
	PARAM_GROUND_REFLECTIONS,
	PARAM_GROUND_REFLECTIONS_COLOR,
	PARAM_GROUND_REFLECTIONS_ROUGHNESS,
	
	PARAM_BG_SKY_BACKPLATE,
	PARAM_BG_SKY_REFLECTIONMAP,
	PARAM_BG_SKY_REFRACTIONMAP,
	PARAM_BG_SKY_BACKPLATE_USE,
	PARAM_BG_SKY_REFLECTIONMAP_USE,
	PARAM_BG_SKY_REFRACTIONMAP_USE,
	
	PARAM_BG_ENABLEALPHA,
	
	PARAM_BG_SKY_FILTER_COLOR,
	PARAM_BG_SKY_SHADOW_SOFTNESS,
	PARAM_BG_SKY_SATURATION,

	PARAM_BG_SKY_GROUND_ALBEDO,
	
	PARAM_BG_LAST
};

class BgManagerMax : public GUP, public ManagerMaxBase, public ReferenceTarget
{
public:
	static BgManagerMax TheManager;

public:
	BgManagerMax();
	~BgManagerMax();

	// GUP Methods
	DWORD Start() override;
	void Stop() override;

	void DeleteThis() override;

	static ClassDesc* GetClassDesc();

	IOResult Save(ISave *isave) override;
	IOResult Load(ILoad *iload) override;

	IParamBlock2 *pblock2 = 0;

private:
	RefTargetHandle m_EnvironmentNodeMonitor = 0;
	RefTargetHandle m_AnalyticalSkyNodeMonitor = 0; // aka Sun

public:
	enum
	{
		TEX_IBLMAP = 1,
		TEX_IBLBACKPLATE,
		TEX_IBLREFROVERRIDE,
		TEX_IBLREFLOVERRIDE,
		TEX_SKYBACKPLATE,
		TEX_SKYREFROVERRIDE,
		TEX_SKYREFLOVERRIDE,
		OBJ_SUNOBJECT, // aka analytical sky
		OBJ_ENVIRONMENT,
		TEX_LASTREF
	};

	BOOL GetProperty(int param, Texmap*& value) override;
	BOOL GetProperty(int param, float& value) override;
	BOOL GetProperty(int param, int& value) override;
	BOOL GetProperty(int param, Color& value) override;

	BOOL SetProperty(int param, Texmap* value, int sender = -1) override;
	BOOL SetProperty(int param, const float &value, int sender = -1) override;
	BOOL SetProperty(int param, int value, int sender = -1) override;
	BOOL SetProperty(int param, const Color &value, int sender = -1) override;

	void FactoryReset();

	void ResetToDefaults() override;

private:
	RefTargetHandle iblMap = 0;
	RefTargetHandle iblBackplate = 0;
	RefTargetHandle iblReflOverride = 0;
	RefTargetHandle iblRefrOverride = 0;
	RefTargetHandle skyBackplate = 0;
	RefTargetHandle skyReflOverride = 0;
	RefTargetHandle skyRefrOverride = 0;

	bool updatingSunFromProperty = false;

#if MAX_RELEASE > MAX_RELEASE_R16
	virtual RefResult NotifyRefChanged(const Interval& changeInt,
		RefTargetHandle hTarget,
		PartID& partID,
		RefMessage message,
		BOOL propagate);
#else
	virtual RefResult NotifyRefChanged(Interval changeInt,
		RefTargetHandle hTarget,
		PartID& partID,
		RefMessage message);
#endif

	virtual int NumRefs();
	virtual RefTargetHandle GetReference(int i);
	virtual BOOL ShouldPersistWeakRef(RefTargetHandle rtarg) override
	{
		UNUSED_PARAM(rtarg);
		return TRUE;
	}

	int NumParamBlocks() override
	{
		return 1;
	}

	IParamBlock2* GetParamBlock(int i) override
	{
		return i == 0 ? pblock2 : NULL;
	}

	IParamBlock2* GetParamBlockByID(BlockID id) override
	{
		return (pblock2->ID() == id) ? pblock2 : NULL;
	}

private:
	virtual void SetReference(int i, RefTargetHandle rtarg);

private:
	//////////////////////////////////////////////////////////////////////////
	// Optimization for building sky hdr
	//
	Point3 prev_sun_direction = Point3(0.f, 0.f, 0.f);
	float prev_sun_altitude = 0.f;
	float prev_haze = 0.f;
	Color prev_ground_color = Color(0.f, 0.f, 0.f);
	Color prev_ground_albedo = Color(0.f, 0.f, 0.f);
	Color prev_filter_color = Color(0.f, 0.f, 0.f);
	float prev_sun_disk_scale = 0.f;
	float prev_saturation = 0.f;
	SkyRgbFloat32 *mSkyBuffer = 0;
	frw::Image mSkyImage;

public:
	// Compute sun angular position, in degrees
	void GetSunPosition(IParamBlock2 *pb, const TimeValue &t, float& outAzimuth, float& outAltitude);
	frw::Image GenerateSky(frw::Scope &scope, IParamBlock2 *pb, const TimeValue &t, float skyIntensity);

	// location picker custom static control
	static void CustomLocationPickerRegister();
	static void CustomLocationPickerUnregister();
	static LRESULT CALLBACK CustomLPProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	
private:
	// world cities database (for fast location picking)
	struct CWorldCity
	{
		std::string name;
		float latitude;
		float longitude;
		float UTCOffset;
		int countryIdx;
	};

	std::vector<std::string> mCountriesData;
	std::vector<CWorldCity> mCitiesData;

public:
	enum
	{
		TZTYPE_BOX,
		TZTYPE_MULTIPOLYGON
	};

	typedef struct tzrecord_t
	{
		std::string name;
		int type;
		std::vector<Point2> verts;
		float UTCoffset;
	} TZRecord;
	
	std::vector<TZRecord> mTzDb;

	inline const std::vector<TZRecord> &GetTzDatabase() { return mTzDb; }

private:
	void BuildWorldCitiesDatabase();
	void AdjustDaylightSavingTime(int& hours, int& day, int& month, int& year) const;

public:
	void LookUpWorldCity(const std::string &searchTerm, HWND inquirer);
	std::string GetCityDesc(size_t i);
	void GetCityData(size_t i, const float** lat, const float** longi, const float** utcoffset);

	float GetWorldUTCOffset(const float& lati, const float &longi);
	
	// viewport objects
	INode *GetEnvironmentNode();
	INode *GetAnalyticalSunNode();
	INode *CreateEnvironmentNode();
	INode *CreateAnalyticalSunNode();
	void DeleteEnvironmentNode();
	void DeleteAnalyticalSunNode();
	// just in case
	INode *FindEnvironmentNode();
	INode *FindAnalyticalSunNode();

	// set the viewport objects when created by something else (eg a file loader)
	void SetEnvironmentNode(INode *node);	
	void SetAnalyticalSunNode(INode *node);

	// called when an external entity (eg UI) modifies the sun's location
	// so we can refresh the position of the widget accordingly
	void UpdateAnalyticalSunNode();
};

FIRERENDER_NAMESPACE_END
