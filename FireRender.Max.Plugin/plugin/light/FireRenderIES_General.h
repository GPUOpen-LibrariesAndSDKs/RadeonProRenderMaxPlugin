#pragma once

#include "FireRenderIES_Panel.h"
#include "Resource.h"

FIRERENDER_NAMESPACE_BEGIN

class IES_General :
	public IES_Panel<IES_General>
{
public:
	static constexpr int DialogId = IDD_FIRERENDER_IES_LIGHT_GENERAL;
	static constexpr TCHAR* PanelName = _T("General");

	static INT_PTR CALLBACK dlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		switch (msg)
		{
		case WM_INITDIALOG:
			return TRUE;
		}

		return FALSE;
	}

	void InitParams(IObjParam* objParam, ULONG flags, Animatable* prev)
	{
	}
};

FIRERENDER_NAMESPACE_END
