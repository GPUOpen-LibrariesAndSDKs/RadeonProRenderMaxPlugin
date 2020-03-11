/**********************************************************************
Copyright 2020 Advanced Micro Devices, Inc
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at
    http://www.apache.org/licenses/LICENSE-2.0
Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
********************************************************************/

#include "CamManager.h"
#include <notify.h>
#include "plugin/ParamBlock.h"

FIRERENDER_NAMESPACE_BEGIN;

CamManagerMax CamManagerMax::TheManager;

class CamManagerMaxClassDesc : public ClassDesc
{
public:
	int             IsPublic() { return FALSE; }
	void*           Create(BOOL loading) { return &CamManagerMax::TheManager; }
	const TCHAR*    ClassName() { return _T("RPRCamManager"); }
	SClass_ID       SuperClassID() { return GUP_CLASS_ID; }
	Class_ID        ClassID() { return CAMMANAGER_MAX_CLASSID; }
	const TCHAR*    Category() { return _T(""); }
};

static CamManagerMaxClassDesc CamManagerMaxCD;

ClassDesc* CamManagerMax::GetClassDesc()
{
	return &CamManagerMaxCD;
}


CamManagerMax::CamManagerMax()
{
	ResetToDefaults();
}

static void NotifyProc(void *param, NotifyInfo *info)
{
	CamManagerMax *cam = reinterpret_cast<CamManagerMax*>(param);
	switch (info->intcode)
	{
	case NOTIFY_FILE_PRE_OPEN:
	case NOTIFY_SYSTEM_STARTUP:
	case NOTIFY_SYSTEM_PRE_RESET:
	case NOTIFY_SYSTEM_PRE_NEW:
		cam->ResetToDefaults();
		break;
	}
}

// Activate and Stay Resident
//

DWORD CamManagerMax::Start()
{
	int res;
	res = RegisterNotification(NotifyProc, this, NOTIFY_FILE_PRE_OPEN);
	res = RegisterNotification(NotifyProc, this, NOTIFY_SYSTEM_STARTUP);
	res = RegisterNotification(NotifyProc, this, NOTIFY_SYSTEM_PRE_RESET);
	res = RegisterNotification(NotifyProc, this, NOTIFY_SYSTEM_PRE_NEW);
	return GUPRESULT_KEEP;
}

void CamManagerMax::Stop()
{
	UnRegisterNotification(NotifyProc, this, NOTIFY_FILE_PRE_OPEN);
	UnRegisterNotification(NotifyProc, this, NOTIFY_SYSTEM_STARTUP);
	UnRegisterNotification(NotifyProc, this, NOTIFY_SYSTEM_PRE_RESET);
	UnRegisterNotification(NotifyProc, this, NOTIFY_SYSTEM_PRE_NEW);
}

void CamManagerMax::DeleteThis()
{
}


//////////////////////////////////////////////////////////////////////////////
// MANAGER PARAMETERS
//


BOOL CamManagerMax::SetProperty(int param, Texmap* value, int sender)
{
	return FALSE;
}

BOOL CamManagerMax::GetProperty(int param, Texmap*& value)
{
	return FALSE;
}


BOOL CamManagerMax::SetProperty(int param, const float &value, int sender)
{
	switch (param)
	{
		case PARAM_CAM_FOCUS_DIST: perspectiveFocalDist = value; break;
		case PARAM_CAM_F_STOP: fStop = value; break;
		case PARAM_CAM_SENSOR_WIDTH: cameraSensorSize = value; break;
		case PARAM_CAM_EXPOSURE: exposure = value; break;
		case PARAM_CAM_FOCAL_LENGTH: cameraFocalLength = value; break;
		case PARAM_CAM_FOV: cameraFOV = value; break;
		case PARAM_CAM_MOTION_BLUR_SCALE: motionblurScale = value; break;
		default: return FALSE;
	}
	SendPropChangeNotifications(param);
	return TRUE;
}

BOOL CamManagerMax::GetProperty(int param, float &value)
{
	switch (param)
	{
		case PARAM_CAM_FOCUS_DIST: value = perspectiveFocalDist; break;
		case PARAM_CAM_F_STOP: value = fStop; break;
		case PARAM_CAM_SENSOR_WIDTH: value = cameraSensorSize; break;
		case PARAM_CAM_EXPOSURE: value = exposure; break;
		case PARAM_CAM_FOCAL_LENGTH: value = cameraFocalLength; break;
		case PARAM_CAM_FOV: value = cameraFOV; break;
		case PARAM_CAM_MOTION_BLUR_SCALE: value = motionblurScale; break;
		default: return FALSE;
	}
	return TRUE;
}

BOOL CamManagerMax::SetProperty(int param, int value, int sender)
{
	switch (param)
	{
		case PARAM_CAM_USE_DOF: useDof = value; break;
		case PARAM_CAM_BLADES: cameraBlades = value; break;
		case PARAM_CAM_OVERWRITE_DOF_SETTINGS: overWriteDof = value; break;
		case PARAM_CAM_USE_FOV: useFov = value; break;
		case PARAM_CAM_TYPE: cameraType = value; break;
		case PARAM_CAM_USE_MOTION_BLUR: enableMotionblur = value; break;
		default: return FALSE;
	}
	SendPropChangeNotifications(param);
	return TRUE;
}

