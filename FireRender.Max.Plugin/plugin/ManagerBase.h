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
#include "Common.h"
#include <set>

FIRERENDER_NAMESPACE_BEGIN;

class ManagerMaxBase;

class ManagerPropertyChangeCallback
{
friend class ManagerMaxBase;
private:
	class ManagerMaxBase *mManager = 0;
public:
	virtual void propChangedCallback(class ManagerMaxBase *owner, int paramId, int sender) = 0;
	
	virtual ~ManagerPropertyChangeCallback();
};

class ManagerMaxBase
{
private:
	std::set<ManagerPropertyChangeCallback *> mPropertyChangeCallbacks;
	int mSender = -1;

public:
	virtual ~ManagerMaxBase() {}
	virtual BOOL GetProperty(int param, Texmap*& value) { return FALSE; }
	virtual BOOL GetProperty(int param, float& value) { return FALSE; }
	virtual BOOL GetProperty(int param, int& value) { return FALSE; }
	virtual BOOL GetProperty(int param, Color& value) { return FALSE; }

	virtual BOOL SetProperty(int param, Texmap* value, int sender = -1) { return FALSE; }
	virtual BOOL SetProperty(int param, const float &value, int sender = -1) { return FALSE; }
	virtual BOOL SetProperty(int param, int value, int sender = -1) { return FALSE; }
	virtual BOOL SetProperty(int param, const Color &value, int sender = -1) { return FALSE; }

	void RegisterPropertyChangeCallback(ManagerPropertyChangeCallback *callback)
	{
		auto ii = mPropertyChangeCallbacks.find(callback);
		if (ii == mPropertyChangeCallbacks.end())
		{
			callback->mManager = this;
			mPropertyChangeCallbacks.insert(callback);
		}
	}

	void UnregisterPropertyChangeCallback(ManagerPropertyChangeCallback *callback)
	{
		auto ii = mPropertyChangeCallbacks.find(callback);
		if (ii != mPropertyChangeCallbacks.end())
		{
			(*ii)->mManager = 0;
			mPropertyChangeCallbacks.erase(ii);
		}
	}

	void SendPropChangeNotifications(int param, int sender = -1)
	{
		for (auto ii = mPropertyChangeCallbacks.begin(); ii != mPropertyChangeCallbacks.end(); ii++)
			(*ii)->propChangedCallback(this, param, (sender != -1) ? sender : mSender);
	}

	virtual void ResetToDefaults()
	{
	}

	inline virtual void SetPropChangeSender(int sender)
	{
		mSender = sender;
	}
};

FIRERENDER_NAMESPACE_END;
