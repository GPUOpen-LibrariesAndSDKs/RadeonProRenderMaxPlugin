#include <array>
#include <functional>

#include "FireRenderIESLight.h"
#include "FireRenderIES_Profiles.h"

#include "FireRenderIES_General.h"

FIRERENDER_NAMESPACE_BEGIN

const int IES_General::DialogId = IDD_FIRERENDER_IES_LIGHT_GENERAL;
const TCHAR* IES_General::PanelName = _T("General");

bool IES_General::InitializePage(TimeValue time)
{
	// Import button
	m_importButton.Capture(m_panel, IDC_FIRERENDER_IES_LIGHT_IMPORT);
	m_importButton.SetType(CustButType::CBT_PUSH);
	m_importButton.SetButtonDownNotify(true);

	// Delete current profile button
	m_deleteCurrentButton.Capture(m_panel, IDC_FIRERENDER_IES_LIGHT_DELETE_PROFILE);
	m_deleteCurrentButton.SetType(CustButType::CBT_PUSH);
	m_deleteCurrentButton.SetButtonDownNotify(true);

	// Profiles combo box
	m_profilesComboBox.Capture(m_panel, IDC_FIRERENDER_IES_LIGHT_PROFILE);
	UpdateProfiles(time);

	// Enabled parameter
	m_enabledControl.Capture(m_panel, IDC_FIRERENDER_IES_LIGHT_ENABLED);
	m_enabledControl.SetCheck(m_parent->GetEnabled(time));

	// Targeted parameter
	m_targetedControl.Capture(m_panel, IDC_FIRERENDER_IES_LIGHT_TARGETED);
	m_targetedControl.SetCheck(m_parent->GetTargeted(time));

	// Target distance parameter
	m_targetDistanceControl.Capture(m_panel,
		IDC_FIRERENDER_IES_LIGHT_TARGET_DISTANCE,
		IDC_FIRERENDER_IES_LIGHT_TARGET_DISTANCE_S);

	m_targetDistanceControl.Bind(EDITTYPE_FLOAT);
	MaxSpinner& targetDistanceSpinner = m_targetDistanceControl.GetSpinner();
	targetDistanceSpinner.SetSettings<FireRenderIESLight::TargetDistanceSettings>();
	m_targetDistanceControl.GetSpinner().SetValue(m_parent->GetTargetDistance(time));

	// Area width parameter
	m_areaWidthControl.Capture(m_panel,
		IDC_FIRERENDER_IES_LIGHT_AREA_WIDTH,
		IDC_FIRERENDER_IES_LIGHT_AREA_WIDTH_S);

	m_areaWidthControl.Bind(EDITTYPE_FLOAT);

	MaxSpinner& areaWidthSpinner = m_areaWidthControl.GetSpinner();
	areaWidthSpinner.SetSettings<FireRenderIESLight::AreaWidthSettings>();
	areaWidthSpinner.SetValue(m_parent->GetAreaWidth(time));

	// Rotation X parameter
	m_RotateX.Capture(m_panel,
		IDC_FIRERENDER_IES_LIGHT_ROTATION_X,
		IDC_FIRERENDER_IES_LIGHT_ROTATION_X_S);

	m_RotateX.Bind(EDITTYPE_FLOAT);

	MaxSpinner& spinnerRotateX = m_RotateX.GetSpinner();
	spinnerRotateX.SetSettings<FireRenderIESLight::LightRotateSettings>();
	spinnerRotateX.SetValue(m_parent->GetRotationX(time));

	// Rotation Y parameter
	m_RotateY.Capture(m_panel,
		IDC_FIRERENDER_IES_LIGHT_ROTATION_Y,
		IDC_FIRERENDER_IES_LIGHT_ROTATION_Y_S);

	m_RotateY.Bind(EDITTYPE_FLOAT);

	MaxSpinner& spinnerRotateY = m_RotateY.GetSpinner();
	spinnerRotateY.SetSettings<FireRenderIESLight::LightRotateSettings>();
	spinnerRotateY.SetValue(m_parent->GetRotationY(time));

	// Rotation Z parameter
	m_RotateZ.Capture(m_panel,
		IDC_FIRERENDER_IES_LIGHT_ROTATION_Z,
		IDC_FIRERENDER_IES_LIGHT_ROTATION_Z_S);

	m_RotateZ.Bind(EDITTYPE_FLOAT);

	MaxSpinner& spinnerRotateZ = m_RotateZ.GetSpinner();
	spinnerRotateZ.SetSettings<FireRenderIESLight::LightRotateSettings>();
	spinnerRotateZ.SetValue(m_parent->GetRotationZ(time));

	return true;
}

void IES_General::UninitializePage()
{
	m_importButton.Release();
	m_profilesComboBox.Release();
	m_enabledControl.Release();
	m_targetedControl.Release();
	m_targetDistanceControl.Release();
	m_areaWidthControl.Release();
}

