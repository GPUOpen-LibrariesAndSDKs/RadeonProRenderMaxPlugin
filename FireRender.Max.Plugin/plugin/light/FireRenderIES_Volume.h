#pragma once

#include "FireRenderIES_Panel.h"
#include "Resource.h"

FIRERENDER_NAMESPACE_BEGIN

/* This class manages IES lights volume options */
class IES_Volume :
	public IES_Panel<IES_Volume>
{
public:
	// IES_Panel static interface implementation
	static constexpr int DialogId = IDD_FIRERENDER_IES_LIGHT_VOLUME;
	static constexpr TCHAR* PanelName = _T("Volume");

	// Use base class constructor
	using BasePanel::BasePanel;

	void UpdateVolumeScaleParam();

	// IES_Panel overrides
	bool InitializePage();
	void UninitializePage();
	INT_PTR OnEditChange(int editId, HWND editHWND);
	INT_PTR OnSpinnerChange(ISpinnerControl* spinner, WORD controlId, bool isDragging);

private:
	MaxEditAndSpinner m_volumeScaleControl;
};

FIRERENDER_NAMESPACE_END
