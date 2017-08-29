#include "FireRenderIESLight.h"
#include "FireRenderIES_Shadows.h"

FIRERENDER_NAMESPACE_BEGIN

void IES_Shadows::UpdateEnabledParam()
{
	m_parent->SetShadowsEnabled(m_enabledControl.IsChecked());
}

void IES_Shadows::UpdateSoftnessParam()
{
	m_parent->SetShadowsSoftness(m_softnessControl.GetEdit().GetValue<float>());
}

void IES_Shadows::UpdateTransparencyParam()
{
	m_parent->SetShadowsTransparency(m_transparencyControl.GetEdit().GetValue<float>());
}

bool IES_Shadows::InitializePage()
{
	// Shadows enabled control
	m_enabledControl.Capture(m_panel, IDC_FIRERENDER_IES_LIGHT_SHADOWS_ENABLED);
	m_enabledControl.SetCheck(m_parent->GetShadowsEnabled());

	// Shadows softness control
	m_softnessControl.Capture(m_panel,
		IDC_FIRERENDER_IES_LIGHT_SHADOWS_SOFTNESS,
		IDC_FIRERENDER_IES_LIGHT_SHADOWS_SOFTNESS_S);

	m_softnessControl.Bind(EditSpinnerType::EDITTYPE_FLOAT);
	m_softnessControl.GetSpinner().SetSettings<FireRenderIESLight::ShadowsSoftnessSettings>();
	m_softnessControl.GetSpinner().SetValue(m_parent->GetShadowsSoftness());

	// Shadows transparency control
	m_transparencyControl.Capture(m_panel,
		IDC_FIRERENDER_IES_LIGHT_SHADOWS_TRANSPARENCY,
		IDC_FIRERENDER_IES_LIGHT_SHADOWS_TRANSPARENCY_S);

	m_transparencyControl.Bind(EditSpinnerType::EDITTYPE_FLOAT);
	m_transparencyControl.GetSpinner().SetSettings<FireRenderIESLight::ShadowsTransparencySettings>();
	m_transparencyControl.GetSpinner().SetValue(m_parent->GetShadowsTransparency());

	return TRUE;
}

void IES_Shadows::UninitializePage()
{
	m_enabledControl.Release();
	m_softnessControl.Release();
	m_transparencyControl.Release();
}

INT_PTR IES_Shadows::HandleControlCommand(WORD code, WORD controlId)
{
	if (code == BN_CLICKED)
	{
		switch (controlId)
		{
		case IDC_FIRERENDER_IES_LIGHT_SHADOWS_ENABLED:
			UpdateEnabledParam();
			return TRUE;
		}
	}

	return FALSE;
}

INT_PTR IES_Shadows::OnEditChange(int controlId, HWND editHWND)
{
	switch (controlId)
	{
	case IDC_FIRERENDER_IES_LIGHT_SHADOWS_SOFTNESS:
		UpdateSoftnessParam();
		return TRUE;

	case IDC_FIRERENDER_IES_LIGHT_SHADOWS_TRANSPARENCY:
		UpdateTransparencyParam();
		return TRUE;
	}

	return FALSE;
}

INT_PTR IES_Shadows::OnSpinnerChange(ISpinnerControl* spinner, WORD controlId, bool isDragging)
{
	switch (controlId)
	{
	case IDC_FIRERENDER_IES_LIGHT_SHADOWS_SOFTNESS_S:
		UpdateSoftnessParam();
		return TRUE;

	case IDC_FIRERENDER_IES_LIGHT_SHADOWS_TRANSPARENCY_S:
		UpdateTransparencyParam();
		return TRUE;
	}

	return FALSE;
}

void IES_Shadows::Enable()
{
	using namespace ies_panel_utils;

	EnableControls<true>(
		m_enabledControl,
		m_softnessControl,
		m_transparencyControl);
}

void IES_Shadows::Disable()
{
	using namespace ies_panel_utils;

	EnableControls<false>(
		m_enabledControl,
		m_softnessControl,
		m_transparencyControl);
}

FIRERENDER_NAMESPACE_END
