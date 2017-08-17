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
	// Enabled parameter
	m_enabledControl.Capture(m_panel, IDC_FIRERENDER_IES_LIGHT_ENABLED);
	m_enabledControl.SetCheck(IsEnabled());

	// Area width parameter
	m_areaWidthControl.Capture(m_panel,
		IDC_FIRERENDER_IES_LIGHT_AREA_WIDTH,
		IDC_FIRERENDER_IES_LIGHT_AREA_WIDTH_S);

	m_areaWidthControl.Bind(EDITTYPE_FLOAT);
	
	auto& spinner = m_areaWidthControl.GetSpinner();
	spinner.SetLimits(0.f, FLT_MAX);
	spinner.SetValue(GetAreaWidth());
	spinner.SetResetValue(1.0f);
	spinner.SetScale(0.001f);

	return true;
}

void IES_General::UninitializePage()
{
	m_enabledControl.Release();
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

INT_PTR IES_General::OnEditChange(int editId, HWND editHWND)
{
	switch (editId)
	{
	case IDC_FIRERENDER_IES_LIGHT_AREA_WIDTH:
		UpdateAreaWidthParam();
		return TRUE;
	}

	return FALSE;
}

INT_PTR IES_General::OnSpinnerChange(ISpinnerControl* spinner, WORD controlId, bool isDragging)
{
	switch (controlId)
	{
	case IDC_FIRERENDER_IES_LIGHT_AREA_WIDTH_S:
		UpdateAreaWidthParam();
		return TRUE;
	}

	return FALSE;
}

void IES_General::SaveCurrent()
{
	FASSERT(!"Not implemented");
}

void IES_General::UpdateEnabledParam()
{
	GetParamBlock()->SetValue(IES_PARAM_ENABLED, 0, m_enabledControl.IsChecked());
}

void IES_General::UpdateAreaWidthParam()
{
	auto pBlock = GetParamBlock();
	auto value = m_areaWidthControl.GetEdit().GetValue<float>();
	pBlock->SetValue(IES_PARAM_AREA_WIDTH, 0, value);
}

bool IES_General::IsEnabled() const
{
	BOOL enabled = FALSE;
	auto ok = GetParamBlock()->GetValue(IES_PARAM_ENABLED, 0, enabled, FOREVER);
	FASSERT(ok);

	return enabled;
}

float IES_General::GetAreaWidth() const
{
	float areaWidth = 1.0f;
	auto ok = GetParamBlock()->GetValue(IES_PARAM_AREA_WIDTH, 0, areaWidth, FOREVER);
	FASSERT(ok);

	return areaWidth;
}

FIRERENDER_NAMESPACE_END