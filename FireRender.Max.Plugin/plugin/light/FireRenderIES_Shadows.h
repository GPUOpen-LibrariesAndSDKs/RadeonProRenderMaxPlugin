#pragma once

#include "FireRenderIES_Panel.h"
#include "Resource.h"

FIRERENDER_NAMESPACE_BEGIN

/* This class manages IES lights shadows options */
class IES_Shadows :
	public IES_Panel<IES_Shadows>
{
public:
	// IES_Panel static interface implementation
	static constexpr int DialogId = IDD_FIRERENDER_IES_LIGHT_SHADOWS;
	static constexpr TCHAR* PanelName = _T("Shadows");

	// Use base class constructor
	using BasePanel::BasePanel;

	void UpdateEnabledParam();
	void UpdateSoftnessParam();
	void UpdateTransparencyParam();

	// IES_Panel overrides
	bool InitializePage();
	void UninitializePage();
	INT_PTR HandleControlCommand(WORD code, WORD controlId);
	INT_PTR OnEditChange(int editId, HWND editHWND);
	INT_PTR OnSpinnerChange(ISpinnerControl* spinner, WORD controlId, bool isDragging);

private:
	WinCheckbox m_enabledControl;
	MaxEditAndSpinner m_softnessControl;
	MaxEditAndSpinner m_transparencyControl;
};

FIRERENDER_NAMESPACE_END
