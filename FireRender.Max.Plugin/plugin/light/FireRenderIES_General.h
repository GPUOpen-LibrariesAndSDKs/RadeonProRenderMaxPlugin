#pragma once

#include "FireRenderIES_Panel.h"
#include "Resource.h"
#include "IESLightParameter.h"

FIRERENDER_NAMESPACE_BEGIN

/* This class manages IES lights general options */
class IES_General :
	public IES_Panel<IES_General>
{
public:
	// IES_Panel static interface implementation
	static constexpr int DialogId = IDD_FIRERENDER_IES_LIGHT_GENERAL;
	static constexpr auto PanelName = _T("General");

	// Use base class constructor
	using BasePanel::BasePanel;

	void UpdateEnabledParam();
	void UpdateTargetedParam();
	void UpdateAreaWidthParam();
	void ImportProfile();
	void ActivateSelectedProfile();
	void DeleteSelectedProfile();

	// IES_Panel overrides
	bool InitializePage();
	void UninitializePage();
	INT_PTR HandleControlCommand(WORD code, WORD controlId);
	INT_PTR OnEditChange(int editId, HWND editHWND);
	INT_PTR OnSpinnerChange(ISpinnerControl* spinner, WORD controlId, bool isDragging);

protected:
	void UpdateProfiles();
	void UpdateDeleteProfileButtonState();

	MaxButton m_importButton;
	MaxButton m_deleteCurrentButton;
	WinCombobox m_profilesComboBox;
	WinButton m_enabledControl;
	WinButton m_targetedControl;
	MaxEditAndSpinner m_areaWidthControl;
};

FIRERENDER_NAMESPACE_END
