/*********************************************************************************************************************************
* Radeon ProRender for 3ds Max plugin
* Copyright (C) 2017 AMD
* All Rights Reserved
*
* A collection of utility functions
*********************************************************************************************************************************/

#pragma once

#include "RadeonProRender.h"
#include "Common.h"
#include "HashValue.h"

#include <iparamm2.h>

#include <vector>

FIRERENDER_NAMESPACE_BEGIN

/// Converts anything to string without any boilerplate code via std::stringstream
template<class T>
std::string toString(const T& x) {
    std::stringstream stream;
    stream << x;
    return stream.str();
}

/// Converts anything to wstring without any boilerplate code via std::wstringstream
template<class T>
std::wstring toWString(const T& x) {
    std::wstringstream stream;
    stream << x;
    return stream.str();
}

/// unicode->ASCII conversion
std::string ToAscii(const std::wstring& input);

/// unicode->ASCII conversion
std::wstring ToUnicode(const std::string& input);

/// We use separate exception from std::exception so we can selectively set breakpoint only for it - 3ds Max throws a LOT of 
/// std::exceptions. 
class Exception : public std::exception {
protected:
    std::string reason;

public:
    Exception(const std::string& reason) : reason(reason) {}

    Exception(const std::wstring& reason) {
        this->reason = ToAscii(reason);
    }

    virtual const char* what() const {
        return this->reason.c_str();
    }
};

/// Prints contents (list of parameters + some info such as type and name) of a single IParamBLock2 into a file. 
/// Useful for reverse-engineering 3ds Max texmaps/material
void PrintPblock(IParamBlock2* pb, std::ostream& output);

/// Prints a line to the debugger console output window, with a timestamp and "RPR" tag. Makes the debugging a bit easier
void debugPrint(const std::wstring& msg);

/// ASCII-version of DebugPrint
inline void debugPrint(const std::string& msg) {
    return debugPrint(ToUnicode(msg));
}

class DebugScope
{
	std::wstring name;
	clock_t begin;
public:
	explicit DebugScope(const wchar_t * str) : name(str)
	{
		debugPrint(std::wstring(L"ENTER ") + name);
		begin = clock();
	}
	explicit DebugScope(const char * str) : DebugScope(ToUnicode(str).c_str()){}

	~DebugScope()
	{
		auto t = clock() - begin;
		wchar_t buf[1024 + 1] = {};
		wsprintf(buf, L"LEAVE %s (%d ms)", name.c_str(), t);
		DebugPrint(buf);
	}
};

/// Returns a folder in which 3ds Max is recommending the plugin to store its configuration, temporary data, and outputs.
/// The path should always return existing, writable folder
std::wstring GetDataStoreFolder();

/// Read only folder
std::wstring GetModuleFolder();

/// Calculates determinant of a matrix.
float GetDeterminant(const Matrix3& input);

/// if SWITCH_AXES is on, flips Y and Z axis of an input matrix. This is used to transform Z-up 3ds Max inputs to Y-up RPR,
/// although it leads to changing the matrix determinant, so we have disabled it
inline Matrix3 FlipAxes(Matrix3 in) {
#ifdef SWITCH_AXES
    static Matrix3 tmp(Point3(1.f,  0.f, 0.f),
                       Point3(0.f,  0.f, 1.f),
                       Point3(0.f,  1.f, 0.f),
                       Point3(0.f,  0.f, 0.f));
    in = in*tmp;
#endif
    return in;
}

/// if SWITCH_AXES is on, corrects TM of a light so it shines in the same direction in 3ds Max and in RPR.
inline Matrix3 fxLightTm(Matrix3 in) {
#ifdef SWITCH_AXES
    static Matrix3 tmp(Point3(1.f, 0.f, 0.f),
                       Point3(0.f, -1.f, 0.f),
                       Point3(0.f, 0.f, -1.f),
                       Point3(0.f, 0.f, 0.f));
    in = tmp*in;
#endif
    return in;
}

/// Creates 4*4 column-major matrix (used by RPR) in out parameter from given 3ds Max 4*3 matrix
inline void CreateFrMatrix(Matrix3 in, float out[16]) {
    in = FlipAxes(in);
    for (int x = 0; x < 4; ++x) {
        for (int y = 0; y < 3; ++y) {
            *out++ = in.GetRow(x)[y];
        }
        *out++ = (x == 3) ? 1.f : 0.f;
    }
}