BOOL CamManagerMax::GetProperty(int param, int &value)
{
	switch (param)
	{
		case PARAM_CAM_USE_DOF: value = useDof; break;
		case PARAM_CAM_BLADES: value = cameraBlades; break;
		case PARAM_CAM_OVERWRITE_DOF_SETTINGS: value = overWriteDof; break;
		case PARAM_CAM_USE_FOV: value = useFov; break;
		case PARAM_CAM_TYPE: value = cameraType; break;
		case PARAM_CAM_USE_MOTION_BLUR: value = enableMotionblur; break;
		default: return FALSE;
	}
	return TRUE;
}

BOOL CamManagerMax::SetProperty(int param, const Color &value, int sender)
{
	return FALSE;
}

BOOL CamManagerMax::GetProperty(int param, Color &value)
{
	return FALSE;
}

#define CAMMANAGER_VERSION_CHUNK (PARAM_CAM_USE_DOF - 1)

#define CAM_SAVE_CHUNK(id, obj)\
isave->BeginChunk(id);\
res = isave->Write(&obj, sizeof(obj), &nb);\
FASSERT(res == IO_OK);\
isave->EndChunk();

#define CAM_LOAD_CHUNK(id, obj)\
else if (ID == id)\
{\
	res = iload->Read(&obj, sizeof(obj), &nb);\
}

IOResult CamManagerMax::Save(ISave *isave)
{
	ULONG nb;
	isave->BeginChunk(CAMMANAGER_VERSION_CHUNK);
	DWORD version = CAMMANAGER_VERSION;
	IOResult res = isave->Write(&version, sizeof(version), &nb);
	isave->EndChunk();
	FASSERT(res == IO_OK);
	CAM_SAVE_CHUNK(PARAM_CAM_USE_DOF, useDof);
	CAM_SAVE_CHUNK(PARAM_CAM_BLADES, cameraBlades);
	CAM_SAVE_CHUNK(PARAM_CAM_FOCUS_DIST, perspectiveFocalDist);
	CAM_SAVE_CHUNK(PARAM_CAM_F_STOP, fStop);
	CAM_SAVE_CHUNK(PARAM_CAM_SENSOR_WIDTH, cameraSensorSize);
	CAM_SAVE_CHUNK(PARAM_CAM_EXPOSURE, exposure);
	CAM_SAVE_CHUNK(PARAM_CAM_FOCAL_LENGTH, cameraFocalLength);
	CAM_SAVE_CHUNK(PARAM_CAM_FOV, cameraFOV);
	CAM_SAVE_CHUNK(PARAM_CAM_OVERWRITE_DOF_SETTINGS, overWriteDof);
	CAM_SAVE_CHUNK(PARAM_CAM_USE_FOV, useFov);
	CAM_SAVE_CHUNK(PARAM_CAM_TYPE, cameraType);
	CAM_SAVE_CHUNK(PARAM_CAM_USE_MOTION_BLUR, enableMotionblur);
	CAM_SAVE_CHUNK(PARAM_CAM_MOTION_BLUR_SCALE, motionblurScale);
	return IO_OK;
}

IOResult CamManagerMax::Load(ILoad *iload)
{
	int ID = 0;
	ULONG nb;
	IOResult res = IO_OK;
	DWORD version = 1;

	while (IO_OK == (res = iload->OpenChunk()))
	{
		ID = iload->CurChunkID();
		if (ID == CAMMANAGER_VERSION_CHUNK)
		{
			res = iload->Read(&version, sizeof(version), &nb);
		}
		CAM_LOAD_CHUNK(PARAM_CAM_USE_DOF, useDof)
		CAM_LOAD_CHUNK(PARAM_CAM_BLADES, cameraBlades)
		CAM_LOAD_CHUNK(PARAM_CAM_FOCUS_DIST, perspectiveFocalDist)
		CAM_LOAD_CHUNK(PARAM_CAM_F_STOP, fStop)
		CAM_LOAD_CHUNK(PARAM_CAM_SENSOR_WIDTH, cameraSensorSize)
		CAM_LOAD_CHUNK(PARAM_CAM_EXPOSURE, exposure)
		CAM_LOAD_CHUNK(PARAM_CAM_FOCAL_LENGTH, cameraFocalLength)
		CAM_LOAD_CHUNK(PARAM_CAM_FOV, cameraFOV)
		CAM_LOAD_CHUNK(PARAM_CAM_OVERWRITE_DOF_SETTINGS, overWriteDof)
		CAM_LOAD_CHUNK(PARAM_CAM_USE_FOV, useFov)
		CAM_LOAD_CHUNK(PARAM_CAM_TYPE, cameraType)
		CAM_LOAD_CHUNK(PARAM_CAM_USE_MOTION_BLUR, enableMotionblur)
		CAM_LOAD_CHUNK(PARAM_CAM_MOTION_BLUR_SCALE, motionblurScale)
		iload->CloseChunk();
		if (res != IO_OK)
			return res;
	}

	if (res == IO_END)
		res = IO_OK;		// don't interrupt GUP data loading

	return res;
}

FIRERENDER_NAMESPACE_END;
