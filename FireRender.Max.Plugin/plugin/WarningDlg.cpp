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

#include "WarningDlg.h"

#include "plugin/FireRenderer.h"

#include "resource.h"

#include "ParamBlock.h"
#include "plugin/FRSettingsFileHandler.h"

FIRERENDER_NAMESPACE_BEGIN


#define TIMER_HANDLE	0xBABA
#define WND_CLASS_NAME	L"NotificationWindowClass"

bool WarningDlg::isClassRegistered = false;
std::string WarningDlg::dontShowNotificationAttrName = "";

WarningDlg::WarningDlg(FireRenderer *r)
{
	renderer = r;

	dontShowChecked = false;

	if (!isClassRegistered)
	{
		WNDCLASS wc = {};
	
		wc.lpfnWndProc = (WNDPROC)WndProc;
		wc.hInstance = FireRender::fireRenderHInstance;
		wc.lpszClassName = WND_CLASS_NAME;

		if (!RegisterClass(&wc))
		{
			DWORD error = GetLastError();
			if (error != 1410) {
				return;
			}
		}

		isClassRegistered = true;
	}
	
	warningWindow = CreateWindowEx(WS_EX_PALETTEWINDOW,
		WND_CLASS_NAME,
		L"Radeon ProRender Warning Notification",
		WS_OVERLAPPEDWINDOW,
		0, 0, 0,0,
		(HWND)NULL,
		(HMENU)NULL,
		FireRender::fireRenderHInstance,
		this);

	if (warningWindow == NULL)
	{
		DWORD error = GetLastError();
	}
	else
	{
		warningDialogBox = CreateDialog(FireRender::fireRenderHInstance,
			MAKEINTRESOURCE(IDD_FIRERENDER_WARNING),
			(HWND)warningWindow,
			(DLGPROC)DlgProc);
		if (warningDialogBox != 0)
			ShowWindow(warningDialogBox, SW_SHOW);
	}
}

WarningDlg::~WarningDlg()
{
	if (NULL!=warningWindow)
	{
		DestroyWindow(warningWindow);
	}
}

//if 1 => no light
//if 2 => point light
void WarningDlg::show(std::wstring textToShow, std::string uniqueAttrName)
{
	WarningDlg::dontShowNotificationAttrName = uniqueAttrName;

	HWND textController = GetDlgItem(warningDialogBox, IDC_WARNING_MESSAGE);
	SendMessage(textController, WM_SETTEXT, 0, (LPARAM)textToShow.c_str());

	if (warningWindow == NULL)
	{
		return;
	}
	
	IParamBlock2* pb = renderer->GetParamBlock(0);

	BOOL globalDontShowWarning;
	pb->GetValue(PARAM_WARNING_DONTSHOW, 0, globalDontShowWarning, Interval());

	if (!globalDontShowWarning)
	{
		std::string temp = FRSettingsFileHandler::getAttributeSettingsFor(FRSettingsFileHandler::GlobalDontShowNotification);
		if (temp.length() > 0)
		{
			globalDontShowWarning = std::stoi(temp);
		}

		// Warnings are globally enabled
		if (!globalDontShowWarning)
		{
			// Warning is locally enabled
			temp = FRSettingsFileHandler::getAttributeSettingsFor(WarningDlg::dontShowNotificationAttrName);

			// Empty entry or entry is false
			if (temp.length() == 0 || std::stoi(temp) == 0)
			{
				CheckDlgButton(warningDialogBox, IDC_WARNING_DONTSHOW, false);
				ShowWindow(warningWindow, SW_SHOWNA);

				// Center it's position
				RECT maxRect;
				GetWindowRect(GetCOREInterface()->GetMAXHWnd(), &maxRect);

				RECT dialogRect;
				GetWindowRect(warningWindow, &dialogRect);

				int dlgWidth = (dialogRect.right - dialogRect.left);
				int dlgHeight = (dialogRect.bottom - dialogRect.top);
				LONG left = (maxRect.left + maxRect.right) * 0.5f - dlgWidth * 0.5;
				LONG top = (maxRect.bottom + maxRect.top) * 0.5f - dlgHeight * 0.5;

				SetWindowPos(warningWindow, NULL, left, top, dlgWidth, dlgHeight, 0);
			}
		}
	}
	
	begin_time = clock();
}

BOOL CALLBACK WarningDlg::DlgProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
		{
			RECT rectDlg;
			GetWindowRect((HWND)hwndDlg, &rectDlg);
	
			Rect wndRect;
			SystemParametersInfo(SPI_GETWORKAREA, NULL, &wndRect, NULL);
	
			float warningWidth = rectDlg.right + rectDlg.left;
			float warningHeight = rectDlg.bottom;
	
			float workAreaWidth = abs(wndRect.right - wndRect.left);
			float workAreaHeight = abs(wndRect.bottom - wndRect.top);
	
			float offset = 15;
	
			SetWindowPos(GetParent(hwndDlg), HWND_TOPMOST, workAreaWidth - warningWidth - offset, workAreaHeight - warningHeight - offset, warningWidth, warningHeight, 0);
		}
		break;
	case WM_COMMAND:
		switch (HIWORD(wParam))
		{
		case BN_CLICKED:
			{
				switch(LOWORD(wParam))
				{
				case IDC_WARNING_CLOSE:
					ShowWindow(GetParent(hwndDlg), SW_HIDE);
					break;
				case IDC_WARNING_DONTSHOW:
					saveSettings(GetParent(hwndDlg));
					break;
				}
			}
			break;
		}
		break;
	}

	return FALSE;
}

void WarningDlg::saveSettings(HWND hWnd)
{
	WarningDlg *wd = reinterpret_cast<WarningDlg*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
	BOOL warningsSUppressGlobal = BOOL(IsDlgButtonChecked(wd->warningDialogBox, IDC_WARNING_DONTSHOW));
	FRSettingsFileHandler::setAttributeSettingsFor(WarningDlg::dontShowNotificationAttrName, std::to_string(warningsSUppressGlobal));
}

//WndProc function
LRESULT CALLBACK WarningDlg::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_NCCREATE:
		{
			CREATESTRUCT * pcs = (CREATESTRUCT*)lParam;
			WarningDlg *wd = (WarningDlg*)pcs->lpCreateParams;
			SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)pcs->lpCreateParams);
		}
		break;
	case WM_CLOSE:
		{
			ShowWindow(hWnd, SW_HIDE);
			return 1;
		}
		break;
	case WM_SHOWWINDOW:
		if (lParam == NULL)
		{
			if (!wParam)
			{
				saveSettings(hWnd);
			}
			else
			{
				//shown
				// UI timer event
				SetTimer(hWnd, TIMER_HANDLE, 33, 0);
			}
		}
		break;
	case WM_DESTROY:
		{
			return 0;
		}
		break;
	case WM_TIMER:
		if (wParam == TIMER_HANDLE)
		{
			WarningDlg *wd = reinterpret_cast<WarningDlg*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
			float visibilityTime = float(clock() - wd->begin_time) / CLOCKS_PER_SEC;
			if (visibilityTime > 10)
			{
				KillTimer(hWnd, TIMER_HANDLE);

				// Why??? Don't hide the warning message from the user after some time
				//ShowWindow(hWnd, SW_HIDE);
			}
		}
		break;
	}

	return DefWindowProc(hWnd, msg, wParam, lParam);
}

FIRERENDER_NAMESPACE_END