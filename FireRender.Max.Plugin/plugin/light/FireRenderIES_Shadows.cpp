#include "FireRenderIESLight.h"
#include "FireRenderIES_Shadows.h"

FIRERENDER_NAMESPACE_BEGIN

bool IES_Shadows::UpdateEnabledParam(TimeValue t)
{
	return m_parent->SetShadowsEnabled(m_enabledControl.IsChecked(), t);
}

bool IES_Shadows::UpdateSoftnessParam(TimeValue t)
{
	return m_parent->SetShadowsSoftness(m_softnessControl.GetValue<float>(), t);
}

bool IES_Shadows::UpdateTransparencyParam(TimeValue t)
{
	return m_parent->SetShadowsTransparency(m_transparencyControl.GetValue<float>(), t);
}

bool IES_Shadows::InitializePage(TimeValue t)
{
	// Shadows enabled control
	m_enabledControl.Capture(m_panel, IDC_FIRERENDER_IES_LIGHT_SHADOWS_ENABLED);
	m_enabledControl.SetCheck(m_parent->GetShadowsEnabled(t));

	// Shadows softness control
	m_softnessControl.Capture(m_panel,
		IDC_FIRERENDER_IES_LIGHT_SHADOWS_SOFTNESS,
		IDC_FIRERENDER_IES_LIGHT_SHADOWS_SOFTNESS_S);

	m_softnessControl.Bind(EditSpinnerType::EDITTYPE_FLOAT);
	m_softnessControl.GetSpinner().SetSettings<FireRenderIESLight::ShadowsSoftnessSettings>();
	m_softnessControl.GetSpinner().SetValue(m_parent->GetShadowsSoftness(t));

	// Shadows transparency control
	m_transparencyControl.Capture(m_panel,
		IDC_FIRERENDER_IES_LIGHT_SHADOWS_TRANSPARENCY,
		IDC_FIRERENDER_IES_LIGHT_SHADOWS_TRANSPARENCY_S);

	m_transparencyControl.Bind(EditSpinnerType::EDITTYPE_FLOAT);
	m_transparencyControl.GetSpinner().SetSettings<FireRenderIESLight::ShadowsTransparencySettings>();
	m_transparencyControl.GetSpinner().SetValue(m_parent->GetShadowsTransparency(t));

	return TRUE;
}

void IES_Shadows::UninitializePage()
{
	m_enabledControl.Release();
	m_softnessControl.Release();
	m_transparencyControl.Release();
}

bool IES_Shadows::HandleControlCommand(TimeValue t, WORD code, WORD controlId)
{
	if (code == BN_CLICKED)
	{
		switch (controlId)
		{
		case IDC_FIRERENDER_IES_LIGHT_SHADOWS_ENABLED:
			return UpdateEnabledParam(t);
		}
	}

	return false;
}

bool IES_Shadows::OnEditChange(TimeValue t, int controlId, HWND editHWND)
{
	switch (controlId)
	{
	case IDC_FIRERENDER_IES_LIGHT_SHADOWS_SOFTNESS:
		return UpdateSoftnessParam(t);

	case IDC_FIRERENDER_IES_LIGHT_SHADOWS_TRANSPARENCY:
		return UpdateTransparencyParam(t);
	}

	return false;
}

bool IES_Shadows::OnSpinnerChange(TimeValue t, ISpinnerControl* spinner, WORD controlId, bool isDragging)
{
	switch (controlId)
	{
	case IDC_FIRERENDER_IES_LIGHT_SHADOWS_SOFTNESS_S:
		return UpdateSoftnessParam(t);

	case IDC_FIRERENDER_IES_LIGHT_SHADOWS_TRANSPARENCY_S:
		return UpdateTransparencyParam(t);
	}

	return false;
}

const TCHAR* IES_Shadows::GetAcceptMessage(WORD controlId) const
{
	switch (controlId)
	{
	case IDC_FIRERENDER_IES_LIGHT_SHADOWS_ENABLED:
		return _T("IES light: change shadows enabled parameter");

	case IDC_FIRERENDER_IES_LIGHT_SHADOWS_SOFTNESS:
	case IDC_FIRERENDER_IES_LIGHT_SHADOWS_SOFTNESS_S:
		return _T("IES light: change shadows softness");

	case IDC_FIRERENDER_IES_LIGHT_SHADOWS_TRANSPARENCY:
	case IDC_FIRERENDER_IES_LIGHT_SHADOWS_TRANSPARENCY_S:
		return _T("IES light: change shadows transparency");
	}

	FASSERT(false);
	return IES_Panel::GetAcceptMessage(controlId);
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
