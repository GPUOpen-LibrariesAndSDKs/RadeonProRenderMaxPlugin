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

#pragma once

#include <max.h>
#include <guplib.h>
#include <notify.h>
#include <iparamb2.h>
#include "Common.h"
#include <string>
#include <map>
#include <set>
#include <algorithm>
#include <string> 
#include "plugin/ManagerBase.h"

FIRERENDER_NAMESPACE_BEGIN;

#define TMMANAGER_VERSION 1


enum TmParameter : ParamID
{
	PARAM_TM_OPERATOR = 411,
	PARAM_TM_REINHARD_PRESCALE = 416,
	PARAM_TM_REINHARD_POSTSCALE = 417,
	PARAM_TM_REINHARD_BURN = 418,
	PARAM_TM_PHOTOLINEAR_ISO = 419,
	PARAM_TM_PHOTOLINEAR_FSTOP = 420,
	PARAM_TM_PHOTOLINEAR_SHUTTERSPEED = 421,
	PARAM_TM_SIMPLIFIED_EXPOSURE = 422,
	PARAM_TM_SIMPLIFIED_CONTRAST = 423,
	PARAM_TM_SIMPLIFIED_WHITEBALANCE = 424,
	PARAM_TM_SIMPLIFIED_TINTCOLOR = 425,
	PARAM_TM_OVERRIDE_MAX_TONEMAPPERS = 504,
};



class TmManagerMax : public GUP, public ManagerMaxBase, public ReferenceMaker
{
public:
	static TmManagerMax TheManager;

public:
	TmManagerMax();
	~TmManagerMax() {}

	// GUP Methods
	DWORD Start() override;
	void Stop() override;

	void DeleteThis() override;

	static ClassDesc* GetClassDesc();

	IOResult Save(ISave *isave) override;
	IOResult Load(ILoad *iload) override;

	void ResetToDefaults() override
	{
		if (pblock2)
			pblock2->ResetAll();
	}

public:
	BOOL GetProperty(int param, Texmap*& value) override;
	BOOL GetProperty(int param, float& value) override;
	BOOL GetProperty(int param, int& value) override;
	BOOL GetProperty(int param, Color& value) override;

	BOOL SetProperty(int param, Texmap* value, int sender = -1) override;
	BOOL SetProperty(int param, const float &value, int sender = -1) override;
	BOOL SetProperty(int param, int value, int sender = -1) override;
	BOOL SetProperty(int param, const Color &value, int sender = -1) override;

private:

//////////////////////////////////////////////////////////////////////////////
// Reference maker
	IParamBlock2 *pblock2 = 0;

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
};

FIRERENDER_NAMESPACE_END;
