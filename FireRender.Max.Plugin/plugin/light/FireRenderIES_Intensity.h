#pragma once

#include "FireRenderIES_Panel.h"
#include "Resource.h"

FIRERENDER_NAMESPACE_BEGIN

/* This class manages IES lights intensity options */
class IES_Intensity :
	public IES_Panel<IES_Intensity>
{
public:
	// IES_Panel static interface implementation
	static constexpr int DialogId = IDD_FIRERENDER_IES_LIGHT_INTENSITY;
	static constexpr TCHAR* PanelName = _T("Intensity");

	// Use base class constructor
	using BasePanel::BasePanel;

	void UpdateIntensityParam();

	// IES_Panel overrides
	bool InitializePage();
	void UninitializePage();
	INT_PTR OnEditChange(int editId, HWND editHWND);
	INT_PTR OnSpinnerChange(ISpinnerControl* spinner, WORD controlId, bool isDragging);

private:
	MaxEditAndSpinner m_intensityControl;
};

FIRERENDER_NAMESPACE_END
