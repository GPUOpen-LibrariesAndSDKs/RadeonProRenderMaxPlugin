#include "FireRenderIESLight.h"

#include "FireRenderIES_General.h"

FIRERENDER_NAMESPACE_BEGIN

namespace
{
	template<typename F>
	void ForEachFileInFoler(F&& f)
	{
		auto profilesFolder = GetDataStoreFolder() + L"IES Profiles\\*";

		WIN32_FIND_DATA ffd;
		auto hFind = FindFirstFile(profilesFolder.c_str(), &ffd);

		if (hFind == INVALID_HANDLE_VALUE)
		{
			return;
		}

		do
		{
			if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				// should it be recursive?
				continue;
			}
			else
			{
				f(ffd.cFileName);
			}
		} while (FindNextFile(hFind, &ffd) != 0);
	}

	std::string OpenIESFile()
	{
		// TODO:
		return "";
	}
}

bool IES_General::InitializePage()
{
	// Import button
	m_importButton.Capture(m_panel, IDC_FIRERENDER_IES_LIGHT_IMPORT);
	m_importButton.SetType(CustButType::CBT_PUSH);

	// Profiles combo box
	m_profilesComboBox.Capture(m_panel, IDC_FIRERENDER_IES_LIGHT_PROFILE);

	ForEachFileInFoler([&](TCHAR* filename)
	{
		m_profilesComboBox.AddItem(filename);
	});

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
	spinner.SetLimits(0.f, FLT_MAX);
	spinner.SetValue(m_parent->GetAreaWidth());
	spinner.SetResetValue(1.0f);
	spinner.SetScale(0.001f);

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

	if (code == CBN_SELCHANGE && controlId == IDC_FIRERENDER_IES_LIGHT_PROFILE)
	{
		auto index = m_profilesComboBox.GetSelectedIndex();
		auto profile = m_profilesComboBox.GetItemText(index);

		m_parent->ActivateProfile(profile.c_str());
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

INT_PTR IES_General::OnButtonClick(WORD controlId)
{
	switch (controlId)
	{
	case IDC_FIRERENDER_IES_LIGHT_IMPORT:
		ImportFile();
		break;
	}
	
	return FALSE;
}

void IES_General::SaveCurrent()
{
	FASSERT(!"Not implemented");
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

void IES_General::ImportFile()
{
	auto file = OpenIESFile();
	m_parent->ImportFile(file.c_str());
}

FIRERENDER_NAMESPACE_END