/// Stores data from RPR frame buffer in 3ds Max Bitmap class. Possibly runs multiple threads to speed the process up
/// \param exposure A constant with which the read data is multiplied before writing it to the bitmap
/// \param isNormals If true, the colors are transformed between RPR and 3ds Max interpretation of normal channel 
///                  (RPR outputs normals in -1..1 range, 3ds Max and other applications expect 0..1 range)
void CompositeFrameBuffersToBitmap(std::vector<float>& fbData, std::vector<float>& fbAlphaData, Bitmap* output, const float exposure, const bool isNormals, bool toneOperatorPreviewExecuting);

void CopyDataToPreviewBitmap(const std::vector<float>& fbData, Bitmap* output, const bool isNormals);
void CopyDataToBitmap(std::vector<float>& data, const std::vector<float>& alphaData, Bitmap* output, const float exposure, const bool isNormals);


/// Calculates an average (gray) value of a color
inline float avg(const Color& in) {
    return (in.r + in.g + in.b) * (1.f / 3.f); // multiplication is faster. 1/3 is calculated by compiler as constant.
}

/// Safely reads data from a 3ds Max IParamBlock2. Triggers an assert if the read fails (for example because the parameter does 
/// not exist or is of different type)
template<class Type>
inline Type GetFromPb(IParamBlock2* pb, const ParamID id, const TimeValue t = 0, Interval* outValid = NULL, const int tabIndex = 0) {
	FASSERT(pb);
	Interval dummy;
	Type retVal;
	BOOL res = pb->GetValue(id, t, retVal, outValid ? *outValid : dummy, tabIndex);
	FASSERT(res); (void)res;
	return retVal;
}

template<class ReturnType, class StorageType>
inline ReturnType GetFromPb(IParamBlock2* pb, const ParamID id, const TimeValue t = 0, Interval* outValid = NULL, const int tabIndex = 0) {
	FASSERT(pb);
	Interval dummy;
	StorageType retVal;
	BOOL res = pb->GetValue(id, t, retVal, outValid ? *outValid : dummy, tabIndex);
	FASSERT(res); (void)res;
	return static_cast<ReturnType>(retVal);
}


/// Loads bool (stored as int)
template<>
inline bool GetFromPb(IParamBlock2* pb, const ParamID id, const TimeValue t, Interval* outValid, const int tabIndex) {
	return GetFromPb<int>(pb, id, t, outValid, tabIndex) != FALSE;
}

/// Safely reads data from a 3ds Max IParamBlock2. Triggers an assert if the read fails (for example because the parameter does 
/// not exist or is of different type)
template<class Type>
inline Type GetFromPbOrDefault(IParamBlock2* pb, const ParamID id, const Type& orDefault, const TimeValue t = 0, Interval* outValid = NULL, const int tabIndex = 0) {
	FASSERT(pb);
	Interval dummy;
	Type retVal;
	if (pb->GetValue(id, t, retVal, outValid ? *outValid : dummy, tabIndex))
		return retVal;

	return  orDefault;
}

/// Loads bool (stored as int)
template<>
inline bool GetFromPbOrDefault(IParamBlock2* pb, const ParamID id, const bool& orDefault, const TimeValue t, Interval* outValid, const int tabIndex) {
	return GetFromPbOrDefault<int>(pb, id, t, orDefault, outValid, tabIndex) != FALSE;
}

/// Safely writes data to a 3ds Max IParamBlock2. Triggers an assert if the write fails (for example because the parameter does 
/// not exist or is of different type)
template<class Type>
inline void SetInPb(IParamBlock2* pb, const ParamID id, const Type& value, const TimeValue t = 0, const int tabIndex = 0) {
    FASSERT(pb);
    BOOL res = pb->SetValue(id, t, value, tabIndex);
    FASSERT(res); (void)res;
}

/// Converts between different 3ds Max formats for viewport data
/// \param outCameraNode where to write pointer to the camera used (or NULL if the input is a free view)
ViewParams ViewExp2viewParams(ViewExp& viewExp, INode*& outCameraNode);

/// Returns a reference to the viewport descriptor (ViewExp) that is currently selected to be rendered in 3ds Max. It can be 
/// the currently active viewport, or another viewport if "Render view lock" is enabled in 3ds Max UI
ViewExp& GetRenderViewExp();

/// returns true if a floating point number is not NaN nor +- infinity
inline bool IsReal(const float in) {
    return !isnan(in) && abs(in) < FLT_MAX;
}

/// Returns a constant that we must multiply any length provided by 3ds Max with, to get the results in millimeters
inline float getSystemUnitsToMillimeters() {
    return float(GetMasterScale(UNITS_MILLIMETERS));
}