bool IES_General::HandleControlCommand(TimeValue t, WORD code, WORD controlId)
{
	if (code == BN_CLICKED)
	{
		switch (controlId)
		{
			case IDC_FIRERENDER_IES_LIGHT_ENABLED:
				return UpdateEnabledParam(t);

			case IDC_FIRERENDER_IES_LIGHT_TARGETED:
				return UpdateTargetedParam(t);
		}
	}

	if (code == BN_BUTTONUP)
	{
		switch (controlId)
		{
			case IDC_FIRERENDER_IES_LIGHT_IMPORT:
				if(m_importButton.CursorIsOver())
					ImportProfile();
				return false;

			case IDC_FIRERENDER_IES_LIGHT_DELETE_PROFILE:
				if (m_deleteCurrentButton.CursorIsOver())
					DeleteSelectedProfile(t);
				return true;
		}
	}

	if (code == CBN_SELCHANGE && controlId == IDC_FIRERENDER_IES_LIGHT_PROFILE)
	{
		bool profileIsActivated = ActivateSelectedProfile(t);
		UpdateDeleteProfileButtonState();
		return profileIsActivated;
	}

	return false;
}

bool IES_General::OnEditChange(TimeValue t, int editId, HWND editHWND)
{
	switch (editId)
	{
		case IDC_FIRERENDER_IES_LIGHT_AREA_WIDTH:
			return UpdateAreaWidthParam(t);

		case IDC_FIRERENDER_IES_LIGHT_TARGET_DISTANCE:
			return UpdateTargetDistanceParam(t);

		case IDC_FIRERENDER_IES_LIGHT_ROTATION_X:
			return UpdateRotationXParam(t);

		case IDC_FIRERENDER_IES_LIGHT_ROTATION_Y:
			return UpdateRotationYParam(t);

		case IDC_FIRERENDER_IES_LIGHT_ROTATION_Z:
			return UpdateRotationZParam(t);
	}

	return false;
}

bool IES_General::OnSpinnerChange(TimeValue t, ISpinnerControl* spinner, WORD controlId, bool isDragging)
{
	switch (controlId)
	{
		case IDC_FIRERENDER_IES_LIGHT_AREA_WIDTH_S:
			return UpdateAreaWidthParam(t);

		case IDC_FIRERENDER_IES_LIGHT_TARGET_DISTANCE_S:
			return UpdateTargetDistanceParam(t);

		case IDC_FIRERENDER_IES_LIGHT_ROTATION_X_S:
			return UpdateRotationXParam(t);

		case IDC_FIRERENDER_IES_LIGHT_ROTATION_Y_S:
			return UpdateRotationYParam(t);

		case IDC_FIRERENDER_IES_LIGHT_ROTATION_Z_S:
			return UpdateRotationZParam(t);
	}

	return false;
}

const TCHAR* IES_General::GetAcceptMessage(WORD controlId) const
{
	switch (controlId)
	{
		case IDC_FIRERENDER_IES_LIGHT_PROFILE:
		case IDC_FIRERENDER_IES_LIGHT_DELETE_PROFILE:
			return _T("IES light: change profile");

		case IDC_FIRERENDER_IES_LIGHT_ENABLED:
			return _T("IES light: change enabled parameter");

		case IDC_FIRERENDER_IES_LIGHT_TARGETED:
			return _T("IES light: change targeted parameter");

		case IDC_FIRERENDER_IES_LIGHT_TARGET_DISTANCE:
		case IDC_FIRERENDER_IES_LIGHT_TARGET_DISTANCE_S:
			return _T("IES light: move target object");

		case IDC_FIRERENDER_IES_LIGHT_AREA_WIDTH:
		case IDC_FIRERENDER_IES_LIGHT_AREA_WIDTH_S:
			return _T("IES light: change area width");

		case IDC_FIRERENDER_IES_LIGHT_ROTATION_X:
		case IDC_FIRERENDER_IES_LIGHT_ROTATION_X_S:
		case IDC_FIRERENDER_IES_LIGHT_ROTATION_Y:
		case IDC_FIRERENDER_IES_LIGHT_ROTATION_Y_S:
		case IDC_FIRERENDER_IES_LIGHT_ROTATION_Z:
		case IDC_FIRERENDER_IES_LIGHT_ROTATION_Z_S:
			return _T("IES light: change light source rotation");
	}

	FASSERT(false);
	return IES_Panel::GetAcceptMessage(controlId);
}

void IES_General::UpdateUI(TimeValue t)
{
	if (!IsInitialized())
	{
		return;
	}

	m_enabledControl.SetCheck(m_parent->GetEnabled(t));

	bool targeted = m_parent->GetTargeted(t);
	m_targetedControl.SetCheck(targeted);

	if (targeted)
	{
		m_targetDistanceControl.Enable();
	}
	else
	{
		m_targetDistanceControl.Disable();
	}

	m_targetDistanceControl.GetSpinner().SetValue(m_parent->GetTargetDistance(t));

	m_areaWidthControl.GetSpinner().SetValue(m_parent->GetAreaWidth(t));

	int activeIndex = -1;

	if (m_parent->ProfileIsSelected(t))
	{
		const TCHAR* activeProfile = m_parent->GetActiveProfile(t);

		// Compute active profile index in the combo box by it's name
		bool profileFound =
			m_profilesComboBox.ForEachItem([&](int index, const std::basic_string<TCHAR>& text)
		{
			if (_tcscmp(text.c_str(), activeProfile) == 0)
			{
				// Active profile is found, break the loop
				activeIndex = index;
				return true;
			}

			// Keep searching
			return false;
		});

		FASSERT(profileFound);
	}

	m_profilesComboBox.SetSelected(activeIndex);
	UpdateDeleteProfileButtonState();
}

