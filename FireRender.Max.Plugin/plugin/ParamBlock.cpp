/*********************************************************************************************************************************
* Radeon ProRender for 3ds Max plugin
* Copyright (C) 2017 AMD
* All Rights Reserved
*
* 3ds Max storage of rendering plugin parameters
*********************************************************************************************************************************/
#include "plugin/ParamBlock.h"
#include "plugin/FireRenderer.h"
#include "plugin/CamManager.h"
#include "plugin/BgManager.h"
#include "ClassDescs.h"
#include "resource.h"
#include <RadeonProRender.h>
#include <map>

FIRERENDER_NAMESPACE_BEGIN

FireRenderClassDesc fireRenderClassDesc;

static BgAccessor theBgAccessor;
static SamplingAccessor theSamplingAccessor;

// Values of raycast epsilon constants in meters
static const float rprRenderRaycastEpsilonMin = 0.0f;
static const float rprRenderRaycastEpsilonMax = 0.01f;
static const float rprRenderRaycastEpsilonDefault = 0.00002f;

static const int bilateralRadiusDefault = 3;
static const int bilateralRadiusMin = 1;
static const int bilateralRadiusMax = 10;

static const int lwrSamplesDefault = 4;
static const int lwrSamplesMin = 1;
static const int lwrSamplesMax = 10;

static const int lwrRadiusDefault = 4;
static const int lwrRadiusMin = 1;
static const int lwrRadiusMax = 10;

static const float lwrBandwidthDefault = 0.1f;
static const float lwrBandwidthMin = 0.1f;
static const float lwrBandwidthMax = 1.0f;

static const float eawColorDefault = 0.1f;
static const float eawColorMin = 0.1f;
static const float eawColorMax = 1.0f;

static const float eawNormalDefault = 0.1f;
static const float eawNormalMin = 0.1f;
static const float eawNormalMax = 1.0f;

static const float eawDepthDefault = 0.1f;
static const float eawDepthMin = 0.1f;
static const float eawDepthMax = 1.0f;

static const float eawObjectIdDefault = 0.1f;
static const float eawObjectIdMin = 0.1f;
static const float eawObjectIdMax = 1.0f;

std::tuple<float, float, float> GetRayCastConstants()
{
	return std::make_tuple(rprRenderRaycastEpsilonMin, rprRenderRaycastEpsilonMax, rprRenderRaycastEpsilonDefault);
}


