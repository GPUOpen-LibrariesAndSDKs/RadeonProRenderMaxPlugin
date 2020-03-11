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

#include <windows.h>
#include <sstream>
#include <max.h>
#include <string>
#include <RadeonProRender.h>

#ifdef max // This macro is defined inside 3ds Max API/windows.h
#   undef max
#endif
#ifdef min
#   undef min
#endif

#define MAX(a,b) ((a)>=(b)?(a):(b))

// We use these to put all our code in a namespace. This prevents name clashes with 3ds Max API which tends to define some very 
// common words as its classes.
#define FIRERENDER_NAMESPACE_BEGIN namespace FireRender {
#define FIRERENDER_NAMESPACE_END }

inline bool bool_cast( int x ) { return (x? true:false); }
inline bool bool_cast( UINT x ) { return (x? true:false); }
inline bool bool_cast( WORD x ) { return (x? true:false); }
inline bool bool_cast( wchar_t x ) { return (x? true:false); }
inline bool bool_cast( void* x ) { return (x!=NULL? true:false); }
inline DWORD DWORD_cast( size_t x ) { return (DWORD)x; }
inline int int_cast( size_t x ) { return (int)x; }
inline int int_cast( wchar_t x ) { return (int)x; }

TCHAR *GetString(int id);

FIRERENDER_NAMESPACE_BEGIN;

/// HINSTANCE of this plugin DLL. It is initialized inside DllMain and later used to load win32 dialogs.
extern HINSTANCE fireRenderHInstance;

/// Shows an assert dialog if something goes wrong in the application
void AssertImpl(const MCHAR* expression, const MCHAR* file, const int line);

// Make a check with log entry and message box
void RPRResultCheckImpl(rpr_int result, const MCHAR* file, const int line, rpr_context context=nullptr, const char* functionName=nullptr);
void RPRLogContextStatus(rpr_int result, rpr_context context, const char* functionName);
void LogErrorStringToMaxLog(const std::wstring& str);


/// We use the "STOP" macro to kill the program execution if it reaches invalid state (such as non-implemented method). Program 
/// is killed in both debug and release configurations, although in debug config it also displays an assert.
#define STOP FASSERT(false); throw ::FireRender::Exception(_T("STOP@") _T(__FILE__) _T("(") + ::FireRender::toWString(__LINE__) + _T(")"));

/// We use the FASSERT macro throughout the application to check for consistency. Here the checks can be either disabled by
/// defining the macro to empty string, or enabled using the assertImpl function
#if defined(FIREMAX_DEBUG)
#   define FASSERT(condition) if((condition)) { } else { ::FireRender::AssertImpl(_T(#condition), _T(__FILE__), __LINE__); }
#else
#   define FASSERT(condition) if((condition)) { }
#endif

#define FCHECK(resultCode) ::FireRender::RPRResultCheckImpl(resultCode, _T(__FILE__), __LINE__)
#define FCHECK_CONTEXT(resultCode, context, functionName) ::FireRender::RPRResultCheckImpl(resultCode, _T(__FILE__), __LINE__, context, functionName)

std::wstring GetRPRErrorString(rpr_int code);

/// Class ID of the main rendering plugin that translates data between Radeon ProRender code and 3ds Max
const Class_ID FIRE_MAX_CID(0x61458ded, 0x9fa91ad5);

/// Class ID of the material importer plugin
const Class_ID FIRERENDER_MTLIMPORTER_CID(0x1c4c3601, 0x3cf61386);

/// Class ID of the material exporter plugin
const Class_ID FIRERENDER_MTLEXPORTER_CID(0x7b6304e1, 0x29266cd0);

/// Class ID of the Radeon ProRender custom material
//const Class_ID OSL_MTL_CID(0x61458deF, 0x9fa91ad6);

/// Class ID of the custom batch auto-testing tool developed for Radeon ProRender
const Class_ID AUTOTESTING_CID(0x2d6c1f12, 0x06cd3ad7);

/// Class ID of the Radeon ProRender custom OSL material (currently abandoned)
const Class_ID FIRERENDER_STANDARDMTL_CID(0x2d6c1f11, 0x06cd3ad8);

