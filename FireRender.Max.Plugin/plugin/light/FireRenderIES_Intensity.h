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
};

FIRERENDER_NAMESPACE_END
