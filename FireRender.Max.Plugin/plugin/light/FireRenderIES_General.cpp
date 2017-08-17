#include "FireRenderIESLight.h"

#include "FireRenderIES_General.h"

FIRERENDER_NAMESPACE_BEGIN

namespace
{
	bool GetIsButtonChecked(HWND wnd, int buttonId)
	{
		auto buttonHandle = GetDlgItem(wnd, buttonId);
		FASSERT(buttonHandle != nullptr);

		auto result = Button_GetCheck(buttonHandle);
		FASSERT(result != BST_INDETERMINATE);

		return result == BST_CHECKED;
	}

	void SetIsButtonChecked(HWND wnd, int buttonId, bool checked)
	{
		auto ok = CheckDlgButton(wnd, buttonId,
			checked ? BST_CHECKED : BST_UNCHECKED);
		FASSERT(ok);
	}
}

bool IES_General::InitializePage()
{
	auto pBlock = GetParamBlock();

	BOOL enabled = FALSE;
	auto ok = pBlock->GetValue(IES_PARAM_ENABLED, 0, enabled, FOREVER);
	FASSERT(ok);

	SetIsButtonChecked(m_panel, IDC_FIRERENDER_IES_LIGHT_ENABLED, enabled);

	m_areaWidthControl.Capture(m_panel,
		IDC_FIRERENDER_IES_LIGHT_AREA_WIDTH,
		IDC_FIRERENDER_IES_LIGHT_AREA_WIDTH_S);

	m_areaWidthControl.Bind(EDITTYPE_FLOAT);
	
	auto& spinner = m_areaWidthControl.GetSpinner();
	spinner.SetLimits(0.f, FLT_MAX);
	spinner.SetValue(1.0f);
	spinner.SetResetValue(1.0f);
	spinner.SetScale(0.001f);

	return true;
}

void IES_General::UninitializePage()
{
	m_areaWidthControl.Release();
}

INT_PTR IES_General::HandleControlCommand(WORD code, WORD controlId)
{
	if (code == BN_CLICKED)
	{
		switch (controlId)
		{
		case IDC_FIRERENDER_IES_LIGHT_ENABLED:
			UpdateEnabledParam();
			return TRUE;

		case IDC_FIRERENDER_IES_LIGHT_SAVE_CURRENT:
			SaveCurrent();
			return TRUE;
		}
	}

	return FALSE;
}

void IES_General::SaveCurrent()
{
	FASSERT(!"Not implemented");
}

void IES_General::UpdateEnabledParam()
{
	auto pBlock = GetParamBlock();
	bool enable = GetIsButtonChecked(m_panel, IDC_FIRERENDER_IES_LIGHT_ENABLED);

	pBlock->SetValue(IES_PARAM_ENABLED, 0, enable);
}

FIRERENDER_NAMESPACE_END