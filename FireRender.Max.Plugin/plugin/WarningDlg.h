/*********************************************************************************************************************************
* Radeon ProRender for 3ds Max plugin
* Copyright (C) 2017 AMD
* All Rights Reserved
*
* Multi-purpose warning dialog box
*********************************************************************************************************************************/

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