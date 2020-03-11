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

#include "TmManager.h"
#include "plugin/ParamBlock.h"
#include "Resource.h"
#include "frWrap.h"

extern HINSTANCE hInstance;

FIRERENDER_NAMESPACE_BEGIN;

TmManagerMax TmManagerMax::TheManager;

class TmManagerMaxClassDesc : public ClassDesc2
{
public:
	int             IsPublic() { return FALSE; }
	void*           Create(BOOL loading) { return &TmManagerMax::TheManager; }
	const TCHAR*    ClassName() { return _T("RPRTmManager"); }
	SClass_ID       SuperClassID() { return GUP_CLASS_ID; }
	Class_ID        ClassID() { return TMMANAGER_MAX_CLASSID; }
	const TCHAR*    Category() { return _T(""); }
};

static TmManagerMaxClassDesc TmManagerMaxCD;

ClassDesc* TmManagerMax::GetClassDesc()
{
	return &TmManagerMaxCD;
}

#if 1
namespace
{
	static ParamBlockDesc2 paramBlock(0, _T("TmManagerPbDesc"), 0, &TmManagerMaxCD, P_AUTO_CONSTRUCT,
		0, //for P_AUTO_CONSTRUCT
		
		PARAM_TM_OVERRIDE_MAX_TONEMAPPERS, _T("tmOverride"), TYPE_BOOL, P_RESET_DEFAULT, 0,
		p_default, FALSE, PB_END,

		PARAM_TM_OPERATOR, _T("tmOperator"), TYPE_INT, P_RESET_DEFAULT, 0,
		p_range, frw::ToneMappingTypeNone, frw::ToneMappingTypeNonLinear, p_default, frw::ToneMappingTypeNone, PB_END,
		
		PARAM_TM_REINHARD_PRESCALE, _T("tmReinhardPrescale"), TYPE_FLOAT, P_RESET_DEFAULT, 0,
		p_default, 0.1, p_range, 0.0, FLT_MAX, PB_END,

		PARAM_TM_REINHARD_POSTSCALE, _T("tmReinhardPostscale"), TYPE_FLOAT, P_RESET_DEFAULT, 0,
		p_default, 1.0, p_range, 0.0, FLT_MAX, PB_END,

		PARAM_TM_REINHARD_BURN, _T("tmReinhardBurn"), TYPE_FLOAT, P_RESET_DEFAULT, 0,
		p_default, 10.0, p_range, 0.0, FLT_MAX, PB_END,
		
		PARAM_TM_PHOTOLINEAR_ISO, _T("tmPhotolinearIso"), TYPE_FLOAT, P_RESET_DEFAULT, 0,
		p_default, 100.0, p_range, 0.0, FLT_MAX, PB_END,

		PARAM_TM_PHOTOLINEAR_FSTOP, _T("tmPhotolinearFStop"), TYPE_FLOAT, P_RESET_DEFAULT, 0,
		p_default, 4.0, p_range, 0.0, FLT_MAX, PB_END,

		PARAM_TM_PHOTOLINEAR_SHUTTERSPEED, _T("tmPhotolinearShutterSpeed"), TYPE_FLOAT, P_RESET_DEFAULT, 0,
		p_default, 1.0, p_range, 0.0, FLT_MAX, PB_END,
		
		PARAM_TM_SIMPLIFIED_EXPOSURE, _T("tmSimplifiedExposure"), TYPE_FLOAT, P_RESET_DEFAULT, 0,
		p_default, 1.0, p_range, 0.0, FLT_MAX, PB_END,
		
		PARAM_TM_SIMPLIFIED_CONTRAST, _T("tmSimplifiedContrast"), TYPE_FLOAT, P_RESET_DEFAULT, 0,
		p_default, 1.0, p_range, 0.0, FLT_MAX, PB_END,
		
		PARAM_TM_SIMPLIFIED_WHITEBALANCE, _T("tmSimplifiedWhitebalance"), TYPE_INT, P_RESET_DEFAULT, 0,
		p_default, 6500, p_range, 1000, 12000, PB_END,
		
		PARAM_TM_SIMPLIFIED_TINTCOLOR, _T("tmSimplifiedTintColor"), TYPE_RGBA, P_RESET_DEFAULT, 0,
		p_default, Color(1.0f, 1.0f, 1.0f), p_ui, TYPE_COLORSWATCH, IDC_TINT, PB_END,
		
		p_end
	);
};
#endif

