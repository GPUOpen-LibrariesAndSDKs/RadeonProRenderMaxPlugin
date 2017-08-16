#pragma once

#include "FireRenderIES_Panel.h"
#include "Resource.h"
#include "IESLightParameter.h"

FIRERENDER_NAMESPACE_BEGIN

class IES_General :
	public IES_Panel<IES_General>
{
public:

	static constexpr int DialogId = IDD_FIRERENDER_IES_LIGHT_GENERAL;
	static constexpr TCHAR* PanelName = _T("General");
	using BasePanel::BasePanel;

	bool InitDialog();
	INT_PTR HandleControlCommand(WORD code, WORD controlId);

protected:
};

FIRERENDER_NAMESPACE_END
