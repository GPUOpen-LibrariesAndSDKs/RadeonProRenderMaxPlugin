#include "FireRenderIESLight.h"
#include "FireRenderIES_Volume.h"

FIRERENDER_NAMESPACE_BEGIN

void IES_Volume::UpdateVolumeScaleParam()
{
	m_parent->SetVolumeScale(m_volumeScaleControl.GetEdit().GetValue<float>());
}

bool IES_Volume::InitializePage()
{
	m_volumeScaleControl.Capture(m_panel,
		IDC_FIRERENDER_IES_LIGHT_VOLUME_SCALE,
		IDC_FIRERENDER_IES_LIGHT_VOLUME_SCALE_S);

	m_volumeScaleControl.Bind(EditSpinnerType::EDITTYPE_FLOAT);
	m_volumeScaleControl.GetSpinner().SetSettings<FireRenderIESLight::VolumeScaleSettings>();
	m_volumeScaleControl.GetSpinner().SetValue(m_parent->GetVolumeScale());

	return TRUE;
}

void IES_Volume::UninitializePage()
{
	m_volumeScaleControl.Release();
}

INT_PTR IES_Volume::OnEditChange(int controlId, HWND editHWND)
{
	switch (controlId)
	{
	case IDC_FIRERENDER_IES_LIGHT_VOLUME_SCALE:
		UpdateVolumeScaleParam();
		return TRUE;
	}

	return FALSE;
}

INT_PTR IES_Volume::OnSpinnerChange(ISpinnerControl* spinner, WORD controlId, bool isDragging)
{
	switch (controlId)
	{
	case IDC_FIRERENDER_IES_LIGHT_VOLUME_SCALE_S:
		UpdateVolumeScaleParam();
		return TRUE;
	}

	return FALSE;
}

void IES_Volume::Enable()
{
	using namespace ies_panel_utils;

	EnableControls<true>(
		m_volumeScaleControl);
}

void IES_Volume::Disable()
{
	using namespace ies_panel_utils;

	EnableControls<false>(
		m_volumeScaleControl);
}

FIRERENDER_NAMESPACE_END
