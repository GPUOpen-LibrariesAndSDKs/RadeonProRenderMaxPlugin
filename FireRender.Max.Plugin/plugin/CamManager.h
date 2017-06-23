/*********************************************************************************************************************************
* Radeon ProRender for 3ds Max plugin
* Copyright (c) 2017 AMD
* All Rights Reserved
*
* Property Manager specialized in handling Camera related properties (IBL, Sun & Sky etc..)
*********************************************************************************************************************************/

#pragma once

#include <max.h>
#include <guplib.h>
#include <iparamb2.h>
#include "Common.h"
#include "plugin/ManagerBase.h"

FIRERENDER_NAMESPACE_BEGIN;

#define CAMMANAGER_VERSION 1

enum FRCameraType
{
	FRCameraType_Default,
	FRCameraType_LatitudeLongitude_360,
	FRCameraType_LatitudeLongitude_Stereo,
	FRCameraType_Cubemap,
	FRCameraType_Cubemap_Stereo,
	FRCameraType_Count
};


enum CamParameter : ParamID
{
	/// BOOL: Whether to use the depth of field effect
	PARAM_CAM_USE_DOF = 300,
	/// INT: How many camera aperture blades are simulated when doing DOF
	PARAM_CAM_BLADES = 301,
	/// WORLD: Focal distance for DOF used when rendering from a free view (i.e. not from camera view, because cameras have their 
	/// own focal distance)
	PARAM_CAM_FOCUS_DIST = 302,
	/// FLOAT: Camera aperture F-Stop number for DOF calculation
	PARAM_CAM_F_STOP = 303,
	/// WORLD: Camera sensor size for DOF calculation. It is in world units, so the number can be vastly different from the 
	/// "usual" value of 36
	PARAM_CAM_SENSOR_WIDTH = 304,
	/// FLOAT: Simple exposure - number that multiplies the final result
	PARAM_CAM_EXPOSURE = 305,
	/// FLOAT: Focal Length
	PARAM_CAM_FOCAL_LENGTH = 306,
	/// FLOAT: Camera FOV
	PARAM_CAM_FOV = 307,
	/// BOOL: overwrite DOF values
	PARAM_CAM_OVERWRITE_DOF_SETTINGS = 308,
	/// BOOL: for general settings
	PARAM_CAM_USE_FOV = 309,
	/// camera type
	PARAM_CAM_TYPE = 310,
	PARAM_CAM_USE_MOTION_BLUR = 435,
	PARAM_CAM_MOTION_BLUR_SCALE = 436,
};



class CamManagerMax : public GUP, public ManagerMaxBase
{
public:
	static CamManagerMax TheManager;

public:
	CamManagerMax();
	~CamManagerMax() {}

	// GUP Methods
	DWORD Start() override;
	void Stop() override;

	void DeleteThis() override;

	static ClassDesc* GetClassDesc();

	IOResult Save(ISave *isave) override;
	IOResult Load(ILoad *iload) override;

public:
	BOOL GetProperty(int param, Texmap*& value) override;
	BOOL GetProperty(int param, float& value) override;
	BOOL GetProperty(int param, int& value) override;
	BOOL GetProperty(int param, Color& value) override;

	BOOL SetProperty(int param, Texmap* value, int sender = -1) override;
	BOOL SetProperty(int param, const float &value, int sender = -1) override;
	BOOL SetProperty(int param, int value, int sender = -1) override;
	BOOL SetProperty(int param, const Color &value, int sender = -1) override;

	void ResetToDefaults() override
	{
		useFov = TRUE;
		useDof = FALSE;
		overWriteDof = FALSE;
		cameraBlades = 6;
		perspectiveFocalDist = 1.f;
		fStop = 5.6f;
		cameraSensorSize = 36.f;
		exposure = 1.f;
		cameraFocalLength = 40.f;
		cameraFOV = 0.78539816339f;
		cameraType = FRCameraType_Default;
		enableMotionblur = FALSE;
		motionblurScale = 100.f;
	}

private:
	BOOL useFov = TRUE;
	BOOL useDof = FALSE;
	BOOL overWriteDof = FALSE;
	int cameraBlades = 6;
	float perspectiveFocalDist = 1.f;
	float fStop = 5.6f;
	float cameraSensorSize = 36.f;
	float exposure = 1.f;
	float cameraFocalLength = 40.f;
	float cameraFOV = 0.78539816339f;
	int cameraType = FRCameraType_Default;
	BOOL enableMotionblur = FALSE;
	float motionblurScale = 100.f;
};

FIRERENDER_NAMESPACE_END;
