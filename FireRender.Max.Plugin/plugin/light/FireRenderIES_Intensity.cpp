#include "FireRenderIESLight.h"

#include "FireRenderIES_Intensity.h"

FIRERENDER_NAMESPACE_BEGIN

namespace
{
	const TCHAR* ToString(IESLightColorMode mode)
	{
		switch (mode)
		{
			case IES_LIGHT_COLOR_MODE_COLOR:		return _T("Color");
			case IES_LIGHT_COLOR_MODE_TEMPERATURE:	return _T("Temperature");
		}

		FASSERT(!"Not implemented");
		return nullptr;
	}

	IESLightColorMode ParseColorMode(const TCHAR* text)
	{
		if (_tcscmp(text, ToString(IES_LIGHT_COLOR_MODE_COLOR)) == 0)
			return IES_LIGHT_COLOR_MODE_COLOR;

		if (_tcscmp(text, ToString(IES_LIGHT_COLOR_MODE_TEMPERATURE)) == 0)
			return IES_LIGHT_COLOR_MODE_TEMPERATURE;

		FASSERT(!"Not implemented");
		return IES_LIGHT_COLOR_MODE_COLOR;
	}

	Color TemperatureToColor(float kelvin)
	{
		return Color();
	}
}

void IES_Intensity::UpdateIntensityParam()
{
	m_parent->SetIntensity(m_intensityControl.GetEdit().GetValue<float>());
}

void IES_Intensity::UpdateColorModeParam()
{
	auto index = m_colorModeControl.GetSelectedIndex();
	auto text = m_colorModeControl.GetItemText(index);
	auto mode = ParseColorMode(text.c_str());

	m_parent->SetColorMode(mode);
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

	// Color mode control
	{
		m_colorModeControl.Capture(m_panel, IDC_FIRERENDER_IES_LIGHT_COLOR_MODE);
		auto colorIdx = m_colorModeControl.AddItem(ToString(IES_LIGHT_COLOR_MODE_COLOR));
		auto tempIdx = m_colorModeControl.AddItem(ToString(IES_LIGHT_COLOR_MODE_TEMPERATURE));

		m_colorModeControl.SetSelected(
			m_parent->GetColorMode() == IES_LIGHT_COLOR_MODE_COLOR ?
			colorIdx : tempIdx);
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

	return TRUE;
}

void IES_Intensity::UninitializePage()
{
	m_intensityControl.Release();
	m_colorModeControl.Release();
	m_colorControl.Release();
	m_temperatureControl.Release();
}

INT_PTR IES_Intensity::HandleControlCommand(WORD code, WORD controlId)
{
	if (code == CBN_SELCHANGE && controlId == IDC_FIRERENDER_IES_LIGHT_COLOR_MODE)
	{
		UpdateColorModeParam();
		return TRUE;
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

FIRERENDER_NAMESPACE_END
