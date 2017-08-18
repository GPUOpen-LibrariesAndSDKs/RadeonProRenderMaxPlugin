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
};

FIRERENDER_NAMESPACE_END
