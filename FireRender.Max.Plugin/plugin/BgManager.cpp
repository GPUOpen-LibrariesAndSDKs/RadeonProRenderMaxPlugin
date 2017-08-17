/*********************************************************************************************************************************
* Radeon ProRender for 3ds Max plugin
* Copyright (c) 2017 AMD
* All Rights Reserved
*
* Property Manager specialized in handling Background related properties (IBL, Sun & Sky, etc..)
*********************************************************************************************************************************/

#include <math.h>
#include "BgManager.h"
#include "resource.h"
#include "plugin/FireRenderEnvironment.h"
#include "plugin/FireRenderAnalyticalSun.h"

#include "plugin/ParamBlock.h"
#include "plugin/SkyGen.h"
#include "utils/utils.h"
#include <wingdi.h>

#include "utils/Thread.h"

#include "AssetManagement/IAssetAccessor.h"
#include "assetmanagement/AssetType.h"
#include "Assetmanagement/iassetmanager.h"

extern HINSTANCE hInstance;

FIRERENDER_NAMESPACE_BEGIN;

BgManagerMax BgManagerMax::TheManager;

#define SKYENV_WIDTH 2048
#define SKYENV_HEIGHT 1024

class BgManagerMaxClassDesc : public ClassDesc2
{
public:
	int             IsPublic() { return FALSE; }
	void*           Create(BOOL loading) { return &BgManagerMax::TheManager; }
	const TCHAR*    ClassName() { return _T("RPRBgManager"); }
	SClass_ID       SuperClassID() { return GUP_CLASS_ID; }
	Class_ID        ClassID() { return BGMANAGER_MAX_CLASSID; }
	const TCHAR*    Category() { return _T(""); }
};

static BgManagerMaxClassDesc BgManagerMaxCD;

ClassDesc* BgManagerMax::GetClassDesc()
{
	return &BgManagerMaxCD;
}

namespace
{
	static ParamBlockDesc2 paramBlock(0, _T("BgManagerPbDesc"), 0, &BgManagerMaxCD, P_AUTO_CONSTRUCT,
		0, //for P_AUTO_CONSTRUCT

		PARAM_BG_IBL_MAP, _T("backgroundMap"), TYPE_TEXMAP, 0, 0,
		p_subtexno, BgManagerMax::TEX_IBLMAP, PB_END,

		PARAM_BG_IBL_BACKPLATE, _T("backplateMap"), TYPE_TEXMAP, 0, 0,
		p_subtexno, BgManagerMax::TEX_IBLBACKPLATE, PB_END,

		PARAM_BG_IBL_REFLECTIONMAP, _T("reflectionMap"), TYPE_TEXMAP, 0, 0,
		p_subtexno, BgManagerMax::TEX_IBLREFLOVERRIDE, PB_END,

		PARAM_BG_IBL_REFRACTIONMAP, _T("refractionMap"), TYPE_TEXMAP, 0, 0,
		p_subtexno, BgManagerMax::TEX_IBLREFROVERRIDE, PB_END,

		PARAM_BG_USE, _T("backgroundOverride"), TYPE_BOOL, 0, 0,
		p_default, FALSE, PB_END,

		PARAM_BG_TYPE, _T("backgroundType"), TYPE_INT, 0, 0,
		p_range, FRBackgroundType_IBL, FRBackgroundType_SunSky, p_default, FRBackgroundType_IBL, PB_END,

		PARAM_BG_IBL_SOLIDCOLOR, _T("backgroundColor"), TYPE_RGBA, 0, IDS_STRING237,
		p_default, Color(0.5f, 0.5f, 0.5f), PB_END,

		PARAM_BG_IBL_INTENSITY, _T("backgroundIntensity"), TYPE_FLOAT, 0, IDS_STRING238,
		p_default, 1.0, p_range, 0.0, FLT_MAX, PB_END,

		PARAM_BG_IBL_MAP_USE, _T("useBackgroundMap"), TYPE_BOOL, 0, 0,
		p_default, FALSE, PB_END,

		PARAM_BG_IBL_BACKPLATE_USE, _T("useBackgroundBackplateMap"), TYPE_BOOL, 0, 0,
		p_default, FALSE, PB_END,

		PARAM_BG_IBL_REFLECTIONMAP_USE, _T("useBackgroundReflectionMap"), TYPE_BOOL, 0, 0,
		p_default, FALSE, PB_END,

		PARAM_BG_IBL_REFRACTIONMAP_USE, _T("useBackgroundRefractionMap"), TYPE_BOOL, 0, 0,
		p_default, FALSE, PB_END,

		PARAM_BG_SKY_TYPE, _T("backgroundSkyType"), TYPE_INT, 0, 0,
		p_range, FRBackgroundSkyType_Analytical, FRBackgroundSkyType_DateTimeLocation,
		p_default, FRBackgroundSkyType_DateTimeLocation, PB_END,

		PARAM_BG_SKY_AZIMUTH, _T("backgroundSkyAzimuth"), TYPE_FLOAT, 0, IDS_STRING239,
		p_default, 0.0, p_range, 0.0, 360.0, PB_END,

		PARAM_BG_SKY_ALTITUDE, _T("backgroundSkyAltitude"), TYPE_FLOAT, 0, IDS_STRING240,
		p_default, 45.0, p_range, 0.0, 180.0, PB_END,

		PARAM_BG_SKY_HOURS, _T("backgroundSkyHours"), TYPE_INT, 0, IDS_STRING258,
		p_default, 12, p_range, 0, 24, PB_END,

		PARAM_BG_SKY_MINUTES, _T("backgroundSkyMinutes"), TYPE_INT, 0, IDS_STRING259,
		p_default, 0, p_range, 0, 59, PB_END,

		PARAM_BG_SKY_SECONDS, _T("backgroundSkySeconds"), TYPE_INT, 0, IDS_STRING260,
		p_default, 0, p_range, 0, 59, PB_END,

		PARAM_BG_SKY_DAY, _T("backgroundSkyDay"), TYPE_INT, 0, IDS_STRING261,
		p_default, 1, p_range, 1, 31, PB_END,

		PARAM_BG_SKY_MONTH, _T("backgroundSkyMonth"), TYPE_INT, 0, IDS_STRING262,
		p_default, 1, p_range, 1, 12, PB_END,

		PARAM_BG_SKY_YEAR, _T("backgroundSkyYear"), TYPE_INT, 0, IDS_STRING263,
		p_default, 2016, p_range, -2000, 6000, PB_END,

		PARAM_BG_SKY_TIMEZONE, _T("backgroundSkyTimezone"), TYPE_FLOAT, 0, IDS_STRING241,
		p_default, 0, p_range, -18.f, 18.f, PB_END,

		PARAM_BG_SKY_LATITUDE, _T("backgroundSkyLatitude"), TYPE_FLOAT, 0, IDS_STRING242,
		p_default, 38.4237, p_range, -90.0, 90.0, PB_END,

		PARAM_BG_SKY_LONGITUDE, _T("backgroundSkyLongitude"), TYPE_FLOAT, 0, IDS_STRING243,
		p_default, 27.1428, p_range, -180.0, 180.0, PB_END,

		PARAM_BG_SKY_DAYLIGHTSAVING, _T("backgroundSkyDaylightSaving"), TYPE_BOOL, 0, IDS_STRING244,
		p_default, TRUE, PB_END,

		PARAM_BG_SKY_HAZE, _T("backgroundSkyHaze"), TYPE_FLOAT, 0, IDS_STRING245,
		p_default, 0.2, p_range, 0.0, 10.0, PB_END,

		PARAM_BG_SKY_GLOW, _T("backgroundSkyGlow"), TYPE_FLOAT, P_INVISIBLE | P_OBSOLETE, IDS_STRING246,
		p_default, 1.0, p_range, 0.0, 10.0, PB_END,

		PARAM_BG_SKY_REDBLUE_SHIFT, _T("backgroundredBlueShift"), TYPE_FLOAT, P_INVISIBLE | P_OBSOLETE, IDS_STRING247,
		p_default, 0.0, p_range, -1.0, 1.0, PB_END,

		PARAM_BG_SKY_GROUND_COLOR, _T("backgroundSkyGroundColor"), TYPE_RGBA, 0, IDS_STRING248,
		p_default, Color(0.4f, 0.4f, 0.4f), PB_END,

		PARAM_BG_SKY_DISCSIZE, _T("backgroundSkySunDisc"), TYPE_FLOAT, 0, IDS_STRING249,
		p_default, 0.5, p_range, 0.0, 10.0, PB_END,

		PARAM_BG_SKY_HORIZON_HEIGHT, _T("backgroundSkyHorizonHeight"), TYPE_FLOAT, P_INVISIBLE | P_OBSOLETE, IDS_STRING250,
		p_default, 0.001, p_range, FLT_MIN, FLT_MAX, PB_END,

		PARAM_BG_SKY_HORIZON_BLUR, _T("backgroundSkyHorizonBlur"), TYPE_FLOAT, P_INVISIBLE | P_OBSOLETE, IDS_STRING251,
		p_default, 0.1, p_range, 0.0, 1.0, PB_END,

		PARAM_BG_SKY_INTENSITY, _T("backgroundSkyIntensity"), TYPE_FLOAT, 0, IDS_STRING252,
		p_default, 1.5, p_range, 0.0, FLT_MAX, PB_END,

		PARAM_BG_SKY_SATURATION, _T("backgroundSkySaturation"), TYPE_FLOAT, 0, IDS_STRING267,
		p_default, 0.5, p_range, 0.0, 1.0, PB_END,

		PARAM_GROUND_ACTIVE, _T("groundActive"), TYPE_BOOL, 0, 0,
		p_default, FALSE, PB_END,

		PARAM_GROUND_RADIUS, _T("groundRadius"), TYPE_FLOAT, 0, IDS_STRING253,
		p_default, 0.0, p_range, 0.0, FLT_MAX, PB_END,

		PARAM_GROUND_GROUND_HEIGHT, _T("groundHeight"), TYPE_FLOAT, 0, IDS_STRING254,
		p_default, 0.0, p_range, -FLT_MAX, FLT_MAX, PB_END,

		PARAM_GROUND_SHADOWS, _T("groundShadows"), TYPE_BOOL, 0, 0,
		p_default, FALSE, PB_END,

		PARAM_GROUND_REFLECTIONS_STRENGTH, _T("groundReflectionsStrength"), TYPE_FLOAT, 0, IDS_STRING255,
		p_default, 0.5, p_range, 0.0, 1.0, PB_END,

		PARAM_GROUND_REFLECTIONS, _T("groundReflections"), TYPE_BOOL, 0, 0,
		p_default, FALSE, PB_END,

		PARAM_GROUND_REFLECTIONS_COLOR, _T("groundReflectionsColor"), TYPE_RGBA, 0, IDS_STRING256,
		p_default, Color(0.5f, 0.5f, 0.5f), PB_END,

		PARAM_GROUND_REFLECTIONS_ROUGHNESS, _T("groundReflectionsRoughness"), TYPE_FLOAT, 0, IDS_STRING257,
		p_default, 0.0, p_range, 0.0, FLT_MAX, PB_END,

		PARAM_BG_SKY_BACKPLATE, _T("skyBackplateMap"), TYPE_TEXMAP, 0, 0,
		p_subtexno, BgManagerMax::TEX_SKYBACKPLATE, PB_END,

		PARAM_BG_SKY_REFLECTIONMAP, _T("skyReflectionMap"), TYPE_TEXMAP, 0, 0,
		p_subtexno, BgManagerMax::TEX_SKYREFLOVERRIDE, PB_END,

		PARAM_BG_SKY_REFRACTIONMAP, _T("skyRefractionMap"), TYPE_TEXMAP, 0, 0,
		p_subtexno, BgManagerMax::TEX_SKYREFROVERRIDE, PB_END,

		PARAM_BG_SKY_BACKPLATE_USE, _T("useSkyBackplateMap"), TYPE_BOOL, 0, 0,
		p_default, FALSE, PB_END,

		PARAM_BG_SKY_REFLECTIONMAP_USE, _T("useSkyReflectionMap"), TYPE_BOOL, 0, 0,
		p_default, FALSE, PB_END,

		PARAM_BG_SKY_REFRACTIONMAP_USE, _T("useSkyRefractionMap"), TYPE_BOOL, 0, 0,
		p_default, FALSE, PB_END,

		PARAM_BG_ENABLEALPHA, _T("enableBackgroundAlpha"), TYPE_BOOL, 0, 0,
		p_default, FALSE, PB_END,

		PARAM_BG_SKY_FILTER_COLOR, _T("backgroundSkyFilterColor"), TYPE_RGBA, 0, 0,
		p_default, Color(1.0f, 1.0f, 1.0f), PB_END,

		PARAM_BG_SKY_GROUND_ALBEDO, _T("backgroundSkyGroundAlbedo"), TYPE_RGBA, 0, IDS_STRING247,
		p_default, Color(0.f, 0.f, 0.f), PB_END,
		
		p_end
	);
};


