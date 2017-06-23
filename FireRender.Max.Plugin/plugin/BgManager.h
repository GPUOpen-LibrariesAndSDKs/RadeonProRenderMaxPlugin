/*********************************************************************************************************************************
* Radeon ProRender for 3ds Max plugin
* Copyright (c) 2017 AMD
* All Rights Reserved
*
* Property Manager specialized in handling Background related properties (IBL, Sun & Sky, etc..)
*
* SunPositionCalculator:
* This work is derived from the SPA Algorithm - U.S. Department of Energy (DOE)/NREL/ALLIANCE
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

FIRERENDER_NAMESPACE_BEGIN;

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
	INode *mEnvironmentNode = 0;
	INode *mAnalyticalSkyNode = 0;

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
		OBJ_SUNOBJECT,
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

	void ResetToDefaults() override
	{
		mEnvironmentNode = 0;
		mAnalyticalSkyNode = 0;

		if (pblock2)
			pblock2->ResetAll();

		prev_sun_direction = Point3(0.f, 0.f, 0.f);
		prev_sun_altitude = 0.f;
		prev_haze = 0.f;
		prev_ground_color = Color(0.f, 0.f, 0.f);
		prev_ground_albedo = Color(0.f, 0.f, 0.f);
		prev_filter_color = Color(0.f, 0.f, 0.f);
		prev_sun_disk_scale = 0.f;
		prev_saturation = 0.f;

		delete[] mSkyBuffer;
		mSkyBuffer = 0;
	}

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
	frw::Image GenerateSky(frw::Scope &scope, IParamBlock2 *pb, const TimeValue &t);

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
	void SetEnvironmentNode(INode *node)
	{
		mEnvironmentNode = node;
	}
	
	void SetAnalyticalSunNode(INode *node)
	{
		ReplaceReference(OBJ_SUNOBJECT, node);
		mAnalyticalSkyNode = node;
	}

	// called when an external entity (eg UI) modifies the sun's location
	// so we can refresh the position of the widget accordingly
	void UpdateAnalyticalSunNode();
};

class SunPositionCalculator
{
public:
	enum {
		SPA_ZA,           //calculate zenith and azimuth
		SPA_ZA_INC,       //calculate zenith, azimuth, and incidence
		SPA_ZA_RTS,       //calculate zenith, azimuth, and sun rise/transit/set values
		SPA_ALL,          //calculate all SPA output values
	};

	int year;            // 4-digit year,      valid range: -2000 to 6000, error code: 1
	int month;           // 2-digit month,         valid range: 1 to  12,  error code: 2
	int day;             // 2-digit day,           valid range: 1 to  31,  error code: 3
	int hour;            // Observer local hour,   valid range: 0 to  24,  error code: 4
	int minute;          // Observer local minute, valid range: 0 to  59,  error code: 5
	double second;       // Observer local second, valid range: 0 to <60,  error code: 6

	double delta_ut1;    // Fractional second difference between UTC and UT which is used
						 // to adjust UTC for earth's irregular rotation rate and is derived
						 // from observation only and is reported in this bulletin:
						 // http://maia.usno.navy.mil/ser7/ser7.dat,
						 // where delta_ut1 = DUT1
						 // valid range: -1 to 1 second (exclusive), error code 17

	double delta_t;      // Difference between earth rotation time and terrestrial time
						 // It is derived from observation only and is reported in this
						 // bulletin: http://maia.usno.navy.mil/ser7/ser7.dat,
						 // where delta_t = 32.184 + (TAI-UTC) - DUT1
						 // valid range: -8000 to 8000 seconds, error code: 7

	double timezone;     // Observer time zone (negative west of Greenwich)
						 // valid range: -18   to   18 hours,   error code: 8

	double longitude;    // Observer longitude (negative west of Greenwich)
						 // valid range: -180  to  180 degrees, error code: 9

	double latitude;     // Observer latitude (negative south of equator)
						 // valid range: -90   to   90 degrees, error code: 10

	double elevation;    // Observer elevation [meters]
						 // valid range: -6500000 or higher meters,    error code: 11

	double pressure;     // Annual average local pressure [millibars]
						 // valid range:    0 to 5000 millibars,       error code: 12

	double temperature;  // Annual average local temperature [degrees Celsius]
						 // valid range: -273 to 6000 degrees Celsius, error code; 13

	double slope;        // Surface slope (measured from the horizontal plane)
						 // valid range: -360 to 360 degrees, error code: 14

	double azm_rotation; // Surface azimuth rotation (measured from south to projection of
						 //     surface normal on horizontal plane, negative east)
						 // valid range: -360 to 360 degrees, error code: 15

	double atmos_refract;// Atmospheric refraction at sunrise and sunset (0.5667 deg is typical)
						 // valid range: -5   to   5 degrees, error code: 16

	int function;        // Switch to choose functions for desired output (from enumeration)

						 //-----------------Intermediate OUTPUT VALUES--------------------

	double jd;          //Julian day
	double jc;          //Julian century

	double jde;         //Julian ephemeris day
	double jce;         //Julian ephemeris century
	double jme;         //Julian ephemeris millennium

	double l;           //earth heliocentric longitude [degrees]
	double b;           //earth heliocentric latitude [degrees]
	double r;           //earth radius vector [Astronomical Units, AU]

	double theta;       //geocentric longitude [degrees]
	double beta;        //geocentric latitude [degrees]

	double x0;          //mean elongation (moon-sun) [degrees]
	double x1;          //mean anomaly (sun) [degrees]
	double x2;          //mean anomaly (moon) [degrees]
	double x3;          //argument latitude (moon) [degrees]
	double x4;          //ascending longitude (moon) [degrees]

	double del_psi;     //nutation longitude [degrees]
	double del_epsilon; //nutation obliquity [degrees]
	double epsilon0;    //ecliptic mean obliquity [arc seconds]
	double epsilon;     //ecliptic true obliquity  [degrees]

	double del_tau;     //aberration correction [degrees]
	double lamda;       //apparent sun longitude [degrees]
	double nu0;         //Greenwich mean sidereal time [degrees]
	double nu;          //Greenwich sidereal time [degrees]

	double alpha;       //geocentric sun right ascension [degrees]
	double delta;       //geocentric sun declination [degrees]

	double h;           //observer hour angle [degrees]
	double xi;          //sun equatorial horizontal parallax [degrees]
	double del_alpha;   //sun right ascension parallax [degrees]
	double delta_prime; //topocentric sun declination [degrees]
	double alpha_prime; //topocentric sun right ascension [degrees]
	double h_prime;     //topocentric local hour angle [degrees]

	double e0;          //topocentric elevation angle (uncorrected) [degrees]
	double del_e;       //atmospheric refraction correction [degrees]
	double e;           //topocentric elevation angle (corrected) [degrees]

	double eot;         //equation of time [minutes]
	double srha;        //sunrise hour angle [degrees]
	double ssha;        //sunset hour angle [degrees]
	double sta;         //sun transit altitude [degrees]

						//---------------------Final OUTPUT VALUES------------------------

	double zenith;       //topocentric zenith angle [degrees]
	double azimuth_astro;//topocentric azimuth angle (westward from south) [for astronomers]
	double azimuth;      //topocentric azimuth angle (eastward from north) [for navigators and solar radiation]
	double incidence;    //surface incidence angle [degrees]

	double suntransit;   //local sun transit time (or solar noon) [fractional hour]
	double sunrise;      //local sunrise time (+/- 30 seconds) [fractional hour]
	double sunset;       //local sunset time (+/- 30 seconds) [fractional hour]

	void calculate_eot_and_sun_rise_transit_set();
	void calculate_geocentric_sun_right_ascension_and_declination();

	int validate_inputs();

	int calculate();
};

FIRERENDER_NAMESPACE_END;
