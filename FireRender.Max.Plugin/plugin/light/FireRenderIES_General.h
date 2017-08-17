#pragma once

#include "FireRenderIES_Panel.h"
#include "Resource.h"
#include "IESLightParameter.h"

FIRERENDER_NAMESPACE_BEGIN

class IES_General :
	public IES_Panel<IES_General>
{
public:

	static constexpr int DialogId = IDD_FIRERENDER_IES_LIGHT_GENERAL;
	static constexpr TCHAR* PanelName = _T("General");
	using BasePanel::BasePanel;

	bool InitializePage();
	void UninitializePage();
	INT_PTR HandleControlCommand(WORD code, WORD controlId);

	void SaveCurrent();
	void UpdateEnabledParam();

protected:
	MaxEditAndSpinner m_areaWidthControl;
};

FIRERENDER_NAMESPACE_END