void IES_General::Enable()
{
	using namespace ies_panel_utils;

	EnableControls<true>(
		m_importButton,
		m_deleteCurrentButton,
		m_profilesComboBox,
		m_enabledControl,
		m_targetedControl,
		m_targetDistanceControl,
		m_areaWidthControl);
}

void IES_General::Disable()
{
	using namespace ies_panel_utils;

	EnableControls<false>(
		m_importButton,
		m_deleteCurrentButton,
		m_profilesComboBox,
		m_enabledControl,
		m_targetedControl,
		m_targetDistanceControl,
		m_areaWidthControl);
}

bool IES_General::UpdateEnabledParam(TimeValue t)
{
	return m_parent->SetEnabled(m_enabledControl.IsChecked(), t);
}

bool IES_General::UpdateTargetedParam(TimeValue t)
{
	return m_parent->SetTargeted(m_targetedControl.IsChecked(), t);
}

bool IES_General::UpdateTargetDistanceParam(TimeValue t)
{
	return m_parent->SetTargetDistance(m_targetDistanceControl.GetValue<float>(), t);
}

bool IES_General::UpdateAreaWidthParam(TimeValue t)
{
	return m_parent->SetAreaWidth(m_areaWidthControl.GetValue<float>(), t);
}

bool IES_General::UpdateRotationXParam(TimeValue t)
{
	return m_parent->SetRotationX(m_RotateX.GetValue<float>(), t);
}

bool IES_General::UpdateRotationYParam(TimeValue t)
{
	return m_parent->SetRotationY(m_RotateY.GetValue<float>(), t);
}

bool IES_General::UpdateRotationZParam(TimeValue t)
{
	return m_parent->SetRotationZ(m_RotateZ.GetValue<float>(), t);
}

void IES_General::ImportProfile()
{
	std::wstring filename;
	size_t nameOffset;
	if (FireRenderIES_Profiles::GetIESFileName(filename, &nameOffset))
	{
		const wchar_t* pathAndName = filename.c_str();
		const wchar_t* name = pathAndName + nameOffset;

		if (FireRenderIES_Profiles::CopyIES_File(filename.c_str(), nameOffset))
		{
			m_profilesComboBox.AddItem(name);
			UpdateDeleteProfileButtonState();
		}
	}
}

bool IES_General::ActivateSelectedProfile(TimeValue t)
{
	int index = m_profilesComboBox.GetSelectedIndex();

	// Nothing is selected
	if (index == -1)
	{
		m_parent->SetActiveProfile(_T(""), t);
		return false;
	}

	return m_parent->SetActiveProfile(m_profilesComboBox.GetItemText(index).c_str(), t);
}

bool IES_General::DeleteSelectedProfile(TimeValue t)
{
	int selIdx = m_profilesComboBox.GetSelectedIndex();

	// Nothing is selected
	if (selIdx < 0)
	{
		return false;
	}

	std::wstring itemText = m_profilesComboBox.GetItemText(selIdx);
	std::wstring profilesDir = FireRenderIES_Profiles::GetIESProfilesDirectory();
	std::wstring path = profilesDir + itemText;

	if (FileExists(path.c_str()) && !DeleteFile(path.c_str()))
	{
		MessageBox(
			GetCOREInterface()->GetMAXHWnd(),
			_T("Failed to delete the file"),
			_T("Error"),
			MB_ICONERROR);

		return false;
	}

	m_profilesComboBox.DeleteItem(selIdx);
	m_profilesComboBox.SetSelected(-1);
	bool profileChanged = ActivateSelectedProfile(t);
	UpdateDeleteProfileButtonState();

	return profileChanged;
}

void IES_General::UpdateProfiles(TimeValue time)
{
	const TCHAR* activeProfile = m_parent->GetActiveProfile(time);
	int activeIndex = -1;

	FireRenderIES_Profiles::ForEachProfile([&](const TCHAR* filename)
	{
		int addIndex = m_profilesComboBox.AddItem(filename);

		if (activeIndex < 0 &&
			_tcscmp(filename, activeProfile) == 0)
		{
			activeIndex = addIndex;
		}
	});

	m_profilesComboBox.SetSelected(activeIndex);

	UpdateDeleteProfileButtonState();
}

void IES_General::UpdateDeleteProfileButtonState()
{
	if (m_profilesComboBox.GetSelectedIndex() < 0)
	{
		m_deleteCurrentButton.Disable();
	}
	else
	{
		m_deleteCurrentButton.Enable();
	}
}

FIRERENDER_NAMESPACE_END
