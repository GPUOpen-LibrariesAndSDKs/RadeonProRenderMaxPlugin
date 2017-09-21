#include "FireRenderIESLight.h"

#include "FireRenderIES_Intensity.h"

FIRERENDER_NAMESPACE_BEGIN

const int IES_Intensity::DialogId = IDD_FIRERENDER_IES_LIGHT_INTENSITY;
const TCHAR* IES_Intensity::PanelName = _T("Intensity");

bool IES_Intensity::UpdateIntensityParam(TimeValue t)
{
	return m_parent->SetIntensity(m_intensityControl.GetValue<float>(), t);
}

bool IES_Intensity::UpdateColorModeParam(TimeValue t)
{
	return m_parent->SetColorMode(
		m_rgbModeControl.IsChecked() ?
		IES_LIGHT_COLOR_MODE_COLOR :
		IES_LIGHT_COLOR_MODE_TEMPERATURE, t);
}

bool IES_Intensity::UpdateColorParam(TimeValue t)
{
	return m_parent->SetColor(m_colorControl.GetColor(), t);
}

bool IES_Intensity::UpdateTemperatureParam(TimeValue t)
{
	return m_parent->SetTemperature(m_temperatureControl.GetTemperature(), t);
}

bool IES_Intensity::InitializePage(TimeValue time)
{
	// Intensity parameter
	m_intensityControl.Capture(m_panel,
		IDC_FIRERENDER_IES_LIGHT_INTENSITY,
		IDC_FIRERENDER_IES_LIGHT_INTENSITY_S);
	m_intensityControl.Bind(EDITTYPE_FLOAT);

	MaxSpinner& intensitySpinner = m_intensityControl.GetSpinner();
	intensitySpinner.SetSettings<FireRenderIESLight::IntensitySettings>();
	intensitySpinner.SetValue(m_parent->GetIntensity(time));

	// Color modes controls
	m_rgbModeControl.Capture(m_panel, IDC_FIRERENDER_IES_LIGHT_RGB_MODE);
	m_kelvinModeControl.Capture(m_panel, IDC_FIRERENDER_IES_LIGHT_KELVIN_MODE);

	m_rgbModeControl.SetCheck(m_parent->GetColorMode(time) == IES_LIGHT_COLOR_MODE_COLOR);
	m_kelvinModeControl.SetCheck(!m_rgbModeControl.IsChecked());

	// Color parameter
	m_colorControl.Capture(m_panel, IDC_FIRERENDER_IES_LIGHT_COLOR);
	m_colorControl.SetColor(m_parent->GetColor(time));

	// Temperature parameter
	m_temperatureControl.Capture(m_panel,
		IDC_FIRERENDER_IES_LIGHT_TEMPERATURE_C,
		IDC_FIRERENDER_IES_LIGHT_TEMPERATURE,
		IDC_FIRERENDER_IES_LIGHT_TEMPERATURE_S);

	m_temperatureControl.SetTemperature(m_parent->GetTemperature(time));

	UpdateControlsEnabled(time);

	return true;
}

void IES_Intensity::UninitializePage()
{
	m_intensityControl.Release();
	m_rgbModeControl.Release();
	m_kelvinModeControl.Release();
	m_colorControl.Release();
	m_temperatureControl.Release();
}

bool IES_Intensity::HandleControlCommand(TimeValue t, WORD code, WORD controlId)
{
	if (code == BN_CLICKED)
	{
		switch (controlId)
		{
			case IDC_FIRERENDER_IES_LIGHT_RGB_MODE:
			case IDC_FIRERENDER_IES_LIGHT_KELVIN_MODE:
			{
				bool ret = UpdateColorModeParam(t);
				UpdateControlsEnabled(t);
				return ret;
			}
		}
	}

	return false;
}

bool IES_Intensity::OnEditChange(TimeValue t, int editId, HWND editHWND)
{
	switch (editId)
	{
	case IDC_FIRERENDER_IES_LIGHT_INTENSITY:
		return UpdateIntensityParam(t);

	case IDC_FIRERENDER_IES_LIGHT_TEMPERATURE:
		m_temperatureControl.UpdateColor();
		return UpdateTemperatureParam(t);
	}

	return false;
}

bool IES_Intensity::OnSpinnerChange(TimeValue t, ISpinnerControl* spinner, WORD controlId, bool isDragging)
{
	switch (controlId)
	{
	case IDC_FIRERENDER_IES_LIGHT_INTENSITY_S:
		return UpdateIntensityParam(t);

	case IDC_FIRERENDER_IES_LIGHT_TEMPERATURE_S:
		m_temperatureControl.UpdateColor();
		return UpdateTemperatureParam(t);
	}

	return false;
}

const TCHAR* IES_Intensity::GetAcceptMessage(WORD controlId) const
{
	switch (controlId)
	{
	case IDC_FIRERENDER_IES_LIGHT_INTENSITY:
	case IDC_FIRERENDER_IES_LIGHT_INTENSITY_S:
		return _T("IES light: change intensity");

	case IDC_FIRERENDER_IES_LIGHT_RGB_MODE:
	case IDC_FIRERENDER_IES_LIGHT_KELVIN_MODE:
		return _T("IES light: change color mode");

	case IDC_FIRERENDER_IES_LIGHT_COLOR:
		return _T("IES light: change color");

	case IDC_FIRERENDER_IES_LIGHT_TEMPERATURE:
	case IDC_FIRERENDER_IES_LIGHT_TEMPERATURE_C:
	case IDC_FIRERENDER_IES_LIGHT_TEMPERATURE_S:
		return _T("IES light: change temperature");
	}

	FASSERT(false);
	return IES_Panel::GetAcceptMessage(controlId);
}

bool IES_Intensity::OnColorSwatchChange(TimeValue t, IColorSwatch* colorSwatch, WORD controlId, bool final)
{
	switch (controlId)
	{
	case IDC_FIRERENDER_IES_LIGHT_COLOR:
		if (final)
		{
			return UpdateColorParam(t);
		}
		break;

	case IDC_FIRERENDER_IES_LIGHT_TEMPERATURE_C:
		m_temperatureControl.UpdateColor();
		break;
	}

	return false;
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

void IES_Intensity::UpdateControlsEnabled(TimeValue time)
{
	if (m_parent->GetColorMode(time) == IES_LIGHT_COLOR_MODE_COLOR)
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
