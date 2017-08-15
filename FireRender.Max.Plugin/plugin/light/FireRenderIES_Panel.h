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
			Derived::dlgProc,
			Derived::PanelName,
			(LPARAM)_this);

		objParam->RegisterDlgWnd(m_panel);
	}

	// Default empty implementations
	bool InitDialog() { return true; }

	void EndEdit(IObjParam* objParam, ULONG flags, Animatable* next)
	{
		FASSERT(m_panel != nullptr);
		objParam->DeleteRollupPage(m_panel);
		m_panel = nullptr;
	}

protected:
	HWND m_panel;
	FireRenderIESLight* m_parent;

private:
	static INT_PTR CALLBACK dlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		bool result = false;

		switch (msg)
		{
		case WM_INITDIALOG:
			result = reinterpret_cast<Derived*>(lParam)->InitDialog();
			break;
		}

		return result ? TRUE : FALSE;
	}
};

FIRERENDER_NAMESPACE_END