BgManagerMax::BgManagerMax()
{
	ResetToDefaults();
}

BgManagerMax::~BgManagerMax()
{
}

static void NotifyProc(void *param, NotifyInfo *info)
{
	BgManagerMax *bg = reinterpret_cast<BgManagerMax*>(param);

	if (info->intcode == NOTIFY_FILE_PRE_OPEN)
	{
		bg->ResetToDefaults();
	}
	else if (info->intcode == NOTIFY_SYSTEM_STARTUP)
	{
		bg->ResetToDefaults();
	}
	else if (info->intcode == NOTIFY_SYSTEM_PRE_RESET)
	{
		bg->ResetToDefaults();
	}
	else if (info->intcode == NOTIFY_SYSTEM_PRE_NEW)
	{
		bg->ResetToDefaults();
	}
	else if (info->intcode == NOTIFY_SCENE_PRE_DELETED_NODE)
	{
		ReferenceTarget *rt = reinterpret_cast<ReferenceTarget*>(info->callParam);
		INode *node = dynamic_cast<INode*>(rt);
		if (node)
		{
			if (node == bg->GetEnvironmentNode())
			{
				bg->SetEnvironmentNode(0);
				bg->DeleteAnalyticalSunNode();
			}
			else if (node == bg->GetAnalyticalSunNode())
			{
				bg->SetAnalyticalSunNode(0);
			}
		}
	}
}

void BgManagerMax::FactoryReset()
{
	DeleteEnvironmentNode();
	DeleteAnalyticalSunNode();
	ResetToDefaults();
}


// Activate and Stay Resident
//

namespace
{
	void tokenize(const std::wstring& str, std::vector<std::wstring>& tokens, const std::wstring& delimiters = L",")
	{
		std::wstring::size_type lastPos = str.find_first_not_of(delimiters, 0);
		std::wstring::size_type pos = str.find_first_of(delimiters, lastPos);

		while (std::wstring::npos != pos || std::wstring::npos != lastPos)
		{
			tokens.push_back(str.substr(lastPos, pos - lastPos));
			lastPos = str.find_first_not_of(delimiters, pos);
			pos = str.find_first_of(delimiters, lastPos);
		}
	}
};

typedef std::vector<std::vector<std::string>> CDatabase;

void BgManagerMax::BuildWorldCitiesDatabase()
{
	auto coname = GetModuleFolder() + L"RadeonProRender_co.dat";
	auto ciname = GetModuleFolder() + L"RadeonProRender_ci.dat";
	auto csname = GetModuleFolder() + L"RadeonProRender_cs.dat";
	bool coExists = _waccess(coname.c_str(), 0) == 0;
	bool ciExists = _waccess(ciname.c_str(), 0) == 0;
	bool csExists = _waccess(csname.c_str(), 0) == 0;
				
	if (coExists && ciExists)
	{
		char buffer[2048];
		FILE *countries_in = 0, *cities_in = 0;
		if (_wfopen_s(&countries_in, coname.c_str(), L"rb") == 0)
		{
			int num_countries = 0;
			fread(&num_countries, sizeof(int), 1, countries_in);
			for (int i = 0; i < num_countries; i++)
			{
				int len = 0;
				fread(&len, sizeof(int), 1, countries_in);
				fread(&buffer, sizeof(char), len, countries_in);
				buffer[len] = '\0';
				mCountriesData.push_back(buffer);
			}
			fclose(countries_in);
		}

		if (_wfopen_s(&cities_in, ciname.c_str(), L"rb") == 0)
		{
			int num_cities = 0;
			fread(&num_cities, sizeof(int), 1, cities_in);
			mCitiesData.resize(num_cities);
			for (int i = 0; i < num_cities; i++)
			{
				int len = 0;
				fread(&len, sizeof(int), 1, cities_in);
				fread(&buffer, sizeof(char), len, cities_in);
				buffer[len] = '\0';
				mCitiesData[i].name = buffer;
				fread(&mCitiesData[i].latitude, sizeof(float), 1, cities_in);
				fread(&mCitiesData[i].longitude, sizeof(float), 1, cities_in);
				fread(&mCitiesData[i].UTCOffset, sizeof(float), 1, cities_in);
				fread(&mCitiesData[i].countryIdx, sizeof(int), 1, cities_in);
			}
			fclose(cities_in);
		}
	}
	
	if (csExists)
	{
		FILE *shapes_in = 0;
		_wfopen_s(&shapes_in, csname.c_str(), L"rb");
		
		int numRecords = 0;
		fread(&numRecords, sizeof(int), 1, shapes_in);

		for (int i = 0; i < numRecords; i++)
		{
			int curRecord = mTzDb.size();
			mTzDb.resize(curRecord + 1);

			int len;
			fread(&len, sizeof(int), 1, shapes_in);
			char buf[1024];
			fread(buf, sizeof(char), len, shapes_in);
			buf[len] = '\0';
			mTzDb[curRecord].name = buf;

			fread(&mTzDb[curRecord].type, sizeof(int), 1, shapes_in);
			fread(&mTzDb[curRecord].UTCoffset, sizeof(float), 1, shapes_in);
			int numVerts = 0;
			fread(&numVerts, sizeof(int), 1, shapes_in);
			for (int j = 0; j < numVerts; j++)
			{
				float lati, longi;
				fread(&longi, sizeof(float), 1, shapes_in);
				fread(&lati, sizeof(float), 1, shapes_in);
				mTzDb[curRecord].verts.push_back(Point2(longi, lati));
			}
		}
		fclose(shapes_in);
	}
}

namespace
{
	bool isPointInRect(const float rect[4], const float &x, const float &y)
	{
		float orect[] = {
				std::min(rect[0], rect[2]),
				std::min(rect[1], rect[3]),
				std::max(rect[0], rect[2]),
				std::max(rect[1], rect[3])
		};
		return (x >= orect[0] && x <= orect[2] && y >= orect[1] && y <= orect[2]);
	}

	bool isPointInPoly(const std::vector<Point2> &poly, const float &x, const float &y)
	{
		bool inside = false;
		int numVerts = poly.size();
		for (int i = 0, j = numVerts - 1; i < numVerts; j = i++)
		{
			if (((poly[i].y > y) != (poly[j].y > y)) &&
				(x < (poly[j].x - poly[i].x) * (y - poly[i].y) / (poly[j].y - poly[i].y) + poly[i].x))
				inside = !inside;
		}
		return inside;
	}
};

float BgManagerMax::GetWorldUTCOffset(const float& longi, const float &lati)
{
	float res = -1.f;
	
	for (auto cc : mTzDb)
	{
		if (cc.type == TZTYPE_MULTIPOLYGON && isPointInPoly(cc.verts, longi, lati))
			return cc.UTCoffset;
		else if (cc.type == TZTYPE_BOX)
		{
			float rect[4] = { cc.verts[0].x, cc.verts[0].y, cc.verts[1].x, cc.verts[1].y };
			if (isPointInRect(rect, longi, lati))
				return cc.UTCoffset;
		}
	}
	
	// compute marine UTC
	res = longi * 24.f / 360.f;

	return res;
}

DWORD BgManagerMax::Start()
{
	GetClassDesc()->MakeAutoParamBlocks(this);
	
	int res;
	res = RegisterNotification(NotifyProc, this, NOTIFY_FILE_PRE_OPEN);
	res = RegisterNotification(NotifyProc, this, NOTIFY_SYSTEM_STARTUP);
	res = RegisterNotification(NotifyProc, this, NOTIFY_SYSTEM_PRE_RESET);
	res = RegisterNotification(NotifyProc, this, NOTIFY_SYSTEM_PRE_NEW);
	res = RegisterNotification(NotifyProc, this, NOTIFY_SCENE_PRE_DELETED_NODE);

	BuildWorldCitiesDatabase();

	CustomLocationPickerRegister();

	return GUPRESULT_KEEP;
}

void BgManagerMax::Stop()
{
	// The reference to rpr image keeps the whole context alive
	// Need to remove this reference when application is about to close
	mSkyImage = nullptr;

	DeleteAllRefsFromMe();

	CustomLocationPickerUnregister();

	UnRegisterNotification(NotifyProc, this, NOTIFY_FILE_PRE_OPEN);
	UnRegisterNotification(NotifyProc, this, NOTIFY_SYSTEM_PRE_RESET);
	UnRegisterNotification(NotifyProc, this, NOTIFY_SYSTEM_PRE_NEW);
	UnRegisterNotification(NotifyProc, this, NOTIFY_SCENE_PRE_DELETED_NODE);
}

void BgManagerMax::DeleteThis()
{
	//delete this;
}

void BgManagerMax::LookUpWorldCity(const std::string &searchTerm, HWND inquirer)
{
	std::string search = searchTerm;
	std::transform(search.begin(), search.end(), search.begin(), ::tolower);

	int numCities = mCitiesData.size();

#pragma omp parallel for
	for (int ii = 0; ii < numCities; ii++)
	{
		const CWorldCity &city = mCitiesData[ii];
		std::string name = city.name;
		std::string::size_type pos = name.find(search);
		if (pos != std::wstring::npos)
		{
			PostMessage(inquirer, WM_LOCSEARCHRESULT, 0, (LPARAM)ii);
		}
	}
}

std::string BgManagerMax::GetCityDesc(size_t i)
{
	FASSERT(i < mCitiesData.size());
	std::string res = mCitiesData[i].name;
	res += " (";
	res += mCountriesData[mCitiesData[i].countryIdx];
	res += ")";
	return res;
}

void BgManagerMax::GetCityData(size_t i, const float** lat, const float** longi, const float** utcoffset)
{
	FASSERT(i < mCitiesData.size());
	*lat = &mCitiesData[i].latitude;
	*longi = &mCitiesData[i].longitude;
	*utcoffset = &mCitiesData[i].UTCOffset;
}

//////////////////////////////////////////////////////////////////////////////
// REFERENCE TARGET
//

#if MAX_RELEASE > MAX_RELEASE_R16
RefResult BgManagerMax::NotifyRefChanged(const Interval& changeInt, RefTargetHandle hTarget,
	PartID& partID, RefMessage message, BOOL /*propagate*/)
#else
RefResult BgManagerMax::NotifyRefChanged(Interval changeInt, RefTargetHandle hTarget,
	PartID& partID, RefMessage message)
#endif
{
	if ((REFMSG_CHANGE == message) && (hTarget == mAnalyticalSkyNode))
	{
		if (!updatingSunFromProperty)
		{
			Interface *ip = GetCOREInterface();
			Matrix3 sunTm = mAnalyticalSkyNode->GetObjTMAfterWSM(ip->GetTime());
			Matrix3 envTm = mEnvironmentNode->GetObjTMAfterWSM(ip->GetTime());
			sunTm = sunTm * Inverse(envTm);

			Point3 vec = sunTm.GetTrans();
			float azimuth = 0;
			if (fabs(vec.x) >= 0.001f || fabs(vec.y) >= 0.001f)
			{

				// compute the azimuth angle (angle of rotation from the North vector, on the x-y plane. positive clockwise)
				azimuth = atan2(vec.y, vec.x);
				azimuth *= 180.0 / PI;
				azimuth -= 90.f;
				if (azimuth < 0.f)
					azimuth += 360.f;
				azimuth = 360.f - azimuth;
				if (azimuth == 360.f)
					azimuth = 0.f;
			}
			else
			{
				// vector is too small, angle could be any - just keep existing value unchanged
				GetProperty(PARAM_BG_SKY_AZIMUTH, azimuth);
			}

			// compute the elevation angle (angle between the equator and the position on the imaginary sphere, positive upwards)
			vec = vec.Normalize();
			float elevation = asin(vec.z);
			elevation *= 180.0 / PI;
			
			SetPropChangeSender(PROP_SENDER_SUNWIDGET);
			BOOL res = SetProperty(PARAM_BG_SKY_AZIMUTH, azimuth); FASSERT(res);
			res = SetProperty(PARAM_BG_SKY_ALTITUDE, elevation); FASSERT(res);
			SetPropChangeSender(-1);
		}
	}
	return REF_DONTCARE;
}

