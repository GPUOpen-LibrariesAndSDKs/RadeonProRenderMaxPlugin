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
	static const int DialogId;
	static const TCHAR* PanelName;

	// Use base class constructor
	using BasePanel::BasePanel;

	bool UpdateEnabledParam(TimeValue t);
	bool UpdateSoftnessParam(TimeValue t);
	bool UpdateTransparencyParam(TimeValue t);

	// IES_Panel overrides
	bool InitializePage(TimeValue t);
	void UninitializePage();
	bool HandleControlCommand(TimeValue t, WORD code, WORD controlId);
	bool OnEditChange(TimeValue t, int editId, HWND editHWND);
	bool OnSpinnerChange(TimeValue t, ISpinnerControl* spinner, WORD controlId, bool isDragging);
	const TCHAR* GetAcceptMessage(WORD controlId) const;
	void Enable();
	void Disable();

private:
	WinButton m_enabledControl;
	MaxEditAndSpinner m_softnessControl;
	MaxEditAndSpinner m_transparencyControl;
};

FIRERENDER_NAMESPACE_END