/// Returns true if the calling thread is the main thread of the application
bool IsMainThread();

/// Returns true if the input texmap is a bump map (as opposed to a normal map). NULL input is valid, and returns false
bool IsBumpMap(Texmap* input);

float GetUnitScale();

template <class T>
T* GetSubAnimByType(Animatable* p, bool recursive = false)
{
	for (int i = 0, n = p->NumSubs(); i < n; i++)
	{
		auto sub = p->SubAnim(i);
		if (auto res = dynamic_cast<T*>(sub))
			return res;

		if (sub && recursive)
		{
			if (auto res = GetSubAnimByType<T>(sub, recursive))
				return res;
		}
	}
	return nullptr;
}

void UpdateBitmapWithToneOperator(Bitmap *bitmap);

Bitmap* RenderTextToBitmap(const MCHAR* text);
void BlitBitmap(Bitmap* Dst, Bitmap* Src, int x, int y, float alpha);

std::wstring GetCPUName();
std::wstring ComputerName();
std::wstring GetFriendlyUsedGPUName();
bool GetProductAndVersion(std::wstring& strProductName, std::wstring& strProductVersion, std::wstring& strCoreVersion);

void EnableGroupboxControls(HWND dialogWnd, int groupId, bool bEnabled, std::vector<int> ignoreIds);

void DumpFRParms(const rpr_material_node& node);
void DumpFRContents(const rpr_material_node& node);

inline Animatable* GetSubAnimByName(Animatable* p, MSTR name, bool recursive = false)
{
	for (int i = 0, n = p->NumSubs(); i < n; i++)
	{
		auto sub = p->SubAnim(i);
		if (p->SubAnimName(i) == name)
			return sub;	// may return null

		if (sub && recursive)
		{
			if (sub = GetSubAnimByName(sub, name, recursive))
				return sub;
		}
	}
	return nullptr;
}

inline bool IsIdentity(const Matrix3& tm)
{
	return tm.IsIdentity() ||
		(
			tm.GetRow(0) == Point3(1, 0, 0) &&
			tm.GetRow(1) == Point3(0, 1, 0) &&
			tm.GetRow(2) == Point3(0, 0, 1) &&
			tm.GetRow(3) == Point3(0, 0, 0)
			);
}

inline bool SaveBitmapToFile(const std::string& path, Bitmap* bitmap)
{
	// Create bitmap
	BitmapInfo bi;
	bi.SetType(BMM_TRUE_32);
	bi.SetFlags(MAP_HAS_ALPHA);
	bi.SetWidth((WORD)bitmap->Width());
	bi.SetHeight((WORD)bitmap->Height());
	bi.SetCustomFlag(0);
	bitmap->Create(&bi);

	// Save bitmap to file
	bi.SetName(ToUnicode(path.c_str()).c_str());
	BMMRES res = bitmap->OpenOutput(&bi);

	if (res != BMMRES_SUCCESS)
		return false;

	res = bitmap->Write(&bi);

	if (res != BMMRES_SUCCESS)
		return false;
	
	res = bitmap->Close(&bi);

	if (res != BMMRES_SUCCESS)
		return false;

	return true;
}

class AccumulationTimer
{
private:
	DWORD mStartedAt;
	DWORD mElapsed;

public:
	AccumulationTimer()
		: mStartedAt(0)
		, mElapsed(0)
	{
	}

	inline void Start()
	{
		FASSERT(mStartedAt == 0);
		mStartedAt = GetTickCount();
	}

	inline void Stop()
	{
		FASSERT(mStartedAt != 0);
		mElapsed += GetTickCount() - mStartedAt;
		mStartedAt = 0;
	}

	inline void Reset()
	{
		mElapsed = 0;
		mStartedAt = 0;
	}

	inline const DWORD &GetElapsed() const
	{
		return mElapsed;
	}
};

bool FileExists(const TCHAR* name);
bool FolderExists(const TCHAR* path);

FIRERENDER_NAMESPACE_END

// format to string
template<typename ... Args>
std::string string_format(const std::string& format, Args ... args)
{
	size_t size = snprintf(nullptr, 0, format.c_str(), args ...) + 1; // Extra space for '\0'
	std::unique_ptr<char[]> buf(new char[size]);
	snprintf(buf.get(), size, format.c_str(), args ...);
	return std::string(buf.get(), buf.get() + size - 1); // We don't want the '\0' inside
}

std::wstring s2ws(const std::string& str);
std::string ws2s(const std::wstring& wstr);
