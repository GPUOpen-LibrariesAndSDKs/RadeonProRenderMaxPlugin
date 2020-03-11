/**********************************************************************
Copyright 2020 Advanced Micro Devices, Inc
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at
    http://www.apache.org/licenses/LICENSE-2.0
Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
********************************************************************/


#include "FireRenderIESLight.h"
#include "FireRenderIES_Volume.h"

FIRERENDER_NAMESPACE_BEGIN

const int IES_Volume::DialogId = IDD_FIRERENDER_IES_LIGHT_VOLUME;
const TCHAR* IES_Volume::PanelName = _T("Volume");

bool IES_Volume::UpdateVolumeScaleParam(TimeValue t)
{
	return m_parent->SetVolumeScale(m_volumeScaleControl.GetValue<float>(), t);
}

bool IES_Volume::InitializePage(TimeValue t)
{
	m_volumeScaleControl.Capture(m_panel,
		IDC_FIRERENDER_IES_LIGHT_VOLUME_SCALE,
		IDC_FIRERENDER_IES_LIGHT_VOLUME_SCALE_S);

	m_volumeScaleControl.Bind(EditSpinnerType::EDITTYPE_FLOAT);
	m_volumeScaleControl.GetSpinner().SetSettings<FireRenderIESLight::VolumeScaleSettings>();
	m_volumeScaleControl.GetSpinner().SetValue(m_parent->GetVolumeScale(t));

	return TRUE;
}

void IES_Volume::UninitializePage()
{
	m_volumeScaleControl.Release();
}

bool IES_Volume::OnEditChange(TimeValue t, int controlId, HWND editHWND)
{
	switch (controlId)
	{
		case IDC_FIRERENDER_IES_LIGHT_VOLUME_SCALE:
			return UpdateVolumeScaleParam(t);
	}

	return false;
}

bool IES_Volume::OnSpinnerChange(TimeValue t, ISpinnerControl* spinner, WORD controlId, bool isDragging)
{
	switch (controlId)
	{
		case IDC_FIRERENDER_IES_LIGHT_VOLUME_SCALE_S:
			return UpdateVolumeScaleParam(t);
	}

	return false;
}

const TCHAR* IES_Volume::GetAcceptMessage(WORD controlId) const
{
	switch (controlId)
	{
		case IDC_FIRERENDER_IES_LIGHT_VOLUME_SCALE:
		case IDC_FIRERENDER_IES_LIGHT_VOLUME_SCALE_S:
			return _T("IES light: change volume scale");
	}

	FASSERT(false);
	return IES_Panel::GetAcceptMessage(controlId);
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
