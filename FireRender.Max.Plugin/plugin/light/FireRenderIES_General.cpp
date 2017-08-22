#include <array>
#include <functional>

#include "FireRenderIESLight.h"
#include "FireRenderIES_Profiles.h"

#include "FireRenderIES_General.h"

FIRERENDER_NAMESPACE_BEGIN

bool IES_General::InitializePage()
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
	UpdateProfiles();

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
	spinner.SetSettings<FireRenderIESLight::AreaWidthSettings>();
	spinner.SetValue(m_parent->GetAreaWidth());

	return true;
}

void IES_General::UninitializePage()
{
	m_importButton.Release();
	m_profilesComboBox.Release();
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

		case IDC_FIRERENDER_IES_LIGHT_TARGETED_CHECKBOX:
			UpdateTargetedParam();
			return TRUE;
		}
	}

	if (code == BN_BUTTONUP)
	{
		switch (controlId)
		{
			case IDC_FIRERENDER_IES_LIGHT_IMPORT:
				if(m_importButton.CursorIsOver())
					ImportProfile();
				return TRUE;

			case IDC_FIRERENDER_IES_LIGHT_DELETE_PROFILE:
				if (m_deleteCurrentButton.CursorIsOver())
					DeleteSelectedProfile();
				return TRUE;
		}
	}

	if (code == CBN_SELCHANGE && controlId == IDC_FIRERENDER_IES_LIGHT_PROFILE)
	{
		ActivateSelectedProfile();
		UpdateDeleteProfileButtonState();
		return TRUE;
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

void IES_General::ImportProfile()
{
	std::wstring filename;
	size_t nameOffset;
	if (FireRenderIES_Profiles::GetIESFileName(filename, &nameOffset))
	{
		auto pathAndName = filename.c_str();
		auto name = pathAndName + nameOffset;

		if (FireRenderIES_Profiles::CopyIES_File(filename.c_str(), nameOffset))
		{
			m_profilesComboBox.AddItem(name);
			UpdateDeleteProfileButtonState();
		}
	}
}

void IES_General::ActivateSelectedProfile()
{
	auto index = m_profilesComboBox.GetSelectedIndex();

	// Nothing is selected
	if (index == -1)
	{
		m_parent->SetActiveProfile(_T(""));
		return;
	}

	m_parent->SetActiveProfile(m_profilesComboBox.GetItemText(index).c_str());
}

void IES_General::DeleteSelectedProfile()
{
	auto selIdx = m_profilesComboBox.GetSelectedIndex();

	// Nothing is selected
	if (selIdx < 0)
	{
		return;
	}

	auto itemText = m_profilesComboBox.GetItemText(selIdx);
	auto profilesDir = FireRenderIES_Profiles::GetIESProfilesDirectory();
	auto path = profilesDir + itemText;

	if (FileExists(path.c_str()) && !DeleteFile(path.c_str()))
	{
		MessageBox(
			GetCOREInterface()->GetMAXHWnd(),
			_T("Failed to delete the file"),
			_T("Error"),
			MB_ICONERROR);

		return;
	}

	m_profilesComboBox.DeleteItem(selIdx);
	m_profilesComboBox.SetSelected(-1);
	ActivateSelectedProfile();
	UpdateDeleteProfileButtonState();
}

void IES_General::UpdateProfiles()
{
	auto activeProfile = m_parent->GetActiveProfile();
	int activeIndex = -1;

	FireRenderIES_Profiles::ForEachProfile([&](const TCHAR* filename)
	{
		auto addIndex = m_profilesComboBox.AddItem(filename);

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