/// Class ID of the Radeon ProRender custom diffuse material node
const Class_ID FIRERENDER_DIFFUSEMTL_CID(0x3ab35032, 0x30a24a76);
/// Class ID of the Radeon ProRender custom blend material node
const Class_ID FIRERENDER_BLENDMTL_CID(0x76095ab2, 0x6e0e0423);
/// Class ID of the Radeon ProRender custom add material node
const Class_ID FIRERENDER_ADDMTL_CID(0x144d646d, 0x2e490a2f);
/// Class ID of the Radeon ProRender custom microfacet material node
const Class_ID FIRERENDER_MICROFACETMTL_CID(0x36d6148, 0x4b945f04);
/// Class ID of the Radeon ProRender custom reflection material node
const Class_ID FIRERENDER_REFLECTIONMTL_CID(0x1b9c5195, 0x68e272a);
/// Class ID of the Radeon ProRender custom arithmetic material node
const Class_ID FIRERENDER_ARITHMMTL_CID(0x570b1b24, 0x679c76db);
/// Class ID of the Radeon ProRender custom input lookup material node
const Class_ID FIRERENDER_INPUTLUMTL_CID(0x46a6376b, 0x7aa10ae2);
/// Class ID of the Radeon ProRender custom blend value material node
const Class_ID FIRERENDER_BLENDVALUEMTL_CID(0x3d943999, 0x19e26bae);
/// Class ID of the Radeon ProRender custom refraction material node
const Class_ID FIRERENDER_REFRACTIONMTL_CID(0x11211188, 0x6fc010be);
/// Class ID of the Radeon ProRender custom microfacet refraction material node
const Class_ID FIRERENDER_MFREFRACTIONMTL_CID(0x67524fec, 0x22ab1ab8);
/// Class ID of the Radeon ProRender custom transparent material node
const Class_ID FIRERENDER_TRANSPARENTMTL_CID(0x28154ffd, 0x4b4956ce);
/// Class ID of the Radeon ProRender custom ward material node
const Class_ID FIRERENDER_WARDMTL_CID(0x1aae4c0f, 0x55fe4a6e);
/// Class ID of the Radeon ProRender custom emissive material node
const Class_ID FIRERENDER_EMISSIVEMTL_CID(0x5fb515b9, 0x753f2469);
/// Class ID of the Radeon ProRender custom fresnel material node
const Class_ID FIRERENDER_FRESNELMTL_CID(0x49730de4, 0x23a86f46);
/// Class ID of the Radeon ProRender custom color material node
const Class_ID FIRERENDER_COLORMTL_CID(0x67fb3b5a, 0x61f83a9a);
/// Class ID of the Radeon ProRender custom average material node
const Class_ID FIRERENDER_AVERAGEMTL_CID(0x32e848a3, 0x2ffa6ecc);
/// Class ID of the Radeon ProRender custom oren-nayar material node
const Class_ID FIRERENDER_ORENNAYARMTL_CID(0x75e11a55, 0x7d100b0b);
/// Class ID of the Radeon ProRender custom fresnel-shlick material node
const Class_ID FIRERENDER_FRESNELSCHLICKMTL_CID(0x3dd86e76, 0x3bb020f4);
/// Class ID of the Radeon ProRender custom displacement material node
const Class_ID FIRERENDER_DISPLACEMENTMTL_CID(0x67d2628d, 0x55970bf5);
/// Class ID of the Radeon ProRender custom normal map node
const Class_ID FIRERENDER_NORMALMTL_CID(0x31607d62, 0x5047654a);
/// Class ID of the Radeon ProRender custom diffuse refraction material node
const Class_ID FIRERENDER_DIFFUSEREFRACTIONMTL_CID(0x7a4c7135, 0x4c0f232e);
/// Class ID of the Radeon ProRender custom material node
const Class_ID FIRERENDER_MATERIALMTL_CID(0x4969393f, 0x31fd5cb3);
/// Class ID of the Radeon ProRender custom Uber material node
const Class_ID FIRERENDER_UBERMTL_CID(0x2acc79c5, 0x314e0f46);
/// Class ID of the Radeon ProRender custom Uber material v2 node
const Class_ID FIRERENDER_UBERMTLV2_CID(0x358559fe, 0x22231035);
/// Class ID of the Radeon ProRender custom Uber material v3 node
const Class_ID FIRERENDER_UBERMTLV3_CID(0x16345e69, 0x7c213ca1);
/// Class ID of the Radeon ProRender custom Volume material node
const Class_ID FIRERENDER_VOLUMEMTL_CID(0x716a2426, 0x37252fc1);
/// Class ID of the Radeon ProRender custom PBR material node
const Class_ID FIRERENDER_PBRMTL_CID(0x162a1228, 0x753a0690);
/// Class ID of the Radeon ProRender custom Shadow Catcher material node
const Class_ID FIRERENDER_SCMTL_CID(0x7b696b89, 0x43032fb6);
/// Class ID of the Radeon ProRender custom Colour Correction material node
const Class_ID FIRERENDER_COLOURCORMTL_CID(0x4f9550e6, 0x756158db);
/// Class ID of the Radeon ProRender custom Ambient Occlusion material node
const Class_ID FIRERENDER_AMBIENTOCCLUSIONMTL_CID(0x1aa975a7, 0xed22891);

