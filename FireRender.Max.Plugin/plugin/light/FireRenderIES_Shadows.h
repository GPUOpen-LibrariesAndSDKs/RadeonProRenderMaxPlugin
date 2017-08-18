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
};

FIRERENDER_NAMESPACE_END
