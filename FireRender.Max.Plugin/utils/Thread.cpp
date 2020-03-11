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


/*********************************************************************************************************************************
* Radeon ProRender for 3ds Max plugin
* Copyright (c) 2017 AMD
* All Rights Reserved
*
* Threading utilities
*********************************************************************************************************************************/

#include <process.h>
#include "thread.h"

FIRERENDER_NAMESPACE_BEGIN;

BaseThread::BaseThread(const char* name, int priority) :
	mThread(0),
	mPriority(priority),
	mRunning(false)
{
	if (name)
		mName = name;
}
	
BaseThread::~BaseThread()
{
	FASSERT(!mRunning.Wait(0));
}

void BaseThread::Abort()
{
	if (mThread)
	{
		mStop.Fire();

		if (GetCurrentThreadId() != mThreadId) // don't wait when terminating self
			WaitForSingleObject(mThread, INFINITE);

		mThread = 0;
	}
}

void BaseThread::AbortImmediate()
{
	if (mThread) 
	{
		mRunning.Reset();
		mStop.Fire();
		mThread = 0;
	}
}

bool BaseThread::Wait(DWORD timeout)
{
	bool ret = false;

	if (WaitForSingleObject(mThread, timeout) == WAIT_OBJECT_0)
		ret = true;

	return ret;
}

void BaseThread::Start()
{
	if (!mThread)
	{
		mThread = (HANDLE)_beginthreadex(NULL, 0, BaseThread::ThreadHandlerProc, this, 0, (unsigned*) &mThreadId);
		
		if (mThread)
		{
			mRunning.Fire();
			SetThreadPriority(mThread, mPriority);

			if (!mName.empty())
				SetThreadName();
		}
	}
}

unsigned BaseThread::ThreadHandlerProc(void* lParam)
{
	BaseThread* bt = reinterpret_cast<BaseThread*>(lParam);

	bt->Worker();
	bt->mRunning.Reset();

	return 0;
}

bool BaseThread::IsRunning()
{
	return mRunning.Wait(0);
}

void BaseThread::SetThreadName()
{
	// Code setting the thread name for use in the debugger.
	// http://msdn.microsoft.com/en-us/library/xcb2z8hs.aspx

	const DWORD MS_VC_EXCEPTION = 0x406D1388;

	struct THREADNAME_INFO
	{
		DWORD  dwType;		// Must be 0x1000.
		LPCSTR szName;		// Pointer to name (in user addr space).
		DWORD  dwThreadID;	// Thread ID (-1=caller thread).
		DWORD  dwFlags;		// Reserved for future use, must be zero.
	};

	THREADNAME_INFO ThreadNameInfo;
	ThreadNameInfo.dwType = 0x1000;
	ThreadNameInfo.szName = mName.c_str();
	ThreadNameInfo.dwThreadID = mThreadId;
	ThreadNameInfo.dwFlags = 0;

	__try
	{
		RaiseException( MS_VC_EXCEPTION, 0, sizeof(ThreadNameInfo)/sizeof(ULONG_PTR), (ULONG_PTR*)&ThreadNameInfo );
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{	
	}
}

FIRERENDER_NAMESPACE_END;
