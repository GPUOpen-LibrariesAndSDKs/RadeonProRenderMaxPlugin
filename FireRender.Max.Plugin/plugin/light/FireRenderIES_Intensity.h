#pragma once

#include "FireRenderIES_Panel.h"
#include "Resource.h"

FIRERENDER_NAMESPACE_BEGIN

class IES_Intensity :
	public IES_Panel<IES_Intensity>
{
public:
	static constexpr int DialogId = IDD_FIRERENDER_IES_LIGHT_INTENSITY;
	static constexpr TCHAR* PanelName = _T("Intensity");
	using BasePanel::BasePanel;

	bool InitDialog()
	{
		return true;
	}
};

FIRERENDER_NAMESPACE_END
