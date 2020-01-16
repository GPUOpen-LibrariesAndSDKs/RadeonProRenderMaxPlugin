#include "stdafx.h"
#include <msi.h>
#include <msiquery.h>
#include <Shellapi.h>
#include "checkCompatibility.h"

#include <string>
#include <iostream>
#include <vector>
#include <tuple>
#include <regex>

#include <filesystem>

#pragma comment(lib, "msi.lib")
#pragma comment(lib, "Shell32.lib")
#pragma comment(lib, "Ole32.lib")


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

std::vector<std::wstring> getMaxVersionWithInstalledPlugin(MSIHANDLE hInstall)
{
	std::wstring maxVersions = getPropertyValue(hInstall, L"MAX_VERSIONS_INSTALLED");

	std::vector<std::wstring> result;

	std::wregex regex(L"\\d{4}");
	std::wsmatch match;

	for (std::wsregex_iterator next(maxVersions.begin(), maxVersions.end(), regex); next != std::wsregex_iterator(); next++)
	{
		result.push_back(next->str());
	}	

	return result;
}

extern "C" __declspec(dllexport) UINT hardwareCheck(MSIHANDLE hInstall)
{
	std::wstring hw_message;
	bool hw_res = checkCompatibility_hardware(hw_message);

	std::wstring sw_message;
	bool sw_res = checkCompatibility_driver(sw_message);

	std::wstring add_message(L"");

	MsiSetProperty(hInstall, L"HARDWARECHECK_RESULT", hw_res ? L"1" : L"0");
	MsiSetProperty(hInstall, L"SOFTWARECHECK_RESULT", sw_res ? L"1" : L"0");
	MsiSetProperty(hInstall, L"ADDITIONALCHECK_RESULT", L"1");

	if (!hw_res || !sw_res)
	{
		std::wstring text;

		if (!hw_res)
			text += L"\r\n" + hw_message;

		if (!sw_res)
			text += L"\r\n" + sw_message;

		std::wstring s = L"Detail info:" + text;

		MsiSetProperty(hInstall, L"CHECK_RESULT_TEXT", s.c_str());
	}

	return ERROR_SUCCESS;
}

extern "C" __declspec(dllexport) UINT postInstall(MSIHANDLE hInstall)
{
	return ERROR_SUCCESS;
}
