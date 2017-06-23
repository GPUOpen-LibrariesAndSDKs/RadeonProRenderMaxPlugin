/*********************************************************************************************************************************
* Radeon ProRender for 3ds Max plugin
* Copyright (c) 2017 AMD
* All Rights Reserved
*
* Threading utilities
*********************************************************************************************************************************/

#pragma once

#include "Common.h"
#include <string>
#include <vector>
#include <atomic>

FIRERENDER_NAMESPACE_BEGIN;

//#define THREAD_DEBUGGING 1

// critical section wrapper
class CriticalSection
{
private:
	CRITICAL_SECTION msec;
	DWORD mOwner;

public:
	// creates a new critical section
	inline CriticalSection()
#if THREAD_DEBUGGING
	: m_BreakOnRecurse(false)
#endif
	{
		::InitializeCriticalSection(&msec);
		mOwner = 0;
	}

	// destructor, unlocks the critical section
	// destructors are guaranteed to be called in case of exception
	inline ~CriticalSection()
	{
		::DeleteCriticalSection(&msec);
	}

	// locks the critical section
	inline void Lock()
	{
		::EnterCriticalSection(&msec);
		mOwner = GetCurrentThreadId();
#if THREAD_DEBUGGING
		if (m_BreakOnRecurse && msec.RecursionCount > 1)
			DebugBreak();
#endif
	}

	// unlocks the critical section
	inline void Unlock()
	{
		mOwner = 0;
		::LeaveCriticalSection(&msec);
	}

	inline DWORD GetOwnerThreadID()
	{
		return mOwner;
	}

	// locks the critical section and returns true
	// returns false if the critical section cannot be locked
	inline bool TryLock()
	{
		return (::TryEnterCriticalSection(&msec) == TRUE);
	}

#if THREAD_DEBUGGING
	bool m_BreakOnRecurse;

	inline int GetLockCount()
	{
		return msec.LockCount;
	}
#endif
};

class ScopeLock
{
private:
	CriticalSection &m_cs;
	ScopeLock() = delete;
	const ScopeLock &operator =(const ScopeLock &) = delete;

public:
	inline ScopeLock(CriticalSection& cs) : m_cs(cs)
	{
		m_cs.Lock();
	}

	inline ~ScopeLock()
	{
		m_cs.Unlock();
	}
	explicit operator bool() const
	{
		return true;
	}
};

class ScopeTryLock
{
private:
	CriticalSection &m_cs;
	ScopeTryLock() = delete;
	const ScopeTryLock &operator =(const ScopeTryLock &) = delete;
	bool m_locked;

public:
	inline ScopeTryLock(CriticalSection& cs) : m_cs(cs)
	{
		m_locked = m_cs.TryLock();
	}

	inline ~ScopeTryLock()
	{
		if (m_locked)
			m_cs.Unlock();
	}

	inline bool IsLocked() const
	{
		return m_locked;
	}
	explicit operator bool() const
	{
		return m_locked;
	}
};


class BaseSynchObject
{
protected:
	HANDLE handle;

public:
	inline virtual ~BaseSynchObject()
	{
		CloseHandle(handle);
	}

	inline const HANDLE &Handle() const {
		return handle;
	}
};


class Event : public BaseSynchObject
{
public:
	inline Event(bool autoreset = true) {
		handle = CreateEventA(NULL, !autoreset, FALSE, NULL);
	}
	inline bool Wait(DWORD timeout = INFINITE) {
		bool ret = false;
		if (WaitForSingleObject(handle, timeout) == WAIT_OBJECT_0)
			ret = true;
		return ret;
	}
	inline void Fire() {
		SetEvent(handle);
	}
	inline void Reset() {
		ResetEvent(handle);
	}
};

class ScopeFireEventOnExit
{
private:
	Event &theEvent;
public:
	ScopeFireEventOnExit(Event &e) : theEvent(e)
	{
	}

	~ScopeFireEventOnExit()
	{
		theEvent.Fire();
	}
};

class ScopeResetEventOnExit
{
private:
	Event &theEvent;
public:
	ScopeResetEventOnExit(Event &e) : theEvent(e)
	{
		theEvent.Fire();
	}

	~ScopeResetEventOnExit()
	{
		theEvent.Reset();
	}
};

