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

	// These methods move values from ui to the light object
	bool UpdateEnabledParam(TimeValue t);
	bool UpdateTargetedParam(TimeValue t);
	bool UpdateTargetDistanceParam(TimeValue t);
	bool UpdateAreaWidthParam(TimeValue t);
	void ImportProfile();
	bool ActivateSelectedProfile(TimeValue t);
	bool DeleteSelectedProfile(TimeValue t);

	// IES_Panel overrides
	bool InitializePage(TimeValue time);
	void UninitializePage();
	bool HandleControlCommand(TimeValue t, WORD code, WORD controlId);
	bool OnEditChange(TimeValue t, int editId, HWND editHWND);
	bool OnSpinnerChange(TimeValue t, ISpinnerControl* spinner, WORD controlId, bool isDragging);
	const TCHAR* GetAcceptMessage(WORD controlId) const;
	void UpdateUI(TimeValue t);
	void Enable();
	void Disable();

protected:
	void UpdateProfiles(TimeValue time);
	void UpdateDeleteProfileButtonState();

	MaxButton m_importButton;
	MaxButton m_deleteCurrentButton;
	WinCombobox m_profilesComboBox;
	WinButton m_enabledControl;
	WinButton m_targetedControl;
	MaxEditAndSpinner m_areaWidthControl;
	MaxEditAndSpinner m_targetDistanceControl;
};

FIRERENDER_NAMESPACE_END