// GUP singleton that manages legacy with old bg objects
const Class_ID BGMANAGER_MAX_CLASSID(0x4069388a, 0x277a415f);

// GUP singleton that manages tonemapper options
const Class_ID TMMANAGER_MAX_CLASSID(0x2410e9e, 0x20793a80);

// GUP singleton that manages camera options
const Class_ID CAMMANAGER_MAX_CLASSID(0x2d887e80, 0x7aee2f0f);

// GUP singleton that manages material preview rendering
const Class_ID MPMANAGER_MAX_CLASSID(0x1d9c21f2, 0x465f38fa);

// GUP singleton that manages RPR scopes
const Class_ID SCOPEMANAGER_MAX_CLASSID(0x5cba2404, 0x75d46dc8);

// GUP singleton that manages production rendering
const Class_ID PRMANAGER_MAX_CLASSID(0x47c00bd0, 0x53566810);

/// Class ID of the Radeon ProRender expoeter
const Class_ID EXPORTER_MAX_CLASSID(0x32171ca9, 0x43517a2d);


/// Interface ID of the Radeon ProRender Render Elements
const ULONG FIRERENDER_RENDERELEMENT_INTERFACEID(0xd8c17f1);

/// Name of the renderer displayed in the UI
#define FIRE_MAX_NAME _T("Radeon ProRender")

/// Data folder has contains configs, logs, etc
#define DATA_FOLDER_NAME _T("\\Radeon ProRender\\")

/// Name of the (unfinished) OSL material displayed in the UI
#define OSL_MTL_NAME _T("RPR OSL Material")

/// Name of the main Radeon ProRender custom material displayed in the UI
#define FIRERENDER_MTL_NAME _T("RPR Standard Material")

/// Name of the automatic testing tool displayed in the UI
#define AUTOTESTING_NAME _T("Radeon ProRender Autotesting tool")

// Next we disable some warning messages so we can use warning level 4 without sacrificing some useful constructs

// "conditional expression is constant" - for while(true), ...
#pragma warning( disable : 4127 )

// "nonstandard extension used : nameless struct/union"
#pragma warning( disable : 4201 )

// "nonstandard extension used : class rvalue used as lvalue" - when using &T()
#pragma warning( disable : 4238 )

// "nonstandard extension used : conversion from T to T& A non-const reference may only be bound to an lvalue" - when using
// foo(T())
#pragma warning( disable : 4239 )

// "initializing - signed/unsigned mismatch"
#pragma warning( disable : 4245 )

// "unreferenced formal parameter"
#pragma warning( disable : 4100 )

// Some defines to deal with 3ds Max changing its UI between versions (without too many #ifdefs)

/// We need this define because the signature of the function changed between 3ds Max 2014 and 2015
#if MAX_PRODUCT_YEAR_NUMBER >= 2015
#   define NOTIFY_REF_CHANGED_PARAMETERS const Interval& interval, RefTargetHandle hTarget, PartID& partId, RefMessage msg, BOOL propagate
#else
#   define NOTIFY_REF_CHANGED_PARAMETERS Interval interval, RefTargetHandle hTarget, PartID& partId, RefMessage msg
#endif

#if MAX_PRODUCT_YEAR_NUMBER >= 2013
#   define CONST_2013 const
#   define PB_END p_end
#   define CLEAR_MAX_ARRAY(x) (x).removeAll()
#else
#   define CONST_2013
#   define PB_END end
#   define CLEAR_MAX_ARRAY(x) (x).Resize(0)
#endif

// Diagnostic: snoop available params
void DumpParams(Animatable *p, int indent = 1);

#define GET_PARAM(type, name, pid)	\
	type name = GetFromPb<type>(pb, pid, timeVal);

// table lookup
#define GET_PARAM_N(type, name, pid, index)	\
	type name = GetFromPb<type>(pb, pid, timeVal, nullptr, index);

#if MAX_PRODUCT_YEAR_NUMBER >= 2017
#define MAX2017_OVERRIDE override
#else
#define MAX2017_OVERRIDE
#endif

FIRERENDER_NAMESPACE_END;