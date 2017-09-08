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

	bool UpdateVolumeScaleParam(TimeValue t);

	// IES_Panel overrides
	bool InitializePage(TimeValue t);
	void UninitializePage();
	bool OnEditChange(TimeValue t, int editId, HWND editHWND);
	bool OnSpinnerChange(TimeValue t, ISpinnerControl* spinner, WORD controlId, bool isDragging);
	const TCHAR* GetAcceptMessage(WORD controlId) const;
	void Enable();
	void Disable();

private:
	MaxEditAndSpinner m_volumeScaleControl;
};

FIRERENDER_NAMESPACE_END