TmManagerMax::TmManagerMax()
{
	ResetToDefaults();
}

static void NotifyProc(void *param, NotifyInfo *info)
{
	TmManagerMax *tm = reinterpret_cast<TmManagerMax*>(param);

	switch (info->intcode)
	{
		case NOTIFY_FILE_PRE_OPEN:
		case NOTIFY_SYSTEM_STARTUP:
		case NOTIFY_SYSTEM_PRE_RESET:
		case NOTIFY_SYSTEM_PRE_NEW:
			tm->ResetToDefaults();
		break;
	}
}

// Activate and Stay Resident
//

DWORD TmManagerMax::Start()
{
	int res;
	GetClassDesc()->MakeAutoParamBlocks(this);
	res = RegisterNotification(NotifyProc, this, NOTIFY_FILE_PRE_OPEN);
	res = RegisterNotification(NotifyProc, this, NOTIFY_SYSTEM_STARTUP);
	res = RegisterNotification(NotifyProc, this, NOTIFY_SYSTEM_PRE_RESET);
	res = RegisterNotification(NotifyProc, this, NOTIFY_SYSTEM_PRE_NEW);
	return GUPRESULT_KEEP;
}

void TmManagerMax::Stop()
{
	UnRegisterNotification(NotifyProc, this, NOTIFY_FILE_PRE_OPEN);
	UnRegisterNotification(NotifyProc, this, NOTIFY_SYSTEM_STARTUP);
	UnRegisterNotification(NotifyProc, this, NOTIFY_SYSTEM_PRE_RESET);
	UnRegisterNotification(NotifyProc, this, NOTIFY_SYSTEM_PRE_NEW);
}

void TmManagerMax::DeleteThis()
{
}


//////////////////////////////////////////////////////////////////////////////
// MANAGER PARAMETERS
//


BOOL TmManagerMax::SetProperty(int param, Texmap* value, int sender)
{
	return FALSE;
}

BOOL TmManagerMax::GetProperty(int param, Texmap*& value)
{
	return FALSE;
}


BOOL TmManagerMax::SetProperty(int param, const float &value, int sender)
{
	switch (param)
	{
		case PARAM_TM_REINHARD_PRESCALE:
		case PARAM_TM_REINHARD_POSTSCALE:
		case PARAM_TM_REINHARD_BURN:
		case PARAM_TM_PHOTOLINEAR_ISO:
		case PARAM_TM_PHOTOLINEAR_FSTOP:
		case PARAM_TM_PHOTOLINEAR_SHUTTERSPEED:
		case PARAM_TM_SIMPLIFIED_EXPOSURE:
		case PARAM_TM_SIMPLIFIED_CONTRAST:
			pblock2->SetValue(param, GetCOREInterface()->GetTime(), value);
			break;
		default: return FALSE;
	}
	SendPropChangeNotifications(param);
	return TRUE;
}

BOOL TmManagerMax::GetProperty(int param, float &value)
{
	switch (param)
	{
		case PARAM_TM_REINHARD_PRESCALE:
		case PARAM_TM_REINHARD_POSTSCALE:
		case PARAM_TM_REINHARD_BURN:
		case PARAM_TM_PHOTOLINEAR_ISO:
		case PARAM_TM_PHOTOLINEAR_FSTOP:
		case PARAM_TM_PHOTOLINEAR_SHUTTERSPEED:
		case PARAM_TM_SIMPLIFIED_EXPOSURE:
		case PARAM_TM_SIMPLIFIED_CONTRAST:
			pblock2->GetValue(param, GetCOREInterface()->GetTime(), value, FOREVER);
			break;
		default: return FALSE;
	}
	return TRUE;
}

