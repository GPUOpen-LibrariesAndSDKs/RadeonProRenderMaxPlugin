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
#include "Common.h"

FIRERENDER_NAMESPACE_BEGIN;

class FireRenderer;

class WarningDlg
{
public:
	WarningDlg(FireRenderer* r);
	~WarningDlg();

	static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	static BOOL CALLBACK DlgProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam);

	void show(std::wstring textToShow, std::string uniqueAttrName);

	static void saveSettings(HWND hWnd);

	HWND warningWindow;
	HWND warningDialogBox;

	static std::string dontShowNotificationAttrName;

	bool dontShowChecked;

	clock_t begin_time;

	static bool isClassRegistered;

protected:
	FireRenderer* renderer;

};

FIRERENDER_NAMESPACE_END;