int BgManagerMax::NumRefs()
{
	return TEX_LASTREF;
}

RefTargetHandle BgManagerMax::GetReference(int i)
{
	switch (i)
	{
		case 0: return pblock2;
		case TEX_IBLMAP: return iblMap;
		case TEX_IBLBACKPLATE: return iblBackplate;
		case TEX_IBLREFROVERRIDE: return iblRefrOverride;
		case TEX_IBLREFLOVERRIDE: return iblReflOverride;
		case TEX_SKYBACKPLATE: return skyBackplate;
		case TEX_SKYREFROVERRIDE: return skyReflOverride;
		case TEX_SKYREFLOVERRIDE: return skyRefrOverride;
		case OBJ_SUNOBJECT: return mAnalyticalSkyNode;
	}
	return 0;
}

void BgManagerMax::SetReference(int i, RefTargetHandle rtarg)
{
	switch (i)
	{
		case 0: pblock2 = (IParamBlock2*)rtarg; break;
		case TEX_IBLMAP: iblMap = (Texmap*)rtarg; break;
		case TEX_IBLBACKPLATE: iblBackplate = (Texmap*)rtarg; break;
		case TEX_IBLREFROVERRIDE: iblRefrOverride = (Texmap*)rtarg; break;
		case TEX_IBLREFLOVERRIDE: iblReflOverride = (Texmap*)rtarg; break;
		case TEX_SKYBACKPLATE: skyBackplate = (Texmap*)rtarg; break;
		case TEX_SKYREFROVERRIDE: skyReflOverride = (Texmap*)rtarg; break;
		case TEX_SKYREFLOVERRIDE: skyRefrOverride = (Texmap*)rtarg; break;
		case OBJ_SUNOBJECT: mAnalyticalSkyNode = (INode*)rtarg; break;
	}
}

//////////////////////////////////////////////////////////////////////////////
// VIEWPORT OBJECTS
//
INode *BgManagerMax::GetEnvironmentNode()
{
	return mEnvironmentNode;
}

INode *BgManagerMax::GetAnalyticalSunNode()
{
	return mAnalyticalSkyNode;
}

INode *BgManagerMax::CreateEnvironmentNode()
{
	if (!mEnvironmentNode)
	{
		Object* obj = reinterpret_cast<Object*>(CreateInstance(LIGHT_CLASS_ID, FIRERENDER_ENVIRONMENT_CLASS_ID));
		mEnvironmentNode = GetCOREInterface()->CreateObjectNode(obj);
		Matrix3 tm(1);
		mEnvironmentNode->SetNodeTM(0, tm);
		GetCOREInterface()->RedrawViews(GetCOREInterface()->GetTime());
	}
	return mEnvironmentNode;
}

INode *BgManagerMax::CreateAnalyticalSunNode()
{
	if (mEnvironmentNode)
	{
		if (!mAnalyticalSkyNode)
		{
			Object* obj = reinterpret_cast<Object*>(CreateInstance(LIGHT_CLASS_ID, FIRERENDER_ANALYTICALSUN_CLASS_ID));
			mAnalyticalSkyNode = GetCOREInterface()->CreateObjectNode(obj);
			mEnvironmentNode->AttachChild(mAnalyticalSkyNode, 0);
			UpdateAnalyticalSunNode();
			GetCOREInterface()->RedrawViews(GetCOREInterface()->GetTime());
			ReplaceReference(OBJ_SUNOBJECT, mAnalyticalSkyNode);
		}
	}
	return mAnalyticalSkyNode;
}

void BgManagerMax::UpdateAnalyticalSunNode()
{
	if (mAnalyticalSkyNode)
	{
		Matrix3 nodeTM = mAnalyticalSkyNode->GetNodeTM(0);
		INode* parent = mAnalyticalSkyNode->GetParentNode();
		Matrix3 parentTM = parent->GetNodeTM(0);
		Matrix3 localTM = nodeTM*Inverse(parentTM);
		
		Point3 pos1 = nodeTM.GetTrans();
		Point3 pos2 = parentTM.GetTrans();
		float dx = pos2.x - pos1.x;
		float dy = pos2.y - pos1.y;
		float dz = pos2.z - pos1.z;
		float radius = sqrt(dx*dx + dy*dy + dz*dz);

		if (radius < 0.1f)
			radius = 50.f;
		
		float az, el;
		GetProperty(PARAM_BG_SKY_AZIMUTH, az);
		GetProperty(PARAM_BG_SKY_ALTITUDE, el);
		az *= (PI / 180.f);
		el *= (PI / 180.f);

		Matrix3 rot = idTM;
		rot.RotateZ(-az);
		rot.PreRotateX(el);
		rot.PreTranslate(Point3(0.f, radius, 0.f));
		nodeTM = rot * parentTM;

		mAnalyticalSkyNode->SetNodeTM(0, nodeTM);
		mAnalyticalSkyNode->InvalidateTM();

		GetCOREInterface()->RedrawViews(GetCOREInterface()->GetTime());
	}
}

void BgManagerMax::DeleteEnvironmentNode()
{
	if (mEnvironmentNode)
	{
		GetCOREInterface()->DeleteNode(mEnvironmentNode);
		mEnvironmentNode = 0;
		GetCOREInterface()->RedrawViews(GetCOREInterface()->GetTime());
	}
}

void BgManagerMax::DeleteAnalyticalSunNode()
{
	if (mAnalyticalSkyNode)
	{
		GetCOREInterface()->DeleteNode(mAnalyticalSkyNode);
		GetCOREInterface()->RedrawViews(GetCOREInterface()->GetTime());
	}
}

namespace
{
	INode *FindEnvironmentObject(INode *parent)
	{
		if (parent)
		{
			auto objRef = parent->GetObjectRef();
			if (objRef)
			{
				if (objRef->ClassID() == FIRERENDER_ENVIRONMENT_CLASS_ID)
					return parent;
			}
			int nc = parent->NumberOfChildren();
			for (int i = 0; i < nc; i++)
			{
				INode *res = FindEnvironmentObject(parent->GetChildNode(i));
				if (res)
					return res;
			}
		}
		return 0;
	}

	INode *FindAnalyticalSunObject(INode *parent)
	{
		if (parent)
		{
			auto objRef = parent->GetObjectRef();
			if (objRef)
			{
				if (objRef->ClassID() == FIRERENDER_ANALYTICALSUN_CLASS_ID)
					return parent;
			}
			int nc = parent->NumberOfChildren();
			for (int i = 0; i < nc; i++)
			{
				INode *res = FindAnalyticalSunObject(parent->GetChildNode(i));
				if (res)
					return res;
			}
		}
		return 0;
	}
};

INode *BgManagerMax::FindEnvironmentNode()
{
	return FindEnvironmentObject(GetCOREInterface()->GetRootNode());
}

INode *BgManagerMax::FindAnalyticalSunNode()
{
	return FindAnalyticalSunObject(GetCOREInterface()->GetRootNode());
}


//////////////////////////////////////////////////////////////////////////////
// MANAGER PARAMETERS
//


BOOL BgManagerMax::SetProperty(int param, Texmap* value, int sender)
{
	if (!pblock2)
		return FALSE;
	BOOL res = pblock2->SetValue(param, GetCOREInterface()->GetTime(), value);
	if (res)
		SendPropChangeNotifications(param);
	return res;
}

BOOL BgManagerMax::GetProperty(int param, Texmap*& value)
{
	if (!pblock2)
		return FALSE;
	BOOL res = pblock2->GetValue(param, GetCOREInterface()->GetTime(), value, FOREVER);
	return res;
}


BOOL BgManagerMax::SetProperty(int param, const float &value, int sender)
{
	BOOL res = pblock2->SetValue(param, GetCOREInterface()->GetTime(), value);
	if (res)
		SendPropChangeNotifications(param);	
	if (param == PARAM_BG_SKY_AZIMUTH || param == PARAM_BG_SKY_ALTITUDE)
	{
		updatingSunFromProperty = true;
		UpdateAnalyticalSunNode();
		updatingSunFromProperty = false;
	}
	return TRUE;
}

BOOL BgManagerMax::GetProperty(int param, float &value)
{
	BOOL res = pblock2->GetValue(param, GetCOREInterface()->GetTime(), value, FOREVER);
	return res;
}

BOOL BgManagerMax::SetProperty(int param, int value, int sender)
{
	BOOL res = pblock2->SetValue(param, GetCOREInterface()->GetTime(), value);
	if (res)
	{
		switch (param)
		{
			case PARAM_BG_USE:
			{
				if (value)
					CreateEnvironmentNode();
				else
					DeleteEnvironmentNode();
			}
			break;
			case PARAM_BG_TYPE:
			{
				if (value == FRBackgroundType_IBL)
					DeleteAnalyticalSunNode();
				else
				{
					int skyType;
					GetProperty(PARAM_BG_SKY_TYPE, skyType);
					if (skyType == FRBackgroundSkyType_Analytical)
						CreateAnalyticalSunNode();
					else
						DeleteAnalyticalSunNode();
				}
			}
			break;

			case PARAM_BG_SKY_TYPE:
			{
				int bgType;
				GetProperty(PARAM_BG_TYPE, bgType);
				if (bgType == FRBackgroundType_SunSky)
				{
					if (value == FRBackgroundSkyType_Analytical)
						CreateAnalyticalSunNode();
					else
						DeleteAnalyticalSunNode();
				}
			}
			break;
		}
		SendPropChangeNotifications(param);
	}
	return res;
}

BOOL BgManagerMax::GetProperty(int param, int &value)
{
	if (param == PARAM_BG_USE)
	{
		value = (mEnvironmentNode == 0) ? FALSE : TRUE;
		return TRUE;
	}
	BOOL res = pblock2->GetValue(param, GetCOREInterface()->GetTime(), value, FOREVER);
	return res;
}

BOOL BgManagerMax::SetProperty(int param, const Color &value, int sender)
{
	BOOL res = pblock2->SetValue(param, GetCOREInterface()->GetTime(), value);
	if (res)
		SendPropChangeNotifications(param);
	return TRUE;
}

BOOL BgManagerMax::GetProperty(int param, Color &value)
{
	BOOL res = pblock2->GetValue(param, GetCOREInterface()->GetTime(), value, FOREVER);
	return res;
}

