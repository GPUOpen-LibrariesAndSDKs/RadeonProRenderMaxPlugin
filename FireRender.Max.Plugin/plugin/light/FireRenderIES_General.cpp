#include "FireRenderIESLight.h"

#include "FireRenderIES_General.h"

FIRERENDER_NAMESPACE_BEGIN

bool IES_General::InitializePage()
{
	// Enabled parameter
	m_enabledControl.Capture(m_panel, IDC_FIRERENDER_IES_LIGHT_ENABLED);
	m_enabledControl.SetCheck(m_parent->GetEnabled());

	// Targeted parameter
	m_targetedControl.Capture(m_panel, IDC_FIRERENDER_IES_LIGHT_TARGETED_CHECKBOX);
	m_targetedControl.SetCheck(m_parent->GetTargeted());

	// Area width parameter
	m_areaWidthControl.Capture(m_panel,
		IDC_FIRERENDER_IES_LIGHT_AREA_WIDTH,
		IDC_FIRERENDER_IES_LIGHT_AREA_WIDTH_S);

	m_areaWidthControl.Bind(EDITTYPE_FLOAT);
	
	auto& spinner = m_areaWidthControl.GetSpinner();
	spinner.SetLimits(0.f, FLT_MAX);
	spinner.SetValue(m_parent->GetAreaWidth());
	spinner.SetResetValue(1.0f);
	spinner.SetScale(0.001f);

	return true;
}

void IES_General::UninitializePage()
{
	m_enabledControl.Release();
	m_targetedControl.Release();
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

		case IDC_FIRERENDER_IES_LIGHT_TARGETED_CHECKBOX:
			UpdateTargetedParam();
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
	m_parent->SetEnabled(m_enabledControl.IsChecked());
}

void IES_General::UpdateTargetedParam()
{
	m_parent->SetTargeted(m_enabledControl.IsChecked());
}

void IES_General::UpdateAreaWidthParam()
{
	m_parent->SetAreaWidth(m_areaWidthControl.GetEdit().GetValue<float>());
}

FIRERENDER_NAMESPACE_END