/// All rendering plugin parameters. Type of parameters cannot be changed and parameters cannot be removed without possibly 
/// breaking backwards compatibility of loading old scenes! Instead of removing a parameter it should be flagged with P_OBSOLETE
/// as shown in the code below. Instead of changing parameter type, the parameter should be declared obsolete and new parameter
/// should be added with the new type, with possible copying of the old param to new one in post load callback. Parameter order 
/// should never be changed. Adding new parameters at the end of the list is always safe.
ParamBlockDesc2 FIRE_MAX_PBDESC(
    0, _T("pbdesc"), IDS_PBDESC, &fireRenderClassDesc, P_VERSION + P_AUTO_CONSTRUCT, FIREMAXVER_LATEST, 0,

    PARAM_MAX_RAY_DEPTH, _T("maxRayDepth"), TYPE_INT, 0, 0,
    p_range, 1, 50, p_default, 5, PB_END,

    // Values from RadeonProRender.h:
    // RPR_RENDER_MODE_GLOBAL_ILLUMINATION              0x1
    // RPR_RENDER_MODE_DIRECT_ILLUMINATION              0x2
    // RPR_RENDER_MODE_DIRECT_ILLUMINATION_NO_SHADOW    0x3
    // RPR_RENDER_MODE_WIREFRAME                        0x4
    // RPR_RENDER_MODE_DIFFUSE_ALBEDO                   0x5
    // RPR_RENDER_MODE_POSITION                         0x6
    // RPR_RENDER_MODE_NORMAL                           0x7
    // RPR_RENDER_MODE_TEXCOORD                         0x8
    // RPR_RENDER_MODE_AMBIENT_OCCLUSION                0x9
    PARAM_RENDER_MODE, _T("renderMode"), TYPE_INT, 0, 0,
    p_range, RPR_RENDER_MODE_GLOBAL_ILLUMINATION, RPR_RENDER_MODE_AMBIENT_OCCLUSION, p_default, RPR_RENDER_MODE_GLOBAL_ILLUMINATION,
    PB_END,

    // Values from RadeonProRender.h:
    // RPR_FILTER_BOX            0x1
    // RPR_FILTER_TRIANGLE       0x2
    // RPR_FILTER_GAUSSIAN       0x3
    // RPR_FILTER_MITCHELL       0x4
    // RPR_FILTER_LANCZOS        0x5
    // RPR_FILTER_BLACKMANHARRIS 0x6
    PARAM_IMAGE_FILTER, _T("imageFilter"), TYPE_INT, 0, 0,
    p_range, RPR_FILTER_BOX, RPR_FILTER_BLACKMANHARRIS, p_default, RPR_FILTER_MITCHELL, PB_END,


    PARAM_TIME_LIMIT, _T("timeLimit"), TYPE_INT, 0, 0,
    p_range, 59, INT_MAX, p_default, 59, PB_END,

    PARAM_PASS_LIMIT, _T("passLimit"), TYPE_INT, 0, 0,
    p_range, 1, INT_MAX, p_default, 1,
	p_accessor, &theSamplingAccessor,
	PB_END,

	PARAM_SAMPLES_MIN, _T("samplesMin"), TYPE_INT, 0, 0,
	p_range, 2, INT_MAX, p_default, 16,
	p_accessor, &theSamplingAccessor,
	PB_END,

	PARAM_SAMPLES_MAX, _T("samplesMax"), TYPE_INT, 0, 0,
	p_range, 0, INT_MAX, p_default, 400,
	PB_END,

	PARAM_ADAPTIVE_NOISE_THRESHOLD, _T("adaptiveNoiseThreshold"), TYPE_FLOAT, 0, 0,
	p_range, 0.0, 1.0, p_default, 0.01,
	PB_END,

	PARAM_ADAPTIVE_TILESIZE, _T("adaptiveTileSize"), TYPE_INT, 0, 0,
	p_range, 4, 16, p_default, 4,
	PB_END,

	OBSOLETE_PARAM_CAMERA_USE_FOV, _T("useFov"), TYPE_BOOL, P_ANIMATABLE|P_INVISIBLE|P_OBSOLETE, IDS_STRING226,
	p_default, TRUE, PB_END,

	OBSOLETE_PARAM_USE_DOF, _T("useDof"), TYPE_BOOL, P_ANIMATABLE | P_INVISIBLE | P_OBSOLETE, IDS_STRING227,
	p_default, FALSE, PB_END,

	OBSOLETE_PARAM_CAMERA_OVERWRITE_DOF_SETTINGS, _T("overWriteDof"), TYPE_BOOL, P_ANIMATABLE | P_INVISIBLE | P_OBSOLETE, IDS_STRING228,
    p_default, FALSE, PB_END,

	OBSOLETE_PARAM_CAMERA_BLADES, _T("cameraBlades"), TYPE_INT, P_ANIMATABLE | P_INVISIBLE | P_OBSOLETE, IDS_STRING229,
    p_range, 3, 64, p_default, 6, PB_END,

	//we are storing it in meters
	OBSOLETE_PARAM_CAMERA_FOCUS_DIST, _T("perspectiveFocalDist"), TYPE_FLOAT, P_ANIMATABLE | P_INVISIBLE | P_OBSOLETE, IDS_STRING230,
    p_range, 1e-6f, 10e20f, p_default, 1.f, PB_END,

	OBSOLETE_PARAM_CAMERA_F_STOP, _T("fStop"), TYPE_FLOAT, P_ANIMATABLE | P_INVISIBLE | P_OBSOLETE, IDS_STRING231,
    p_range, 0.01f, 999.f, p_default, 5.6f,
	PB_END,

	OBSOLETE_PARAM_CAMERA_SENSOR_WIDTH, _T("cameraSensorSize"), TYPE_FLOAT, P_ANIMATABLE | P_INVISIBLE | P_OBSOLETE, IDS_STRING232,
    p_range, 0.001f, 9999.f, p_default, 36.f, PB_END,

	OBSOLETE_PARAM_CAMERA_EXPOSURE, _T("exposure"), TYPE_FLOAT, P_ANIMATABLE | P_INVISIBLE | P_OBSOLETE, IDS_STRING233,
    p_range, 0.f, 10e20f, p_default, 1.f, PB_END,

	OBSOLETE_PARAM_CAMERA_FOCAL_LENGTH, _T("cameraFocalLength"), TYPE_FLOAT, P_ANIMATABLE | P_INVISIBLE | P_OBSOLETE, IDS_STRING234,
	p_range, 1e-6f, 10e20f, p_default, 40.f, PB_END,

	OBSOLETE_PARAM_CAMERA_FOV, _T("cameraFOV"), TYPE_FLOAT, P_ANIMATABLE | P_INVISIBLE | P_OBSOLETE, IDS_STRING235,
	p_range, -360.f, 360.f, p_default, 0.78539816339f, PB_END,

	OBSOLETE_PARAM_CAMERA_TYPE, _T("cameraType"), TYPE_INT, P_OBSOLETE, 0,
    p_range, 0, FRCameraType_Count-1, p_default, FRCameraType_Default,
	PB_END,

    PARAM_LOCK_MAX, _T("lockMax"), TYPE_BOOL, P_OBSOLETE, 0,
    p_default, FALSE, PB_END,

    PARAM_USE_CPU, _T("useCpu"), TYPE_BOOL, P_OBSOLETE, 0,
    p_default, FALSE, PB_END,

    PARAM_GPU_COUNT, _T("useGpu"), TYPE_INT, P_OBSOLETE, 0,
    p_range, 1, 4, p_default, 1, PB_END,

    PARAM_MTL_PREVIEW_PASSES, _T("mtlPreviewPasses"), TYPE_INT, 0, 0,
    p_range, 1, 10000, p_default, 32, PB_END,

    PARAM_INTERACTIVE_MODE, _T("interactiveMode"), TYPE_BOOL, P_ANIMATABLE | P_INVISIBLE | P_OBSOLETE, 0,
    p_default, TRUE, PB_END,

	PARAM_TRACEDUMP_BOOL, _T("tracedumpbool"), TYPE_BOOL, 0, 0,
	p_default, FALSE, PB_END,

	PARAM_USE_TEXTURE_COMPRESSION, _T("texturecompression"), TYPE_BOOL, 0, 0,
	p_default, FALSE, PB_END,

	PARAM_TRACEDUMP_PATH, _T("tracedumppath"), TYPE_STRING, 0, 0,
	p_default, "", PB_END,

    406, _T("obsolete"), TYPE_INT, P_OBSOLETE, 0, PB_END, // has to be left in so old scenes will load

    PARAM_IMAGE_FILTER_WIDTH, _T("imageFilterSize"), TYPE_FLOAT, 0, 0,
    p_default, 1.5f, p_range, 1.f, 10.f, PB_END,

    PARAM_VFB_REFRESH, _T("vfbRefresh"), TYPE_INT, 0, 0,
    p_default, 10, p_range, 10, 5000, PB_END,

	/////////////////////////////////////////////////////////////////////////////
	// TONEMAPPERS
	//

	OBSOLETE_PARAM_OVERRIDE_MAX_TONEMAPPERS, _T("overrideTonemap"), TYPE_BOOL, 0, 0,
	p_default, TRUE, PB_END,

    OBSOLETE_PARAM_TONEMAP_OPERATOR, _T("tonemapOperator"), TYPE_INT, 0, 0, 
    p_default, frw::ToneMappingTypeNone, p_range, frw::ToneMappingTypeNone, frw::ToneMappingTypeNonLinear, PB_END,

	OBSOLETE_PARAM_TONEMAP_LINEAR_SCALE, _T("tonemapLinearScale"), TYPE_FLOAT, P_ANIMATABLE|P_INVISIBLE|P_OBSOLETE, 0,
	p_default, 1.f, PB_END,
	OBSOLETE_PARAM_TONEMAP_PHOTOLINEAR_SENSITIVITY, _T("tonemapPhotoSensitivity"), TYPE_FLOAT, P_ANIMATABLE|P_INVISIBLE|P_OBSOLETE, 0,
	p_default, 1.f, PB_END,
	OBSOLETE_PARAM_TONEMAP_PHOTOLINEAR_EXPOSURE, _T("tonemapPhotoExposure"), TYPE_FLOAT, P_ANIMATABLE|P_INVISIBLE|P_OBSOLETE, 0,
	p_default, 1.f, PB_END,
	OBSOLETE_PARAM_TONEMAP_PHOTOLINEAR_FSTOP, _T("tonemapPhotoFstop"), TYPE_FLOAT, P_ANIMATABLE | P_INVISIBLE | P_OBSOLETE, 0,
	p_default, 1.f, PB_END,
	OBSOLETE_PARAM_TONEMAP_REINHARD_PRESCALE, _T("tonemapReinhardPrescale"), TYPE_FLOAT, P_ANIMATABLE | P_INVISIBLE | P_OBSOLETE, 0,
	p_default, 1.f, PB_END,
	OBSOLETE_PARAM_TONEMAP_REINHARD_POSTSCALE, _T("tonemapReinhardPostscale"), TYPE_FLOAT, P_ANIMATABLE | P_INVISIBLE | P_OBSOLETE, 0,
	p_default, 1.f, PB_END,
	OBSOLETE_PARAM_TONEMAP_REINHARD_BURN, _T("tonemapReinhardBurn"), TYPE_FLOAT, P_ANIMATABLE | P_INVISIBLE | P_OBSOLETE, 0,
	p_default, 1.f, PB_END,
	OBSOLETE_PARAM_TONEMAP_WHITE_BALANCE, _T("tonemapWhiteBalance"), TYPE_FLOAT, P_ANIMATABLE | P_INVISIBLE | P_OBSOLETE, 0,
	p_default, 1.f, PB_END,
	OBSOLETE_PARAM_TONEMAP_CONTRAST, _T("tonemapContrast"), TYPE_FLOAT, P_ANIMATABLE | P_INVISIBLE | P_OBSOLETE, 0,
	p_default, 1.f, PB_END,
	OBSOLETE_PARAM_TONEMAP_SHUTTER_SPEED, _T("tonemapShutterSpeed"), TYPE_FLOAT, P_ANIMATABLE | P_INVISIBLE | P_OBSOLETE, 0,
	p_default, 1.f, PB_END,
	OBSOLETE_PARAM_TONEMAP_SENSOR_WIDTH, _T("tonemapSensorWidth"), TYPE_FLOAT, P_ANIMATABLE | P_INVISIBLE | P_OBSOLETE, 0,
	p_default, 1.f, PB_END,

    OBSOLETE_PARAM_AA_GRID_SIZE, _T("aaGridSize"), TYPE_FLOAT, P_OBSOLETE, 0,
    p_default, 4.f, p_range, 1.f, float(16.0f /*RPR_MAX_AA_GRID_SIZE*/), PB_END,

	OBSOLETE_PARAM_AA_SAMPLE_COUNT, _T("aaSampleCount"), TYPE_FLOAT, P_OBSOLETE, 0,
    p_default, 1.f, p_range, 1.f, float(32.0f /*RPR_MAX_AA_SAMPLES*/), PB_END,

	// Values from ParamBlock.h:
	// RPR_GLOBAL_ILLUMINATION_SOLVER_PATH_TRACING 0x1
	// RPR_GLOBAL_ILLUMINATION_SOLVER_CACHEDGI 0x2
	PARAM_GLOBAL_ILLUMINATION_SOLVER, _T("globalIlluminationSolver"), TYPE_INT, 0, 0,
	p_range, RPR_GLOBAL_ILLUMINATION_SOLVER_PATH_TRACING, RPR_GLOBAL_ILLUMINATION_SOLVER_CACHEDGI, p_default, RPR_GLOBAL_ILLUMINATION_SOLVER_PATH_TRACING,
	PB_END,

	PARAM_USE_PHOTOLINEAR_TONEMAP, _T("photolinearTonemap"), TYPE_BOOL, P_OBSOLETE|P_ANIMATABLE, 0,
	p_default, TRUE, PB_END,

	// Values from ParamBlock.h:
	// RPR_QUALITY_PRESET_PRODUCTION 0x1
	// RPR_QUALITY_PRESET_PREVIEW 0x2
	PARAM_QUALITY_PRESET, _T("qualityPreset"), TYPE_INT, P_ANIMATABLE|P_OBSOLETE, 0,
	p_range, RPR_QUALITY_PRESET_PRODUCTION, RPR_QUALITY_PRESET_PREVIEW, p_default, RPR_QUALITY_PRESET_PREVIEW,
	PB_END,

	PARAM_LIMIT_LIGHT_BOUNCE, _T("limitLightBounce"), TYPE_BOOL, 0, 0,
	p_default, FALSE, PB_END,

	PARAM_MAX_LIGHT_BOUNCES, _T("maxLightBounces"), TYPE_INT, 0, 0,
	p_range, 1, 5, p_default, 4,
	PB_END,

	PARAM_RENDER_DEVICE, _T("renderDevice"), TYPE_INT, P_OBSOLETE | P_ANIMATABLE | P_INVISIBLE, 0,
	p_range, 0, 0, p_default, 0,
	PB_END,

	PARAM_USE_IRRADIANCE_CLAMP, _T("useIrradianceClamp"), TYPE_BOOL, 0, 0,
	p_default, TRUE, PB_END,

	PARAM_IRRADIANCE_CLAMP, _T("irradianceClamp"), TYPE_FLOAT, 0, 0,
	p_default, 1.f, p_range, 1.f, 999999.f, PB_END,

	OBSOLETE_PARAM_TONEMAP_PHOTOLINEAR_ISO, _T("tonemapPhotoISO"), TYPE_INT, P_ANIMATABLE | P_INVISIBLE | P_OBSOLETE, 0,
	p_default, 100, p_range, 1, 100000, PB_END,

	PARAM_GPU_WAS_COMPATIBLE, _T("gpuWasCompatible"), TYPE_BOOL, P_OBSOLETE, 0, p_default, TRUE, PB_END,
	PARAM_GPU_INCOMPATIBILITY_WARNING_WAS_SHOWN, _T("gpuIncompatibilityWarningWasShown"), TYPE_BOOL, 0, 0, p_default, FALSE, PB_END,

	OBSOLETE_PARAM_USE_MOTION_BLUR, _T("enableMotionblur"), TYPE_BOOL, 0, 0,
    p_default, FALSE, PB_END,

	OBSOLETE_PARAM_MOTION_BLUR_SCALE, _T("motionblurScale"), TYPE_FLOAT, P_ANIMATABLE, IDS_STRING236,
	p_default, 100.0, p_range, 0.0, 10e20f, PB_END,

	PARAM_GPU_SELECTED_BY_USER, _T("gpuUserSelection"), TYPE_BOOL, P_OBSOLETE, 0,
	p_default, FALSE, PB_END,

	PARAM_WARNING_DONTSHOW, _T("warning_DoNotShow"), TYPE_BOOL, 0, 0,
	p_default, false, PB_END,

	// Values from ParamBlock.h:
	// RPR_RENDER_LIMIT_PASS			0x01
	// RPR_RENDER_LIMIT_TIME			0x02
	// RPR_RENDER_LIMIT_UNLIMITED		0x03
	// RPR_RENDER_LIMIT_PASS_OR_TIME	0x04
	PARAM_RENDER_LIMIT, _T("renderLimit"), TYPE_INT, 0, 0,
	p_range, TerminationCriteria::enum_first, TerminationCriteria::enum_last, p_default, TerminationCriteria::Termination_None,
	p_accessor, &theSamplingAccessor,
	PB_END,

	PARAM_TEXMAP_RESOLUTION, _T("texmapResolution"), TYPE_INT, 0, 0,
	p_default, 512, p_range, 4, 16384, p_end,

	/////////////////////////////////////////////////////////////////////////////
	// BACKGROUND AND GROUND TRANSIENTS
	//
	TRPARAM_BG_USE, _T("backgroundOverride"), TYPE_BOOL, P_TRANSIENT, 0,
	p_default, FALSE, p_accessor, &theBgAccessor, PB_END,

	TRPARAM_BG_TYPE, _T("backgroundType"), TYPE_INT, P_TRANSIENT, 0,
	p_range, FRBackgroundType_IBL, FRBackgroundType_SunSky, p_default, FRBackgroundType_IBL, p_accessor, &theBgAccessor, PB_END,

	TRPARAM_BG_IBL_SOLIDCOLOR, _T("backgroundColor"), TYPE_RGBA, P_ANIMATABLE | P_TRANSIENT, IDS_STRING237,
	p_default, Color(0.5f, 0.5f, 0.5f), p_accessor, &theBgAccessor, PB_END,

	TRPARAM_BG_IBL_INTENSITY, _T("backgroundIntensity"), TYPE_FLOAT, P_ANIMATABLE | P_TRANSIENT, IDS_STRING238,
	p_default, 1.0, p_range, 0.0, FLT_MAX, p_accessor, &theBgAccessor, PB_END,

	TRPARAM_BG_IBL_MAP, _T("backgroundMap"), TYPE_TEXMAP, P_TRANSIENT, 0, p_accessor, &theBgAccessor, PB_END,
		
	TRPARAM_BG_IBL_BACKPLATE, _T("backgroundBackplate"), TYPE_TEXMAP, P_TRANSIENT, 0, p_accessor, &theBgAccessor, PB_END,

	TRPARAM_BG_IBL_REFLECTIONMAP, _T("backgroundReflectionMap"), TYPE_TEXMAP, P_TRANSIENT, 0, p_accessor, &theBgAccessor, PB_END,

	TRPARAM_BG_IBL_REFRACTIONMAP, _T("backgroundRefractionMap"), TYPE_TEXMAP, P_TRANSIENT, 0, p_accessor, &theBgAccessor, PB_END,

	TRPARAM_BG_IBL_MAP_USE, _T("useBackgroundMap"), TYPE_BOOL, P_TRANSIENT, 0,
	p_default, FALSE, p_accessor, &theBgAccessor, PB_END,

	TRPARAM_BG_IBL_BACKPLATE_USE, _T("useBackgroundBackplateMap"), TYPE_BOOL, P_TRANSIENT, 0,
	p_default, FALSE, p_accessor, &theBgAccessor, PB_END,

	TRPARAM_BG_IBL_REFLECTIONMAP_USE, _T("useBackgroundReflectionMap"), TYPE_BOOL, P_TRANSIENT, 0,
	p_default, FALSE, p_accessor, &theBgAccessor, PB_END,

	TRPARAM_BG_IBL_REFRACTIONMAP_USE, _T("useBackgroundRefractionMap"), TYPE_BOOL, P_TRANSIENT, 0,
	p_default, FALSE, p_accessor, &theBgAccessor, PB_END,

	TRPARAM_BG_SKY_TYPE, _T("backgroundSkyType"), TYPE_INT, P_TRANSIENT, 0,
	p_range, FRBackgroundSkyType_Analytical, FRBackgroundSkyType_DateTimeLocation,
	p_default, FRBackgroundSkyType_DateTimeLocation, p_accessor, &theBgAccessor, PB_END,

	TRPARAM_BG_SKY_AZIMUTH, _T("backgroundSkyAzimuth"), TYPE_FLOAT, P_ANIMATABLE | P_TRANSIENT, IDS_STRING239,
	p_default, 0.0, p_range, 0.0, 360.0, p_accessor, &theBgAccessor, PB_END,

	TRPARAM_BG_SKY_ALTITUDE, _T("backgroundSkyAltitude"), TYPE_FLOAT, P_ANIMATABLE | P_TRANSIENT, IDS_STRING240,
	p_default, 45.0, p_range, -90.0, 90.0, p_accessor, &theBgAccessor, PB_END,

	TRPARAM_BG_SKY_HOURS, _T("backgroundSkyHours"), TYPE_INT, P_ANIMATABLE | P_TRANSIENT, IDS_STRING258,
	p_default, 12, p_range, 0, 24, p_accessor, &theBgAccessor, PB_END,

	TRPARAM_BG_SKY_MINUTES, _T("backgroundSkyMinutes"), TYPE_INT, P_ANIMATABLE | P_TRANSIENT, IDS_STRING259,
	p_default, 0, p_range, 0, 59, p_accessor, &theBgAccessor, PB_END,

	TRPARAM_BG_SKY_SECONDS, _T("backgroundSkySeconds"), TYPE_INT, P_ANIMATABLE | P_TRANSIENT, IDS_STRING260,
	p_default, 0, p_range, 0, 59, p_accessor, &theBgAccessor, PB_END,

	TRPARAM_BG_SKY_DAY, _T("backgroundSkyDay"), TYPE_INT, P_ANIMATABLE | P_TRANSIENT, IDS_STRING261,
	p_default, 1, p_range, 1, 31, p_accessor, &theBgAccessor, PB_END,

	TRPARAM_BG_SKY_MONTH, _T("backgroundSkyMonth"), TYPE_INT, P_ANIMATABLE | P_TRANSIENT, IDS_STRING262,
	p_default, 1, p_range, 1, 12, p_accessor, &theBgAccessor, PB_END,

	TRPARAM_BG_SKY_YEAR, _T("backgroundSkyYear"), TYPE_INT, P_ANIMATABLE | P_TRANSIENT, IDS_STRING263,
	p_default, 2016, p_range, -2000, 6000, p_accessor, &theBgAccessor, PB_END,

	TRPARAM_BG_SKY_TIMEZONE, _T("backgroundSkyTimezone"), TYPE_FLOAT, P_ANIMATABLE | P_TRANSIENT, IDS_STRING241,
	p_default, 0, p_range, -18.f, 18.f, p_accessor, &theBgAccessor, PB_END,

	TRPARAM_BG_SKY_LATITUDE, _T("backgroundSkyLatitude"), TYPE_FLOAT, P_ANIMATABLE | P_TRANSIENT, IDS_STRING242,
	p_default, 38.4237, p_range, -90.0, 90.0, p_accessor, &theBgAccessor, PB_END,

	TRPARAM_BG_SKY_LONGITUDE, _T("backgroundSkyLongitude"), TYPE_FLOAT, P_ANIMATABLE | P_TRANSIENT, IDS_STRING243,
	p_default, 27.1428, p_range, -180.0, 180.0, p_accessor, &theBgAccessor, PB_END,

	TRPARAM_BG_SKY_DAYLIGHTSAVING, _T("backgroundSkyDaylightSaving"), TYPE_BOOL, P_ANIMATABLE | P_TRANSIENT, IDS_STRING244,
	p_default, TRUE, p_accessor, &theBgAccessor, PB_END,

	TRPARAM_BG_SKY_HAZE, _T("backgroundSkyHaze"), TYPE_FLOAT, P_ANIMATABLE | P_TRANSIENT, IDS_STRING245,
	p_default, 0.001, p_range, 0.0, 100.0, p_accessor, &theBgAccessor, PB_END,

	TRPARAM_BG_SKY_GLOW, _T("backgroundSkyGlow"), TYPE_FLOAT, P_ANIMATABLE | P_TRANSIENT | P_INVISIBLE | P_OBSOLETE, IDS_STRING246,
	p_default, 0.1, p_range, 0.0, 100.0, p_accessor, &theBgAccessor, PB_END,

	TRPARAM_BG_SKY_REDBLUE_SHIFT, _T("backgroundSkyRbs"), TYPE_FLOAT, P_INVISIBLE | P_OBSOLETE | P_RESET_DEFAULT, 0,
	p_default, 0.0, p_range, -1.0, 1.0, p_accessor, &theBgAccessor, PB_END,

	TRPARAM_BG_SKY_GROUND_COLOR, _T("backgroundSkyGroundColor"), TYPE_RGBA, P_ANIMATABLE | P_TRANSIENT, IDS_STRING248,
	p_default, Color(0.4f, 0.4f, 0.4f), p_accessor, &theBgAccessor, PB_END,

	TRPARAM_BG_SKY_DISCSIZE, _T("backgroundSkySunDisc"), TYPE_FLOAT, P_ANIMATABLE | P_TRANSIENT, IDS_STRING249,
	p_default, 1.0, p_range, 0.0001, 10.0, p_accessor, &theBgAccessor, PB_END,

	TRPARAM_BG_SKY_HORIZON_HEIGHT, _T("backgroundSkyHorizonHeight"), TYPE_FLOAT, P_ANIMATABLE | P_TRANSIENT | P_INVISIBLE | P_OBSOLETE, IDS_STRING250,
	p_default, 0.001, p_range, FLT_MIN, FLT_MAX, p_accessor, &theBgAccessor, PB_END,

	TRPARAM_BG_SKY_HORIZON_BLUR, _T("backgroundSkyHorizonBlur"), TYPE_FLOAT, P_ANIMATABLE | P_TRANSIENT | P_INVISIBLE | P_OBSOLETE, IDS_STRING251,
	p_default, 0.1, p_range, 0.0, 1.0, p_accessor, &theBgAccessor, PB_END,

	TRPARAM_BG_SKY_INTENSITY, _T("backgroundSkyIntensity"), TYPE_FLOAT, P_ANIMATABLE | P_TRANSIENT, IDS_STRING252,
	p_default, 2.0, p_range, 0.0, FLT_MAX, p_accessor, &theBgAccessor, PB_END,

	TRPARAM_GROUND_ACTIVE, _T("groundActive"), TYPE_BOOL, P_TRANSIENT, 0,
	p_default, FALSE, p_accessor, &theBgAccessor, PB_END,

	TRPARAM_GROUND_RADIUS, _T("groundRadius"), TYPE_FLOAT, P_ANIMATABLE | P_TRANSIENT, IDS_STRING253,
	p_default, 0.0, p_range, 0.0, FLT_MAX, p_accessor, &theBgAccessor, PB_END,

	TRPARAM_GROUND_GROUND_HEIGHT, _T("groundHeight"), TYPE_FLOAT, P_ANIMATABLE | P_TRANSIENT, IDS_STRING254,
	p_default, 0.0, p_range, -FLT_MAX, FLT_MAX, p_accessor, &theBgAccessor, PB_END,

	TRPARAM_GROUND_SHADOWS, _T("groundShadows"), TYPE_BOOL, P_TRANSIENT, 0,
	p_default, FALSE, p_accessor, &theBgAccessor, PB_END,

	TRPARAM_GROUND_REFLECTIONS_STRENGTH, _T("groundReflectionsStrength"), TYPE_FLOAT, P_ANIMATABLE | P_TRANSIENT, IDS_STRING255,
	p_default, 0.5, p_range, 0.0, 1.0, p_accessor, &theBgAccessor, PB_END,

	TRPARAM_GROUND_REFLECTIONS, _T("groundReflections"), TYPE_BOOL, P_TRANSIENT, 0,
	p_default, FALSE, p_accessor, &theBgAccessor, PB_END,

	TRPARAM_GROUND_REFLECTIONS_COLOR, _T("groundReflectionsColor"), TYPE_RGBA, P_ANIMATABLE | P_TRANSIENT, IDS_STRING256,
	p_default, Color(0.5f, 0.5f, 0.5f), p_accessor, &theBgAccessor, PB_END,

	TRPARAM_GROUND_REFLECTIONS_ROUGHNESS, _T("groundReflectionsRoughness"), TYPE_FLOAT, P_ANIMATABLE | P_TRANSIENT, IDS_STRING257,
	p_default, 0.0, p_range, 0.0, FLT_MAX, p_accessor, &theBgAccessor, PB_END,

	TRPARAM_BG_SKY_BACKPLATE, _T("backgroundBackplate"), TYPE_TEXMAP, P_TRANSIENT, 0, p_accessor, &theBgAccessor, PB_END,

	TRPARAM_BG_SKY_REFLECTIONMAP, _T("skyReflectionMap"), TYPE_TEXMAP, P_TRANSIENT, 0, p_accessor, &theBgAccessor, PB_END,

	TRPARAM_BG_SKY_REFRACTIONMAP, _T("skyRefractionMap"), TYPE_TEXMAP, P_TRANSIENT, 0, p_accessor, &theBgAccessor, PB_END,

	TRPARAM_BG_SKY_BACKPLATE_USE, _T("useSkyBackplateMap"), TYPE_BOOL, P_TRANSIENT, 0,
	p_default, FALSE, p_accessor, &theBgAccessor, PB_END,

	TRPARAM_BG_SKY_REFLECTIONMAP_USE, _T("useSkyReflectionMap"), TYPE_BOOL, P_TRANSIENT, 0,
	p_default, FALSE, p_accessor, &theBgAccessor, PB_END,

	TRPARAM_BG_SKY_REFRACTIONMAP_USE, _T("useSkyRefractionMap"), TYPE_BOOL, P_TRANSIENT, 0,
	p_default, FALSE, p_accessor, &theBgAccessor, PB_END,

	TRPARAM_BG_ENABLEALPHA, _T("enableBackgroundAlpha"), TYPE_BOOL, P_TRANSIENT, 0,
	p_default, FALSE, p_accessor, &theBgAccessor, PB_END,

	PARAM_STAMP_ENABLED, _T("enableRenderStamp"), TYPE_BOOL, 0, 0,
	p_default, FALSE, PB_END,

	PARAM_STAMP_TEXT, _T("renderStamp"), TYPE_STRING, 0, 0,
	p_default, DEFAULT_RENDER_STAMP, PB_END,

	PARAM_OVERRIDE_MAX_PREVIEW_SHAPES, _T("overrideMaxMaterialPreviewShapes"), TYPE_BOOL, 0, 0,
	p_default, TRUE, PB_END,

	TRPARAM_BG_SKY_FILTER_COLOR, _T("backgroundSkyFilterColor"), TYPE_RGBA, P_ANIMATABLE | P_TRANSIENT, IDS_STRING264,
	p_default, Color(0.0f, 0.0f, 0.0f), p_accessor, &theBgAccessor, PB_END,

	TRPARAM_BG_SKY_SHADOW_SOFTNESS, _T("backgroundSkyShadowSoftness"), TYPE_FLOAT, P_ANIMATABLE | P_TRANSIENT, IDS_STRING266,
	p_default, 0.0, p_range, 0.0, 1.0, p_accessor, &theBgAccessor, PB_END,

	TRPARAM_BG_SKY_SATURATION, _T("backgroundSkySaturation"), TYPE_FLOAT, P_ANIMATABLE | P_TRANSIENT, IDS_STRING267,
	p_default, 1.0, p_range, 0.0, 1.0, p_accessor, &theBgAccessor, PB_END,

	PARAM_AA_GRID_SIZE, _T("aaGridSize"), TYPE_INT, P_OBSOLETE, 0,
	p_default, 4, p_range, 1, 16 /*RPR_MAX_AA_GRID_SIZE*/, PB_END,

	PARAM_AA_SAMPLE_COUNT, _T("aaSampleCount"), TYPE_INT, P_OBSOLETE, 0,
	p_default, 1, p_range, 1, 32 /*RPR_MAX_AA_SAMPLES*/, PB_END,
	
	TRPARAM_BG_SKY_GROUND_ALBEDO, _T("backgroundSkyGroundAlbedo"), TYPE_RGBA, P_ANIMATABLE | P_TRANSIENT, IDS_STRING247,
	p_default, Color(1.0f, 1.0f, 1.0f), p_accessor, &theBgAccessor, PB_END,

	PARAM_QUALITY_RAYCAST_EPSILON, _T("raycastEpsilon"), TYPE_FLOAT, 0, 0, p_default, rprRenderRaycastEpsilonDefault,
	p_range, rprRenderRaycastEpsilonMin, rprRenderRaycastEpsilonMax, PB_END,

	// Denoiser
	PARAM_DENOISER_ENABLED, _T("enableDenoiser"), TYPE_BOOL, 0, 0,
	p_default, FALSE, PB_END,

	PARAM_DENOISER_TYPE, _T("denoiserType"), TYPE_INT, 0, 0,
	p_default, DenoiserMl, PB_END,

	PARAM_DENOISER_BILATERAL_RADIUS, _T("bilateralRadius"), TYPE_INT, 0, 0, p_default,
	bilateralRadiusDefault, p_range, bilateralRadiusMin, bilateralRadiusMax, PB_END,

	PARAM_DENOISER_LWR_SAMPLES, _T("lwrSamples"), TYPE_INT, 0, 0, p_default,
	lwrSamplesDefault, p_range, lwrSamplesMin, lwrSamplesMax, PB_END,

	PARAM_DENOISER_LWR_RADIUS, _T("lwrRadius"), TYPE_INT, 0, 0, p_default,
	lwrRadiusDefault, p_range, lwrRadiusMin, lwrRadiusMax, PB_END,

	PARAM_DENOISER_LWR_BANDWIDTH, _T("lwrBandwidth"), TYPE_FLOAT, 0, 0, p_default,
	lwrBandwidthDefault, p_range, lwrBandwidthMin, lwrBandwidthMax, PB_END,

	PARAM_DENOISER_EAW_COLOR, _T("eawColor"), TYPE_FLOAT, 0, 0, p_default,
	eawColorDefault, p_range, eawColorMin, eawColorMax, PB_END,

	PARAM_DENOISER_EAW_NORMAL, _T("eawNormal"), TYPE_FLOAT, 0, 0, p_default,
	eawNormalDefault, p_range, eawNormalMin, eawNormalMax, PB_END,

	PARAM_DENOISER_EAW_DEPTH, _T("eawDepth"), TYPE_FLOAT, 0, 0, p_default,
	eawDepthDefault, p_range, eawDepthMin, eawDepthMax, PB_END,

	PARAM_DENOISER_EAW_OBJECTID, _T("eawObjectId"), TYPE_FLOAT, 0, 0, p_default,
	eawObjectIdDefault, p_range, eawObjectIdMin, eawObjectIdMax, PB_END,

	PARAM_CONTEXT_ITERATIONS, _T("iterations"), TYPE_INT, 0, 0, // effectively the same parameter as aasamples
	p_default, 1, p_range, 1, INT_MAX /*RPR_MAX_AA_GRID_SIZE*/, PB_END,

PB_END);


