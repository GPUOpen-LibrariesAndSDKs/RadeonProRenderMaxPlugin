#pragma once

#include "FireRenderIES_Panel.h"
#include "Resource.h"

FIRERENDER_NAMESPACE_BEGIN

class IES_Volume :
	public IES_Panel<IES_Volume>
{
public:
	static constexpr int DialogId = IDD_FIRERENDER_IES_LIGHT_VOLUME;
	static constexpr TCHAR* PanelName = _T("Volume");
	using BasePanel::BasePanel;

	bool InitDialog()
	{
		return true;
	}
};

FIRERENDER_NAMESPACE_END