void BgManagerMax::GetSunPosition(IParamBlock2 *pb, const TimeValue &t, float& outAzimuth, float& outAltitude)
{
	int skyType = 0;
	pb->GetValue(TRPARAM_BG_SKY_TYPE, t, skyType, FOREVER);
	bool isSkyAnalytical = (skyType == FRBackgroundSkyType_Analytical);

	if (isSkyAnalytical)
	{
		// azimuth and elevation are provided
		pb->GetValue(TRPARAM_BG_SKY_AZIMUTH, t, outAzimuth, FOREVER);
		pb->GetValue(TRPARAM_BG_SKY_ALTITUDE, t, outAltitude, FOREVER);
		return;
	}

	// compute sun azimuth and altitude from Earth location and time
	int skyHours;
	int skyMinutes;
	int skySeconds;
	int skyDay;
	int skyMonth;
	int skyYear;
	float skyTimezone;
	float skyLatitude;
	float skyLongitude;
	BOOL skyDaysaving;

	pb->GetValue(TRPARAM_BG_SKY_HOURS, t, skyHours, FOREVER);
	pb->GetValue(TRPARAM_BG_SKY_MINUTES, t, skyMinutes, FOREVER);
	pb->GetValue(TRPARAM_BG_SKY_SECONDS, t, skySeconds, FOREVER);
	pb->GetValue(TRPARAM_BG_SKY_DAY, t, skyDay, FOREVER);
	pb->GetValue(TRPARAM_BG_SKY_MONTH, t, skyMonth, FOREVER);
	pb->GetValue(TRPARAM_BG_SKY_YEAR, t, skyYear, FOREVER);
	pb->GetValue(TRPARAM_BG_SKY_TIMEZONE, t, skyTimezone, FOREVER);
	pb->GetValue(TRPARAM_BG_SKY_LATITUDE, t, skyLatitude, FOREVER);
	pb->GetValue(TRPARAM_BG_SKY_LONGITUDE, t, skyLongitude, FOREVER);
	pb->GetValue(TRPARAM_BG_SKY_DAYLIGHTSAVING, t, skyDaysaving, FOREVER);

	SunPositionCalculator SunPos;

	// DST adjustment
	if (skyDaysaving)
	{
		static const int MonthDays[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

		skyHours--;
		if (skyHours == -1)
		{
			skyHours = 23;
			skyDay--;
			if (skyDay == 0)
			{
				skyMonth--;
				if (skyMonth == 0)
				{
					skyMonth = 12;
					skyDay = 31;
					skyYear--;
				}
				else
				{
					if (skyMonth == 2)
					{
						bool leapdays = (skyYear % 4 == 0) && (skyYear % 400 == 0 || skyYear % 100 != 0);
						if (leapdays)
							skyDay = 29;
						else
							skyDay = 28;
					}
					else
						skyDay = MonthDays[skyMonth - 1];
				}
			}
		}
	}

	SunPos.year = skyYear;
	SunPos.month = skyMonth;
	SunPos.day = skyDay;
	SunPos.hour = skyHours;
	SunPos.minute = skyMinutes;
	SunPos.second = skySeconds;
	SunPos.timezone = skyTimezone;
	SunPos.delta_ut1 = 0;
	SunPos.delta_t = 67;
	SunPos.longitude = skyLongitude;
	SunPos.latitude = skyLatitude;
	SunPos.elevation = 0.0;
	SunPos.pressure = 820;
	SunPos.temperature = 11;
	SunPos.slope = 0;
	SunPos.azm_rotation = 0;
	SunPos.atmos_refract = 0.5667;
	SunPos.function = SunPositionCalculator::SPA_ALL;
	int res = SunPos.calculate();
	
	outAzimuth = SunPos.azimuth;
	outAltitude = SunPos.e;
}

// Compare 2 floats with some equality threshold
static bool ValuesAreSame(float a, float b, float eps = 0.0001f)
{
	return fabsf(a - b) < eps;
}

frw::Image BgManagerMax::GenerateSky(frw::Scope &scope, IParamBlock2 *pb, const TimeValue &t, float skyIntensity)
{
	float skyHaze;
	Color skyGroundColor;
	Color skyGroundAlbedo;
	Color skyFilterColor;
	float skySunDiscSize;
	float skySaturation;
	
	pb->GetValue(TRPARAM_BG_SKY_HAZE, t, skyHaze, FOREVER);
	pb->GetValue(TRPARAM_BG_SKY_GROUND_COLOR, t, skyGroundColor, FOREVER);
	pb->GetValue(TRPARAM_BG_SKY_GROUND_ALBEDO, t, skyGroundAlbedo, FOREVER);
	pb->GetValue(TRPARAM_BG_SKY_DISCSIZE, t, skySunDiscSize, FOREVER);
	pb->GetValue(TRPARAM_BG_SKY_FILTER_COLOR, t, skyFilterColor, FOREVER);
	pb->GetValue(TRPARAM_BG_SKY_SATURATION, t, skySaturation, FOREVER);

	// get Sun's angular location
	float skyAzimuth, skyAltitude;
	GetSunPosition(pb, t, skyAzimuth, skyAltitude);

	skyAzimuth = 0;
	skyAltitude *= (PI / 180.f);
	
	Point3 vec(0.f, std::sin(skyAltitude), std::cos(skyAltitude));

	bool recompute = false;

	if (!mSkyBuffer ||
		!ValuesAreSame(prev_sun_altitude, skyAltitude) ||
		!ValuesAreSame(prev_haze, skyHaze) ||
		!ValuesAreSame(prev_sun_disk_scale, skySunDiscSize) ||
		!ValuesAreSame(prev_saturation, skySaturation) ||
		(prev_ground_color != skyGroundColor) ||
		(prev_ground_albedo != skyGroundAlbedo) ||
		(prev_filter_color != skyFilterColor))
		recompute = true;

	if (recompute)
	{
		SkyGen sg;
		sg.mSaturation = skySaturation;
		sg.mSunIntensity = 100.0;
		sg.mSunDirection = vec;
		sg.mGroundColor = skyGroundColor;
		sg.mGroundAlbedo = skyGroundAlbedo;
		sg.mSunScale = skySunDiscSize;
		sg.mMultiplier = 1.0f; // controlled outside, as RPR light's parameter
		sg.mFilterColor = skyFilterColor;
		sg.mElevation = skyAltitude;
		sg.mTurbidity = 1.f + skyHaze * 0.09f;

		prev_sun_altitude = skyAltitude;
		prev_haze = skyHaze;
		prev_ground_color = skyGroundColor;
		prev_ground_albedo = skyGroundAlbedo;
		prev_sun_disk_scale = skySunDiscSize;
		prev_saturation = skySaturation;
		prev_filter_color = skyFilterColor;

		if (!mSkyBuffer)
			mSkyBuffer = new SkyRgbFloat32[SKYENV_WIDTH * SKYENV_HEIGHT];
		memset(mSkyBuffer, 0, sizeof(SkyRgbFloat32) * SKYENV_WIDTH * SKYENV_HEIGHT);

		float maxSunIntensity;

		if (skyIntensity < std::numeric_limits<float>::epsilon())
		{
			maxSunIntensity = std::numeric_limits<float>::max();
		}
		else if (skyIntensity > 1.f)
		{
			maxSunIntensity = skyIntensity;
		}
		else
		{
			maxSunIntensity = 1.f / skyIntensity;
		}

		sg.GenerateSkyHosek(SKYENV_WIDTH, SKYENV_HEIGHT, mSkyBuffer, maxSunIntensity);
		
		static const Point3 up(0, 0, 1);
		Point3 xaxis = CrossProd(up, vec);
		xaxis = FNormalize(xaxis);

		Point3 yaxis = CrossProd(vec, xaxis);
		yaxis = FNormalize(yaxis);

#ifdef DEBUG_GENERATED_SKY
		// set DEBUG_GENERATED_SKY_FILENAME to something like L"<my path>\\sky.exr"
		// where <my path> is the path to the image being saved
		BitmapInfo bi;
		bi.SetType(BMM_FLOAT_RGBA_32);
		bi.SetWidth((WORD)SKYENV_WIDTH);
		bi.SetHeight((WORD)SKYENV_HEIGHT);
		bi.SetCustomFlag(0);
		bi.SetPath(DEBUG_GENERATED_SKY_FILENAME);

		Bitmap *sky_bmap = 0;
		sky_bmap = ::TheManager->Create(&bi);
		for (int i = 0; i < SKYENV_HEIGHT; i++)
		{
			std::vector<BMM_Color_fl> pixelscheck(SKYENV_WIDTH);
			for (int j = 0; j < SKYENV_WIDTH; j++)
			{
				SkyRgbFloat32 &bpix = mSkyBuffer[i * SKYENV_WIDTH + j];
				pixelscheck[j] = BMM_Color_fl(bpix.r, bpix.g, bpix.b, 1.f);
			}
			sky_bmap->PutPixels(0, i, SKYENV_WIDTH, &pixelscheck[0]);
		}
		sky_bmap->OpenOutput(&bi);
		sky_bmap->Write(&bi, BMM_SINGLEFRAME);
		sky_bmap->Close(&bi);
		::TheManager->DelBitmap(sky_bmap);
#endif

		fr_image_desc imgDesc = {};
		imgDesc.image_width = SKYENV_WIDTH;
		imgDesc.image_height = SKYENV_HEIGHT;
		mSkyImage = frw::Image(scope, { 3, FR_COMPONENT_TYPE_FLOAT32 }, imgDesc, mSkyBuffer);
	}

	return mSkyImage;
}

//////////////////////////////////////////////////////////////////////////////
// Sun Position Calculator
//

#define SUN_RADIUS 0.26667

#define L_COUNT 6
#define B_COUNT 2
#define R_COUNT 5
#define Y_COUNT 63

#define L_MAX_SUBCOUNT 64
#define B_MAX_SUBCOUNT 5
#define R_MAX_SUBCOUNT 40

namespace
{

enum { TERM_A, TERM_B, TERM_C, TERM_COUNT };
enum { TERM_X0, TERM_X1, TERM_X2, TERM_X3, TERM_X4, TERM_X_COUNT };
enum { TERM_PSI_A, TERM_PSI_B, TERM_EPS_C, TERM_EPS_D, TERM_PE_COUNT };
enum { JD_MINUS, JD_ZERO, JD_PLUS, JD_COUNT };
enum { SUN_TRANSIT, SUN_RISE, SUN_SET, SUN_COUNT };

#define TERM_Y_COUNT TERM_X_COUNT

const int l_subcount[L_COUNT] = { 64,34,20,7,3,1 };
const int b_subcount[B_COUNT] = { 5,2 };
const int r_subcount[R_COUNT] = { 40,10,6,2,1 };

///////////////////////////////////////////////////
///  Earth Periodic Terms
///////////////////////////////////////////////////
const double L_TERMS[L_COUNT][L_MAX_SUBCOUNT][TERM_COUNT] =
{
	{
		{ 175347046.0,0,0 },
		{ 3341656.0,4.6692568,6283.07585 },
		{ 34894.0,4.6261,12566.1517 },
		{ 3497.0,2.7441,5753.3849 },
		{ 3418.0,2.8289,3.5231 },
		{ 3136.0,3.6277,77713.7715 },
		{ 2676.0,4.4181,7860.4194 },
		{ 2343.0,6.1352,3930.2097 },
		{ 1324.0,0.7425,11506.7698 },
		{ 1273.0,2.0371,529.691 },
		{ 1199.0,1.1096,1577.3435 },
		{ 990,5.233,5884.927 },
		{ 902,2.045,26.298 },
		{ 857,3.508,398.149 },
		{ 780,1.179,5223.694 },
		{ 753,2.533,5507.553 },
		{ 505,4.583,18849.228 },
		{ 492,4.205,775.523 },
		{ 357,2.92,0.067 },
		{ 317,5.849,11790.629 },
		{ 284,1.899,796.298 },
		{ 271,0.315,10977.079 },
		{ 243,0.345,5486.778 },
		{ 206,4.806,2544.314 },
		{ 205,1.869,5573.143 },
		{ 202,2.458,6069.777 },
		{ 156,0.833,213.299 },
		{ 132,3.411,2942.463 },
		{ 126,1.083,20.775 },
		{ 115,0.645,0.98 },
		{ 103,0.636,4694.003 },
		{ 102,0.976,15720.839 },
		{ 102,4.267,7.114 },
		{ 99,6.21,2146.17 },
		{ 98,0.68,155.42 },
		{ 86,5.98,161000.69 },
		{ 85,1.3,6275.96 },
		{ 85,3.67,71430.7 },
		{ 80,1.81,17260.15 },
		{ 79,3.04,12036.46 },
		{ 75,1.76,5088.63 },
		{ 74,3.5,3154.69 },
		{ 74,4.68,801.82 },
		{ 70,0.83,9437.76 },
		{ 62,3.98,8827.39 },
		{ 61,1.82,7084.9 },
		{ 57,2.78,6286.6 },
		{ 56,4.39,14143.5 },
		{ 56,3.47,6279.55 },
		{ 52,0.19,12139.55 },
		{ 52,1.33,1748.02 },
		{ 51,0.28,5856.48 },
		{ 49,0.49,1194.45 },
		{ 41,5.37,8429.24 },
		{ 41,2.4,19651.05 },
		{ 39,6.17,10447.39 },
		{ 37,6.04,10213.29 },
		{ 37,2.57,1059.38 },
		{ 36,1.71,2352.87 },
		{ 36,1.78,6812.77 },
		{ 33,0.59,17789.85 },
		{ 30,0.44,83996.85 },
		{ 30,2.74,1349.87 },
		{ 25,3.16,4690.48 }
	},
	{
		{ 628331966747.0,0,0 },
		{ 206059.0,2.678235,6283.07585 },
		{ 4303.0,2.6351,12566.1517 },
		{ 425.0,1.59,3.523 },
		{ 119.0,5.796,26.298 },
		{ 109.0,2.966,1577.344 },
		{ 93,2.59,18849.23 },
		{ 72,1.14,529.69 },
		{ 68,1.87,398.15 },
		{ 67,4.41,5507.55 },
		{ 59,2.89,5223.69 },
		{ 56,2.17,155.42 },
		{ 45,0.4,796.3 },
		{ 36,0.47,775.52 },
		{ 29,2.65,7.11 },
		{ 21,5.34,0.98 },
		{ 19,1.85,5486.78 },
		{ 19,4.97,213.3 },
		{ 17,2.99,6275.96 },
		{ 16,0.03,2544.31 },
		{ 16,1.43,2146.17 },
		{ 15,1.21,10977.08 },
		{ 12,2.83,1748.02 },
		{ 12,3.26,5088.63 },
		{ 12,5.27,1194.45 },
		{ 12,2.08,4694 },
		{ 11,0.77,553.57 },
		{ 10,1.3,6286.6 },
		{ 10,4.24,1349.87 },
		{ 9,2.7,242.73 },
		{ 9,5.64,951.72 },
		{ 8,5.3,2352.87 },
		{ 6,2.65,9437.76 },
		{ 6,4.67,4690.48 }
	},
	{
		{ 52919.0,0,0 },
		{ 8720.0,1.0721,6283.0758 },
		{ 309.0,0.867,12566.152 },
		{ 27,0.05,3.52 },
		{ 16,5.19,26.3 },
		{ 16,3.68,155.42 },
		{ 10,0.76,18849.23 },
		{ 9,2.06,77713.77 },
		{ 7,0.83,775.52 },
		{ 5,4.66,1577.34 },
		{ 4,1.03,7.11 },
		{ 4,3.44,5573.14 },
		{ 3,5.14,796.3 },
		{ 3,6.05,5507.55 },
		{ 3,1.19,242.73 },
		{ 3,6.12,529.69 },
		{ 3,0.31,398.15 },
		{ 3,2.28,553.57 },
		{ 2,4.38,5223.69 },
		{ 2,3.75,0.98 }
	},
	{
		{ 289.0,5.844,6283.076 },
		{ 35,0,0 },
		{ 17,5.49,12566.15 },
		{ 3,5.2,155.42 },
		{ 1,4.72,3.52 },
		{ 1,5.3,18849.23 },
		{ 1,5.97,242.73 }
	},
	{
		{ 114.0,3.142,0 },
		{ 8,4.13,6283.08 },
		{ 1,3.84,12566.15 }
	},
	{
		{ 1,3.14,0 }
	}
};

const double B_TERMS[B_COUNT][B_MAX_SUBCOUNT][TERM_COUNT] =
{
	{
		{ 280.0,3.199,84334.662 },
		{ 102.0,5.422,5507.553 },
		{ 80,3.88,5223.69 },
		{ 44,3.7,2352.87 },
		{ 32,4,1577.34 }
	},
	{
		{ 9,3.9,5507.55 },
		{ 6,1.73,5223.69 }
	}
};

const double R_TERMS[R_COUNT][R_MAX_SUBCOUNT][TERM_COUNT] =
{
	{
		{ 100013989.0,0,0 },
		{ 1670700.0,3.0984635,6283.07585 },
		{ 13956.0,3.05525,12566.1517 },
		{ 3084.0,5.1985,77713.7715 },
		{ 1628.0,1.1739,5753.3849 },
		{ 1576.0,2.8469,7860.4194 },
		{ 925.0,5.453,11506.77 },
		{ 542.0,4.564,3930.21 },
		{ 472.0,3.661,5884.927 },
		{ 346.0,0.964,5507.553 },
		{ 329.0,5.9,5223.694 },
		{ 307.0,0.299,5573.143 },
		{ 243.0,4.273,11790.629 },
		{ 212.0,5.847,1577.344 },
		{ 186.0,5.022,10977.079 },
		{ 175.0,3.012,18849.228 },
		{ 110.0,5.055,5486.778 },
		{ 98,0.89,6069.78 },
		{ 86,5.69,15720.84 },
		{ 86,1.27,161000.69 },
		{ 65,0.27,17260.15 },
		{ 63,0.92,529.69 },
		{ 57,2.01,83996.85 },
		{ 56,5.24,71430.7 },
		{ 49,3.25,2544.31 },
		{ 47,2.58,775.52 },
		{ 45,5.54,9437.76 },
		{ 43,6.01,6275.96 },
		{ 39,5.36,4694 },
		{ 38,2.39,8827.39 },
		{ 37,0.83,19651.05 },
		{ 37,4.9,12139.55 },
		{ 36,1.67,12036.46 },
		{ 35,1.84,2942.46 },
		{ 33,0.24,7084.9 },
		{ 32,0.18,5088.63 },
		{ 32,1.78,398.15 },
		{ 28,1.21,6286.6 },
		{ 28,1.9,6279.55 },
		{ 26,4.59,10447.39 }
	},
	{
		{ 103019.0,1.10749,6283.07585 },
		{ 1721.0,1.0644,12566.1517 },
		{ 702.0,3.142,0 },
		{ 32,1.02,18849.23 },
		{ 31,2.84,5507.55 },
		{ 25,1.32,5223.69 },
		{ 18,1.42,1577.34 },
		{ 10,5.91,10977.08 },
		{ 9,1.42,6275.96 },
		{ 9,0.27,5486.78 }
	},
	{
		{ 4359.0,5.7846,6283.0758 },
		{ 124.0,5.579,12566.152 },
		{ 12,3.14,0 },
		{ 9,3.63,77713.77 },
		{ 6,1.87,5573.14 },
		{ 3,5.47,18849.23 }
	},
	{
		{ 145.0,4.273,6283.076 },
		{ 7,3.92,12566.15 }
	},
	{
		{ 4,2.56,6283.08 }
	}
};

////////////////////////////////////////////////////////////////
///  Periodic Terms for the nutation in longitude and obliquity
////////////////////////////////////////////////////////////////

const int Y_TERMS[Y_COUNT][TERM_Y_COUNT] =
{
	{ 0,0,0,0,1 },
	{ -2,0,0,2,2 },
	{ 0,0,0,2,2 },
	{ 0,0,0,0,2 },
	{ 0,1,0,0,0 },
	{ 0,0,1,0,0 },
	{ -2,1,0,2,2 },
	{ 0,0,0,2,1 },
	{ 0,0,1,2,2 },
	{ -2,-1,0,2,2 },
	{ -2,0,1,0,0 },
	{ -2,0,0,2,1 },
	{ 0,0,-1,2,2 },
	{ 2,0,0,0,0 },
	{ 0,0,1,0,1 },
	{ 2,0,-1,2,2 },
	{ 0,0,-1,0,1 },
	{ 0,0,1,2,1 },
	{ -2,0,2,0,0 },
	{ 0,0,-2,2,1 },
	{ 2,0,0,2,2 },
	{ 0,0,2,2,2 },
	{ 0,0,2,0,0 },
	{ -2,0,1,2,2 },
	{ 0,0,0,2,0 },
	{ -2,0,0,2,0 },
	{ 0,0,-1,2,1 },
	{ 0,2,0,0,0 },
	{ 2,0,-1,0,1 },
	{ -2,2,0,2,2 },
	{ 0,1,0,0,1 },
	{ -2,0,1,0,1 },
	{ 0,-1,0,0,1 },
	{ 0,0,2,-2,0 },
	{ 2,0,-1,2,1 },
	{ 2,0,1,2,2 },
	{ 0,1,0,2,2 },
	{ -2,1,1,0,0 },
	{ 0,-1,0,2,2 },
	{ 2,0,0,2,1 },
	{ 2,0,1,0,0 },
	{ -2,0,2,2,2 },
	{ -2,0,1,2,1 },
	{ 2,0,-2,0,1 },
	{ 2,0,0,0,1 },
	{ 0,-1,1,0,0 },
	{ -2,-1,0,2,1 },
	{ -2,0,0,0,1 },
	{ 0,0,2,2,1 },
	{ -2,0,2,0,1 },
	{ -2,1,0,2,1 },
	{ 0,0,1,-2,0 },
	{ -1,0,1,0,0 },
	{ -2,1,0,0,0 },
	{ 1,0,0,0,0 },
	{ 0,0,1,2,0 },
	{ 0,0,-2,2,2 },
	{ -1,-1,1,0,0 },
	{ 0,1,1,0,0 },
	{ 0,-1,1,2,2 },
	{ 2,-1,-1,2,2 },
	{ 0,0,3,2,2 },
	{ 2,-1,0,2,2 },
};

const double PE_TERMS[Y_COUNT][TERM_PE_COUNT] = {
	{ -171996,-174.2,92025,8.9 },
	{ -13187,-1.6,5736,-3.1 },
	{ -2274,-0.2,977,-0.5 },
	{ 2062,0.2,-895,0.5 },
	{ 1426,-3.4,54,-0.1 },
	{ 712,0.1,-7,0 },
	{ -517,1.2,224,-0.6 },
	{ -386,-0.4,200,0 },
	{ -301,0,129,-0.1 },
	{ 217,-0.5,-95,0.3 },
	{ -158,0,0,0 },
	{ 129,0.1,-70,0 },
	{ 123,0,-53,0 },
	{ 63,0,0,0 },
	{ 63,0.1,-33,0 },
	{ -59,0,26,0 },
	{ -58,-0.1,32,0 },
	{ -51,0,27,0 },
	{ 48,0,0,0 },
	{ 46,0,-24,0 },
	{ -38,0,16,0 },
	{ -31,0,13,0 },
	{ 29,0,0,0 },
	{ 29,0,-12,0 },
	{ 26,0,0,0 },
	{ -22,0,0,0 },
	{ 21,0,-10,0 },
	{ 17,-0.1,0,0 },
	{ 16,0,-8,0 },
	{ -16,0.1,7,0 },
	{ -15,0,9,0 },
	{ -13,0,7,0 },
	{ -12,0,6,0 },
	{ 11,0,0,0 },
	{ -10,0,5,0 },
	{ -8,0,3,0 },
	{ 7,0,-3,0 },
	{ -7,0,0,0 },
	{ -7,0,3,0 },
	{ -7,0,3,0 },
	{ 6,0,0,0 },
	{ 6,0,-3,0 },
	{ 6,0,-3,0 },
	{ -6,0,3,0 },
	{ -6,0,3,0 },
	{ 5,0,0,0 },
	{ -5,0,3,0 },
	{ -5,0,3,0 },
	{ -5,0,3,0 },
	{ 4,0,0,0 },
	{ 4,0,0,0 },
	{ 4,0,0,0 },
	{ -4,0,0,0 },
	{ -4,0,0,0 },
	{ -4,0,0,0 },
	{ 3,0,0,0 },
	{ -3,0,0,0 },
	{ -3,0,0,0 },
	{ -3,0,0,0 },
	{ -3,0,0,0 },
	{ -3,0,0,0 },
	{ -3,0,0,0 },
	{ -3,0,0,0 },
};

///////////////////////////////////////////////
	
	double rad2deg(double radians)
	{
		return (180.0 / PI)*radians;
	}

	double deg2rad(double degrees)
	{
		return (PI / 180.0)*degrees;
	}

	int integer(double value)
	{
		return int(value);
	}

	double limit_degrees180pm(double degrees)
	{
		double limited;

		degrees /= 360.0;
		limited = 360.0*(degrees - floor(degrees));
		if (limited < -180.0) limited += 360.0;
		else if (limited >  180.0) limited -= 360.0;

		return limited;
	}

	double limit_degrees180(double degrees)
	{
		double limited;

		degrees /= 180.0;
		limited = 180.0*(degrees - floor(degrees));
		if (limited < 0) limited += 180.0;

		return limited;
	}

	double limit_zero2one(double value)
	{
		double limited;

		limited = value - floor(value);
		if (limited < 0) limited += 1.0;

		return limited;
	}

	double limit_minutes(double minutes)
	{
		double limited = minutes;

		if (limited < -20.0) limited += 1440.0;
		else if (limited >  20.0) limited -= 1440.0;

		return limited;
	}

	double dayfrac_to_local_hr(double dayfrac, double timezone)
	{
		return 24.0*limit_zero2one(dayfrac + timezone / 24.0);
	}

	double limit_degrees(double degrees)
	{
		double limited;

		degrees /= 360.0;
		limited = 360.0*(degrees - floor(degrees));
		if (limited < 0) limited += 360.0;

		return limited;
	}

	double third_order_polynomial(double a, double b, double c, double d, double x)
	{
		return ((a*x + b)*x + c)*x + d;
	}

	double julian_day(int year, int month, int day, int hour, int minute, double second, double dut1, double tz)
	{
		double day_decimal, julian_day, a;

		day_decimal = day + (hour - tz + (minute + (second + dut1) / 60.0) / 60.0) / 24.0;

		if (month < 3) {
			month += 12;
			year--;
		}

		julian_day = integer(365.25*(year + 4716.0)) + integer(30.6001*(month + 1)) + day_decimal - 1524.5;

		if (julian_day > 2299160.0) {
			a = integer(year / 100);
			julian_day += (2 - a + integer(a / 4));
		}

		return julian_day;
	}

	double julian_century(double jd)
	{
		return (jd - 2451545.0) / 36525.0;
	}

	double julian_ephemeris_day(double jd, double delta_t)
	{
		return jd + delta_t / 86400.0;
	}

	double julian_ephemeris_century(double jde)
	{
		return (jde - 2451545.0) / 36525.0;
	}

	double julian_ephemeris_millennium(double jce)
	{
		return (jce / 10.0);
	}

	double earth_periodic_term_summation(const double terms[][TERM_COUNT], int count, double jme)
	{
		int i;
		double sum = 0;

		for (i = 0; i < count; i++)
			sum += terms[i][TERM_A] * cos(terms[i][TERM_B] + terms[i][TERM_C] * jme);

		return sum;
	}

	double earth_values(double term_sum[], int count, double jme)
	{
		int i;
		double sum = 0;

		for (i = 0; i < count; i++)
			sum += term_sum[i] * pow(jme, i);

		sum /= 1.0e8;

		return sum;
	}

	double earth_heliocentric_longitude(double jme)
	{
		double sum[L_COUNT];
		int i;

		for (i = 0; i < L_COUNT; i++)
			sum[i] = earth_periodic_term_summation(L_TERMS[i], l_subcount[i], jme);

		return limit_degrees(rad2deg(earth_values(sum, L_COUNT, jme)));

	}

	double earth_heliocentric_latitude(double jme)
	{
		double sum[B_COUNT];
		int i;

		for (i = 0; i < B_COUNT; i++)
			sum[i] = earth_periodic_term_summation(B_TERMS[i], b_subcount[i], jme);

		return rad2deg(earth_values(sum, B_COUNT, jme));

	}

	double earth_radius_vector(double jme)
	{
		double sum[R_COUNT];
		int i;

		for (i = 0; i < R_COUNT; i++)
			sum[i] = earth_periodic_term_summation(R_TERMS[i], r_subcount[i], jme);

		return earth_values(sum, R_COUNT, jme);

	}

	double geocentric_longitude(double l)
	{
		double theta = l + 180.0;

		if (theta >= 360.0) theta -= 360.0;

		return theta;
	}

	double geocentric_latitude(double b)
	{
		return -b;
	}

	double mean_elongation_moon_sun(double jce)
	{
		return third_order_polynomial(1.0 / 189474.0, -0.0019142, 445267.11148, 297.85036, jce);
	}

	double mean_anomaly_sun(double jce)
	{
		return third_order_polynomial(-1.0 / 300000.0, -0.0001603, 35999.05034, 357.52772, jce);
	}

	double mean_anomaly_moon(double jce)
	{
		return third_order_polynomial(1.0 / 56250.0, 0.0086972, 477198.867398, 134.96298, jce);
	}

	double argument_latitude_moon(double jce)
	{
		return third_order_polynomial(1.0 / 327270.0, -0.0036825, 483202.017538, 93.27191, jce);
	}

	double ascending_longitude_moon(double jce)
	{
		return third_order_polynomial(1.0 / 450000.0, 0.0020708, -1934.136261, 125.04452, jce);
	}

	double xy_term_summation(int i, double x[TERM_X_COUNT])
	{
		int j;
		double sum = 0;

		for (j = 0; j < TERM_Y_COUNT; j++)
			sum += x[j] * Y_TERMS[i][j];

		return sum;
	}

	void nutation_longitude_and_obliquity(double jce, double x[TERM_X_COUNT], double *del_psi,
		double *del_epsilon)
	{
		int i;
		double xy_term_sum, sum_psi = 0, sum_epsilon = 0;

		for (i = 0; i < Y_COUNT; i++) {
			xy_term_sum = deg2rad(xy_term_summation(i, x));
			sum_psi += (PE_TERMS[i][TERM_PSI_A] + jce*PE_TERMS[i][TERM_PSI_B])*sin(xy_term_sum);
			sum_epsilon += (PE_TERMS[i][TERM_EPS_C] + jce*PE_TERMS[i][TERM_EPS_D])*cos(xy_term_sum);
		}

		*del_psi = sum_psi / 36000000.0;
		*del_epsilon = sum_epsilon / 36000000.0;
	}

	double ecliptic_mean_obliquity(double jme)
	{
		double u = jme / 10.0;

		return 84381.448 + u*(-4680.93 + u*(-1.55 + u*(1999.25 + u*(-51.38 + u*(-249.67 +
			u*(-39.05 + u*(7.12 + u*(27.87 + u*(5.79 + u*2.45)))))))));
	}

	double ecliptic_true_obliquity(double delta_epsilon, double epsilon0)
	{
		return delta_epsilon + epsilon0 / 3600.0;
	}

	double aberration_correction(double r)
	{
		return -20.4898 / (3600.0*r);
	}

	double apparent_sun_longitude(double theta, double delta_psi, double delta_tau)
	{
		return theta + delta_psi + delta_tau;
	}

	double greenwich_mean_sidereal_time(double jd, double jc)
	{
		return limit_degrees(280.46061837 + 360.98564736629 * (jd - 2451545.0) +
			jc*jc*(0.000387933 - jc / 38710000.0));
	}

	double greenwich_sidereal_time(double nu0, double delta_psi, double epsilon)
	{
		return nu0 + delta_psi*cos(deg2rad(epsilon));
	}

	double geocentric_right_ascension(double lamda, double epsilon, double beta)
	{
		double lamda_rad = deg2rad(lamda);
		double epsilon_rad = deg2rad(epsilon);

		return limit_degrees(rad2deg(atan2(sin(lamda_rad)*cos(epsilon_rad) -
			tan(deg2rad(beta))*sin(epsilon_rad), cos(lamda_rad))));
	}

	double geocentric_declination(double beta, double epsilon, double lamda)
	{
		double beta_rad = deg2rad(beta);
		double epsilon_rad = deg2rad(epsilon);

		return rad2deg(asin(sin(beta_rad)*cos(epsilon_rad) +
			cos(beta_rad)*sin(epsilon_rad)*sin(deg2rad(lamda))));
	}

	double observer_hour_angle(double nu, double longitude, double alpha_deg)
	{
		return limit_degrees(nu + longitude - alpha_deg);
	}

	double sun_equatorial_horizontal_parallax(double r)
	{
		return 8.794 / (3600.0 * r);
	}

	void right_ascension_parallax_and_topocentric_dec(double latitude, double elevation,
		double xi, double h, double delta, double *delta_alpha, double *delta_prime)
	{
		double delta_alpha_rad;
		double lat_rad = deg2rad(latitude);
		double xi_rad = deg2rad(xi);
		double h_rad = deg2rad(h);
		double delta_rad = deg2rad(delta);
		double u = atan(0.99664719 * tan(lat_rad));
		double y = 0.99664719 * sin(u) + elevation*sin(lat_rad) / 6378140.0;
		double x = cos(u) + elevation*cos(lat_rad) / 6378140.0;

		delta_alpha_rad = atan2(-x*sin(xi_rad) *sin(h_rad),
			cos(delta_rad) - x*sin(xi_rad) *cos(h_rad));

		*delta_prime = rad2deg(atan2((sin(delta_rad) - y*sin(xi_rad))*cos(delta_alpha_rad),
			cos(delta_rad) - x*sin(xi_rad) *cos(h_rad)));

		*delta_alpha = rad2deg(delta_alpha_rad);
	}

	double topocentric_right_ascension(double alpha_deg, double delta_alpha)
	{
		return alpha_deg + delta_alpha;
	}

	double topocentric_local_hour_angle(double h, double delta_alpha)
	{
		return h - delta_alpha;
	}

	double topocentric_elevation_angle(double latitude, double delta_prime, double h_prime)
	{
		double lat_rad = deg2rad(latitude);
		double delta_prime_rad = deg2rad(delta_prime);

		return rad2deg(asin(sin(lat_rad)*sin(delta_prime_rad) +
			cos(lat_rad)*cos(delta_prime_rad) * cos(deg2rad(h_prime))));
	}

	double atmospheric_refraction_correction(double pressure, double temperature,
		double atmos_refract, double e0)
	{
		double del_e = 0;

		if (e0 >= -1 * (SUN_RADIUS + atmos_refract))
			del_e = (pressure / 1010.0) * (283.0 / (273.0 + temperature)) *
			1.02 / (60.0 * tan(deg2rad(e0 + 10.3 / (e0 + 5.11))));

		return del_e;
	}

	double topocentric_elevation_angle_corrected(double e0, double delta_e)
	{
		return e0 + delta_e;
	}

	double topocentric_zenith_angle(double e)
	{
		return 90.0 - e;
	}

	double topocentric_azimuth_angle_astro(double h_prime, double latitude, double delta_prime)
	{
		double h_prime_rad = deg2rad(h_prime);
		double lat_rad = deg2rad(latitude);

		return limit_degrees(rad2deg(atan2(sin(h_prime_rad),
			cos(h_prime_rad)*sin(lat_rad) - tan(deg2rad(delta_prime))*cos(lat_rad))));
	}

	double topocentric_azimuth_angle(double azimuth_astro)
	{
		return limit_degrees(azimuth_astro + 180.0);
	}

	double surface_incidence_angle(double zenith, double azimuth_astro, double azm_rotation,
		double slope)
	{
		double zenith_rad = deg2rad(zenith);
		double slope_rad = deg2rad(slope);

		return rad2deg(acos(cos(zenith_rad)*cos(slope_rad) +
			sin(slope_rad)*sin(zenith_rad) * cos(deg2rad(azimuth_astro - azm_rotation))));
	}

	double sun_mean_longitude(double jme)
	{
		return limit_degrees(280.4664567 + jme*(360007.6982779 + jme*(0.03032028 +
			jme*(1 / 49931.0 + jme*(-1 / 15300.0 + jme*(-1 / 2000000.0))))));
	}

	double eotf(double m, double alpha, double del_psi, double epsilon)
	{
		return limit_minutes(4.0*(m - 0.0057183 - alpha + del_psi*cos(deg2rad(epsilon))));
	}

	double approx_sun_transit_time(double alpha_zero, double longitude, double nu)
	{
		return (alpha_zero - longitude - nu) / 360.0;
	}

	double sun_hour_angle_at_rise_set(double latitude, double delta_zero, double h0_prime)
	{
		double h0 = -99999;
		double latitude_rad = deg2rad(latitude);
		double delta_zero_rad = deg2rad(delta_zero);
		double argument = (sin(deg2rad(h0_prime)) - sin(latitude_rad)*sin(delta_zero_rad)) /
			(cos(latitude_rad)*cos(delta_zero_rad));

		if (fabs(argument) <= 1) h0 = limit_degrees180(rad2deg(acos(argument)));

		return h0;
	}

	void approx_sun_rise_and_set(double *m_rts, double h0)
	{
		double h0_dfrac = h0 / 360.0;

		m_rts[SUN_RISE] = limit_zero2one(m_rts[SUN_TRANSIT] - h0_dfrac);
		m_rts[SUN_SET] = limit_zero2one(m_rts[SUN_TRANSIT] + h0_dfrac);
		m_rts[SUN_TRANSIT] = limit_zero2one(m_rts[SUN_TRANSIT]);
	}

	double rts_alpha_delta_prime(double *ad, double n)
	{
		double a = ad[JD_ZERO] - ad[JD_MINUS];
		double b = ad[JD_PLUS] - ad[JD_ZERO];

		if (fabs(a) >= 2.0) a = limit_zero2one(a);
		if (fabs(b) >= 2.0) b = limit_zero2one(b);

		return ad[JD_ZERO] + n * (a + b + (b - a)*n) / 2.0;
	}

	double rts_sun_altitude(double latitude, double delta_prime, double h_prime)
	{
		double latitude_rad = deg2rad(latitude);
		double delta_prime_rad = deg2rad(delta_prime);

		return rad2deg(asin(sin(latitude_rad)*sin(delta_prime_rad) +
			cos(latitude_rad)*cos(delta_prime_rad)*cos(deg2rad(h_prime))));
	}

	double sun_rise_and_set(double *m_rts, double *h_rts, double *delta_prime, double latitude,
		double *h_prime, double h0_prime, int sun)
	{
		return m_rts[sun] + (h_rts[sun] - h0_prime) /
			(360.0*cos(deg2rad(delta_prime[sun]))*cos(deg2rad(latitude))*sin(deg2rad(h_prime[sun])));
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////
// Calculate required SPA parameters to get the right ascension (alpha) and declination (delta)
// Note: JD must be already calculated and in structure
////////////////////////////////////////////////////////////////////////////////////////////////
void SunPositionCalculator::calculate_geocentric_sun_right_ascension_and_declination()
{
	double x[TERM_X_COUNT];

	jc = julian_century(jd);

	jde = julian_ephemeris_day(jd, delta_t);
	jce = julian_ephemeris_century(jde);
	jme = julian_ephemeris_millennium(jce);

	l = earth_heliocentric_longitude(jme);
	b = earth_heliocentric_latitude(jme);
	r = earth_radius_vector(jme);

	theta = geocentric_longitude(l);
	beta = geocentric_latitude(b);

	x[TERM_X0] = x0 = mean_elongation_moon_sun(jce);
	x[TERM_X1] = x1 = mean_anomaly_sun(jce);
	x[TERM_X2] = x2 = mean_anomaly_moon(jce);
	x[TERM_X3] = x3 = argument_latitude_moon(jce);
	x[TERM_X4] = x4 = ascending_longitude_moon(jce);

	nutation_longitude_and_obliquity(jce, x, &(del_psi), &(del_epsilon));

	epsilon0 = ecliptic_mean_obliquity(jme);
	epsilon = ecliptic_true_obliquity(del_epsilon, epsilon0);

	del_tau = aberration_correction(r);
	lamda = apparent_sun_longitude(theta, del_psi, del_tau);
	nu0 = greenwich_mean_sidereal_time(jd, jc);
	nu = greenwich_sidereal_time(nu0, del_psi, epsilon);

	alpha = geocentric_right_ascension(lamda, epsilon, beta);
	delta = geocentric_declination(beta, epsilon, lamda);
}

////////////////////////////////////////////////////////////////////////
// Calculate Equation of Time (EOT) and Sun Rise, Transit, & Set (RTS)
////////////////////////////////////////////////////////////////////////

void SunPositionCalculator::calculate_eot_and_sun_rise_transit_set()
{
	double nu_, m, h0, n;
	double alpha_[JD_COUNT], delta_[JD_COUNT];
	double m_rts[SUN_COUNT], nu_rts[SUN_COUNT], h_rts[SUN_COUNT];
	double alpha_prime[SUN_COUNT], delta_prime[SUN_COUNT], h_prime[SUN_COUNT];
	double h0_prime = -1 * (SUN_RADIUS + atmos_refract);
	int i;

	m = sun_mean_longitude(jme);
	eot = eotf(m, alpha, del_psi, epsilon);

	hour = minute = second = 0;
	delta_ut1 = timezone = 0.0;

	jd = julian_day(year, month, day, hour,
		minute, second, delta_ut1, timezone);

	calculate_geocentric_sun_right_ascension_and_declination();
	nu_ = nu;

	delta_t = 0;
	jd--;
	for (i = 0; i < JD_COUNT; i++) {
		calculate_geocentric_sun_right_ascension_and_declination();
		alpha_[i] = alpha;
		delta_[i] = delta;
		jd++;
	}

	m_rts[SUN_TRANSIT] = approx_sun_transit_time(alpha_[JD_ZERO], longitude, nu_);
	h0 = sun_hour_angle_at_rise_set(latitude, delta_[JD_ZERO], h0_prime);

	if (h0 >= 0) {

		approx_sun_rise_and_set(m_rts, h0);

		for (i = 0; i < SUN_COUNT; i++) {

			nu_rts[i] = nu_ + 360.985647*m_rts[i];

			n = m_rts[i] + delta_t / 86400.0;
			alpha_prime[i] = rts_alpha_delta_prime(alpha_, n);
			delta_prime[i] = rts_alpha_delta_prime(delta_, n);

			h_prime[i] = limit_degrees180pm(nu_rts[i] + longitude - alpha_prime[i]);

			h_rts[i] = rts_sun_altitude(latitude, delta_prime[i], h_prime[i]);
		}

		srha = h_prime[SUN_RISE];
		ssha = h_prime[SUN_SET];
		sta = h_rts[SUN_TRANSIT];

		suntransit = dayfrac_to_local_hr(m_rts[SUN_TRANSIT] - h_prime[SUN_TRANSIT] / 360.0,
			timezone);

		sunrise = dayfrac_to_local_hr(sun_rise_and_set(m_rts, h_rts, delta_prime,
			latitude, h_prime, h0_prime, SUN_RISE), timezone);

		sunset = dayfrac_to_local_hr(sun_rise_and_set(m_rts, h_rts, delta_prime,
			latitude, h_prime, h0_prime, SUN_SET), timezone);

	}
	else srha = ssha = sta = suntransit = sunrise = sunset = -99999;
}


int SunPositionCalculator::validate_inputs()
{
	if ((year        < -2000) || (year        > 6000)) return 1;
	if ((month       < 1) || (month       > 12)) return 2;
	if ((day         < 1) || (day         > 31)) return 3;
	if ((hour        < 0) || (hour        > 24)) return 4;
	if ((minute      < 0) || (minute      > 59)) return 5;
	if ((second      < 0) || (second >= 60)) return 6;
	if ((pressure    < 0) || (pressure    > 5000)) return 12;
	if ((temperature <= -273) || (temperature > 6000)) return 13;
	if ((delta_ut1 <= -1) || (delta_ut1 >= 1)) return 17;
	if ((hour == 24) && (minute      > 0)) return 5;
	if ((hour == 24) && (second      > 0)) return 6;

	if (fabs(delta_t)       > 8000) return 7;
	if (fabs(timezone)      > 18) return 8;
	if (fabs(longitude)     > 180) return 9;
	if (fabs(latitude)      > 90) return 10;
	if (fabs(atmos_refract) > 5) return 16;
	if (elevation      < -6500000) return 11;

	if ((function == SPA_ZA_INC) || (function == SPA_ALL))
	{
		if (fabs(slope)         > 360) return 14;
		if (fabs(azm_rotation)  > 360) return 15;
	}

	return 0;
}

int SunPositionCalculator::calculate()
{
	int result;

	result = validate_inputs();

	if (result == 0)
	{
		jd = julian_day(year, month, day, hour,
			minute, second, delta_ut1, timezone);

		calculate_geocentric_sun_right_ascension_and_declination();

		h = observer_hour_angle(nu, longitude, alpha);
		xi = sun_equatorial_horizontal_parallax(r);

		right_ascension_parallax_and_topocentric_dec(latitude, elevation, xi,
			h, delta, &(del_alpha), &(delta_prime));

		alpha_prime = topocentric_right_ascension(alpha, del_alpha);
		h_prime = topocentric_local_hour_angle(h, del_alpha);

		e0 = topocentric_elevation_angle(latitude, delta_prime, h_prime);
		del_e = atmospheric_refraction_correction(pressure, temperature,
			atmos_refract, e0);
		e = topocentric_elevation_angle_corrected(e0, del_e);

		zenith = topocentric_zenith_angle(e);
		azimuth_astro = topocentric_azimuth_angle_astro(h_prime, latitude,
			delta_prime);
		azimuth = topocentric_azimuth_angle(azimuth_astro);

		if ((function == SPA_ZA_INC) || (function == SPA_ALL))
			incidence = surface_incidence_angle(zenith, azimuth_astro,
				azm_rotation, slope);

		if ((function == SPA_ZA_RTS) || (function == SPA_ALL))
			calculate_eot_and_sun_rise_transit_set();
	}

	return result;
}

//////////////////////////////////////////////////////////////////////////////
// LOCATION PICKER CUSTOM STATIC CONTROL
//

#define CELLSIZE        48
#define REDCOLOR       RGB(255,0,0)
#define LIGHTCOLOR      RGB(241,179,0)

namespace
{
	typedef struct CustomLPDatas {
		HWND hwnd;
		DWORD style;
		HBRUSH hbrLight;
		HBRUSH hbrRed;
		HBITMAP hBitmap;
		Point2 coord;
		float UTCoffset;
	} CustomLPData;

	void CustomPaint(CustomLPData* pData, HDC hDC, RECT* rcDirty, BOOL bErase)
	{
		int x, y;
		RECT r;
		HBRUSH hBrush;

		HDC hSRC = CreateCompatibleDC(hDC);
		HBITMAP hbmOld = (HBITMAP)SelectObject(hSRC, pData->hBitmap);
		BitBlt(hDC, rcDirty->left, rcDirty->top, rcDirty->right - rcDirty->left, rcDirty->bottom - rcDirty->top, hSRC, rcDirty->left, rcDirty->top, SRCCOPY);
		SelectObject(hSRC, hbmOld);
		SelectBrush(hDC, pData->hbrRed);

		RECT rect;
		GetClientRect(pData->hwnd, &rect);
		int cx = rect.right - rect.left;
		int cy = rect.bottom - rect.top;
		
		x = int(((pData->coord.x + 180.f) * float(cx)) / 360.f);
		y = cy - int(((pData->coord.y + 90.f) * float(cy)) / 180.f);
		Ellipse(hDC, x - 5, y - 5, x + 5, y + 5);
		DeleteDC(hSRC);
	}

	void CustomDoubleBuffer(CustomLPData* pData, PAINTSTRUCT* pPaintStruct)
	{
		int cx = pPaintStruct->rcPaint.right - pPaintStruct->rcPaint.left;
		int cy = pPaintStruct->rcPaint.bottom - pPaintStruct->rcPaint.top;
		HDC hMemDC;
		HBITMAP hBmp;
		HBITMAP hOldBmp;
		POINT ptOldOrigin;

		// Create new bitmap-back device context, large as the dirty rectangle.
		hMemDC = CreateCompatibleDC(pPaintStruct->hdc);
		hBmp = CreateCompatibleBitmap(pPaintStruct->hdc, cx, cy);
		hOldBmp = (HBITMAP)SelectObject(hMemDC, hBmp);

		// Do the painting into the memory bitmap.
		OffsetViewportOrgEx(hMemDC, -(pPaintStruct->rcPaint.left),
			-(pPaintStruct->rcPaint.top), &ptOldOrigin);
		CustomPaint(pData, hMemDC, &pPaintStruct->rcPaint, TRUE);
		SetViewportOrgEx(hMemDC, ptOldOrigin.x, ptOldOrigin.y, NULL);

		// Blit the bitmap into the screen. This is really fast operation and altough
		// the CustomPaint() can be complex and slow there will be no flicker any more.
		BitBlt(pPaintStruct->hdc, pPaintStruct->rcPaint.left, pPaintStruct->rcPaint.top,
			cx, cy, hMemDC, 0, 0, SRCCOPY);

		// Clean up.
		SelectObject(hMemDC, hOldBmp);
		DeleteObject(hBmp);
		DeleteDC(hMemDC);
	}
};

LRESULT CALLBACK BgManagerMax::CustomLPProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	CustomLPData* pData = reinterpret_cast<CustomLPData*>(GetWindowLongPtr(hwnd, 0));

	switch (uMsg) {
	case WM_ERASEBKGND:
		return FALSE;  // Defer erasing into WM_PAINT

	case WM_LATLONGSET:
	{
		LATLONGSETDATA *pCoord = reinterpret_cast<LATLONGSETDATA*>(lParam);
		pData->coord = pCoord->first;
		pData->UTCoffset = pCoord->second;
		InvalidateRect(hwnd, 0, TRUE);
		UpdateWindow(hwnd);
	}
	break;

	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		BeginPaint(hwnd, &ps);
		// We let application to choose whether to use double buffering or not by using the style CCS_DOUBLEBUFFER.
		if (pData->style & XXS_DOUBLEBUFFER)
			CustomDoubleBuffer(pData, &ps);
		else
			CustomPaint(pData, ps.hdc, &ps.rcPaint, ps.fErase);
		EndPaint(hwnd, &ps);
		return 0;
	}

	case WM_PRINTCLIENT:
	{
		RECT rc;
		GetClientRect(hwnd, &rc);
		CustomPaint(pData, (HDC)wParam, &rc, TRUE);
		return 0;
	}

	case WM_STYLECHANGED:
		if (wParam == GWL_STYLE)
			pData->style = lParam;
		break;

	case WM_NCCREATE:
		pData = new CustomLPData;
		if (pData == NULL)
			return FALSE;
		SetWindowLongPtr(hwnd, 0, (LONG_PTR)pData);
		pData->hwnd = hwnd;
		pData->style = ((CREATESTRUCT*)lParam)->style;
		pData->hbrRed = CreateSolidBrush(REDCOLOR);
		pData->hbrLight = CreateSolidBrush(LIGHTCOLOR);
		pData->hBitmap = LoadBitmap(FireRender::fireRenderHInstance, MAKEINTRESOURCE(IDB_WORLDMAP));
		return TRUE;

	case WM_LBUTTONDOWN:
	{
		RECT rect;
		GetClientRect(hwnd, &rect);
		int cx = rect.right - rect.left;
		int cy = rect.bottom - rect.top;
		int x = LOWORD(lParam);
		int y = cy - HIWORD(lParam);
		pData->coord.x = (float(x) * 360.f / float(cx)) - 180.f;
		pData->coord.y = ((float(y) * 180.f / float(cy)) - 90.f);
		pData->UTCoffset = BgManagerMax::TheManager.GetWorldUTCOffset(pData->coord.x, pData->coord.y);
		LATLONGSETDATA coordUtc = std::make_pair(pData->coord, pData->UTCoffset);
		SendMessage(GetParent(hwnd), WM_LATLONGSET, 0, (LPARAM)&coordUtc);
		InvalidateRect(hwnd, 0, TRUE);
		UpdateWindow(hwnd);
	}
	return TRUE;

	case WM_NCDESTROY:
		if (pData != NULL) {
			DeleteObject(pData->hbrRed);
			DeleteObject(pData->hbrLight);
			DeleteObject(pData->hBitmap);
			delete pData;
		}
		return 0;
	}
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void BgManagerMax::CustomLocationPickerRegister()
{
	WNDCLASS wc = { 0 };

	// Note we do not use CS_HREDRAW and CS_VREDRAW.
	// This means when the control is resized, WM_SIZE (as handled by DefWindowProc()) 
	// invalidates only the newly uncovered area.
	// With those class styles, it would invalidate complete client rectangle.
	wc.style = CS_GLOBALCLASS;
	wc.lpfnWndProc = CustomLPProc;
	wc.cbWndExtra = sizeof(CustomLPData*);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.lpszClassName = CUSTOMLP_WC;
	RegisterClass(&wc);
}


void BgManagerMax::CustomLocationPickerUnregister()
{
	UnregisterClass(CUSTOMLP_WC, NULL);
}

#define BGMANAGER_BASE_CHUNK		4000
#define BGMANAGER_IBLMAP_CHUNK		4001
#define BGMANAGER_IBLBACKP_CHUNK	4002
#define BGMANAGER_IBLREFR_CHUNK		4003
#define BGMANAGER_IBLREFL_CHUNK		4004
#define BGMANAGER_SKYBACKP_CHUNK	4005
#define BGMANAGER_SKYREFR_CHUNK		4006
#define BGMANAGER_SKYREFL_CHUNK		4007

#define SAVE_TEX(id, chunkid)\
{\
	Texmap *map = 0;\
	GetProperty(id, map);\
	if (map)\
	{\
		isave->BeginChunk(chunkid);\
		ULONG id = isave->GetRefID(map);\
		res = isave->Write(&id, sizeof(ULONG), &nb);\
		isave->EndChunk();\
		FASSERT(res == IO_OK);\
	}\
}

IOResult BgManagerMax::Save(ISave *isave)
{
	ULONG nb;
	Texmap *iblMap;
	IOResult res = IO_OK;

	SAVE_TEX(PARAM_BG_IBL_MAP, BGMANAGER_IBLMAP_CHUNK);
	SAVE_TEX(PARAM_BG_IBL_BACKPLATE, BGMANAGER_IBLBACKP_CHUNK);
	SAVE_TEX(PARAM_BG_IBL_REFLECTIONMAP, BGMANAGER_IBLREFL_CHUNK);
	SAVE_TEX(PARAM_BG_IBL_REFRACTIONMAP, BGMANAGER_IBLREFR_CHUNK);
	SAVE_TEX(PARAM_BG_SKY_BACKPLATE, BGMANAGER_SKYREFR_CHUNK);
	SAVE_TEX(PARAM_BG_SKY_REFLECTIONMAP, BGMANAGER_SKYREFR_CHUNK);
	SAVE_TEX(PARAM_BG_SKY_REFRACTIONMAP, BGMANAGER_SKYREFR_CHUNK);

	isave->BeginChunk(BGMANAGER_BASE_CHUNK);
	res = pblock2->Save(isave);
	isave->EndChunk();
	FASSERT(res == IO_OK);
	return res;
}

namespace
{
	class PLCB : public PostLoadCallback
	{
	public:
		BgManagerMax*	p;
		PLCB(BgManagerMax* pp) { p = pp; }

		Texmap *iblMap = 0;
		Texmap *iblBackplateMap = 0;
		Texmap *iblRefractionMap = 0;
		Texmap *iblReflectionMap = 0;
		Texmap *skyBackplateMap = 0;
		Texmap *skyRefractionMap = 0;
		Texmap *skyReflectionMap = 0;

		void proc(ILoad *iload)
		{
			if (iblMap)
				p->SetProperty(PARAM_BG_IBL_MAP, iblMap);
			if (iblBackplateMap)
				p->SetProperty(PARAM_BG_IBL_BACKPLATE, iblBackplateMap);
			if (iblRefractionMap)
				p->SetProperty(PARAM_BG_IBL_REFRACTIONMAP, iblRefractionMap);
			if (iblReflectionMap)
				p->SetProperty(PARAM_BG_IBL_REFLECTIONMAP, iblReflectionMap);
			if (skyBackplateMap)
				p->SetProperty(PARAM_BG_SKY_BACKPLATE, skyBackplateMap);
			if (skyRefractionMap)
				p->SetProperty(PARAM_BG_SKY_REFRACTIONMAP, skyRefractionMap);
			if (skyReflectionMap)
				p->SetProperty(PARAM_BG_SKY_REFLECTIONMAP, skyReflectionMap);
			delete this;
		}
	};
}

#define LOAD_TEX(chunkid, var)\
else if (ID == chunkid)\
{\
	ULONG id;\
	res = iload->Read(&id, sizeof(ULONG), &nb);\
	if ((res == IO_OK) && (id != 0xffffffff))\
		iload->RecordBackpatch(id, (void**)&var);\
}

#define BG_LOAD_BOOL_CHUNK(param) \
else if (ID == param) \
{ \
	BOOL v; \
	res = iload->Read(&v, sizeof(v), &nb); \
	if (res == IO_OK) \
		SetProperty(param, v); \
}

#define BG_LOAD_INT_CHUNK(param) \
else if (ID == param) \
{ \
	int v; \
	res = iload->Read(&v, sizeof(v), &nb); \
	if (res == IO_OK) \
		SetProperty(param, v); \
}

#define BG_LOAD_FLOAT_CHUNK(param) \
else if (ID == param) \
{ \
	float v; \
	res = iload->Read(&v, sizeof(v), &nb); \
	if (res == IO_OK) \
		SetProperty(param, v); \
}

#define BG_LOAD_COLOR_CHUNK(param) \
else if (ID == param) \
{ \
	Color v; \
	res = iload->Read(&v, sizeof(v), &nb); \
	if (res == IO_OK) \
		SetProperty(param, v); \
}


IOResult BgManagerMax::Load(ILoad *iload)
{
	int ID = 0;
	ULONG nb;
	IOResult res = IO_OK;
	
	PLCB* plcb = new PLCB(this);
	iload->RegisterPostLoadCallback(plcb);

	Class_ID cid;
	SClass_ID sid;

	while (IO_OK == (res = iload->OpenChunk()))
	{
		ID = iload->CurChunkID();
		
		if (ID == BGMANAGER_BASE_CHUNK)
		{
			res = pblock2->Load(iload);
		}
		LOAD_TEX(BGMANAGER_IBLMAP_CHUNK, plcb->iblMap)
		LOAD_TEX(BGMANAGER_IBLBACKP_CHUNK, plcb->iblBackplateMap)
		LOAD_TEX(BGMANAGER_IBLREFR_CHUNK, plcb->iblRefractionMap)
		LOAD_TEX(BGMANAGER_IBLREFL_CHUNK, plcb->iblReflectionMap)
		LOAD_TEX(BGMANAGER_SKYBACKP_CHUNK, plcb->skyBackplateMap)
		LOAD_TEX(BGMANAGER_SKYREFR_CHUNK, plcb->skyRefractionMap)
		LOAD_TEX(BGMANAGER_SKYREFL_CHUNK, plcb->skyReflectionMap)
		// LEGACY
		BG_LOAD_INT_CHUNK(PARAM_BG_TYPE)
		BG_LOAD_COLOR_CHUNK(PARAM_BG_IBL_SOLIDCOLOR)
		BG_LOAD_FLOAT_CHUNK(PARAM_BG_IBL_INTENSITY)
		BG_LOAD_BOOL_CHUNK(PARAM_BG_IBL_MAP_USE)
		BG_LOAD_BOOL_CHUNK(PARAM_BG_IBL_BACKPLATE_USE)
		BG_LOAD_BOOL_CHUNK(PARAM_BG_IBL_REFLECTIONMAP_USE)
		BG_LOAD_BOOL_CHUNK(PARAM_BG_IBL_REFRACTIONMAP_USE)
		BG_LOAD_FLOAT_CHUNK(PARAM_BG_SKY_AZIMUTH)
		BG_LOAD_FLOAT_CHUNK(PARAM_BG_SKY_ALTITUDE)
		BG_LOAD_INT_CHUNK(PARAM_BG_SKY_TYPE)
		BG_LOAD_INT_CHUNK(PARAM_BG_SKY_HOURS)
		BG_LOAD_INT_CHUNK(PARAM_BG_SKY_MINUTES)
		BG_LOAD_INT_CHUNK(PARAM_BG_SKY_SECONDS)
		BG_LOAD_INT_CHUNK(PARAM_BG_SKY_DAY)
		BG_LOAD_INT_CHUNK(PARAM_BG_SKY_MONTH)
		BG_LOAD_INT_CHUNK(PARAM_BG_SKY_YEAR)
		BG_LOAD_FLOAT_CHUNK(PARAM_BG_SKY_TIMEZONE)
		BG_LOAD_FLOAT_CHUNK(PARAM_BG_SKY_LATITUDE)
		BG_LOAD_FLOAT_CHUNK(PARAM_BG_SKY_LONGITUDE)
		BG_LOAD_BOOL_CHUNK(PARAM_BG_SKY_DAYLIGHTSAVING)
		BG_LOAD_FLOAT_CHUNK(PARAM_BG_SKY_HAZE)
		BG_LOAD_FLOAT_CHUNK(PARAM_BG_SKY_GLOW)
		BG_LOAD_FLOAT_CHUNK(PARAM_BG_SKY_REDBLUE_SHIFT)
		BG_LOAD_COLOR_CHUNK(PARAM_BG_SKY_GROUND_COLOR)
		BG_LOAD_FLOAT_CHUNK(PARAM_BG_SKY_DISCSIZE)
		BG_LOAD_FLOAT_CHUNK(PARAM_BG_SKY_HORIZON_HEIGHT)
		BG_LOAD_FLOAT_CHUNK(PARAM_BG_SKY_HORIZON_BLUR)
		BG_LOAD_FLOAT_CHUNK(PARAM_BG_SKY_INTENSITY)
		BG_LOAD_FLOAT_CHUNK(PARAM_BG_SKY_SHADOW_SOFTNESS)
		BG_LOAD_FLOAT_CHUNK(PARAM_BG_SKY_SATURATION)
		BG_LOAD_BOOL_CHUNK(PARAM_GROUND_ACTIVE)
		BG_LOAD_FLOAT_CHUNK(PARAM_GROUND_RADIUS)
		BG_LOAD_FLOAT_CHUNK(PARAM_GROUND_GROUND_HEIGHT)
		BG_LOAD_BOOL_CHUNK(PARAM_GROUND_SHADOWS)
		BG_LOAD_BOOL_CHUNK(PARAM_GROUND_REFLECTIONS)
		BG_LOAD_FLOAT_CHUNK(PARAM_GROUND_REFLECTIONS_STRENGTH)
		BG_LOAD_COLOR_CHUNK(PARAM_GROUND_REFLECTIONS_COLOR)
		BG_LOAD_FLOAT_CHUNK(PARAM_GROUND_REFLECTIONS_ROUGHNESS)
		
		iload->CloseChunk();
		FASSERT(res == IO_OK);
		if (res != IO_OK)
			return res;
	}

	if (res == IO_END)
		res = IO_OK;		// don't interrupt GUP data loading

	return res;
}

FIRERENDER_NAMESPACE_END;