void BgAccessor::Get(PB2Value& v, ReferenceMaker* owner, ParamID id, int tabIndex, TimeValue t, Interval &valid)
{
	ParamID bgParam = TranslateParamID(id);
	
	IParamBlock2 *pblock = owner->GetParamBlock(0);
	auto type = pblock->GetParameterType(id);
	
	switch (type)
	{
		case TYPE_FLOAT:
		{
			Control* ctrl = pblock->GetControllerByID(id);
			if (ctrl)
				ctrl->GetValue(t, &v.f, FOREVER);
			else
			{
				FASSERT(BgManagerMax::TheManager.GetProperty(bgParam, v.f));
			}
		}
		break;
		
		
		case TYPE_BOOL:
		{
			Control* ctrl = pblock->GetControllerByID(id);
			if (ctrl)
				ctrl->GetValue(t, &v.i, FOREVER);
			else
			{
				BOOL bv = FALSE;
				FASSERT(BgManagerMax::TheManager.GetProperty(bgParam, bv));
				v.i = int(bv);
			}
		}
		break;

		case TYPE_INT:
		{
			Control* ctrl = pblock->GetControllerByID(id);
			if (ctrl)
				ctrl->GetValue(t, &v.i, FOREVER);
			else
			{
				FASSERT(BgManagerMax::TheManager.GetProperty(bgParam, v.i));
			}
		}
		break;
		
		case TYPE_RGBA:
		{
			Control* ctrl = pblock->GetControllerByID(id);
			if (ctrl)
				ctrl->GetValue(t, v.p, FOREVER);
			else
			{
				Color c;
				FASSERT(BgManagerMax::TheManager.GetProperty(bgParam, c));
				v.p->x = c.r;
				v.p->y = c.g;
				v.p->z = c.b;
			}
		}
		break;

		case TYPE_TEXMAP:
		{
			Texmap *tex = 0;
			FASSERT(BgManagerMax::TheManager.GetProperty(bgParam, tex));
			v.r = tex;
		}
		break;
	}
}

