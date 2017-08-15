#pragma once

#include "FireRenderIES_Panel.h"
#include "Resource.h"

FIRERENDER_NAMESPACE_BEGIN

class IES_Shadows :
	public IES_Panel<IES_Shadows>
{
public:
	static constexpr int DialogId = IDD_FIRERENDER_IES_LIGHT_SHADOWS;
	static constexpr TCHAR* PanelName = _T("Shadows");
	using BasePanel::BasePanel;

	bool InitDialog()
	{
		return true;
	}
};

FIRERENDER_NAMESPACE_END
