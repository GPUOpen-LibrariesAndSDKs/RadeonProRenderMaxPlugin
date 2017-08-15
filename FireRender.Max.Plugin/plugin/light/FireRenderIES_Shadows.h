#pragma once

#include "FireRenderIES_Panel.h"
#include "Resource.h"

FIRERENDER_NAMESPACE_BEGIN

class IES_Shadows :
	public IES_Panel<IES_Shadows>
{
public:
	static constexpr int DialogId = IDD_FIRERENDER_IES_LIGHT_SHADOWS;
	static constexpr TCHAR* PanelName = _T("Shadows");

	static INT_PTR CALLBACK dlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		switch (msg)
		{
		case WM_INITDIALOG:
			return TRUE;
		}

		return FALSE;
	}
};

FIRERENDER_NAMESPACE_END