void BgAccessor::Set(PB2Value& v, ReferenceMaker* owner, ParamID id, int tabIndex, TimeValue t)
{
	ParamID bgParam = TranslateParamID(id);
	ParamBlockDesc2 *desc = owner->GetParamBlock(0)->GetDesc();
	FASSERT(desc);
	ParamDef& pdef = desc->GetParamDef(id);
	switch (pdef.type)
	{
	case TYPE_FLOAT:
		FASSERT(BgManagerMax::TheManager.SetProperty(bgParam, v.f));
		break;

	case TYPE_INT:
		FASSERT(BgManagerMax::TheManager.SetProperty(bgParam, v.i));
		break;

	case TYPE_BOOL:
		FASSERT(BgManagerMax::TheManager.SetProperty(bgParam, BOOL(v.i)));
		break;

	case TYPE_RGBA:
	{
		Color c(v.p->x, v.p->y, v.p->z);
		FASSERT(BgManagerMax::TheManager.SetProperty(bgParam, c));
	}
	break;

	case TYPE_TEXMAP:
	{
		Texmap *tex = (Texmap*)v.r;
		FASSERT(BgManagerMax::TheManager.SetProperty(bgParam, tex));
	}
	break;
	}

	BgManagerMax::TheManager.NotifyDependents(FOREVER, 0, REFMSG_CHANGE);
}

