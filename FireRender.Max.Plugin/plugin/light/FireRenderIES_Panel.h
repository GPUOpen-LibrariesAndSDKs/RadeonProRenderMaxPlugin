#pragma once

#include "frScope.h"

FIRERENDER_NAMESPACE_BEGIN

template<typename Derived>
class IES_Panel
{
public:
	IES_Panel() :
		m_panel(nullptr)
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

		_this->InitParams(objParam, flags, prev);
	}

	// Default empty implementation
	void InitParams(IObjParam* objParam, ULONG flags, Animatable* prev)
	{
		
	}

	void EndEdit(IObjParam* objParam, ULONG flags, Animatable* next)
	{
		FASSERT(m_panel != nullptr);
		objParam->DeleteRollupPage(m_panel);
		m_panel = nullptr;
	}

protected:
	HWND m_panel;
};

FIRERENDER_NAMESPACE_END
