#include "stdafx.h"
#include <msi.h>
#include <msiquery.h>
#include <Shellapi.h>

#include <string>
#include <iostream>
#include <vector>
#include <tuple>
#include <regex>

#include <filesystem>

#pragma comment(lib, "msi.lib")
#pragma comment(lib, "Shell32.lib")
#pragma comment(lib, "Ole32.lib")
#pragma comment(lib, "User32.lib")

std::wstring getPropertyValue(MSIHANDLE hInstall, const std::wstring& propertyName)
{
	std::wstring result;

	DWORD valueSize = 0;

	UINT status = MsiGetProperty(hInstall, propertyName.c_str(), L"", &valueSize);

	if (ERROR_MORE_DATA == status)
	{
		valueSize++; // additional space for null terminator

		std::vector<WCHAR> value(valueSize);

		status = MsiGetProperty(hInstall, propertyName.c_str(), &value[0], &valueSize);

		if (ERROR_SUCCESS == status && !value.empty())
		{
			result = value.data();
		}
	}

	return std::move(result);
}

extern "C" __declspec(dllexport) UINT postInstall(MSIHANDLE hInstall)
{
	return ERROR_SUCCESS;
}