BOOL BgAccessor::KeyFrameAtTime(ReferenceMaker* owner, ParamID id, int tabIndex, TimeValue t)
{
	return FALSE;
}

ParamID BgAccessor::TranslateParamID(ParamID id)
{
	// Previously, it was possible to have a direct
	// mapping between parameter IDs, but newly
	// added parameters need to be mapped explicitly.
	switch (id)
	{
		case TRPARAM_BG_SKY_FILTER_COLOR:
			return PARAM_BG_SKY_FILTER_COLOR;

		case TRPARAM_BG_SKY_SHADOW_SOFTNESS:
			return PARAM_BG_SKY_SHADOW_SOFTNESS;

		case TRPARAM_BG_SKY_SATURATION:
			return PARAM_BG_SKY_SATURATION;

		case TRPARAM_BG_SKY_GROUND_ALBEDO:
			return PARAM_BG_SKY_GROUND_ALBEDO;

		default:
			return id - (TRPARAM_BG_USE - PARAM_BG_FIRST);
	}
}


void SamplingAccessor::Get(PB2Value& v, ReferenceMaker* owner, ParamID id, int tabIndex, TimeValue t, Interval &valid)
{
	IParamBlock2 *pblock = owner->GetParamBlock(0);
	switch (id)
	{
	case PARAM_SAMPLES_MIN:
		{
			// for older scenes saved before the p_range defined minimum of 2,
			// need to enforce manually, otherwise scene may render as black
			v.i = MAX( v.i, 2 ); // enforce minimum of 2; 
		}
		break;

	case PARAM_PASS_LIMIT:
		{
			int samplesMax = pblock->GetInt(PARAM_SAMPLES_MAX);
			int contextIterations = pblock->GetInt(PARAM_CONTEXT_ITERATIONS);
			if (contextIterations <= 1) contextIterations = 1;
			// if maximum number of samples is not evenly divisble by context iterations,
			// add one extra pass to overshoot, not undershoot, the intended render quality
			int passes = samplesMax / contextIterations;
			if( (passes*contextIterations) < samplesMax ) // add one extra pass if needed
				passes++;
			v.i = passes;
		}
		break;

	case PARAM_RENDER_LIMIT:
		{
			int samplesMax = pblock->GetInt(PARAM_SAMPLES_MAX);
			int timeLimit = pblock->GetInt(PARAM_TIME_LIMIT);
			if ((samplesMax <= 0) && (timeLimit <= 0))
				v.i = TerminationCriteria::Termination_None;
			else if (samplesMax <= 0)
				v.i = TerminationCriteria::Termination_Time;
			else if (timeLimit <= 0)
				v.i = TerminationCriteria::Termination_Passes;
			else
				v.i = TerminationCriteria::Termination_PassesOrTime;
		}
		break;
	}
}

