#include "FireRenderIESLight.h"

#include "FireRenderIES_Intensity.h"

FIRERENDER_NAMESPACE_BEGIN

void IES_Intensity::UpdateIntensityParam()
{
	m_parent->SetIntensity(m_intensityControl.GetEdit().GetValue<float>());
}

void IES_Intensity::UpdateColorModeParam()
{
	m_parent->SetColorMode(
		m_rgbModeControl.IsChecked() ?
		IES_LIGHT_COLOR_MODE_COLOR :
		IES_LIGHT_COLOR_MODE_TEMPERATURE);
}

void IES_Intensity::UpdateColorParam()
{
	m_parent->SetColor(m_colorControl.GetColor());
}

void IES_Intensity::UpdateTemperatureParam()
{
	m_parent->SetTemperature(m_temperatureControl.GetTemperature());
}

bool IES_Intensity::InitializePage()
{
	// Intensity parameter
	{
		m_intensityControl.Capture(m_panel,
			IDC_FIRERENDER_IES_LIGHT_INTENSITY,
			IDC_FIRERENDER_IES_LIGHT_INTENSITY_S);
		m_intensityControl.Bind(EDITTYPE_FLOAT);

		auto& spinner = m_intensityControl.GetSpinner();
		spinner.SetSettings<FireRenderIESLight::IntensitySettings>();
		spinner.SetValue(m_parent->GetIntensity());
	}

	// Color modes controls
	{
		m_rgbModeControl.Capture(m_panel, IDC_FIRERENDER_IES_LIGHT_RGB_MODE);
		m_kelvinModeControl.Capture(m_panel, IDC_FIRERENDER_IES_LIGHT_KELVIN_MODE);

		m_rgbModeControl.SetCheck(m_parent->GetColorMode() == IES_LIGHT_COLOR_MODE_COLOR);
		m_kelvinModeControl.SetCheck(!m_rgbModeControl.IsChecked());
	}

	// Color parameter
	{
		m_colorControl.Capture(m_panel, IDC_FIRERENDER_IES_LIGHT_COLOR);
		m_colorControl.SetColor(m_parent->GetColor());
	}

	// Temperature parameter
	{
		m_temperatureControl.Capture(m_panel,
			IDC_FIRERENDER_IES_LIGHT_TEMPERATURE_C,
			IDC_FIRERENDER_IES_LIGHT_TEMPERATURE,
			IDC_FIRERENDER_IES_LIGHT_TEMPERATURE_S);

		m_temperatureControl.SetTemperature(m_parent->GetTemperature());
	}

	UpdateControlsEnabled();

	return TRUE;
}

void IES_Intensity::UninitializePage()
{
	m_intensityControl.Release();
	m_rgbModeControl.Release();
	m_kelvinModeControl.Release();
	m_colorControl.Release();
	m_temperatureControl.Release();
}

INT_PTR IES_Intensity::HandleControlCommand(WORD code, WORD controlId)
{
	if (code == BN_CLICKED)
	{
		switch (controlId)
		{
		case IDC_FIRERENDER_IES_LIGHT_RGB_MODE:
		case IDC_FIRERENDER_IES_LIGHT_KELVIN_MODE:
			UpdateColorModeParam();
			UpdateControlsEnabled();
			return TRUE;
		}
	}

	return FALSE;
}

INT_PTR IES_Intensity::OnEditChange(int editId, HWND editHWND)
{
	switch (editId)
	{
	case IDC_FIRERENDER_IES_LIGHT_INTENSITY:
		UpdateIntensityParam();
		break;

	case IDC_FIRERENDER_IES_LIGHT_TEMPERATURE:
		m_temperatureControl.UpdateColor();
		UpdateTemperatureParam();
		break;
	}

	return FALSE;
}

INT_PTR IES_Intensity::OnSpinnerChange(ISpinnerControl* spinner, WORD controlId, bool isDragging)
{
	switch (controlId)
	{
	case IDC_FIRERENDER_IES_LIGHT_INTENSITY_S:
		UpdateIntensityParam();
		break;

	case IDC_FIRERENDER_IES_LIGHT_TEMPERATURE_S:
		m_temperatureControl.UpdateColor();
		UpdateTemperatureParam();
		break;
	}

	return FALSE;
}

INT_PTR IES_Intensity::OnColorSwatchChange(IColorSwatch* colorSwatch, WORD controlId, bool final)
{
	switch (controlId)
	{
	case IDC_FIRERENDER_IES_LIGHT_COLOR:
		if (final)
		{
			UpdateColorParam();
		}
		break;

	case IDC_FIRERENDER_IES_LIGHT_TEMPERATURE_C:
		m_temperatureControl.UpdateColor();
		break;
	}

	return FALSE;
}

void IES_Intensity::Enable()
{
	using namespace ies_panel_utils;

	EnableControls<true>(
		m_intensityControl,
		m_rgbModeControl,
		m_kelvinModeControl,
		m_colorControl,
		m_temperatureControl);
}

void IES_Intensity::Disable()
{
	using namespace ies_panel_utils;

	EnableControls<false>(
		m_intensityControl,
		m_rgbModeControl,
		m_kelvinModeControl,
		m_colorControl,
		m_temperatureControl);
}

void IES_Intensity::UpdateControlsEnabled()
{
	if (m_parent->GetColorMode() == IES_LIGHT_COLOR_MODE_COLOR)
	{
		m_temperatureControl.Disable();
		m_colorControl.Enable();
	}
	else
	{
		m_temperatureControl.Enable();
		m_colorControl.Disable();
	}
}

FIRERENDER_NAMESPACE_END
