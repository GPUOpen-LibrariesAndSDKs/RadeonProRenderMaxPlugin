#include "FireRenderIESLight.h"

#include "FireRenderIES_Intensity.h"

FIRERENDER_NAMESPACE_BEGIN

void IES_Intensity::UpdateIntensityParam()
{
	m_parent->SetIntensity(m_intensityControl.GetEdit().GetValue<float>());
}

bool IES_Intensity::InitializePage()
{
	// Intensity parameter
	m_intensityControl.Capture(m_panel,
		IDC_FIRERENDER_IES_LIGHT_INTENSITY,
		IDC_FIRERENDER_IES_LIGHT_INTENSITY_S);

	m_intensityControl.Bind(EDITTYPE_FLOAT);

	auto& spinner = m_intensityControl.GetSpinner();
	spinner.SetSettings<FireRenderIESLight::IntensitySettings>();
	spinner.SetValue(m_parent->GetIntensity());

	return TRUE;
}

void IES_Intensity::UninitializePage()
{
	m_intensityControl.Release();
}

INT_PTR IES_Intensity::OnEditChange(int editId, HWND editHWND)
{
	switch (editId)
	{
	case IDC_FIRERENDER_IES_LIGHT_INTENSITY:
		UpdateIntensityParam();
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
	}

	return FALSE;
}

FIRERENDER_NAMESPACE_END