void SamplingAccessor::Set(PB2Value& v, ReferenceMaker* owner, ParamID id, int tabIndex, TimeValue t)
{
	IParamBlock2 *pblock = owner->GetParamBlock(0);
	ParamBlockDesc2 *desc = owner->GetParamBlock(0)->GetDesc();
	FASSERT(desc);
	ParamDef& pdef = desc->GetParamDef(id);
	switch (id)
	{
	case PARAM_PASS_LIMIT:
		if( v.i<=0 ) // unlimited
			pblock->SetValue(PARAM_SAMPLES_MAX, 0, 0);
		else
		{
			int contextIterations = pblock->GetInt(PARAM_CONTEXT_ITERATIONS);
			if( contextIterations <= 1 ) contextIterations = 1;
			pblock->SetValue(PARAM_SAMPLES_MAX, 0, contextIterations * v.i);
		}
		break;

	case PARAM_RENDER_LIMIT:
		if( v.i == TerminationCriteria::Termination_None )
		{
			pblock->SetValue(PARAM_SAMPLES_MAX,0,0);
			pblock->SetValue(PARAM_TIME_LIMIT, 0, 0);
		}
		else if( v.i == TerminationCriteria::Termination_Time )
		{
			pblock->SetValue(PARAM_SAMPLES_MAX, 0, 0);
		}
		else if( v.i == TerminationCriteria::Termination_Passes )
		{
			pblock->SetValue(PARAM_TIME_LIMIT, 0, 0);
		}

		if( (v.i == TerminationCriteria::Termination_Time) || (v.i == TerminationCriteria::Termination_PassesOrTime) )
		{
			if (pblock->GetInt(PARAM_TIME_LIMIT) == 0) // Set time limit to default nonzero if necessary
				pblock->SetValue(PARAM_TIME_LIMIT, 0, desc->GetParamDef(PARAM_TIME_LIMIT).def.i);
		}

		if( (v.i == TerminationCriteria::Termination_Passes) || (v.i == TerminationCriteria::Termination_PassesOrTime) )
		{
			if (pblock->GetInt(PARAM_SAMPLES_MAX) == 0) // Set max samples to default nonzero if necessary
				pblock->SetValue(PARAM_SAMPLES_MAX, 0, desc->GetParamDef(PARAM_SAMPLES_MAX).def.i);
		}
		break;
	}
}

BOOL SamplingAccessor::KeyFrameAtTime(ReferenceMaker* owner, ParamID id, int tabIndex, TimeValue t)
{
	return FALSE;
}


FIRERENDER_NAMESPACE_END