class Semaphore : public BaseSynchObject
{
public:
	inline Semaphore(LONG initialCount = 0, LONG maxCount = LONG_MAX) {
		handle = CreateSemaphore(NULL, initialCount, maxCount, NULL);
	}
	inline bool Wait(DWORD timeout = INFINITE) {
		bool ret = false;
		if (WaitForSingleObject(handle, timeout) == WAIT_OBJECT_0)
			ret = true;
		return ret;
	}
	inline void Fire() {
		ReleaseSemaphore(handle, 1, NULL);
	}
};

class WaitableTimer : public BaseSynchObject
{
private:
	LARGE_INTEGER liDueTime;

public:
	inline WaitableTimer(const wchar_t *name = NULL, bool manualReset = false) {
		handle = CreateWaitableTimer(NULL, (BOOL)manualReset, name);
	}

	inline virtual ~WaitableTimer() {
		Cancel();
	}

	inline bool Set(const LONG& msecs) {
		if (!handle)
			return false;
		liDueTime.QuadPart = -(10000 * msecs);
		return (bool)SetWaitableTimer(handle, &liDueTime, msecs, NULL, NULL, FALSE);
	}

	inline bool Cancel() {
		if (!handle)
			return false;
		return (bool)CancelWaitableTimer(handle);
	}

	inline bool Wait(DWORD timeout = INFINITE) {
		bool ret = false;
		if (handle && WaitForSingleObject(handle, timeout) == WAIT_OBJECT_0)
			ret = true;
		return ret;
	}
};

class WaitMultiple
{
private:
	std::vector<HANDLE> handles;

public:
	inline WaitMultiple() {
	}
	inline ~WaitMultiple() {
	}
	inline void AddObject(const BaseSynchObject &o)
	{
		handles.push_back(o.Handle());
	}
	inline int Wait(const DWORD &milliseconds = INFINITE, bool waitAll = false)
	{
		DWORD res = WaitForMultipleObjects(
			handles.size(),
			&handles[0],
			waitAll,
			milliseconds);
		return res - WAIT_OBJECT_0;
	}
};

//////////////////////////////////////////////////////////////////////////////
// EventSpin is a manual event with spin count
//
// It is useful when multiple actors want to participate at raising a signal
// and keep it raised for as long as needed. Consumers of this event
// will have to wait until all actors have reset the signal.
//
// When the event is fired, the following happens:
// - when the spin count is zero, the event is fired, then spin is incremented
//   by one
// - when the spin is not zero, it is further incremented by one
// When the event is reset, the following happens:
// - is spin is zero, the event was not fired: ignore
// - spin is decremented by one
// - if spin has reached zero, the event is reset
//

class EventSpin
{
private:
	LONG mSpin = 0;
	CriticalSection mSec;
	Event mEvent;

public:
	EventSpin()
		: mEvent(false)
	{
	}

	void Fire()
	{
		mSec.Lock();
		if (mSpin == 0)
			mEvent.Fire();
		mSpin++;
		mSec.Unlock();
	}

	bool Wait(DWORD timeout)
	{
		return mEvent.Wait(timeout);
	}

	void Reset()
	{
		mSec.Lock();
		if (mSpin > 0)
		{
			mSpin--;
			if (mSpin == 0)
				mEvent.Reset();
		}
		mSec.Unlock();
	}
};

class BaseThread
{
protected:
	Event mStop;			// running yes/no
	HANDLE mThread;			// thread handle
	unsigned int mThreadId;	// thread id
	bool mSelfDelete;		// self-destruct on exit yes/no
	int mPriority;			// the thread priority
	std::string mName;		// optional thread name
	Event mRunning;			// signalled during runtime

public:
	BaseThread(const char* name, int priority = THREAD_PRIORITY_NORMAL);
	
	virtual ~BaseThread();

	// Aborts the thread
	virtual void Abort();
	virtual void AbortImmediate();

	// wait for thread's completion
	bool Wait(DWORD timeout = INFINITE);

	virtual void Start();

	static unsigned __stdcall ThreadHandlerProc(void* lParam);

	virtual void Worker() = 0;

	bool IsRunning();

private:
	void SetThreadName();
};

FIRERENDER_NAMESPACE_END;
