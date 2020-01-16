// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once


#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>
#include <string>
#include <stdio.h>
#include <vector>
#include <fstream>
#include <sstream>
#include <iterator>
#include <iostream>
#include <shlobj.h>
#include <assert.h>


#include "Logger.h"

template <typename... Args>
inline void LogSystem(const char *format, const Args&... args)
{
	Logger::Printf(Logger::LevelInfo, format, args...);
}

std::wstring s2ws(const std::string& str);
std::string ws2s(const std::wstring& wstr);

std::vector<std::wstring> split(const std::wstring &s, wchar_t delim);
void copyStringToClipboard(const std::wstring &str);
std::wstring GetSystemFolderPaths(int csidl);
std::wstring & URLfirendly(std::wstring & str);
