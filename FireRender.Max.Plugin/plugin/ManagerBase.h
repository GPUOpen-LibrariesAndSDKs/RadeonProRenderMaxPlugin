/*********************************************************************************************************************************
* Radeon ProRender for 3ds Max plugin
* Copyright (c) 2017 AMD
* All Rights Reserved
*
* Base class for property managers. Property managers handle property changes and propagate them to registered listeners.
*********************************************************************************************************************************/

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
