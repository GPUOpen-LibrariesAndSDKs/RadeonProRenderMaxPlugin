#pragma once

#include "frScope.h"

FIRERENDER_NAMESPACE_BEGIN

class FireRenderIESLight;

template<typename Derived>
class IES_Panel
{
public:
	using BasePanel = IES_Panel;

	IES_Panel(FireRenderIESLight* parent) :
		m_panel(nullptr),
		m_parent(parent)
	{}

	void BeginEdit(IObjParam* objParam, ULONG flags, Animatable* prev)
	{
		FASSERT(m_panel == nullptr);

		auto _this = static_cast<Derived*>(this);

		m_panel = objParam->AddRollupPage(
			fireRenderHInstance,
			MAKEINTRESOURCE(Derived::DialogId),
			Derived::DlgProc,
			Derived::PanelName,
			(LPARAM)_this);

		_this->InitDialog();
		objParam->RegisterDlgWnd(m_panel);
		
		auto wndContext = reinterpret_cast<LONG_PTR>(_this);
		auto prevLong = SetWindowLongPtr(m_panel, GWLP_USERDATA, wndContext);
		FASSERT(prevLong == 0);
	}

	void EndEdit(IObjParam* objParam, ULONG flags, Animatable* next)
	{
		FASSERT(m_panel != nullptr);
		objParam->DeleteRollupPage(m_panel);
		m_panel = nullptr;
	}

	// Default empty implementations
	bool InitDialog() { return true; }
	INT_PTR HandleControlCommand(WORD code, WORD controlId) { return FALSE; }

protected:

	HWND m_panel;
	FireRenderIESLight* m_parent;

private:

	static INT_PTR CALLBACK DlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		switch (msg)
		{
			case WM_INITDIALOG:
				return TRUE;
				break;

			case WM_COMMAND:
			{
				if (lParam != 0)
				{
					auto code = HIWORD(wParam);
					auto controlId = LOWORD(wParam);
					auto wndUserData = GetWindowLongPtr(hWnd, GWLP_USERDATA);
					auto _this = reinterpret_cast<Derived*>(wndUserData);
					FASSERT(_this != nullptr);

					return _this->HandleControlCommand(code, controlId);
				}
			}
			break;
		}

		return FALSE;
	}
};

FIRERENDER_NAMESPACE_END