BOOL TmManagerMax::SetProperty(int param, int value, int sender)
{
	switch (param)
	{
		case PARAM_TM_OVERRIDE_MAX_TONEMAPPERS:
			pblock2->SetValue(param, GetCOREInterface()->GetTime(), (BOOL)value);
			break;
		case PARAM_TM_OPERATOR:
		case PARAM_TM_SIMPLIFIED_WHITEBALANCE:
			pblock2->SetValue(param, GetCOREInterface()->GetTime(), value);
			break;
		default: return FALSE;
	}
	SendPropChangeNotifications(param);
	return TRUE;
}

BOOL TmManagerMax::GetProperty(int param, int &value)
{
	switch (param)
	{
		case PARAM_TM_OVERRIDE_MAX_TONEMAPPERS:
		{
			BOOL v;
			pblock2->GetValue(param, GetCOREInterface()->GetTime(), v, FOREVER);
			value = int(v);
		}
		break;
		case PARAM_TM_OPERATOR:
		case PARAM_TM_SIMPLIFIED_WHITEBALANCE:
			pblock2->GetValue(param, GetCOREInterface()->GetTime(), value, FOREVER);
			break;
		default: return FALSE;
	}

	return TRUE;
}

BOOL TmManagerMax::SetProperty(int param, const Color &value, int sender)
{
	switch (param)
	{
		case PARAM_TM_SIMPLIFIED_TINTCOLOR:
			pblock2->SetValue(param, GetCOREInterface()->GetTime(), value);
			break;
		default: return FALSE;
	}
	SendPropChangeNotifications(param);
	return TRUE;
}

BOOL TmManagerMax::GetProperty(int param, Color &value)
{
	switch (param)
	{
		case PARAM_TM_SIMPLIFIED_TINTCOLOR:
			pblock2->GetValue(param, GetCOREInterface()->GetTime(), value, FOREVER);
			break;
		default: return FALSE;
	}
	return TRUE;
}

#define TMMANAGER_BASE_CHUNK (PARAM_TM_OPERATOR - 1)

IOResult TmManagerMax::Save(ISave *isave)
{
	isave->BeginChunk(TMMANAGER_BASE_CHUNK);
	IOResult res = pblock2->Save(isave);
	isave->EndChunk();
	return res;
}

namespace
{
	class PLCB : public PostLoadCallback
	{
	public:
		TmManagerMax*	p;
		PLCB(TmManagerMax* pp) { p = pp; }
		bool resetTm = false;

		void proc(ILoad *iload)
		{
			if (resetTm)
				p->GetClassDesc()->MakeAutoParamBlocks(p);
			delete this;
		}
	};
}

IOResult TmManagerMax::Load(ILoad *iload)
{
	IOResult res = IO_OK;
	bool resetTm = false;

	PLCB* plcb = new PLCB(this);
	iload->RegisterPostLoadCallback(plcb);

	while (IO_OK == (res = iload->OpenChunk()))
	{
		int ID = iload->CurChunkID();
		if (ID == TMMANAGER_BASE_CHUNK)
		{
			res = pblock2->Load(iload);
		}
		else
			plcb->resetTm = true; // loading old obsolete stuff
		iload->CloseChunk();
		if (res != IO_OK)
			return res;
	}

	if (res == IO_END)
		res = IO_OK;		// don't interrupt GUP data loading

	return res;
}

//////////////////////////////////////////////////////////////////////////////
// Reference maker
//
#if MAX_RELEASE > MAX_RELEASE_R16
RefResult TmManagerMax::NotifyRefChanged(const Interval& changeInt, RefTargetHandle hTarget,
	PartID& partID, RefMessage message, BOOL /*propagate*/)
#else
RefResult TmManagerMax::NotifyRefChanged(Interval changeInt, RefTargetHandle hTarget,
	PartID& partID, RefMessage message)
#endif
{
	return REF_DONTCARE;
}

int TmManagerMax::NumRefs()
{
	return 1;
}

RefTargetHandle TmManagerMax::GetReference(int i)
{
	switch (i)
	{
		case 0: return pblock2;
	}
	return 0;
}

void TmManagerMax::SetReference(int i, RefTargetHandle rtarg)
{
	switch (i)
	{
		case 0: pblock2 = (IParamBlock2*)rtarg; break;
	}
}
FIRERENDER_NAMESPACE_END;
