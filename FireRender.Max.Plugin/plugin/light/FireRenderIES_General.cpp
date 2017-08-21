#include <array>
#include <functional>

#include "FireRenderIESLight.h"

#include "FireRenderIES_General.h"

FIRERENDER_NAMESPACE_BEGIN

namespace
{
	bool DoFindSession(const TCHAR* searchRequest,
		std::function<bool(WIN32_FIND_DATA&)> f)
	{
		WIN32_FIND_DATA ffd;
		auto hFind = FindFirstFile(searchRequest, &ffd);

		if (hFind == INVALID_HANDLE_VALUE)
		{
			return false;
		}

		do
		{
			if (f(ffd))
			{
				return true;
			}
		} while (FindNextFile(hFind, &ffd) != 0);

		auto closeRes = FindClose(hFind);
		FASSERT(closeRes);

		return false;
	}

	bool FileExists(const TCHAR* name)
	{
		DWORD dwAttrib = GetFileAttributes(name);

		return
			dwAttrib != INVALID_FILE_ATTRIBUTES &&
			(dwAttrib & FILE_ATTRIBUTE_DIRECTORY) == 0;
	}

	bool FolderExists(const TCHAR* path)
	{
		DWORD dwAttrib = GetFileAttributes(path);

		return
			dwAttrib != INVALID_FILE_ATTRIBUTES &&
			(dwAttrib & FILE_ATTRIBUTE_DIRECTORY) != 0;
	}

	std::wstring GetIESProfilesDirectory()
	{
		return GetDataStoreFolder() + L"IES Profiles\\";
	}

	template<typename F>
	void ForEachFileInFoler(F&& f)
	{
		auto profilesFolder = GetIESProfilesDirectory() + L'*';

		DoFindSession(profilesFolder.c_str(), [&](WIN32_FIND_DATA& ffd)
		{
			if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				// should it be recursive?
			}
			else
			{
				f(ffd.cFileName);
			}

			return false;
		});
	}

	//	Prompts user to select an IES file
	//	Returns false if there is any error or user canceled the dialog
	//		'filename' - mandatory output parameter - contains full file name
	//		'nFileOffset' - optional output parameter - the zero-based offset,
	//			in characters, from the beginning of the path to the file name in 'filename' string
	//		'nExtOffset' - optional output parameter - The zero-based offset,
	//			in characters, from the beginning of the path to the file name extension in 'filename' string
	bool GetIESFileName(std::wstring& filename,
		size_t* nFileOffset = nullptr,
		size_t* nExtOffset = nullptr)
	{
		constexpr size_t fileBufferSize = MAX_PATH;
		std::array<TCHAR, fileBufferSize> fileBuffer;
		std::fill(fileBuffer.begin(), fileBuffer.end(), 0);

		OPENFILENAME ofn;
		ZeroMemory(&ofn, sizeof(OPENFILENAME));
		ofn.lStructSize = sizeof(OPENFILENAME);
		ofn.hwndOwner = GetCOREInterface()->GetMAXHWnd();
		ofn.lpstrFilter = _T("IES Files(*.IES)\0*.IES\0All files(*.*)\0*.*\0\0");
		ofn.nMaxCustFilter = 0;
		ofn.nFilterIndex = 1;
		ofn.lpstrFile = &fileBuffer[0];
		ofn.nMaxFile = fileBufferSize;
		ofn.lpstrTitle = _T("Select IES file");
		ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
		ofn.lpstrDefExt = _T("IES");

		if (GetOpenFileName(&ofn) == FALSE)
		{
			auto errorCode = CommDlgExtendedError();
			FASSERT(errorCode == 0);
			return false;
		}

		filename = ofn.lpstrFile;

		if (nFileOffset != nullptr) *nFileOffset = ofn.nFileOffset;
		if (nExtOffset != nullptr) *nExtOffset = ofn.nFileExtension;

		return true;
	}

	// Copies profile file to the profiles directory
	// Returns true if new file was created
	// Returns false if there is any error or file was replaced
	bool CopyIES_File(const TCHAR* from, size_t nameOffset)
	{
		auto profilesDir = GetIESProfilesDirectory();
		auto newName = profilesDir + (from + nameOffset);
		auto rawNewName = newName.c_str();

		bool replace = false;

		// Ask user to replace an existing profile
		if (FileExists(rawNewName))
		{
			replace = true;

			auto result = MessageBox(
				GetCOREInterface()->GetMAXHWnd(),
				_T("You are going to replace an existing profile. Continue?"),
				_T("Warning"),
				MB_ICONQUESTION | MB_YESNO);

			FASSERT(result != 0);

			if (result != IDYES)
			{
				return false;
			}
		}

		// Make sure that profiles directory exists
		if (!FolderExists(profilesDir.c_str()))
		{
			auto res = CreateDirectory(profilesDir.c_str(), NULL);
			FASSERT(res && "Failed to create profiles directory");
		}

		// Copy profile file
		auto res = CopyFile(from, rawNewName, FALSE);
		FASSERT(res);

		return replace ? false : res;
	}
}

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

			case IDC_FIRERENDER_IES_LIGHT_DELETE_CURRENT:
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
	if (GetIESFileName(filename, &nameOffset))
	{
		auto pathAndName = filename.c_str();
		auto name = pathAndName + nameOffset;

		if (CopyIES_File(filename.c_str(), nameOffset))
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
		m_parent->ActivateProfile(_T(""));
		return;
	}

	auto profile = m_profilesComboBox.GetItemText(index);
	auto filename = GetIESProfilesDirectory() + profile;

	m_parent->ActivateProfile(filename.c_str());
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
	auto profilesDir = GetIESProfilesDirectory();
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
	ForEachFileInFoler([&](TCHAR* filename)
	{
		m_profilesComboBox.AddItem(filename);
	});

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