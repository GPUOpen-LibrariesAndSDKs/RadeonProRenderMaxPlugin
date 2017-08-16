#include "FireRenderIESLight.h"

#include "FireRenderIES_General.h"

FIRERENDER_NAMESPACE_BEGIN

namespace
{
	bool IsButtonChecked(HWND wnd, int buttonId)
	{
		auto buttonHandle = GetDlgItem(wnd, buttonId);
		FASSERT(buttonHandle != nullptr);

		auto result = Button_GetCheck(buttonHandle);
		FASSERT(result != BST_INDETERMINATE);

		return result == BST_CHECKED;
	}
}

bool IES_General::InitDialog()
{
	auto pBlock = m_parent->GetParamBlock(0);

	BOOL enabled = FALSE;
	auto ok = pBlock->GetValue(IES_PARAM_ENABLED, 0, enabled, FOREVER);
	FASSERT(ok);

	ok = CheckDlgButton(
		m_panel,
		IDC_FIRERENDER_IES_LIGHT_ENABLED,
		enabled == FALSE ? BST_UNCHECKED : BST_CHECKED);
	FASSERT(ok);

	return true;
}

INT_PTR IES_General::HandleControlCommand(WORD code, WORD controlId)
{
	if (code == BN_CLICKED && controlId == IDC_FIRERENDER_IES_LIGHT_ENABLED)
	{
		auto pBlock = m_parent->GetParamBlock(0);
		FASSERT(pBlock != nullptr);

		bool enable = IsButtonChecked(m_panel, controlId);
		pBlock->SetValue(IES_PARAM_ENABLED, 0, enable);

		return TRUE;
	}

	return FALSE;
}

FIRERENDER_NAMESPACE_END