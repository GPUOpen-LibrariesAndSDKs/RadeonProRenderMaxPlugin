/*********************************************************************************************************************************
* Radeon ProRender for 3ds Max plugin
* Copyright (C) 2017 AMD
* All Rights Reserved
*
* 3ds Max render settings UI dialog implementation:
* Custom dialog box code for the various render settings rollouts
*********************************************************************************************************************************/

#include "plugin/ParamDlg.h"
#include "plugin/FireRenderer.h"
#include "RadeonProRender.h"
#include "utils/Utils.h"
#include "plugin/FRSettingsFileHandler.h"
#include "plugin/FireRenderEnvironment.h"
#include "plugin/FireRenderAnalyticalSun.h"
#include "plugin/ScopeManager.h"
#include "plugin/CamManager.h"
#include "plugin/TmManager.h"
#include "plugin/BgManager.h"
#include "plugin/PrManager.h"

#include "resource.h"

#include <IPathConfigMgr.h>

#include <fstream>
#include <algorithm>
#include <string> 
#include <shlobj.h>
#include <Commctrl.h>
#include <Richedit.h>
#include "maxscript/maxscript.h"

inline LONG LONG_cast( size_t x ) { return (LONG)x; }

FIRERENDER_NAMESPACE_BEGIN

#define ID_MAPCLEAR_IBL				WM_USER + 101
#define ID_MAPCLEAR_IBLBACKPLATE	WM_USER + 102
#define ID_MAPCLEAR_IBLREFR			WM_USER + 103
#define ID_MAPCLEAR_IBLREFL			WM_USER + 104
#define ID_MAPCLEAR_SKYBACKPLATE	WM_USER + 105
#define ID_MAPCLEAR_SKYREFR			WM_USER + 106
#define ID_MAPCLEAR_SKYREFL			WM_USER + 107

#define PROP_SENDER_COLORDIAL 102

// UI has units in mm, but core needs it in meters 1m = 1000mm
static const float rayCastEpsilonContextCoeff = 1000.0f;

static MSTR GetIBLRootDirectory()
{
	// Reference: installer/nsiexec2/nsiexec2/main.cpp
	// RPR plugin could have HDRI libraries installed under "Documents" folder.
	PWSTR path = NULL;
	HRESULT hr = SHGetKnownFolderPath(FOLDERID_Documents, 0, NULL, &path);
	std::wstring directoryOfMyDocumentsRpr;
	if (!FAILED(hr))
	{
		directoryOfMyDocumentsRpr = std::wstring(path) + L"\\Radeon ProRender\\3ds Max";
	}
	CoTaskMemFree(path);
	if (!directoryOfMyDocumentsRpr.empty())
	{
		DWORD attributes = GetFileAttributesW(directoryOfMyDocumentsRpr.c_str());
		if ((attributes != INVALID_FILE_ATTRIBUTES) && (attributes & FILE_ATTRIBUTE_DIRECTORY))
		{
			static const wchar_t* knownHdriLibs[] =
			{
				L"HDRIHaven2",
				L"SunnyRoad2"
			};
			for (int i = 0; i < sizeof(knownHdriLibs) / sizeof(knownHdriLibs[0]); i++)
			{
				std::wstring hdriLibPath = directoryOfMyDocumentsRpr + L"\\" + knownHdriLibs[i];
				attributes = GetFileAttributesW(hdriLibPath.c_str());
				if ((attributes != INVALID_FILE_ATTRIBUTES) && (attributes & FILE_ATTRIBUTE_DIRECTORY))
				{
					return directoryOfMyDocumentsRpr.c_str();
				}
			}
		}
	}

	// Failed to find known installed HDRI library, use something else
	IPathConfigMgr* path_mgr = IPathConfigMgr::GetPathConfigMgr();
	MaxSDK::Util::Path folder = path_mgr->GetDir(APP_IMAGE_DIR); // maxroot
	return folder.GetString();
}

ToolTipExtender& GetToolTipExtender()
{
	static ToolTipExtender tipExtender;
	return tipExtender;
}

inline void SetSpinnerScale(ISpinnerControl* spin, float scale)
{
	spin->SetAutoScale(FALSE);
	spin->SetScale(scale);
}

bool FireRenderParamDlg::mIsUncertifiedMessageShown = false;

FireRenderParamDlg::FireRenderParamDlg(IRendParams* ir, BOOL readOnly, FireRenderer* renderer)
{
	IParamBlock2* p = renderer->GetParamBlock(0);

	this->iRendParams = ir;
	this->renderer = renderer;
	this->isReadOnly = readOnly;

	mGeneralSettings.Init(ir, IDD_RENDER_GENERALSETTINGS, (const MCHAR*)_T("General"), this, false, &kSettingsTabID);
		
	mHardwareSetting.Init(ir, IDD_RENDER_HWSETTINGS, (const MCHAR*)_T("Hardware"), this, true, &kSettingsTabID);
		
	mQualitySettings.Init(ir, IDD_RENDER_QUALITYSETTINGS, (const MCHAR*)_T("Quality"), this, true, &kSettingsTabID);

	mDenoiserSettings.Init(ir, IDD_RENDER_DENOISER, (const MCHAR*)_T("Denoiser"), this, true, &kSettingsTabID);

	mCameraSetting.Init(ir, IDD_RENDER_CAMERASETTINGS, (const MCHAR*)_T("Camera"), this, true, &kSettingsTabID);

	mTonemapSetting.Init(ir, IDD_RENDER_TONEMAPPING, (const MCHAR*)_T("Tonemap"), this, true, &kSettingsTabID);

	mBackgroundSettings.Init(ir, IDD_RENDER_BG, (const MCHAR*)_T("Environment and Scene"), this, true, &kSettingsTabID);

	mAntialiasSettings.Init(ir, IDD_RENDER_AASETTINGS, (const MCHAR*)_T("Anti Aliasing"), this, true, &kSettingsTabID);

	mAdvancedSettings.Init(ir, IDD_RENDER_ADVANCEDSETTINGS, (const MCHAR*)_T("Advanced"), this, false, &kAdvSettingsTabID);

	mScripts.Init(ir, IDD_RENDER_SCRIPTS, (const MCHAR*)_T("Scripts"), this, false, &kScriptsTabID);

	mQualitySettings.setupUIFromData();

	std::wstring strProductName;
	std::wstring strProductVersion;
	std::wstring strCoreVersion;
	std::wstring strVersion;

	if (GetProductAndVersion(strProductName, strProductVersion, strCoreVersion))
	{
		strVersion = strProductName + L" " + strProductVersion + L" (core " + strCoreVersion + L")";
	}

	SetDlgItemText(mAdvancedSettings.Hwnd(), IDC_VERSION, strVersion.c_str());
	SetDlgItemText(mScripts.Hwnd(), IDC_VERSION, strVersion.c_str());

	if (!readOnly)
		processUncertifiedGpuWarning();
}

FireRenderParamDlg::~FireRenderParamDlg()
{
	mScripts.DeleteRollout();
	mAdvancedSettings.DeleteRollout();
	mAntialiasSettings.DeleteRollout();
	mBackgroundSettings.DeleteRollout();
	mTonemapSetting.DeleteRollout();
	mCameraSetting.DeleteRollout();
	mDenoiserSettings.DeleteRollout();
	mQualitySettings.DeleteRollout();
	mHardwareSetting.DeleteRollout();
	mGeneralSettings.DeleteRollout();
}

struct CameraTypeDesc
{
	LPTSTR name;
	FRCameraType type;
	bool stereoSupport;
};

CameraTypeDesc cameraTypeDescs[] = {
	{ _T("Default"), FRCameraType::FRCameraType_Default, false },
	//{ _T("Perspective"), FRCameraType::FRCameraType_Perspective, false },
	//{ _T("Orthographic"), FRCameraType::FRCameraType_Orthographic, false },
	{ _T("SphericalPanorama"), FRCameraType::FRCameraType_LatitudeLongitude_360, false },
	{ _T("SphericalPanoramaStereo"), FRCameraType::FRCameraType_LatitudeLongitude_Stereo, true },
	{ _T("CubeMap"), FRCameraType::FRCameraType_Cubemap, false },
	{ _T("CubeMapStereo"), FRCameraType::FRCameraType_Cubemap_Stereo, true },
	//{ _T("Cubemap"), FRCameraType::Cubemap, true }
};

const CameraTypeDesc & GetCameraTypeDesc(FRCameraType type)
{
	FASSERT(sizeof(cameraTypeDescs) / sizeof(CameraTypeDesc) == FRCameraType::FRCameraType_Count);
	return cameraTypeDescs[type];
}

/// Utility value that select an item in combobox with given value (ITEMDATA)
void setListboxValue(HWND listbox, const int toSet) {
    for (int i = 0;; ++i) {
        const int value = int(SendMessage(listbox, CB_GETITEMDATA, WPARAM(i), 0));
        if (value == toSet) {
            SendMessage(listbox, CB_SETCURSEL, WPARAM(i), 0L);
            break;
        }
        if (i > 9999) { 
            // item with this value does not exist - probably save/load problem of render settings. 
            // Select first item and show error to user.
            SendMessage(listbox, CB_SETCURSEL, WPARAM(0), 0L);
            MessageBox(GetCOREInterface()->GetMAXHWnd(), _T("Error in listbox"), _T("Radeon ProRender exception"), MB_OK);
            break;
        }
    }
}

void FireRenderParamDlg::setWarningSuppress(bool value)
{
	int controlId = IDC_WARNINGS_SUPPRESS_GLOBAL;
	HWND hWnd = mAdvancedSettings.Hwnd();
	FASSERT(GetDlgItem(hWnd, controlId));
	CheckDlgButton(hWnd, controlId, value);
}

void FireRenderParamDlg::processUncertifiedGpuWarning()
{
	IParamBlock2* pb = renderer->GetParamBlock(0);
	
	if (mIsUncertifiedMessageShown)
		return;
	
	int uncertifiedGpuNum = 0;
	
	uncertifiedGpuNames.clear();

	GpuInfoArray& gpuInfoArray = ScopeManagerMax::TheManager.gpuInfoArray;
	
	for (int i = 0; i < gpuInfoArray.size(); i++)
	{
		const GpuInfo& gpuInfo = gpuInfoArray[i];

		if (!gpuInfo.isWhiteListed)
		{
			if (uncertifiedGpuNum == 0)
				uncertifiedGpuNames += gpuInfo.name;
			else
				uncertifiedGpuNames += std::string(", ") + gpuInfo.name;

			++uncertifiedGpuNum;
		}
	}

	if (!uncertifiedGpuNames.empty())
	{
		const std::string attrName = "Uncertified Device Notification";
		std::string val = FRSettingsFileHandler::getAttributeSettingsFor(attrName);

		if (val.length() == 0 || std::stoi(val) == 0)
		{

			std::string warningText = "Your graphics card(s) :\r\n{ " + uncertifiedGpuNames + " }\r\n\r\n" +
				"have not been certified for use with Radeon ProRender.\r\n"
				"Only certified rendering devices will used by default to avoid any potential rendering or stability issues."
				" CPU rendering will instead be used.\r\n"
				"You can override this behavior in the Radeon ProRender Settings Dialog.";

			MessageBoxA(GetCOREInterface()->GetMAXHWnd(), warningText.c_str(), "Warning", MB_OK);

			FRSettingsFileHandler::setAttributeSettingsFor(attrName, "1");

			mIsUncertifiedMessageShown = true;
		}
	}

	if (ScopeManagerMax::TheManager.getGpuNamesThatHaveOldDriver().size() != 0)
	{
		const std::string attrName = "Device Old Driver Notification";
		std::string val = FRSettingsFileHandler::getAttributeSettingsFor(attrName);

		if (val.length() == 0 || std::stoi(val) == 0)
		{
			const std::vector<std::string>& vec = ScopeManagerMax::TheManager.getGpuNamesThatHaveOldDriver();

			outDatedDriverGpuNames.clear();

			for (int i = 0; i < vec.size(); i++)
			{
				const std::string &gpuName = vec[i];

				if (i == 0)
					outDatedDriverGpuNames += gpuName;
				else
					outDatedDriverGpuNames += std::string(", ") + gpuName;
			}

			std::string warningText = "Your graphics card(s) :\r\n{ " + outDatedDriverGpuNames + " }\r\n\r\n" + 
				"have too old drivers for Radeon ProRender.\r\n"
				"Only rendering devices that have up to date drivers will used by default to avoid any potential"
				" rendering or stability issues. CPU rendering will instead be used.\r\n"
				"Please update your drivers.";

			MessageBoxA(GetCOREInterface()->GetMAXHWnd(), warningText.c_str(), "Warning", MB_OK);

			FRSettingsFileHandler::setAttributeSettingsFor(attrName, "1");

			mIsUncertifiedMessageShown = true;
		}
	}
}

void FireRenderParamDlg::AcceptParams()
{
}


//////////////////////////////////////////////////////////////////////////////
// GENERAL SETTINGS ROLLOUT
//

INT_PTR FireRenderParamDlg::CGeneralSettings::DlgProc(UINT msg, WPARAM wParam, LPARAM lParam)
{
	IParamBlock2* pb = mOwner->renderer->GetParamBlock(0);
	switch (msg)
	{
		case WM_COMMAND:
		{
			switch (HIWORD(wParam))
			{
			case EN_CHANGE:
			{
				if (mIsReady)
				{
					int idText = (int)LOWORD(wParam);

					if (idText == IDC_STAMP_TEXT)
					{
						int len = SendDlgItemMessage(mHwnd, IDC_STAMP_TEXT, WM_GETTEXTLENGTH, 0, 0);
						TSTR temp;
						temp.Resize(len + 1);
						SendDlgItemMessage(mHwnd, IDC_STAMP_TEXT, WM_GETTEXT, len + 1, (LPARAM)temp.data());
						BOOL res = pb->SetValue(PARAM_STAMP_TEXT, 0, temp); FASSERT(res);
					}
				}
			}
			break;

			case BN_CLICKED:
			{
				int idButton = (int)LOWORD(wParam);
				switch (idButton)
				{
					case IDC_RENDER_PASS:
					{
						EnableRenderLimitControls(TRUE, FALSE);
						BOOL res = pb->SetValue(PARAM_RENDER_LIMIT, 0, RPR_RENDER_LIMIT_PASS); FASSERT(res);
					}
					break;
					case IDC_RENDER_TIME:
					{
						EnableRenderLimitControls(FALSE, FALSE);
						BOOL res = pb->SetValue(PARAM_RENDER_LIMIT, 0, RPR_RENDER_LIMIT_TIME); FASSERT(res);
					}
					break;
					case IDC_RENDER_UNLIMITED:
					{
						EnableRenderLimitControls(FALSE, TRUE);
						BOOL res = pb->SetValue(PARAM_RENDER_LIMIT, 0, RPR_RENDER_LIMIT_UNLIMITED); FASSERT(res);
					}
					break;
					case IDC_ENABLE_STAMP:
					{
						BOOL res = pb->SetValue(PARAM_STAMP_ENABLED, 0, BOOL(IsDlgButtonChecked(mHwnd, IDC_ENABLE_STAMP))); FASSERT(res);
					}
					break;
					case IDC_STAMP_RESET:
						if (MessageBox(GetCOREInterface()->GetMAXHWnd(), _T("Would you like to reset Render Stamp text to default value?"), _T("Warning"), MB_YESNO) == IDYES)
						{
							HWND stampEdit = GetDlgItem(mHwnd, IDC_STAMP_TEXT);
							FASSERT(stampEdit);
							SetWindowText(stampEdit, DEFAULT_RENDER_STAMP);
						}
					break;
					case IDC_STAMP_HELP:
						MessageBox(GetCOREInterface()->GetMAXHWnd(), PRManagerMax::GetStampHelp(), _T("Information"), MB_OK);
					break;
				}
			}
			break;
			// should handle focus for text editor controls, otherwise we won't be able to type anything there
			case EN_SETFOCUS:
				DisableAccelerators();
				break;
			case EN_KILLFOCUS:
				EnableAccelerators();
				break;
			}
		}
		break;

		case CC_SPINNER_CHANGE:
		{
			if (mIsReady && !mOwner->isReadOnly)
			{
				int idSpinner = (int)LOWORD(wParam);
				if (idSpinner == IDC_TIME_H_S || idSpinner == IDC_TIME_M_S || idSpinner == IDC_TIME_S_S)
				{
					const int timeLimit = controls.timeLimitH->GetIVal() * 3600 + controls.timeLimitM->GetIVal() * 60 + controls.timeLimitS->GetIVal();
					BOOL res = pb->SetValue(PARAM_TIME_LIMIT, 0, timeLimit);
					FASSERT(res);
				}
				else
					CommitSpinnerToParam(pb, idSpinner);
			}
		}
		break;

		case WM_NOTIFY:
		{
			switch (((NMHDR *)lParam)->code)
			{
			case NM_CLICK:
			case NM_RETURN:
				PNMLINK pNMLink = (PNMLINK)lParam;
				LITEM item = pNMLink->item;
				ShellExecute(NULL, L"open", item.szUrl, NULL, NULL, SW_SHOW);
				break;
			}
		}
		break;
	}
	return TRUE;
}

void FireRenderParamDlg::CGeneralSettings::InitDialog()
{
	IParamBlock2* pb = mOwner->renderer->GetParamBlock(0);

	controls.passLimit = SetupSpinner(pb, PARAM_PASS_LIMIT, IDC_PASSES, IDC_PASSES_S);
	controls.timeLimitH = SetupSpinner(pb, PARAM_TIME_LIMIT, IDC_TIME_H, IDC_TIME_H_S);
	controls.timeLimitM = SetupSpinner(pb, PARAM_TIME_LIMIT, IDC_TIME_M, IDC_TIME_M_S);
	controls.timeLimitS = SetupSpinner(pb, PARAM_TIME_LIMIT, IDC_TIME_S, IDC_TIME_S_S);
	const int timeLimit = pb->GetInt(PARAM_TIME_LIMIT);
	controls.timeLimitH->SetLimits(0, 24 * 7);
	controls.timeLimitH->SetValue(timeLimit / 3600, TRUE);
	controls.timeLimitM->SetLimits(0, 59);
	controls.timeLimitM->SetValue((timeLimit / 60) % 60, TRUE);
	controls.timeLimitS->SetLimits(0, 59);
	controls.timeLimitS->SetValue(timeLimit % 60, TRUE);

	int renderLimitType = 0;
	BOOL res = pb->GetValue(PARAM_RENDER_LIMIT, 0, renderLimitType, Interval());
	FASSERT(res);
	switch (renderLimitType)
	{
		case RPR_RENDER_LIMIT_PASS:
			CheckRadioButton(IDC_RENDER_PASS);
			EnableRenderLimitControls(TRUE, FALSE);
			break;
		case RPR_RENDER_LIMIT_TIME:
			CheckRadioButton(IDC_RENDER_TIME);
			EnableRenderLimitControls(FALSE, FALSE);
			break;
		case RPR_RENDER_LIMIT_UNLIMITED:
			CheckRadioButton(IDC_RENDER_UNLIMITED);
			EnableRenderLimitControls(FALSE, TRUE);
			break;
	}
	
	SetupCheckbox(pb, PARAM_STAMP_ENABLED, IDC_ENABLE_STAMP);

	const TCHAR* stampText = NULL;
	pb->GetValue(PARAM_STAMP_TEXT, 0, stampText, Interval());
	HWND stampEdit = GetDlgItem(mHwnd, IDC_STAMP_TEXT);
	FASSERT(stampEdit);
	SetWindowText(stampEdit, stampText);
	
	// setup some tooltips
	GetToolTipExtender().SetToolTip(GetDlgItem(mHwnd, IDC_STAMP_RESET), _T("Reset RenderStamp to default value"));
	GetToolTipExtender().SetToolTip(GetDlgItem(mHwnd, IDC_STAMP_HELP), _T("Display tokens which could be used in RenderStamp"));
	
	mIsReady = true;
}

void FireRenderParamDlg::CGeneralSettings::DestroyDialog()
{
	if (mIsReady)
	{
		mIsReady = false;
		RemoveAllSpinnerAssociations();
		ReleaseISpinner(controls.timeLimitS);
		ReleaseISpinner(controls.timeLimitM);
		ReleaseISpinner(controls.timeLimitH);
		ReleaseISpinner(controls.passLimit);
	}
}

void FireRenderParamDlg::CGeneralSettings::EnableRenderLimitControls(BOOL enablePassLimit, BOOL disableAll)
{
	EnableControl(IDC_PASSES, (disableAll ? FALSE : enablePassLimit));
	EnableControl(IDC_PASSES_S, (disableAll ? FALSE : enablePassLimit));

	EnableControl(IDC_TIME_H, (disableAll ? FALSE : !enablePassLimit));
	EnableControl(IDC_TIME_H_S, (disableAll ? FALSE : !enablePassLimit));

	EnableControl(IDC_TIME_M, (disableAll ? FALSE : !enablePassLimit));
	EnableControl(IDC_TIME_M_S, (disableAll ? FALSE : !enablePassLimit));

	EnableControl(IDC_TIME_S, (disableAll ? FALSE : !enablePassLimit));
	EnableControl(IDC_TIME_S, (disableAll ? FALSE : !enablePassLimit));
}

//////////////////////////////////////////////////////////////////////////////
// HARDWARE SETTINGS ROLLOUT
//

INT_PTR FireRenderParamDlg::CHardwareSettings::DlgProc(UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_COMMAND:
		if (mIsReady && HIWORD(wParam) == BN_CLICKED)
		{
			int idButton = (int) LOWORD(wParam);

			if (IDC_CPU_OVERRIDE_THREADS == idButton)
			{
				BOOL isChecked = IsDlgButtonChecked(mHwnd, IDC_CPU_OVERRIDE_THREADS);
				
				isChecked ? controls.spinnerCpuThreadsNum->Enable() : controls.spinnerCpuThreadsNum->Disable();
				ScopeManagerMax::TheManager.cpuInfo.isCpuThreadsNumOverriden = bool_cast( isChecked );
			}
		}
		break;
	
	case CC_SPINNER_CHANGE:
		if (mIsReady)
		{
			int idSpinner = (int) LOWORD(wParam);
			ScopeManagerMax::TheManager.cpuInfo.numCpuThreads = controls.spinnerCpuThreadsNum->GetIVal();
		}
		break;

		case WM_NOTIFY:
		{
			if (LOWORD(wParam) == IDC_LIST_HW)
			{
				if (((LPNMHDR)lParam)->code == LVN_ITEMCHANGED)
				{
					LPNMLISTVIEW pnmv = (LPNMLISTVIEW)lParam;

					if (pnmv->uChanged & LVIF_STATE)
					{
						switch (pnmv->uNewState & LVIS_STATEIMAGEMASK)
						{
						case INDEXTOSTATEIMAGEMASK(2):
							// pnmv->iItem was checked
							UpdateHwSelected(pnmv->iItem);
							break;

						case INDEXTOSTATEIMAGEMASK(1):
							// pnmv->iItem was unchecked
							UpdateHwSelected(pnmv->iItem);
							break;
						}
					}
				}
			}
		}
		break;

	default:
		return FALSE;
	}

	return TRUE;
}

void FireRenderParamDlg::CHardwareSettings::InitDialog()
{
	IParamBlock2* pb = mOwner->renderer->GetParamBlock(0);
	
	int numCpuThreads = ScopeManagerMax::TheManager.cpuInfo.numCpuThreads;

	if (0 == numCpuThreads)
	{
		numCpuThreads = std::thread::hardware_concurrency();

		if (numCpuThreads < cpuThreadsMin)
			numCpuThreads = cpuThreadsMin;

		if (numCpuThreads > cpuThreadsMax)
			numCpuThreads = cpuThreadsMax;

		ScopeManagerMax::TheManager.cpuInfo.numCpuThreads = numCpuThreads;
	}

	// setup spinner
	HWND textControl = GetDlgItem(mHwnd, IDC_CPU_NUM_THREADS);
	HWND spinControl = GetDlgItem(mHwnd, IDC_CPU_NUM_THREADS_S);

	controls.spinnerCpuThreadsNum = GetISpinner(spinControl);
	controls.spinnerCpuThreadsNum->LinkToEdit(textControl, EDITTYPE_INT);
	controls.spinnerCpuThreadsNum->SetResetValue(numCpuThreads);
	controls.spinnerCpuThreadsNum->SetLimits(cpuThreadsMin, cpuThreadsMax);
	controls.spinnerCpuThreadsNum->SetAutoScale(TRUE);
	controls.spinnerCpuThreadsNum->SetValue(numCpuThreads, FALSE);

	// setup checkbox
	bool isCpuThreadsOverriden = ScopeManagerMax::TheManager.cpuInfo.isCpuThreadsNumOverriden;

	SetCheckboxValue(isCpuThreadsOverriden, IDC_CPU_OVERRIDE_THREADS);
	isCpuThreadsOverriden ? controls.spinnerCpuThreadsNum->Enable() : controls.spinnerCpuThreadsNum->Disable();

	// fill in devices lists
	int numberCompatibleGPUs = 0;
	bool hasCompatibleGPU = ScopeManagerMax::TheManager.hasGpuCompatibleWithFR(numberCompatibleGPUs);
	
	// if there is no compatible GPU select CPU
	if (!hasCompatibleGPU)
	{
		ScopeManagerMax::TheManager.cpuInfo.isUsed = true;
	}

	InitGPUCompatibleList();

	mIsReady = true;
}

void FireRenderParamDlg::CHardwareSettings::DestroyDialog()
{
	if (mIsReady)
	{
		mIsReady = false;
		ReleaseISpinner(controls.spinnerCpuThreadsNum);
	}
	// Store any UI changes as preferences for the future
	ScopeManagerMax::TheManager.SaveAttributeSettings();
}

void FireRenderParamDlg::CHardwareSettings::InitGPUCompatibleList()
{
	HWND hListView = GetDlgItem(mHwnd, IDC_LIST_HW);
	ListView_SetExtendedListViewStyle(hListView, LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

	const int numGPU = int_cast( ScopeManagerMax::TheManager.gpuInfoArray.size() );

	// add GPU(s) info
	for (int i = 0; i < numGPU; i++)
	{
		const GpuInfo& gpuInfo = ScopeManagerMax::TheManager.gpuInfoArray[i];

		LVITEMA item{ 0 };
		item.mask = LVIF_TEXT;
		item.iItem = i;

		std::string fullNameItem = gpuInfo.name;
		
		if ( !gpuInfo.isWhiteListed )
		{
			fullNameItem += " (Device NOT certified)";
		}

		item.pszText = (char*) fullNameItem.c_str();

		bool isUsed = gpuInfo.isUsed;

		SendMessage(hListView, LVM_INSERTITEMA, 0, (LPARAM) &item);

		if (isUsed)
		{
			ListView_SetCheckState(hListView, i, 1);
		}
	}

	// add CPU info
	int cpuIndex = numGPU;

	LVITEMA item{ 0 };
	item.mask = LVIF_TEXT;
	item.iItem = cpuIndex;
	item.pszText = (char*) "CPU";

	bool isUsed = ScopeManagerMax::TheManager.cpuInfo.isUsed;

	SendMessage(hListView, LVM_INSERTITEMA, 0, (LPARAM) &item);
	
	if (isUsed)
	{
		ListView_SetCheckState(hListView, cpuIndex, 1);
	}
}

void FireRenderParamDlg::CHardwareSettings::UpdateHwSelected(int deviceIndex)
{
	IParamBlock2* pb = mOwner->renderer->GetParamBlock(0);
	FASSERT(pb);

	HWND hListView = GetDlgItem(mHwnd, IDC_LIST_HW);

	if (ListView_GetItemCount(hListView) == 0)
		return;

	if (ScopeManagerMax::TheManager.gpuInfoArray.size() == deviceIndex)
	{
		ScopeManagerMax::TheManager.cpuInfo.isUsed = bool_cast( ListView_GetCheckState(hListView, deviceIndex) );
	}
	else
	{
		GpuInfo& gpuInfo = ScopeManagerMax::TheManager.gpuInfoArray[deviceIndex];
		gpuInfo.isUsed = bool_cast( ListView_GetCheckState(hListView, deviceIndex) );
	}
}


//////////////////////////////////////////////////////////////////////////////
// CAMERA SETTINGS ROLLOUT
//

void FireRenderParamDlg::CCameraSettings::ApplyFOV(BOOL useFOV)
{
	controls.cameraFOV->Enable(useFOV);
	controls.cameraFocalLength->Enable(!useFOV);

	SetVisibleDlg( bool_cast(!useFOV), IDC_DOF_STATIC_FL);
	SetVisibleDlg( bool_cast(useFOV), IDC_DOF_STATIC_SF);

	float sensorWidth = controls.cameraSensorSize->GetFVal();
	float FOVvalue = controls.cameraFOV->GetFVal() * PI / 180.f;
	float focalLength = controls.cameraFocalLength->GetFVal();
	
	if (useFOV)
		FOVvalue = CalcFov(sensorWidth, focalLength);
	else
		focalLength = CalcFocalLength(sensorWidth, FOVvalue);

	CamManagerMax::TheManager.SetProperty(PARAM_CAM_USE_FOV, useFOV);
	CamManagerMax::TheManager.SetProperty(PARAM_CAM_FOCAL_LENGTH, focalLength);
	CamManagerMax::TheManager.SetProperty(PARAM_CAM_FOV, FOVvalue);
}

float FireRenderParamDlg::CCameraSettings::CalcFov(float sensorWidth, float focalLength)
{
	return std::atanf(sensorWidth / (focalLength * 2.f)) * 2.0;// *180.f / PI;
}

float FireRenderParamDlg::CCameraSettings::CalcFocalLength(float sensorWidth, float fov)
{
	return sensorWidth / (2.f * std::tanf(fov * 0.5));
}

INT_PTR FireRenderParamDlg::CCameraSettings::DlgProc(UINT msg, WPARAM wParam, LPARAM lParam)
{
	IParamBlock2* pb = mOwner->renderer->GetParamBlock(0);
	switch (msg)
	{
		case WM_COMMAND:
		{
			int controlId = LOWORD(wParam);
			int submsg = HIWORD(wParam);
			if (submsg == CBN_SELCHANGE)
			{
				int idComboBox = (int)LOWORD(wParam);
				if (idComboBox == IDC_CAMERA_TYPES)
				{
					HWND hwndComboBox = (HWND)lParam;
					int dummy = int(SendMessage(hwndComboBox, CB_GETCURSEL, 0, 0));
					FASSERT(dummy != CB_ERR);
					int val = (SendMessage(hwndComboBox, CB_GETITEMDATA, WPARAM(dummy), 0));
					FASSERT(val >= 0 && val < FRCameraType_Count);
					CamManagerMax::TheManager.SetProperty(PARAM_CAM_TYPE, val);
				}
			}
			else if (submsg == 0) 
			{
				if(mIsReady)
					switch (controlId)
					{
					case IDC_OVERWRITE_DOF_SETTINGS:
					{
						BOOL res = CamManagerMax::TheManager.SetProperty(PARAM_CAM_OVERWRITE_DOF_SETTINGS, BOOL(IsDlgButtonChecked(mHwnd, IDC_OVERWRITE_DOF_SETTINGS))); FASSERT(res);
					}
					break;

					case IDC_USE_DOF:
					{
						BOOL useDOF = BOOL(IsDlgButtonChecked(mHwnd, IDC_USE_DOF));
						BOOL res = CamManagerMax::TheManager.SetProperty(PARAM_CAM_USE_DOF, useDOF); FASSERT(res);
						SetVisibleDOFSettings( bool_cast(useDOF) );
					}
					break;

					case IDC_USE_FOV:
					{
						ApplyFOV(IsDlgButtonChecked(mHwnd, IDC_USE_FOV));
					}
					break;

					case IDC_USE_MOTION_BLUR:
					{
						BOOL res = CamManagerMax::TheManager.SetProperty(PARAM_CAM_USE_MOTION_BLUR, BOOL(IsDlgButtonChecked(mHwnd, IDC_USE_MOTION_BLUR))); FASSERT(res);
					}
					break;
					}
			}
		}
		break;

		case CC_SPINNER_CHANGE:
		{
			if (mIsReady)
			{
				int controlId = LOWORD(wParam);
				if (controlId == IDC_FOCUS_DIST_S)
				{
					float  focusDistance = controls.cameraPerspectiveFocalDist->GetFVal() * getSystemUnitsToMillimeters() / 1000.f;
					CamManagerMax::TheManager.SetProperty(PARAM_CAM_FOCUS_DIST, focusDistance);
				}
				else if (controlId == IDC_FOV_S)
				{
					float fov = controls.cameraFOV->GetFVal() / 180 * PI;
					CamManagerMax::TheManager.SetProperty(PARAM_CAM_FOV, fov);
					

					// Update focal length value
					float sensorWidth;
					CamManagerMax::TheManager.GetProperty(PARAM_CAM_SENSOR_WIDTH, sensorWidth);

					float focalLength = CalcFocalLength(sensorWidth, fov);

					if (!isinf(focalLength))
					{
						controls.cameraFocalLength->SetValue(focalLength, false);
						CamManagerMax::TheManager.SetProperty(PARAM_CAM_FOCAL_LENGTH, focalLength);
					}
				}
				else if (controlId == IDC_FOCAL_LENGTH_S)
				{
					CommitSpinnerToParam(CamManagerMax::TheManager, controlId);

					// Update fov value
					float sensorWidth;
					float focalLength;
					CamManagerMax::TheManager.GetProperty(PARAM_CAM_SENSOR_WIDTH, sensorWidth);
					CamManagerMax::TheManager.GetProperty(PARAM_CAM_FOCAL_LENGTH, focalLength);

					float fov = CalcFov(sensorWidth, focalLength);

					if (!isinf(fov))
					{
						controls.cameraFOV->SetValue(fov / PI * 180, false);
						CamManagerMax::TheManager.SetProperty(PARAM_CAM_FOV, fov);
					}
				}
				else
					CommitSpinnerToParam(CamManagerMax::TheManager, controlId);
			}
		}
		break;

		default:
			return FALSE;
	}

	return TRUE;
}

void FireRenderParamDlg::CCameraSettings::InitDialog()
{
	IParamBlock2* pb = mOwner->renderer->GetParamBlock(0);
	
	SetupCheckbox(CamManagerMax::TheManager, PARAM_CAM_USE_FOV, IDC_USE_FOV);
	SetupCheckbox(CamManagerMax::TheManager, PARAM_CAM_USE_DOF, IDC_USE_DOF);
	SetupCheckbox(CamManagerMax::TheManager, PARAM_CAM_OVERWRITE_DOF_SETTINGS, IDC_OVERWRITE_DOF_SETTINGS);

	bool useDof = IsDlgButtonChecked(mHwnd, IDC_USE_DOF) == 1;
	SetVisibleDOFSettings(useDof);

	SetupCheckbox(CamManagerMax::TheManager, PARAM_CAM_USE_MOTION_BLUR, IDC_USE_MOTION_BLUR);
	controls.motionBlurScale = SetupSpinnerFloat(CamManagerMax::TheManager, PARAM_CAM_MOTION_BLUR_SCALE, IDC_MOTION_BLUR_SCALE, IDC_MOTION_BLUR_SCALE_S, 0.f, 10e20f);

	constexpr float minCameraExposure = -10e20f;
	constexpr float maxCameraExposure =  10e20f;

	controls.cameraPerspectiveFocalDist = SetupSpinnerFloat(CamManagerMax::TheManager, PARAM_CAM_FOCUS_DIST, IDC_FOCUS_DIST, IDC_FOCUS_DIST_S, 1e-6f, 10e20f);
	//stored in meters we want to show it in cm
	controls.cameraPerspectiveFocalDist->SetValue(this->controls.cameraPerspectiveFocalDist->GetFVal()*100.f, false);
	controls.cameraBlades = SetupSpinnerInt(CamManagerMax::TheManager, PARAM_CAM_BLADES, IDC_APERTURE_BLADES, IDC_APERTURE_BLADES_S, 3, 64);
	controls.cameraFStop = SetupSpinnerFloat(CamManagerMax::TheManager, PARAM_CAM_F_STOP, IDC_FSTOP, IDC_FSTOP_S, 0.01f, 999.f);
	controls.cameraSensorSize = SetupSpinnerFloat(CamManagerMax::TheManager, PARAM_CAM_SENSOR_WIDTH, IDC_SENSOR_SIZE, IDC_SENSOR_SIZE_S, 0.001f, 9999.f);
	controls.cameraExposure = SetupSpinnerFloat(CamManagerMax::TheManager, PARAM_CAM_EXPOSURE, IDC_CAMERA_EXPOSURE, IDC_CAMERA_EXPOSURE_S, minCameraExposure, maxCameraExposure);
	controls.cameraFocalLength = SetupSpinnerFloat(CamManagerMax::TheManager, PARAM_CAM_FOCAL_LENGTH, IDC_FOCAL_LENGTH, IDC_FOCAL_LENGTH_S, 1e-6f, 10e20f);
	controls.cameraFOV = SetupSpinnerFloat(CamManagerMax::TheManager, PARAM_CAM_FOV, IDC_FOV, IDC_FOV_S, -360.f, 360.f);
	//stored in radian, we want to show it in degrees
	float val = this->controls.cameraFOV->GetFVal() / PI * 180.f;
	controls.cameraFOV->SetValue(val, false);

	bool useFov = IsDlgButtonChecked(mHwnd, IDC_USE_FOV) == 1;
	controls.cameraFocalLength->Enable(useDof && !useFov);
	controls.cameraFOV->Enable(useDof && useFov);

	InitCameraTypesDialog();

	mIsReady = true;
}

void FireRenderParamDlg::CCameraSettings::DestroyDialog()
{
	if (mIsReady)
	{
		mIsReady = false;
		RemoveAllSpinnerAssociations();
		ReleaseISpinner(controls.cameraBlades);
		ReleaseISpinner(controls.motionBlurScale);
		ReleaseISpinner(controls.cameraFStop);
		ReleaseISpinner(controls.cameraExposure);
		ReleaseISpinner(controls.cameraFocalLength);
		ReleaseISpinner(controls.cameraFOV);
		ReleaseISpinner(controls.cameraPerspectiveFocalDist);
		ReleaseISpinner(controls.cameraSensorSize);
	}
}

void FireRenderParamDlg::CCameraSettings::InitCameraTypesDialog()
{
	IParamBlock2* pb = mOwner->renderer->GetParamBlock(0);

	int cameraType = 0;
	BOOL res = CamManagerMax::TheManager.GetProperty(PARAM_CAM_TYPE, cameraType);
	FASSERT(res);

	HWND cameraTypesHwnd = GetDlgItem(mHwnd, IDC_CAMERA_TYPES);
	FASSERT(cameraTypesHwnd);
	SendMessage(cameraTypesHwnd, CB_RESETCONTENT, 0L, 0L);

	for (auto desc : cameraTypeDescs)
	{
		auto index = SendMessage(cameraTypesHwnd, CB_ADDSTRING, 0L, (LPARAM)desc.name);
		SendMessage(cameraTypesHwnd, CB_SETITEMDATA, index, desc.type);
	}

	SendMessage(cameraTypesHwnd, CB_SETCURSEL, cameraType, 0);

	const CameraTypeDesc &desc = cameraTypeDescs[cameraType];
}

void FireRenderParamDlg::CCameraSettings::SetVisibleDOFSettings(bool visible)
{
	int useFov;
	CamManagerMax::TheManager.GetProperty(PARAM_CAM_USE_FOV, useFov);

	//static texts:
	SetVisibleDlg(visible, IDC_DOF_STATIC_FD);
	SetVisibleDlg(visible, IDC_DOF_STATIC_SW);
	SetVisibleDlg(visible, IDC_DOF_STATIC_FS);
	SetVisibleDlg(visible, IDC_DOF_STATIC_AB);
	SetVisibleDlg(visible && !useFov, IDC_DOF_STATIC_FL);
	SetVisibleDlg(visible && useFov, IDC_DOF_STATIC_SF);

	//spinners:
	SetVisibleDlg(visible, IDC_FOCUS_DIST);
	SetVisibleDlg(visible, IDC_SENSOR_SIZE);
	SetVisibleDlg(visible, IDC_FSTOP);
	SetVisibleDlg(visible, IDC_APERTURE_BLADES);
	SetVisibleDlg(visible && !useFov, IDC_FOCAL_LENGTH);
	SetVisibleDlg(visible && useFov, IDC_FOV);

	SetVisibleDlg(visible, IDC_FOCUS_DIST_S);
	SetVisibleDlg(visible, IDC_SENSOR_SIZE_S);
	SetVisibleDlg(visible, IDC_FSTOP_S);
	SetVisibleDlg(visible, IDC_APERTURE_BLADES_S);
	SetVisibleDlg(visible && !useFov, IDC_FOCAL_LENGTH_S);
	SetVisibleDlg(visible && useFov, IDC_FOV_S);

	SetVisibleDlg(visible, IDC_USE_FOV);
}


//////////////////////////////////////////////////////////////////////////////
// TONEMAPPER SETTINGS ROLLOUT
//

INT_PTR FireRenderParamDlg::CTonemapSettings::DlgProc(UINT msg, WPARAM wParam, LPARAM lParam)
{
	IParamBlock2* pb = mOwner->renderer->GetParamBlock(0);

	switch (msg)
	{
		case WM_COMMAND:
		{
			if (mIsReady)
			{
				int controlId = LOWORD(wParam);
				int submsg = HIWORD(wParam);
				BOOL res;
				if (submsg == 0)
					switch (controlId)
					{
					case IDC_OVERRIDE: res = TmManagerMax::TheManager.SetProperty(PARAM_TM_OVERRIDE_MAX_TONEMAPPERS, BOOL(IsDlgButtonChecked(mHwnd, IDC_OVERRIDE))); FASSERT(res); break;
					case IDR_NONE: res = TmManagerMax::TheManager.SetProperty(PARAM_TM_OPERATOR, frw::ToneMappingTypeNone); FASSERT(res); break;
					case IDR_NONLINEAR: res = TmManagerMax::TheManager.SetProperty(PARAM_TM_OPERATOR, frw::ToneMappingTypeNonLinear); FASSERT(res); break;
					case IDR_LINEAR: res = TmManagerMax::TheManager.SetProperty(PARAM_TM_OPERATOR, frw::ToneMappingTypeLinear); FASSERT(res); break;
					case IDR_SIMPLIFIED: res = TmManagerMax::TheManager.SetProperty(PARAM_TM_OPERATOR, frw::ToneMappingTypeSimplified); FASSERT(res); break;
					}
			}
		}
		break;

		case CC_SPINNER_CHANGE:
		{
			if (mIsReady)
			{
				int controlId = LOWORD(wParam);
				CommitSpinnerToParam(TmManagerMax::TheManager, controlId);
			}
		}
		break;

		case CC_COLOR_CHANGE:
		{
			if (mIsReady)
			{
				int controlId = LOWORD(wParam);
				switch (controlId)
				{
					case IDC_TINT:
					{
						Color col = controls.tintcolor->GetAColor();
						TmManagerMax::TheManager.SetProperty(PARAM_TM_SIMPLIFIED_TINTCOLOR, col, PROP_SENDER_COLORDIAL);
					}
					break;

				}
			}
		}
		break;

		default:
			return FALSE;
	}

	return TRUE;
}

void FireRenderParamDlg::CTonemapSettings::InitDialog()
{
	IParamBlock2* pb = mOwner->renderer->GetParamBlock(0);

	TmManagerMax::TheManager.RegisterPropertyChangeCallback(this);

	SetupCheckbox(TmManagerMax::TheManager, PARAM_TM_OVERRIDE_MAX_TONEMAPPERS, IDC_OVERRIDE);
	
	controls.burn = SetupSpinnerFloat(TmManagerMax::TheManager, PARAM_TM_REINHARD_BURN, IDC_BURN, IDC_BURN_S, 0.f, FLT_MAX);
	controls.preScale = SetupSpinnerFloat(TmManagerMax::TheManager, PARAM_TM_REINHARD_PRESCALE, IDC_PRESCALE, IDC_PRESCALE_S, 0.f, FLT_MAX);
	controls.postScale = SetupSpinnerFloat(TmManagerMax::TheManager, PARAM_TM_REINHARD_POSTSCALE, IDC_POSTSCALE, IDC_POSTSCALE_S, 0.f, FLT_MAX);

	controls.iso = SetupSpinnerFloat(TmManagerMax::TheManager, PARAM_TM_PHOTOLINEAR_ISO, IDC_ISO, IDC_ISO_S, 0.f, FLT_MAX);
	controls.fstop = SetupSpinnerFloat(TmManagerMax::TheManager, PARAM_TM_PHOTOLINEAR_FSTOP, IDC_FSTOP, IDC_FSTOP_S, 0.f, FLT_MAX);
	controls.shutterspeed = SetupSpinnerFloat(TmManagerMax::TheManager, PARAM_TM_PHOTOLINEAR_SHUTTERSPEED, IDC_SHUTTERSPEED, IDC_SHUTTERSPEED_S, 0.f, FLT_MAX);

	controls.exposure = SetupSpinnerFloat(TmManagerMax::TheManager, PARAM_TM_SIMPLIFIED_EXPOSURE, IDC_EXPOSURE, IDC_EXPOSURE_S, -FLT_MAX, FLT_MAX);
	controls.contrast = SetupSpinnerFloat(TmManagerMax::TheManager, PARAM_TM_SIMPLIFIED_CONTRAST, IDC_CONTRAST, IDC_CONTRAST_S, 0.f, FLT_MAX);
	controls.whitebalance = SetupSpinnerInt(TmManagerMax::TheManager, PARAM_TM_SIMPLIFIED_WHITEBALANCE, IDC_WHITEBALANCE, IDC_WHITEBALANCE_S, 1000, 12000);

	{
		Color col;
		BOOL res = TmManagerMax::TheManager.GetProperty(PARAM_TM_SIMPLIFIED_TINTCOLOR, col);
		FASSERT(res);

		HWND swatchControl = GetDlgItem(mHwnd, IDC_TINT);
		FASSERT(swatchControl);

		IColorSwatch* swatch = GetIColorSwatch(swatchControl);
		swatch->SetColor(col);

		controls.tintcolor = swatch;

		// Temporarily disable tint color, Core doesn't support it yet
		ShowWindow(swatchControl, SW_HIDE);
	}


	int tonemapType = 0;
	BOOL res = TmManagerMax::TheManager.GetProperty(PARAM_TM_OPERATOR, tonemapType); FASSERT(res);
	switch (tonemapType)
	{
		case 0:
			CheckRadioButton(IDR_NONE);
		break;

		case 1:
			CheckRadioButton(IDR_SIMPLIFIED);
		break;

		case 2:
			CheckRadioButton(IDR_LINEAR);
		break;

		case 3:
			CheckRadioButton(IDR_NONLINEAR);
		break;

		default:
			CheckRadioButton(IDR_NONE);
		break;
	}

	mIsReady = true;
}

void FireRenderParamDlg::CTonemapSettings::DestroyDialog()
{
	if (mIsReady)
	{
		TmManagerMax::TheManager.UnregisterPropertyChangeCallback(this);

		mIsReady = false;
		RemoveAllSpinnerAssociations();

		ReleaseISpinner(controls.burn);
		ReleaseISpinner(controls.preScale);
		ReleaseISpinner(controls.postScale);

		ReleaseISpinner(controls.iso);
		ReleaseISpinner(controls.fstop);
		ReleaseISpinner(controls.shutterspeed);

		ReleaseISpinner(controls.exposure);
		ReleaseISpinner(controls.contrast);
		ReleaseISpinner(controls.whitebalance);
		ReleaseIColorSwatch(controls.tintcolor);
	}
}

void FireRenderParamDlg::CTonemapSettings::propChangedCallback(class ManagerMaxBase *owner, int paramId, int sender)
{
}


//////////////////////////////////////////////////////////////////////////////
// ADVANCED SETTINGS ROLLOUT
//

#define RTF_ABOUT_SIZE 10187

namespace
{
	DWORD CALLBACK EditStreamCallback(DWORD_PTR dwCookie, LPBYTE lpBuff, LONG cb, PLONG pcb)
	{
		std::vector<BYTE> *vecByte = (std::vector<BYTE> *)dwCookie;

		if (vecByte->size() < cb)
		{
			*pcb = LONG_cast( vecByte->size() );
			memcpy(lpBuff, vecByte->data(), *pcb);
			vecByte->clear();
		}
		else
		{
			*pcb = cb;
			memcpy(lpBuff, vecByte->data(), *pcb);
			std::vector<BYTE> temp(vecByte->size() - (*pcb));
			std::copy(vecByte->begin() + cb, vecByte->end(), temp.begin());
			*vecByte = temp;
		}
		return 0;
	}

	HMODULE GetCurrentModule()
	{
		HMODULE hModule = NULL;
		GetModuleHandleEx(
			GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
			(LPCTSTR)GetCurrentModule,
			&hModule);

		return hModule;
	}

	RECT aboutDialogRect;

	INT_PTR CALLBACK AboutDlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		switch (message)
		{
		case WM_INITDIALOG:
		{
			GetWindowRect(hWnd, &aboutDialogRect);
			
			HMODULE hModule = GetCurrentModule();

			// First search for the resource of type "RTF_RESOURCE"
			// from the current application with ID IDR_RTF_RESOURCE2.
			HRSRC hRTFResource = FindResource(hModule, MAKEINTRESOURCE(IDR_RTF_ABOUT), TEXT("RTF_RESOURCE"));

			if (hRTFResource)
			{
				// Next load the resource into memory. 
				HGLOBAL hGlobalRTFResource = LoadResource(hModule, hRTFResource);
				if (hGlobalRTFResource)
				{
					// Next lock the resource in memory and obtain
					// a pointer to it.
					LPVOID lpvRTFResource = LockResource(hGlobalRTFResource);
					if (lpvRTFResource)
					{
						// Unfortunately, there is no API to determine the size of
						// a custom resource so we have to pre-determine this size.
						std::vector<BYTE> vecByte;
						vecByte.resize(RTF_ABOUT_SIZE, 0);

						// Copy the RTF resource into our BYTE array.  
						memcpy(vecByte.data(), lpvRTFResource, RTF_ABOUT_SIZE);

						EDITSTREAM es;
						es.dwCookie = (DWORD_PTR)&vecByte;
						es.dwError = 0;
						es.pfnCallback = EditStreamCallback;

						//wrap
						SendMessage(GetDlgItem(hWnd, IDC_RICHEDIT21), EM_STREAMIN, SF_RTF, (LPARAM)&es);
					}
				}
			}
		}
		break;

		case WM_SIZE:
		{
			RECT newRect;
			GetWindowRect(hWnd, &newRect);

			int dx = (newRect.right - newRect.left) - (aboutDialogRect.right - aboutDialogRect.left);
			int dy = (newRect.bottom - newRect.top) - (aboutDialogRect.bottom - aboutDialogRect.top);

			RECT r;
			
			GetWindowRect(GetDlgItem(hWnd, IDC_RICHEDIT21), &r);
			MapWindowPoints(HWND_DESKTOP, hWnd, (LPPOINT)&r, 2);
						
			int nw = (r.right - r.left) + dx;
			int nh = (r.bottom  - r.top) + dy;
			MoveWindow(GetDlgItem(hWnd, IDC_RICHEDIT21), r.left, r.top, nw, nh, TRUE);

			GetWindowRect(GetDlgItem(hWnd, IDOK), &r);
			MapWindowPoints(HWND_DESKTOP, hWnd, (LPPOINT)&r, 2);

			MoveWindow(GetDlgItem(hWnd, IDOK), r.left + dx, r.top + dy, (r.right - r.left), (r.bottom - r.top), TRUE);

			aboutDialogRect = newRect;
		}
		break;

		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case IDOK:
				case IDCANCEL:
				return EndDialog(hWnd, wParam);
			}
			break;

		case WM_CLOSE:
			return EndDialog(hWnd, 0);
		}
		return FALSE;
	}
};

INT_PTR FireRenderParamDlg::CAdvancedSettings::DlgProc(UINT msg, WPARAM wParam, LPARAM lParam)
{
	IParamBlock2* pb = mOwner->renderer->GetParamBlock(0);
	switch (msg)
	{
		case WM_COMMAND:
		{
			switch (HIWORD(wParam))
			{
				case BN_CLICKED:
				{
					int idButton = (int)LOWORD(wParam);
					if (idButton == IDC_UNLIMITED_BOUNCE)
					{
						EnableLimitLightBounceControls(FALSE);
					}
					else if (idButton == IDC_LIMIT_BOUNCES)
					{
						EnableLimitLightBounceControls(TRUE);
					}
					else if (idButton == IDC_EXPORTMODEL_CHECK)
					{
						BOOL isChecked = IsDlgButtonChecked(mHwnd, IDC_EXPORTMODEL_CHECK);
						EnableExportModelControls(isChecked);
						if (isChecked)
							GetCOREInterface()->ReplacePrompt(L"FRS file will be exported after rendering.");
						BOOL res = pb->SetValue(PARAM_EXPORTMODEL_CHECK, 0, isChecked); FASSERT(res);
					}
					else if (idButton == IDC_BROWSE_EXPORT_FILE)
					{
						if (GetExportFileName())
						{
							SetDlgItemText(mHwnd, IDC_EXPORT_FILENAME, exportFRSceneFileName.c_str());
							mOwner->renderer->SetFireRenderExportSceneFilename(exportFRSceneFileName);
						}
					}
					else if (idButton == IDC_CLAMP_IRRADIANCE)
					{
						BOOL isChecked = Button_GetCheck(GetDlgItem(mHwnd, IDC_CLAMP_IRRADIANCE));
						EnableIrradianceClampControls(isChecked);
						BOOL res = pb->SetValue(PARAM_USE_IRRADIANCE_CLAMP, 0, isChecked); FASSERT(res);
					}
					else if (idButton == IDC_WARNINGS_SUPPRESS_GLOBAL)
					{
						BOOL on = BOOL(IsDlgButtonChecked(mHwnd, idButton));
						FRSettingsFileHandler::setAttributeSettingsFor(FRSettingsFileHandler::GlobalDontShowNotification, std::to_string(on));
					}
					else if (idButton == IDC_OPEN_TRACE_DIR)
					{
						std::wstring path = ScopeManagerMax::TheManager.GetRPRTraceDirectory(mOwner->renderer->GetParamBlock(0));
						CreateDirectory(path.c_str(), NULL); // Ensure directory exists
						ShellExecute(NULL, L"open", path.c_str(), NULL, NULL, SW_SHOW);
					}
					else if (idButton == IDC_OVERRIDE_MAX_PREVIEW_SHAPES)
					{
						BOOL on = BOOL(IsDlgButtonChecked(mHwnd, idButton));
						pb->SetValue(PARAM_OVERRIDE_MAX_PREVIEW_SHAPES, 0, on);
					}
					else if (idButton == IDC_TRACEDUMP_CHECK)
					{
						BOOL res = pb->SetValue(PARAM_TRACEDUMP_BOOL, 0, BOOL(IsDlgButtonChecked(mHwnd, IDC_TRACEDUMP_CHECK))); FASSERT(res);
					}
					else if (idButton == IDC_TEXTURE_COMPRESSION_CHECK)
					{
						BOOL res = pb->SetValue(PARAM_USE_TEXTURE_COMPRESSION, 0, BOOL(IsDlgButtonChecked(mHwnd, IDC_TEXTURE_COMPRESSION_CHECK))); FASSERT(res);
					}
					else if (idButton == IDC_WARNINGS_SUPPRESS)
					{
						BOOL res = pb->SetValue(PARAM_WARNING_DONTSHOW, 0, BOOL(IsDlgButtonChecked(mHwnd, IDC_WARNINGS_SUPPRESS))); FASSERT(res);
					}
				}
				break;

				case CBN_SELCHANGE:
				{
					if (mIsReady)
					{
						int idCombo = (int)LOWORD(wParam);
						if (idCombo == IDC_RENDER_MODE)
						{
							HWND renderModeCtrl = (HWND)lParam;
							int dummy = int(SendMessage(renderModeCtrl, CB_GETCURSEL, 0, 0));
							FASSERT(dummy != CB_ERR);
							const int renderMode = int(SendMessage(renderModeCtrl, CB_GETITEMDATA, WPARAM(dummy), 0));
							FASSERT(renderMode != CB_ERR);
							BOOL res; res = pb->SetValue(PARAM_RENDER_MODE, 0, renderMode); FASSERT(res);
						}
					}
				}
				break;
			}
		}
		break;

		case WM_NOTIFY:
		{
			int idButton = (int)LOWORD(wParam);
			int code = ((LPNMHDR)lParam)->code;
			if ((idButton == IDC_ABOUT_LINKS1 || idButton == IDC_ABOUT_LINKS2) && (code == NM_CLICK || code == NM_RETURN))
			{
				// clicked SysLink in "about" dialog, we're using <a id="id"> tag to recognize clicked parts
				PNMLINK pNMLink = (PNMLINK)lParam;
				const TCHAR* id = pNMLink->item.szID;
				const TCHAR* url = NULL;
				// process action
				if (wcscmp(id, _T("doc")) == 0)
				{
					url = _T("https://www2.ati.com/relnotes/radeonprorender_3dsmax_user_guide_v1.0.pdf");
				}
				else if (wcscmp(id, _T("community")) == 0)
				{
					url = _T("https://community.amd.com/community/prorender");
				}
				else if (wcscmp(id, _T("main_site")) == 0)
				{
					url = _T("https://pro.radeon.com/en/software/prorender/3ds-max/");
				}
				else if (wcscmp(id, _T("downloads")) == 0)
				{
					url = _T("https://pro.radeon.com/en/software/prorender/3ds-max/");
				}
				else if (wcscmp(id, _T("bugs")) == 0)
				{
					url = _T("https://community.amd.com/community/prorender/3ds-max");
				}
				// execute URL
				if (url != NULL)
				{
					ShellExecute(NULL, L"open", url, NULL, NULL, SW_SHOW);
				}
				else
				{
					// this is a temporary placeholder for missing actions
		            MessageBox(GetCOREInterface()->GetMAXHWnd(), id, _T("Radeon ProRender: unknown action"), MB_OK);
				}

			}
			else if ((idButton == IDC_ABOUT2 || idButton == IDC_ABOUT3) && (code == NM_CLICK || code == NM_RETURN))
			{
				DialogBoxParam(FireRender::fireRenderHInstance, MAKEINTRESOURCE(IDD_ABOUT), Hwnd(), AboutDlgProc, 0);
			}
		}
		break;

		case CC_SPINNER_CHANGE:
		{
			if (mIsReady)
			{
				int controlId = LOWORD(wParam);
				CommitSpinnerToParam(pb, controlId);
				mOwner->mQualitySettings.setupUIFromData();
			}
		}
		break;

		default:
			return FALSE;
	}
	return TRUE;
}

extern "C" bool IsGltfExportEnabled();

void FireRenderParamDlg::CAdvancedSettings::InitDialog()
{
	LRESULT n;
	IParamBlock2* pb = mOwner->renderer->GetParamBlock(0);

	// Render mode dialog
	HWND renderMode = GetDlgItem(mHwnd, IDC_RENDER_MODE);
	FASSERT(renderMode);
	SendMessage(renderMode, CB_RESETCONTENT, 0L, 0L);

	n = SendMessage(renderMode, CB_ADDSTRING, 0L, (LPARAM)_T("GI"));
	SendMessage(renderMode, CB_SETITEMDATA, n, RPR_RENDER_MODE_GLOBAL_ILLUMINATION);
	n = SendMessage(renderMode, CB_ADDSTRING, 0L, (LPARAM)_T("Direct"));
	SendMessage(renderMode, CB_SETITEMDATA, n, RPR_RENDER_MODE_DIRECT_ILLUMINATION);
	n = SendMessage(renderMode, CB_ADDSTRING, 0L, (LPARAM)_T("Direct No shadows"));
	SendMessage(renderMode, CB_SETITEMDATA, n, RPR_RENDER_MODE_DIRECT_ILLUMINATION_NO_SHADOW);
	n = SendMessage(renderMode, CB_ADDSTRING, 0L, (LPARAM)_T("Wireframe"));
	SendMessage(renderMode, CB_SETITEMDATA, n, RPR_RENDER_MODE_WIREFRAME);
	n = SendMessage(renderMode, CB_ADDSTRING, 0L, (LPARAM)_T("Material Idx"));
	SendMessage(renderMode, CB_SETITEMDATA, n, RPR_RENDER_MODE_MATERIAL_INDEX);
	n = SendMessage(renderMode, CB_ADDSTRING, 0L, (LPARAM)_T("Position"));
	SendMessage(renderMode, CB_SETITEMDATA, n, RPR_RENDER_MODE_POSITION);
	n = SendMessage(renderMode, CB_ADDSTRING, 0L, (LPARAM)_T("Normal"));
	SendMessage(renderMode, CB_SETITEMDATA, n, RPR_RENDER_MODE_NORMAL);
	n = SendMessage(renderMode, CB_ADDSTRING, 0L, (LPARAM)_T("Tex coords"));
	SendMessage(renderMode, CB_SETITEMDATA, n, RPR_RENDER_MODE_TEXCOORD);
	n = SendMessage(renderMode, CB_ADDSTRING, 0L, (LPARAM)_T("Ambient occlusion"));
	SendMessage(renderMode, CB_SETITEMDATA, n, RPR_RENDER_MODE_AMBIENT_OCCLUSION);
	setListboxValue(renderMode, pb->GetInt(PARAM_RENDER_MODE));

	controls.meditQuality = SetupSpinner(pb, PARAM_MTL_PREVIEW_PASSES, IDC_MEDIT_QUALITY, IDC_MEDIT_QUALITY_S);
	controls.texmapSize = SetupSpinner(pb, PARAM_TEXMAP_RESOLUTION, IDC_TEXMAP_SIZE, IDC_TEXMAP_SIZE_S);

	SetupCheckbox(pb, PARAM_OVERRIDE_MAX_PREVIEW_SHAPES, IDC_OVERRIDE_MAX_PREVIEW_SHAPES);
	SetupCheckbox(pb, PARAM_TRACEDUMP_BOOL, IDC_TRACEDUMP_CHECK);
	SetupCheckbox(pb, PARAM_USE_TEXTURE_COMPRESSION, IDC_TEXTURE_COMPRESSION_CHECK);
	SetupCheckbox(pb, PARAM_WARNING_DONTSHOW, IDC_WARNINGS_SUPPRESS);

	{
		bool warningsSuppressGlobal = false;
		std::string temp = FRSettingsFileHandler::getAttributeSettingsFor(FRSettingsFileHandler::GlobalDontShowNotification);
		if (temp.length() > 0) {
			warningsSuppressGlobal = bool_cast( std::stoi(temp) );
		}
		SetCheckboxValue(warningsSuppressGlobal, IDC_WARNINGS_SUPPRESS_GLOBAL);
	}

	// setup export buttons
	HWND hExportCheckBox = GetDlgItem(mHwnd, IDC_EXPORTMODEL_CHECK);

	if ( IsGltfExportEnabled() )
	{
		Button_Enable(hExportCheckBox, TRUE);
		BOOL enableExport = FALSE;
		pb->GetValue(PARAM_EXPORTMODEL_CHECK, 0, enableExport, Interval());
		EnableExportModelControls(enableExport);
	}
	else
	{
		EnableExportModelControls(FALSE);
		Button_SetCheck(hExportCheckBox, FALSE);
		Button_Enable(hExportCheckBox, FALSE);
	}

	// setup irradiance
	SetupCheckbox(pb, PARAM_USE_IRRADIANCE_CLAMP, IDC_CLAMP_IRRADIANCE);
	BOOL useIrradianceClamp = FALSE;
	pb->GetValue(PARAM_USE_IRRADIANCE_CLAMP, 0, useIrradianceClamp, Interval());
	EnableIrradianceClampControls(useIrradianceClamp);

	controls.irradianceClamp = SetupSpinner(pb, PARAM_IRRADIANCE_CLAMP, IDC_CLAMP_IRRADIANCE_VAL, IDC_CLAMP_IRRADIANCE_VAL_S);
	controls.maxRayDepth = SetupSpinner(pb, PARAM_MAX_RAY_DEPTH, IDC_MAX_RAY_DEPTH, IDC_MAX_RAY_DEPTH_S);

	mIsReady = true;
}

void FireRenderParamDlg::CAdvancedSettings::DestroyDialog()
{
	if (mIsReady)
	{
		mIsReady = false;
		RemoveAllSpinnerAssociations();
		ReleaseISpinner(controls.irradianceClamp);
		ReleaseISpinner(controls.meditQuality);
		ReleaseISpinner(controls.texmapSize);
		ReleaseISpinner(controls.maxRayDepth);
	}
}

void FireRenderParamDlg::CAdvancedSettings::EnableLimitLightBounceControls(BOOL enableMaxLimit)
{
	HWND ctrl;
	ctrl = GetDlgItem(mHwnd, IDC_MAX_LIGHT_BOUNCES);
	FASSERT(ctrl);
	EnableWindow(ctrl, enableMaxLimit);
	ctrl = GetDlgItem(mHwnd, IDC_MAX_LIGHT_BOUNCES_S);
	FASSERT(ctrl);
	EnableWindow(ctrl, enableMaxLimit);
}

void FireRenderParamDlg::CAdvancedSettings::EnableIrradianceClampControls(BOOL enable)
{
	HWND ctrl = GetDlgItem(mHwnd, IDC_CLAMP_IRRADIANCE_VAL);
	FASSERT(ctrl);
	EnableWindow(ctrl, enable);
	ctrl = GetDlgItem(mHwnd, IDC_CLAMP_IRRADIANCE_VAL_S);
	FASSERT(ctrl);
	EnableWindow(ctrl, enable);
}

void FireRenderParamDlg::CAdvancedSettings::EnableExportModelControls(BOOL enableExport)
{
	HWND ctrl = GetDlgItem(mHwnd, IDC_EXPORT_FILENAME);
	FASSERT(ctrl);
	EnableWindow(ctrl, enableExport);
	ctrl = GetDlgItem(mHwnd, IDC_BROWSE_EXPORT_FILE);
	FASSERT(ctrl);
	EnableWindow(ctrl, enableExport);
}

BOOL FireRenderParamDlg::CAdvancedSettings::GetExportFileName()
{
	int tried = 0;
	FilterList filterList;
	HWND hWnd = mHwnd;
	static int filterIndex = 1;
	OPENFILENAME  ofn;
	std::wstring initialDir;
	TCHAR fname[MAX_PATH] = { '\0' };
	TCHAR my_documents[MAX_PATH];
	HRESULT result = SHGetFolderPath(NULL, CSIDL_PERSONAL, NULL, SHGFP_TYPE_CURRENT, my_documents);
	
	static const std::pair<const wchar_t*, const wchar_t*> filters[] = 
	{
		{ L"Radeon ProRender Format", L"rpr" },
		{ L"GL Transmission Format", L"gltf" },
	};

	if (result == S_OK)
	{
		initialDir = std::wstring(my_documents) + L"\\3dsMax\\export\\";
	}

	for (const auto& filter : filters)
	{
		std::wstring mask(L"*.");
		mask += filter.second;

		filterList.Append(filter.first);
		filterList.Append(mask.c_str());
	}

	memset(&ofn, 0, sizeof(ofn));

	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = hWnd;

	ofn.nFilterIndex = filterIndex;
	ofn.lpstrFilter = filterList;
	ofn.lpstrFile = fname;
	ofn.lpstrTitle = _T("Export Radeon ProRender Scene");
	ofn.nMaxFile = MAX_PATH;

	Interface *iface = GetCOREInterface();

	if (initialDir[0])
		ofn.lpstrInitialDir = initialDir.c_str();
	else
		ofn.lpstrInitialDir = iface->GetDir(APP_SCENE_DIR);

	ofn.Flags = OFN_HIDEREADONLY | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOREADONLYRETURN | OFN_EXPLORER | OFN_ENABLEHOOK | OFN_ENABLESIZING;
	ofn.lpfnHook = NULL;
	ofn.lCustData = 0;
	ofn.lpstrDefExt = filters[0].second;

	while (GetSaveFileName(&ofn))
	{
		exportFRSceneFileName = ofn.lpstrFile;

		return TRUE;
	}

	return FALSE;
}

//////////////////////////////////////////////////////////////////////////////
// SCRIPTS ROLLOUT
//

INT_PTR FireRenderParamDlg::CScripts::DlgProc(UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
		case WM_COMMAND:
		{
			switch (HIWORD(wParam))
			{
				case BN_CLICKED:
				{
					int idButton = (int)LOWORD(wParam);
					if (idButton == IDC_RUN_SCRIPT_BTN && selectedScriptID != RPR_SCRIPT_INVALID)
					{
						BOOL success = false;
						switch(selectedScriptID)
						{
							case RPR_CONVERT_MATERIALS_FROM_VRAY_TO_PRORENDER:
							{
								success = ExecuteMAXScriptScript(
											L"for o in geometry do											 \n"
											L"(																 \n"
											L"	if classof o.material == VRayMtl do							 \n"
											L"	(															 \n"
											L"		tt = Rpr_Uber_Material()								 \n"
											L"		tt.DiffuseColorTexmap = o.material.texmap_diffuse		 \n"
											L"		tt.DiffuseColor = o.material.diffuse					 \n"
											L"		tt.UseGlossy = true										 \n"
											L"		o.material = tt											 \n"
											L"	)															 \n"
											L"	if classof o.material == Multimaterial then					 \n"
											L"	for m = 1 to o.material.numsubs do							 \n"
											L"	(															 \n"
											L"		if classof o.material[m] == VRayMtl do					 \n"
											L"		(														 \n"
											L"			tt = Rpr_Uber_Material()							 \n"
											L"			tt.DiffuseColorTexmap = o.material[m].texmap_diffuse \n"
											L"			tt.DiffuseColor = o.material[m].diffuse				 \n"
											L"			tt.UseGlossy = true									 \n"
											L"			o.material[m] = tt									 \n"
											L"		)														 \n"
											L"	)															 \n"
											L")																 \n");
							}
							break;
						}
						

						if(success)
							MessageBox(GetCOREInterface()->GetMAXHWnd(), _T("Vray materials successfully converted to ProRender materials"), _T("Radeon ProRender"), MB_OK);
						else
							MessageBox(GetCOREInterface()->GetMAXHWnd(), _T("Error in script"), _T("Radeon ProRender exception"), MB_OK);
					}
				}
				break;
	
				case CBN_SELCHANGE:
				{
					if (mIsReady)
					{
						int idCombo = (int)LOWORD(wParam);
						if (idCombo == IDC_RENDER_SCRIPTS)
						{
							HWND scriptComboBox = (HWND)lParam;
							int selectionIndex = int(SendMessage(scriptComboBox, CB_GETCURSEL, 0, 0));
							if(selectionIndex >= 0)
							{
								selectedScriptID = int(SendMessage(scriptComboBox, CB_GETITEMDATA, WPARAM(selectionIndex), 0));
							}
						}
					}
				}
				break;
			}
		}
		break;
	
		default:
			return FALSE;
	}
	return TRUE;
}

void FireRenderParamDlg::CScripts::InitDialog()
{
	//Render device

	// disabled because shouldn't be in UI according to JIRA ticket
#ifdef VRAY_MATERIAL_CONVERTION_ENABLED
	LRESULT n;
	HWND renderScript = GetDlgItem(mHwnd, IDC_RENDER_SCRIPTS);
	FASSERT(renderScript);
	SendMessage(renderScript, CB_RESETCONTENT, 0L, 0L);
	n = SendMessage(renderScript, CB_ADDSTRING, 0L, (LPARAM)_T("Convert Vray materials to ProRender materials"));
	SendMessage(renderScript, CB_SETITEMDATA, n, RPR_CONVERT_MATERIALS_FROM_VRAY_TO_PRORENDER);
	mIsReady = true;
#endif
}

void FireRenderParamDlg::CScripts::DestroyDialog()
{
	mIsReady = false;
}


//////////////////////////////////////////////////////////////////////////////
// BACKGROUND SETTINGS ROLLOUT
//

namespace
{
	INT_PTR CALLBACK SearchDlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		switch (message)
		{
			case WM_INITDIALOG:
			{
				SetWindowLongPtr(hWnd, GWLP_USERDATA, lParam);
			}
			break;

			case WM_LOCSEARCHRESULT:
			{
				HWND results = GetDlgItem(hWnd, IDC_RESULTSBOX);
				int idx = (int)lParam;
				std::wstring display = ToUnicode(BgManagerMax::TheManager.GetCityDesc(idx));

				LRESULT item = SendMessage(results, LB_ADDSTRING, 0, (LPARAM)display.c_str());
				if (item != LB_ERR)
				{
					SendMessage(results, LB_SETITEMDATA, item, (LPARAM)idx);
				}
			}
			break;

			case WM_COMMAND:
				switch (LOWORD(wParam))
				{
					case IDOK:
					{
						size_t* city = reinterpret_cast<size_t*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
						HWND results = GetDlgItem(hWnd, IDC_RESULTSBOX);
						LRESULT item = SendMessage(results, LB_GETCURSEL, 0, 0);
						if (item != LB_ERR)
							*city = SendMessage(results, LB_GETITEMDATA, item, 0);
					}
					case IDCANCEL:
						return EndDialog(hWnd, wParam);

					case IDC_SEARCHBOX:
					{
						if (HIWORD(wParam) == EN_CHANGE)
						{
							HWND results = GetDlgItem(hWnd, IDC_RESULTSBOX);
							SendMessage(results, LB_RESETCONTENT, 0, 0);

							TCHAR buf[1024];
							GetWindowText((HWND)lParam, buf, 1024);
							std::wstring search = buf;
							
							if (search.size() > 2)
								BgManagerMax::TheManager.LookUpWorldCity(ToAscii(search), hWnd);
						}
					}
					return TRUE;

					case IDC_RESULTSBOX:
					{
						if (HIWORD(wParam) == LBN_SELCHANGE)
						{
							HWND results = GetDlgItem(hWnd, IDC_RESULTSBOX);
							LRESULT item = SendMessage(results, LB_GETCURSEL, 0, 0);
							if (item != LB_ERR)
							{
								size_t city = SendMessage(results, LB_GETITEMDATA, item, 0);
								HWND parent = GetParent(hWnd);

								const float *lat, *longi, *utcoff;
								BgManagerMax::TheManager.GetCityData(city, &lat, &longi, &utcoff);
								LATLONGSETDATA data;
								data.first = Point2(*longi, *lat);
								data.second = *utcoff;
								SendMessage(GetDlgItem(parent, IDC_MAP), WM_LATLONGSET, 0, (LPARAM)&data);
							}
						}
					}
					break;
				}
			break;

			case WM_CLOSE:
				return EndDialog(hWnd, 0);
		}
		return FALSE;
	}

	INT_PTR CALLBACK ModalDlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		switch (message)
		{
			case WM_INITDIALOG:
			{
				LATLONGSETDATA* pResult = reinterpret_cast<LATLONGSETDATA*>(lParam);
				SetWindowLongPtr(hWnd, GWLP_USERDATA, lParam);
				SendMessage(GetDlgItem(hWnd, IDC_MAP), WM_LATLONGSET, 0, (LPARAM)pResult);
			}
			break;

			case WM_COMMAND:
				switch (LOWORD(wParam))
				{
					case IDOK:
					case IDCANCEL:
						return EndDialog(hWnd, wParam);
					case IDC_SEARCH:
					{
						size_t city = 0;
						if (DialogBoxParam(FireRender::fireRenderHInstance, MAKEINTRESOURCE(IDD_CITYSEARCH), hWnd, SearchDlgProc, (LPARAM)&city) == IDOK)
						{
							LATLONGSETDATA *pDest = reinterpret_cast<LATLONGSETDATA*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
							const float *lat, *longi, *utcoff;
							BgManagerMax::TheManager.GetCityData(city, &lat, &longi, &utcoff);
							pDest->first = Point2(*longi, *lat);
							pDest->second = *utcoff;
							SendMessage(GetDlgItem(hWnd, IDC_MAP), WM_LATLONGSET, 0, (LPARAM)pDest);
						}
					}
					return TRUE;
				}
			break;

			case WM_CLOSE:
				return EndDialog(hWnd, 0);

			case WM_LATLONGSET:
			{
				LATLONGSETDATA *pResult = reinterpret_cast<LATLONGSETDATA*>(lParam);
				LATLONGSETDATA* pDest = reinterpret_cast<LATLONGSETDATA*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
				pDest->first = pResult->first;
				pDest->second = pResult->second;
			}
			return TRUE;
		}
		return FALSE;
	}
};

void FireRenderParamDlg::CBackgroundSettings::UpdateGroupEnables()
{
	IParamBlock2* pb = mOwner->renderer->GetParamBlock(0);
	TimeValue t = GetCOREInterface()->GetTime();

	int bgUse = 0;
	pb->GetValue(TRPARAM_BG_USE, t, bgUse, FOREVER);
	if (!bgUse)
	{
		EnableGroupboxControls(mHwnd, 0, false, { IDC_BG_USE, IDC_BG_ENABLEALPHA });
		return;
	}

	// Disable window redrawing as we're changing enabled state of some controls
	// multiple times, this causes flicker.
	LockWindowUpdate(mHwnd);

	EnableGroupboxControls(mHwnd, 0, true, { IDC_BG_USE });

	int bgType = 0;
	pb->GetValue(TRPARAM_BG_TYPE, t, bgType, FOREVER);
	bool bUseIBL = (bgType == FRBackgroundType_IBL);
	EnableGroupboxControls(mHwnd, IDC_BG_USEIBL_GROUP, bUseIBL, { IDC_BG_USEIBL });
	EnableGroupboxControls(mHwnd, IDC_BG_USESKY_GROUP, !bUseIBL, { IDC_BG_USESKY });

	if (bgType != FRBackgroundType_IBL)
	{
		int bgSkyType = 0;
		pb->GetValue(TRPARAM_BG_SKY_TYPE, 0, bgSkyType, FOREVER);
		bool bUseAnalytical = (bgSkyType == FRBackgroundSkyType_Analytical);
		EnableGroupboxControls(mHwnd, IDC_BG_SKYANALYTICAL_GROUP, bUseAnalytical, { IDC_BG_SKYANALYTICAL });
		EnableGroupboxControls(mHwnd, IDC_BG_SKYDATETIMELOC_GROUP, !bUseAnalytical, { IDC_BG_SKYDATETIMELOC });
	}

	LockWindowUpdate(NULL);
}

INT_PTR FireRenderParamDlg::CBackgroundSettings::DlgProc(UINT msg, WPARAM wParam, LPARAM lParam)
{
	IParamBlock2* pb = mOwner->renderer->GetParamBlock(0);

	switch (msg)
	{
	case WM_COMMAND:
	{
		int controlId = LOWORD(wParam);
		int submsg = HIWORD(wParam);

		if (submsg == BN_RIGHTCLICK)
		{
			switch (controlId)
			{
				case IDC_BG_IBLMAP:
				case IDC_BG_IBLBACKPLATEMAP:
				case IDC_BG_IBLREFLECTIONMAP:
				case IDC_BG_IBLREFRACTIONMAP:
				case IDC_BG_SKYBACKPLATEMAP:
				case IDC_BG_SKYREFLECTIONMAP:
				case IDC_BG_SKYREFRACTIONMAP:
				{
					HMENU hPopMenu = CreatePopupMenu();
					int menuId = 0;
					switch (controlId)
					{
						case IDC_BG_IBLMAP: menuId = ID_MAPCLEAR_IBL; break;
						case IDC_BG_IBLBACKPLATEMAP: menuId = ID_MAPCLEAR_IBLBACKPLATE; break;
						case IDC_BG_IBLREFLECTIONMAP: menuId = ID_MAPCLEAR_IBLREFR; break;
						case IDC_BG_IBLREFRACTIONMAP: menuId = ID_MAPCLEAR_IBLREFL; break;
						case IDC_BG_SKYBACKPLATEMAP: menuId = ID_MAPCLEAR_SKYBACKPLATE; break;
						case IDC_BG_SKYREFLECTIONMAP: menuId = ID_MAPCLEAR_SKYREFR; break;
						case IDC_BG_SKYREFRACTIONMAP: menuId = ID_MAPCLEAR_SKYREFL; break;
					}
					InsertMenu(hPopMenu, 0, MF_BYPOSITION | MF_STRING, menuId, L"Clear");
					SetForegroundWindow(Hwnd());
					POINT pt;
					GetCursorPos(&pt);
					TrackPopupMenu(hPopMenu, TPM_BOTTOMALIGN | TPM_LEFTALIGN, pt.x, pt.y, 0, Hwnd(), NULL);
				}
				break;
			}
		}
		else if (submsg == 0)
		switch (controlId)
		{
			case IDC_BG_RESET:
			{
				BgManagerMax::TheManager.FactoryReset();
				DestroyDialog();
				InitDialog();
			}
			break;

			case IDC_BG_SKYNOW:
			{
				SYSTEMTIME time;
				GetLocalTime(&time);
				controls.skyHours->SetValue(time.wHour, TRUE);
				controls.skyMinutes->SetValue(time.wMinute, TRUE);
				controls.skySeconds->SetValue(time.wSecond, TRUE);
				controls.skyDay->SetValue(time.wDay, TRUE);
				controls.skyMonth->SetValue(time.wMonth, TRUE);
				controls.skyYear->SetValue(time.wYear, TRUE);
			}
			break;
		
			case IDC_BG_USE:
			{
				BOOL res = pb->SetValue(TRPARAM_BG_USE, 0, BOOL(IsDlgButtonChecked(mHwnd, IDC_BG_USE))); FASSERT(res);
				UpdateGroupEnables();
				FASSERT(res);
			}
			break;

			case IDC_BG_ENABLEALPHA: { BOOL res = pb->SetValue(TRPARAM_BG_ENABLEALPHA, 0, BOOL(IsDlgButtonChecked(mHwnd, IDC_BG_ENABLEALPHA))); FASSERT(res); } break;

			case IDC_BG_USEIBL:
			case IDC_BG_USESKY:
			{
				bool bUseIBL = IsDlgButtonChecked(mHwnd, IDC_BG_USEIBL) != 0;
				BOOL res = pb->SetValue(TRPARAM_BG_TYPE, 0, bUseIBL ? FRBackgroundType_IBL : FRBackgroundType_SunSky);
				UpdateGroupEnables();
			}
			break;

			case IDC_BG_SKYANALYTICAL:
			case IDC_BG_SKYDATETIMELOC:
			{
				bool bUseAnalytical = GetRadioButtonValue(IDC_BG_SKYANALYTICAL) != 0;
				BOOL res = pb->SetValue(TRPARAM_BG_SKY_TYPE, 0, bUseAnalytical ? FRBackgroundSkyType_Analytical : FRBackgroundSkyType_DateTimeLocation);
				FASSERT(res);
				UpdateGroupEnables();
			}
			break;

			case IDC_BG_IBLUSE: { BOOL res = pb->SetValue(TRPARAM_BG_IBL_MAP_USE, 0, BOOL(IsDlgButtonChecked(mHwnd, IDC_BG_IBLUSE))); FASSERT(res); } break;
			case IDC_BG_IBLBACKPLATEUSE: { BOOL res = pb->SetValue(TRPARAM_BG_IBL_BACKPLATE_USE, 0, BOOL(IsDlgButtonChecked(mHwnd, IDC_BG_IBLBACKPLATEUSE))); FASSERT(res); } break;
			case IDC_BG_IBLREFLECTIONUSE: { BOOL res = pb->SetValue(TRPARAM_BG_IBL_REFLECTIONMAP_USE, 0, BOOL(IsDlgButtonChecked(mHwnd, IDC_BG_IBLREFLECTIONUSE))); FASSERT(res); } break;
			case IDC_BG_IBLREFRACTIONUSE: { BOOL res = pb->SetValue(TRPARAM_BG_IBL_REFRACTIONMAP_USE, 0, BOOL(IsDlgButtonChecked(mHwnd, IDC_BG_IBLREFRACTIONUSE))); FASSERT(res); } break;
			case IDC_BG_SKYBACKPLATEUSE: { BOOL res = pb->SetValue(TRPARAM_BG_SKY_BACKPLATE_USE, 0, BOOL(IsDlgButtonChecked(mHwnd, IDC_BG_SKYBACKPLATEUSE))); FASSERT(res); } break;
			case IDC_BG_SKYREFLECTIONUSE: { BOOL res = pb->SetValue(TRPARAM_BG_SKY_REFLECTIONMAP_USE, 0, BOOL(IsDlgButtonChecked(mHwnd, IDC_BG_SKYREFLECTIONUSE))); FASSERT(res); } break;
			case IDC_BG_SKYREFRACTIONUSE: { BOOL res = pb->SetValue(TRPARAM_BG_SKY_REFRACTIONMAP_USE, 0, BOOL(IsDlgButtonChecked(mHwnd, IDC_BG_SKYREFRACTIONUSE))); FASSERT(res); } break;
			case IDC_BG_SKYDAYLIGHTSAVING: { BOOL res = pb->SetValue(TRPARAM_BG_SKY_DAYLIGHTSAVING, 0, BOOL(IsDlgButtonChecked(mHwnd, IDC_BG_SKYDAYLIGHTSAVING))); FASSERT(res); } break;

			case IDC_BG_SKYGETLOCATION:
			{
				LATLONGSETDATA coord;
				coord.first = Point2(controls.skyLongitude->GetFVal(), controls.skyLatitude->GetFVal());
				if (DialogBoxParam(FireRender::fireRenderHInstance, MAKEINTRESOURCE(IDD_LOCATIONPICKER), GetCOREInterface()->GetMAXHWnd(), ModalDlgProc, (LPARAM)&coord) == IDOK)
				{
					controls.skyLongitude->SetValue(coord.first.x, TRUE);
					controls.skyLatitude->SetValue(coord.first.y, TRUE);
					controls.skyTimezone->SetValue(coord.second, TRUE);
				}
			}
			break;

			case IDC_BG_IBLMAP:
			case IDC_BG_IBLBACKPLATEMAP:
			case IDC_BG_IBLREFRACTIONMAP:
			case IDC_BG_IBLREFLECTIONMAP:
			case IDC_BG_SKYBACKPLATEMAP:
			case IDC_BG_SKYREFRACTIONMAP:
			case IDC_BG_SKYREFLECTIONMAP:
			{
				// setup parameters depending on control
				CTexmapButton* button = NULL;
				switch (controlId)
				{
				case IDC_BG_IBLMAP:
					button = controls.iblMap;
					break;
			
				case IDC_BG_IBLBACKPLATEMAP:
					button = controls.iblBackplate;
					break;
			
				case IDC_BG_IBLREFRACTIONMAP:
					button = controls.iblRefrOverride;
					break;
			
				case IDC_BG_IBLREFLECTIONMAP:
					button = controls.iblReflOverride;
					break;

				case IDC_BG_SKYBACKPLATEMAP:
					button = controls.skyBackplate;
					break;

				case IDC_BG_SKYREFRACTIONMAP:
					button = controls.skyRefrOverride;
					break;

				case IDC_BG_SKYREFLECTIONMAP:
					button = controls.skyReflOverride;
						break;
				}
				
				FASSERT(button != NULL);
				ParamID param = button->mParamId;

				MSTR filename;
				Texmap* m;
				pb->GetValue(param, 0, m, FOREVER);
				if (m && m->ClassID() == Class_ID(BMTEX_CLASS_ID, 0))
				{
					// get filename of existing texture
					BitmapTex* bitmapTex = (BitmapTex*)m;
					filename = bitmapTex->GetMap().GetFullFilePath();
				}
				if (filename.isNull())
				{
					// Important: path should ends with "\", otherwise file dialog will not change its directory.
					filename = GetIBLRootDirectory() + L"\\";
				}

			#if 1
				BitmapInfo bi;
				bi.SetName(filename.data());
				// use Max's open texture dialog
				if (TheManager->SelectFileInput(&bi, Hwnd(), L"Select Bitmap Image File"))
				{
					filename = bi.Name();
					// try to load texture
					BMMRES status;
					MaxSDK::Util::Path path(filename.ToMCHAR());
					bi.SetPath(path);
					Bitmap *bmap = ::TheManager->Load(&bi, &status);
					if (status == BMMRES_SUCCESS)
					{
						// create new Texmap from this texture
						BitmapTex* bitmapTex = static_cast<BitmapTex*>(CreateInstance(TEXMAP_CLASS_ID, Class_ID(BMTEX_CLASS_ID, 0)));
						bitmapTex->SetBitmapInfo(bi);
						bitmapTex->SetName(path.StripToLeaf().GetString());
						m = bitmapTex;

						// commit changes
						BOOL res = pb->SetValue(param, 0, m);
						FASSERT(res);
						button->SetValue(m);
				}
			}
			#else
				// old code, kept here just in case (if we'll need to use Texmap selection for some controls)
				BOOL newMat, cancel;
				m = (Texmap *)GetCOREInterface()->DoMaterialBrowseDlg(
					mOwner->GetAnyWindowHandle(), BROWSE_MAPSONLY | BROWSE_INCNONE, newMat, cancel);
				if (!cancel)
				{
					if (newMat)
						m->RecursInitSlotType(MAPSLOT_ENVIRON);

					BOOL res = pb->SetValue(param, 0, m);
					FASSERT(res);
					button->SetValue(m);
				}
			#endif
			}
			break;

			case ID_MAPCLEAR_IBL:
				controls.iblMap->SetValue(NULL);
				break;
			case ID_MAPCLEAR_IBLBACKPLATE:
				controls.iblBackplate->SetValue(NULL);
				break;
			case ID_MAPCLEAR_IBLREFR:
				controls.iblRefrOverride->SetValue(NULL);
				break;
			case ID_MAPCLEAR_IBLREFL:
				controls.iblReflOverride->SetValue(NULL);
				break;
			case ID_MAPCLEAR_SKYBACKPLATE:
				controls.skyBackplate->SetValue(NULL);
				break;
			case ID_MAPCLEAR_SKYREFR:
				controls.skyRefrOverride->SetValue(NULL);
				break;
			case ID_MAPCLEAR_SKYREFL:
				controls.skyReflOverride->SetValue(NULL);
				break;
		}
	}
	break;

	case WM_NOTIFY:
	{
		int idButton = (int)LOWORD(wParam);
		int code = ((LPNMHDR)lParam)->code;
		if ((idButton == IDC_IBL_LINKS) && (code == NM_CLICK || code == NM_RETURN))
		{
			// clicked SysLink in "background" dialog, we're using <a id="url"> tag to recognize clicked parts
			PNMLINK pNMLink = (PNMLINK)lParam;
			const TCHAR* id = pNMLink->item.szID;
			std::wstring url = std::wstring(L"http://") + id;
			// execute URL
			ShellExecute(NULL, L"open", url.c_str(), NULL, NULL, SW_SHOW);
		}
	}
	break;

	case CC_COLOR_CHANGE:
	{
		if (mIsReady)
		{
			int controlId = LOWORD(wParam);
			switch (controlId)
			{
				case IDC_BG_IBLCOLOR:
				{
					Color col = controls.iblColor->GetAColor();
					BgManagerMax::TheManager.SetProperty(PARAM_BG_IBL_SOLIDCOLOR, col, PROP_SENDER_COLORDIAL);
				}
				break;

				case IDC_BG_SKYGROUNDCOLOR:
				{
					Color col = controls.skyGroundColor->GetAColor();
					BgManagerMax::TheManager.SetProperty(PARAM_BG_SKY_GROUND_COLOR, col, PROP_SENDER_COLORDIAL);
				}
				break;

				case IDC_BG_SKYGROUNDALBEDO:
				{
					Color col = controls.skyGroundAlbedo->GetAColor();
					BgManagerMax::TheManager.SetProperty(PARAM_BG_SKY_GROUND_ALBEDO, col, PROP_SENDER_COLORDIAL);
				}
				break;

				case IDC_BG_SKYFILTER:
				{
					Color col = controls.skyFilterColor->GetAColor();
					BgManagerMax::TheManager.SetProperty(PARAM_BG_SKY_FILTER_COLOR, col, PROP_SENDER_COLORDIAL);
				}
				break;
			}
		}
	}
	break;

	case CC_SPINNER_CHANGE:
	{
		if (mIsReady)
		{
			int controlId = LOWORD(wParam);
			return CommitSpinnerToParam(pb, controlId);
		}
	}
	break;

	default:
		return FALSE;
	}

	return TRUE;
}

void FireRenderParamDlg::CBackgroundSettings::InitDialog()
{
	IParamBlock2* pb = mOwner->renderer->GetParamBlock(0);
	TimeValue t = GetCOREInterface()->GetTime();

	BgManagerMax::TheManager.RegisterPropertyChangeCallback(this);

	SetupCheckbox(pb, TRPARAM_BG_USE, IDC_BG_USE);

	// setup environment type groups

	int bgType = 0;
	pb->GetValue(TRPARAM_BG_TYPE, t, bgType, FOREVER);
	if (bgType == FRBackgroundType_IBL)
	{
		CheckRadioButton(IDC_BG_USEIBL);
		UnCheckRadioButton(IDC_BG_USESKY);
	}
	else
	{
		CheckRadioButton(IDC_BG_USESKY);
		UnCheckRadioButton(IDC_BG_USEIBL);
	}

	int bgSkyType = 0;
	pb->GetValue(TRPARAM_BG_SKY_TYPE, 0, bgSkyType, FOREVER);
	if (bgSkyType == FRBackgroundSkyType_Analytical)
	{
		CheckRadioButton(IDC_BG_SKYANALYTICAL);
		UnCheckRadioButton(IDC_BG_SKYDATETIMELOC);
	}
	else
	{
		CheckRadioButton(IDC_BG_SKYDATETIMELOC);
		UnCheckRadioButton(IDC_BG_SKYANALYTICAL);
	}
	UpdateGroupEnables();

	// setup everything else

	BOOL useBgMap = FALSE;
	BOOL useBgBackMap = FALSE;

	SetupCheckbox(pb, TRPARAM_BG_ENABLEALPHA, IDC_BG_ENABLEALPHA);
	
	pb->GetValue(TRPARAM_BG_IBL_MAP_USE, t, useBgMap, FOREVER);
	pb->GetValue(TRPARAM_BG_IBL_BACKPLATE_USE, t, useBgBackMap, FOREVER);
	CheckDlgButton(mHwnd, IDC_BG_IBLUSE, useBgMap ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(mHwnd, IDC_BG_IBLBACKPLATEUSE, useBgBackMap ? BST_CHECKED : BST_UNCHECKED);
	
	SetupCheckbox(pb, TRPARAM_BG_IBL_REFLECTIONMAP_USE, IDC_BG_IBLREFLECTIONUSE);
	SetupCheckbox(pb, TRPARAM_BG_IBL_REFRACTIONMAP_USE, IDC_BG_IBLREFRACTIONUSE);

	SetupCheckbox(pb, TRPARAM_BG_SKY_BACKPLATE_USE, IDC_BG_SKYBACKPLATEUSE);
	SetupCheckbox(pb, TRPARAM_BG_SKY_REFLECTIONMAP_USE, IDC_BG_SKYREFLECTIONUSE);
	SetupCheckbox(pb, TRPARAM_BG_SKY_REFRACTIONMAP_USE, IDC_BG_SKYREFRACTIONUSE);

	SetupCheckbox(pb, TRPARAM_BG_SKY_DAYLIGHTSAVING, IDC_BG_SKYDAYLIGHTSAVING);

	controls.iblColor = SetupSwatch(pb, TRPARAM_BG_IBL_SOLIDCOLOR, IDC_BG_IBLCOLOR);
	controls.skyGroundColor = SetupSwatch(pb, TRPARAM_BG_SKY_GROUND_COLOR, IDC_BG_SKYGROUNDCOLOR);
	controls.skyGroundAlbedo = SetupSwatch(pb, TRPARAM_BG_SKY_GROUND_ALBEDO, IDC_BG_SKYGROUNDALBEDO);
	controls.skyFilterColor = SetupSwatch(pb, TRPARAM_BG_SKY_FILTER_COLOR, IDC_BG_SKYFILTER);

	controls.iblIntensity = SetupSpinner(pb, TRPARAM_BG_IBL_INTENSITY, IDC_BG_IBLINTENSITY, IDC_BG_IBLINTENSITY_S);
	controls.skyAzimuth = SetupSpinner(pb, TRPARAM_BG_SKY_AZIMUTH, IDC_BG_SKYAZIMUTH, IDC_BG_SKYAZIMUTH_S);
	controls.skyAltitude = SetupSpinner(pb, TRPARAM_BG_SKY_ALTITUDE, IDC_BG_SKYALTITUDE, IDC_BG_SKYALTITUDE_S);
	controls.skyHours = SetupSpinner(pb, TRPARAM_BG_SKY_HOURS, IDC_BG_SKYHOURS, IDC_BG_SKYHOURS_S);
	controls.skyMinutes = SetupSpinner(pb, TRPARAM_BG_SKY_MINUTES, IDC_BG_SKYMINUTES, IDC_BG_SKYMINUTES_S);
	controls.skySeconds = SetupSpinner(pb, TRPARAM_BG_SKY_SECONDS, IDC_BG_SKYSECONDS, IDC_BG_SKYSECONDS_S);
	controls.skyDay = SetupSpinner(pb, TRPARAM_BG_SKY_DAY, IDC_BG_SKYDAY, IDC_BG_SKYDAY_S);
	controls.skyMonth = SetupSpinner(pb, TRPARAM_BG_SKY_MONTH, IDC_BG_SKYMONTH, IDC_BG_SKYMONTH_S);
	controls.skyYear = SetupSpinner(pb, TRPARAM_BG_SKY_YEAR, IDC_BG_SKYYEAR, IDC_BG_SKYYEAR_S);
	controls.skyTimezone = SetupSpinner(pb, TRPARAM_BG_SKY_TIMEZONE, IDC_BG_SKYTIMEZONE, IDC_BG_SKYTIMEZONE_S);
	controls.skyLatitude = SetupSpinner(pb, TRPARAM_BG_SKY_LATITUDE, IDC_BG_SKYLATITUDE, IDC_BG_SKYLATITUDE_S);
	controls.skyLongitude = SetupSpinner(pb, TRPARAM_BG_SKY_LONGITUDE, IDC_BG_SKYLONGITUDE, IDC_BG_SKYLONGITUDE_S);
	controls.skyHaze = SetupSpinner(pb, TRPARAM_BG_SKY_HAZE, IDC_BG_SKYHAZE, IDC_BG_SKYHAZE_S);
	controls.skyDiscSize = SetupSpinner(pb, TRPARAM_BG_SKY_DISCSIZE, IDC_BG_SKYSUNDISC, IDC_BG_SKYSUNDISC_S);
	controls.skyIntensity = SetupSpinner(pb, TRPARAM_BG_SKY_INTENSITY, IDC_BG_SKYINTENSITY, IDC_BG_SKYINTENSITY_S);
	controls.skySaturation = SetupSpinner(pb, TRPARAM_BG_SKY_SATURATION, IDC_BG_SKYSATURATION, IDC_BG_SKYSATURATION_S);

	// By default, year will jump over 20 years when arrow keys are pressed - when "AutoScale" is enabled.
	// Make it jumping with 1 year instead.
	controls.skyYear->SetAutoScale(FALSE);

	SetControlTooltip(IDC_BG_SKYAZIMUTH_S, _T("0=North\n90=East\n180=South\n270=West"));
	SetControlTooltip(IDC_BG_SKYAZIMUTH, _T("0=North\n90=East\n180=South\n270=West"));
	SetControlTooltip(IDC_BG_SKYALTITUDE_S, _T("0=Horizon\n90=Zenith\n-90=Nadir"));
	SetControlTooltip(IDC_BG_SKYALTITUDE, _T("0=Horizon\n90=Zenith\n-90=Nadir"));
	SetControlTooltip(IDC_BG_SKYLATITUDE_S, _T("90=North Pole\n0=Equator\n-90=South Pole"));
	SetControlTooltip(IDC_BG_SKYLATITUDE, _T("90=North Pole\n0=Equator\n-90=South Pole"));
	SetControlTooltip(IDC_BG_SKYLONGITUDE_S, _T("0=Greenwich\n0..180=East\n0..-180=West"));
	SetControlTooltip(IDC_BG_SKYLONGITUDE, _T("0=Greenwich\n0..180=East\n0..-180=West"));
	SetControlTooltip(IDC_BG_SKYHAZE_S, _T("Amount of suspended particles that create a foggy effect\n0=clear sky\n20=completely foggy"));
	SetControlTooltip(IDC_BG_SKYHAZE, _T("Amount of suspended particles that create a foggy effect\n0=clear sky\n20=completely foggy"));
	SetControlTooltip(IDC_BG_SKYGLOW_S, _T("Glow factor defines the visible extent of the sun's radiance"));
	SetControlTooltip(IDC_BG_SKYGLOW, _T("Glow factor defines the visible extent of the sun's radiance"));
	SetControlTooltip(IDC_BG_SKYFILTER, _T("Shifts the sky color towards the specified filter"));
	SetControlTooltip(IDC_BG_SKYSUNDISC_S, _T("Scale factor for the visible sun disk"));
	SetControlTooltip(IDC_BG_SKYSUNDISC, _T("Scale factor for the visible sun disk"));
	SetControlTooltip(IDC_BG_SKYHORIZONHEIGHT_S, _T("Height, in meters, at which the horizon line is defined"));
	SetControlTooltip(IDC_BG_SKYHORIZONHEIGHT, _T("Height, in meters, at which the horizon line is defined"));
	SetControlTooltip(IDC_BG_SKYHORIZONBLUR_S, _T("Blur factor for the horizon line\nThe higher the factor, the blurrier the horizon line"));
	SetControlTooltip(IDC_BG_SKYHORIZONBLUR, _T("Blur factor for the horizon line\nThe higher the factor, the blurrier the horizon line"));
	SetControlTooltip(IDC_BG_SKYINTENSITY_S, _T("Scale factor for the overall intensity of the sky and sun system"));
	SetControlTooltip(IDC_BG_SKYINTENSITY, _T("Scale factor for the overall intensity of the sky and sun system"));
	SetControlTooltip(IDC_BG_SKYGROUNDCOLOR, _T("Color of the ground (the half hemisphere under the horizon line)"));
	SetControlTooltip(IDC_BG_SKYSATURATION, _T("The saturation factor\nThe higher the factor, the greater the sky saturation"));
	SetControlTooltip(IDC_BG_SKYSATURATION_S, _T("The saturation factor\nThe higher the factor, the greater the sky saturation"));

	SetSpinnerScale(controls.iblIntensity, 0.1f);
	SetSpinnerScale(controls.skyAzimuth, 0.1f);
	SetSpinnerScale(controls.skyAltitude, 0.1f);
	SetSpinnerScale(controls.skyTimezone, 1.0f);
	SetSpinnerScale(controls.skyLatitude, 0.1f);
	SetSpinnerScale(controls.skyLongitude, 0.1f);
	SetSpinnerScale(controls.skyHaze, 0.1f);
	SetSpinnerScale(controls.skyDiscSize, 0.1f);
	SetSpinnerScale(controls.skyIntensity, 0.1f);
	SetSpinnerScale(controls.skySaturation, 0.1f);

	controls.iblMap = SetupTexmapButton(pb, TRPARAM_BG_IBL_MAP, IDC_BG_IBLMAP);
	controls.iblBackplate = SetupTexmapButton(pb, TRPARAM_BG_IBL_BACKPLATE, IDC_BG_IBLBACKPLATEMAP);
	controls.iblReflOverride = SetupTexmapButton(pb, TRPARAM_BG_IBL_REFLECTIONMAP, IDC_BG_IBLREFLECTIONMAP);
	controls.iblRefrOverride = SetupTexmapButton(pb, TRPARAM_BG_IBL_REFRACTIONMAP, IDC_BG_IBLREFRACTIONMAP);
	controls.skyBackplate = SetupTexmapButton(pb, TRPARAM_BG_SKY_BACKPLATE, IDC_BG_SKYBACKPLATEMAP);
	controls.skyReflOverride = SetupTexmapButton(pb, TRPARAM_BG_SKY_REFLECTIONMAP, IDC_BG_SKYREFLECTIONMAP);
	controls.skyRefrOverride = SetupTexmapButton(pb, TRPARAM_BG_SKY_REFRACTIONMAP, IDC_BG_SKYREFRACTIONMAP);

	mIsReady = true;
}

void FireRenderParamDlg::CBackgroundSettings::DestroyDialog()
{
	if (mIsReady)
	{
		BgManagerMax::TheManager.UnregisterPropertyChangeCallback(this);

		mIsReady = false;
		RemoveAllSpinnerAssociations();

		ReleaseISpinner(controls.iblIntensity);
		ReleaseISpinner(controls.skyAzimuth);
		ReleaseISpinner(controls.skyAltitude);
		ReleaseISpinner(controls.skyHours);
		ReleaseISpinner(controls.skyMinutes);
		ReleaseISpinner(controls.skySeconds);
		ReleaseISpinner(controls.skyDay);
		ReleaseISpinner(controls.skyMonth);
		ReleaseISpinner(controls.skyYear);
		ReleaseISpinner(controls.skyTimezone);
		ReleaseISpinner(controls.skyLatitude);
		ReleaseISpinner(controls.skyLongitude);
		ReleaseISpinner(controls.skyHaze);
		ReleaseISpinner(controls.skyDiscSize);
		ReleaseISpinner(controls.skyIntensity);
		ReleaseISpinner(controls.skySaturation);
		ReleaseIColorSwatch(controls.iblColor);
		ReleaseIColorSwatch(controls.skyGroundColor);
		ReleaseIColorSwatch(controls.skyGroundAlbedo);
		ReleaseIColorSwatch(controls.skyFilterColor);
		delete controls.iblMap;
		delete controls.iblBackplate;
		delete controls.iblReflOverride;
		delete controls.iblRefrOverride;
		delete controls.skyBackplate;
		delete controls.skyReflOverride;
		delete controls.skyRefrOverride;
	}
}

void FireRenderParamDlg::CBackgroundSettings::propChangedCallback(class ManagerMaxBase *owner, int paramId, int sender)
{
	IParamBlock2* pb = mOwner->renderer->GetParamBlock(0);
	auto ip = GetCOREInterface();
	if (sender == PROP_SENDER_SUNWIDGET)
	{
		bool wasReady = mIsReady;
		mIsReady = false;
		if (paramId == PARAM_BG_SKY_AZIMUTH)
		{
			float az = 0.f;
			pb->GetValue(TRPARAM_BG_SKY_AZIMUTH, ip->GetTime(), az, FOREVER);
			controls.skyAzimuth->SetValue(az, TRUE);
		}
		else if (paramId == PARAM_BG_SKY_ALTITUDE)
		{
			float el = 0.f;
			pb->GetValue(TRPARAM_BG_SKY_ALTITUDE, ip->GetTime(), el, FOREVER);
			controls.skyAltitude->SetValue(el, TRUE);
		}
		mIsReady = wasReady;
	}
}

//////////////////////////////////////////////////////////////////////////////
// HARDCODED PRESET SUPPORT
//

struct PresetDataValue
{
	int		param;				// use value <= 0 as end of list marker
	float	value;				// can use float for all parameter types
};

struct PresetData
{
	const wchar_t* presetName;	// use NULL as end of list marker
	const PresetDataValue* data;
};

static int GetPresetCount(const PresetData* presetData)
{
	for (int presetIndex = 0; /* empty */; presetIndex++)
	{
		const PresetData& preset = presetData[presetIndex];
		if (preset.presetName == NULL) // end of list
			return presetIndex;
	}
	return 0; // should never go here
}

static int InitPresetComboBox(const PresetData* presetData, HWND control, bool addCustom = false)
{
	int presetIndex;
	for (presetIndex = 0; /* empty */; presetIndex++)
	{
		const PresetData& preset = presetData[presetIndex];
		if (preset.presetName == NULL) // end of list
			break;
		int n = SendMessage(control, CB_ADDSTRING, 0L, (LPARAM)preset.presetName);
		SendMessage(control, CB_SETITEMDATA, n, presetIndex);
	}

	if (addCustom) {
		int n = SendMessage(control, CB_ADDSTRING, 0L, (LPARAM)L"Custom");
		SendMessage(control, CB_SETITEMDATA, n, presetIndex+1);
	}
	else {
		--presetIndex;
	}

	return presetIndex; // number of presets
}

static void ApplyPreset(const PresetData* presetData, HWND dialog, int presetIndex)
{
	if (presetIndex < 0) return; // wrong value
	const PresetData* preset = presetData;
	for (int i = 0; i < presetIndex; i++, preset++)
	{
		if (preset->presetName == NULL)
			return; // end of list, i.e. not found
	}

	if(preset->data == NULL) return;

	for (int i = 0; /* empty */; i++)
	{
		const PresetDataValue& value = preset->data[i];
		if (value.param <= 0) break; // end of list

		// at the moment we're working only with float spinners, so not worth making code more complicated
		HWND controlWnd = GetDlgItem(dialog, value.param);
		FASSERT(controlWnd);
		ISpinnerControl* spinner = GetISpinner(controlWnd);
		FASSERT(spinner);
		spinner->SetValue(value.value, TRUE);
	}
}

static int FindActivePreset(const PresetData* presetData, HWND dialog)
{
	const PresetData* preset = presetData;
	for (int presetIndex = 0; /* empty */; presetIndex++, preset++)
	{
		if (preset->presetName == NULL)
			return -1; // end of list, i.e. not found
		bool allMatch = true;
		for (int j = 0; /* empty */; j++)
		{
			const PresetDataValue& value = preset->data[j];
			if (value.param <= 0) break; // end of list
			// at the moment we're working only with float spinners, so not worth making code more complicated
			HWND controlWnd = GetDlgItem(dialog, value.param);
			FASSERT(controlWnd);
			ISpinnerControl* spinner = GetISpinner(controlWnd);
			FASSERT(spinner);
			float v = spinner->GetFVal();
			if (fabs(v - value.value) > 0.001f)
			{
				allMatch = false;
				break;
			}
		}
		if (allMatch)
			return presetIndex;
	}
	return -1;
}


//////////////////////////////////////////////////////////////////////////////
// ANTIALIAS ROLLOUT
//

static const PresetDataValue aa_lowPreset[] =
{
	{ IDC_PARAM_CONTEXT_ITERATIONS_S, 1 },
	{ -1, 0 }
};

static const PresetDataValue aa_mediumPreset[] =
{
	{ IDC_PARAM_CONTEXT_ITERATIONS_S, 4 },
	{ -1, 0 }
};

static const PresetDataValue aa_highPreset[] =
{
	{ IDC_PARAM_CONTEXT_ITERATIONS_S, 8 },
	{ -1, 0 }
};

static const PresetData aa_presetTable[] =
{
	{ L"Low", aa_lowPreset },
	{ L"Medium", aa_mediumPreset },
	{ L"High", aa_highPreset },
	{ NULL, NULL }
};

INT_PTR FireRenderParamDlg::CAntialiasSettings::DlgProc(UINT msg, WPARAM wParam, LPARAM lParam)
{
	IParamBlock2* pb = mOwner->renderer->GetParamBlock(0);
	switch (msg)
	{
		case CC_SPINNER_CHANGE:
		{
			if (mIsReady)
			{
				int idSpinner = (int)LOWORD(wParam);
				CommitSpinnerToParam(pb, idSpinner);

				mOwner->mQualitySettings.setupUIFromData();
			}
		}
		break;

		case WM_COMMAND:
		{
			switch (HIWORD(wParam))
			{
				case CBN_SELCHANGE:
				{
					if (mIsReady)
					{
						int idComboBox = (int)LOWORD(wParam);
						if (idComboBox == IDC_IMAGE_FILTER)
						{
							HWND imageFilterCtrl = (HWND)lParam;
							int dummy = int(SendMessage(imageFilterCtrl, CB_GETCURSEL, 0, 0));
							FASSERT(dummy != CB_ERR);
							const int imageFilter = int(SendMessage(imageFilterCtrl, CB_GETITEMDATA, WPARAM(dummy), 0));
							FASSERT(imageFilter != CB_ERR);
							BOOL res = pb->SetValue(PARAM_IMAGE_FILTER, 0, imageFilter); FASSERT(res);
						}

						mOwner->mQualitySettings.setupUIFromData();
					}
				}
				break;
			}
		}
		break;

		default:
			return FALSE;
	}

	return TRUE;
}

void FireRenderParamDlg::CAntialiasSettings::InitDialog()
{
	IParamBlock2* pb = mOwner->renderer->GetParamBlock(0);
	LRESULT n;

	// Image reconstruction filter
	HWND imgFilter = GetDlgItem(mHwnd, IDC_IMAGE_FILTER);
	FASSERT(imgFilter);
	SendMessage(imgFilter, CB_RESETCONTENT, 0L, 0L);
	n = SendMessage(imgFilter, CB_ADDSTRING, 0L, (LPARAM)_T("Box"));
	SendMessage(imgFilter, CB_SETITEMDATA, n, RPR_FILTER_BOX);
	n = SendMessage(imgFilter, CB_ADDSTRING, 0L, (LPARAM)_T("Triangle"));
	SendMessage(imgFilter, CB_SETITEMDATA, n, RPR_FILTER_TRIANGLE);
	n = SendMessage(imgFilter, CB_ADDSTRING, 0L, (LPARAM)_T("Gaussian"));
	SendMessage(imgFilter, CB_SETITEMDATA, n, RPR_FILTER_GAUSSIAN);
	n = SendMessage(imgFilter, CB_ADDSTRING, 0L, (LPARAM)_T("Mitchell"));
	SendMessage(imgFilter, CB_SETITEMDATA, n, RPR_FILTER_MITCHELL);
	n = SendMessage(imgFilter, CB_ADDSTRING, 0L, (LPARAM)_T("Lanczos"));
	SendMessage(imgFilter, CB_SETITEMDATA, n, RPR_FILTER_LANCZOS);
	n = SendMessage(imgFilter, CB_ADDSTRING, 0L, (LPARAM)_T("Blackmanharris"));
	SendMessage(imgFilter, CB_SETITEMDATA, n, RPR_FILTER_BLACKMANHARRIS);
	setListboxValue(imgFilter, pb->GetInt(PARAM_IMAGE_FILTER));

	controls.aaFilterWidth = SetupSpinner(pb, PARAM_IMAGE_FILTER_WIDTH, IDC_IMAGE_FILTER_WIDTH, IDC_IMAGE_FILTER_WIDTH_S);
	controls.iterationsCount = SetupSpinner(pb, PARAM_CONTEXT_ITERATIONS, IDC_PARAM_CONTEXT_ITERATIONS, IDC_PARAM_CONTEXT_ITERATIONS_S);

	mIsReady = true;
}

void FireRenderParamDlg::CAntialiasSettings::DestroyDialog()
{
	if (mIsReady)
	{
		mIsReady = false;
		RemoveAllSpinnerAssociations();
		ReleaseISpinner(controls.aaFilterWidth);
		ReleaseISpinner(controls.iterationsCount);
	}
}

void FireRenderParamDlg::CAntialiasSettings::SetQualityPresets(int qualityLevel)
{
	ApplyPreset(aa_presetTable, mHwnd, qualityLevel);
	GetCOREInterface()->DisplayTempPrompt(L"Max Ray Depth, AA Samples, and AA Grid have now been updated", 5000);
}



//////////////////////////////////////////////////////////////////////////////
// QUALITY SETTINGS ROLLOUT
//

static const PresetDataValue global_lowPreset[] =
{
	{ IDC_MAX_RAY_DEPTH_S, 8 },
	{ -1, 0 }
};

static const PresetDataValue global_mediumPreset[] =
{
	{ IDC_MAX_RAY_DEPTH_S, 10 },
	{ -1, 0 }
};

static const PresetDataValue global_highPreset[] =
{
	{ IDC_MAX_RAY_DEPTH_S, 20 },
	{ -1, 0 }
};

static const PresetData global_presetTable[] =
{
	{ L"Low", global_lowPreset },
	{ L"Medium", global_mediumPreset },
	{ L"High", global_highPreset },
	{ NULL, NULL }
};

INT_PTR FireRenderParamDlg::CQualitySettings::DlgProc(UINT msg, WPARAM wParam, LPARAM lParam)
{
	IParamBlock2* pb = mOwner->renderer->GetParamBlock(0);
	switch (msg)
	{
		case CC_SPINNER_CHANGE:
		{
			if (mIsReady)
			{
				int idSpinner = (int)LOWORD(wParam);

				if (idSpinner == IDC_QUALITY_RAY_EPSILON_S)
				{
					float val = controls.raycastEpsilon->GetFVal();
					BOOL res = pb->SetValue(PARAM_QUALITY_RAYCAST_EPSILON, 0, val / rayCastEpsilonContextCoeff);
					FASSERT(res);
				}
				else
				{
					CommitSpinnerToParam(pb, idSpinner);
				}							
			}
		}
		break;

		case WM_COMMAND:
		{
			switch (HIWORD(wParam))
			{
				case CBN_SELCHANGE:
				{
					if (mIsReady)
					{
						int idComboBox = (int)LOWORD(wParam);
						if (idComboBox == IDC_GLOBAL_QUALITY_PRESETS)
						{
							HWND hwndComboBox = (HWND)lParam;
							int qualityPreset = int(SendMessage(hwndComboBox, CB_GETCURSEL, 0, 0));
							FASSERT(qualityPreset != CB_ERR);
							SetQualityPresets(qualityPreset);
						}
					}
				}
				break;
			}
		}
		break;

		default:
			return FALSE;
	}

	return TRUE;
}

void FireRenderParamDlg::CQualitySettings::setupUIFromData()
{
	// Important
	if(!mOwner->mAntialiasSettings.Hwnd() || !mOwner->mAdvancedSettings.Hwnd())
		return;

	int aa_activePreset = FindActivePreset(aa_presetTable, mOwner->mAntialiasSettings.Hwnd());
	int global_activePreset = FindActivePreset(global_presetTable, mOwner->mAdvancedSettings.Hwnd());

	int numPresets = sizeof(global_presetTable) / sizeof(global_presetTable[0]) - 1; // -1 last element doesn't count

	int newPreset;
	if(aa_activePreset < 0 || global_presetTable < 0 || aa_activePreset != global_activePreset)
	{
		newPreset = numPresets;
	}
	else // they are equal and not negative
	{
		newPreset = global_activePreset;
	}
	
	HWND hwndQualityPresets = GetDlgItem(mHwnd, IDC_GLOBAL_QUALITY_PRESETS);
	int oldPreset = int(SendMessage(hwndQualityPresets, CB_GETCURSEL, 0, 0));
	
	// Only update the UI when value changed, because it's flickering on update
	if(oldPreset != newPreset)
	{
		SendMessage(hwndQualityPresets, CB_SETCURSEL, newPreset, 0);
	}
}

void FireRenderParamDlg::CQualitySettings::InitDialog()
{
	IParamBlock2* pb = mOwner->renderer->GetParamBlock(0);
	
	HWND qualityPreset = GetDlgItem(mHwnd, IDC_GLOBAL_QUALITY_PRESETS);
	FASSERT(qualityPreset);
	SendMessage(qualityPreset, CB_RESETCONTENT, 0L, 0L);
	
	int numPresets = InitPresetComboBox(global_presetTable, qualityPreset, true);

	controls.raycastEpsilon = SetupSpinner(pb, PARAM_QUALITY_RAYCAST_EPSILON, IDC_QUALITY_RAY_EPSILON, IDC_QUALITY_RAY_EPSILON_S);

	std::tuple<float, float, float> constants = GetRayCastConstants();

	controls.raycastEpsilon->SetLimits(std::get<0>(constants) * rayCastEpsilonContextCoeff,
		std::get<1>(constants) * rayCastEpsilonContextCoeff);

	float value = pb->GetFloat(PARAM_QUALITY_RAYCAST_EPSILON);
	controls.raycastEpsilon->SetValue(value * rayCastEpsilonContextCoeff, TRUE);
	
	mIsReady = true;
}

void FireRenderParamDlg::CQualitySettings::DestroyDialog()
{
	if (mIsReady)
	{
		mIsReady = false;
		ReleaseISpinner(controls.raycastEpsilon);
	}
}

void FireRenderParamDlg::CQualitySettings::SetQualityPresets(int qualityLevel)
{
	mOwner->mAntialiasSettings.SetQualityPresets(qualityLevel);
	ApplyPreset(global_presetTable, mOwner->mAdvancedSettings.Hwnd(), qualityLevel);
}



void FireRenderParamDlg::CDenoiserSettings::InitDialog()
{
	IParamBlock2* pb = mOwner->renderer->GetParamBlock(0);

	// setup "Enable" checkbox
	SetupCheckbox(pb, PARAM_DENOISER_ENABLED, IDC_DENOISER_ENABLE);

	// setup radio buttons based on denoise type
	int denoiserType = DenoiserNone;
	BOOL res = pb->GetValue(PARAM_DENOISER_TYPE, 0, denoiserType, Interval());
	FASSERT(res);

	struct DenoiserTypeItem
	{
		DenoiserType mType;
		int          mUiId;
	};

	static const std::vector<DenoiserTypeItem> types =
	{ 
		{ DenoiserBilateral, IDC_DENOISER_BILATERAL },
		{ DenoiserLwr,       IDC_DENOISER_LWR },
		{ DenoiserEaw,       IDC_DENOISER_EAW }
	};

	for (const DenoiserTypeItem& type : types)
	{
		denoiserType == type.mType ? CheckRadioButton(type.mUiId) : UnCheckRadioButton(type.mUiId);
	}

	// Bilateral parameters
	controls.bilateralRadius = SetupSpinner(pb, PARAM_DENOISER_BILATERAL_RADIUS, IDC_DENOISER_BILATERAL_RADIUS, IDC_DENOISER_BILATERAL_RADIUS_S);

	// LWR parameters
	controls.lwrSamples = SetupSpinner(pb, PARAM_DENOISER_LWR_SAMPLES, IDC_DENOISER_LWR_SAMPLES, IDC_DENOISER_LWR_SAMPLES_S);
	controls.lwrRadius = SetupSpinner(pb, PARAM_DENOISER_LWR_RADIUS, IDC_DENOISER_LWR_RADIUS, IDC_DENOISER_LWR_RADIUS_S);
	controls.lwrBandwidth = SetupSpinner(pb, PARAM_DENOISER_LWR_BANDWIDTH, IDC_DENOISER_LWR_BANDWIDTH, IDC_DENOISER_LWR_BANDWIDTH_S);

	// EAW parameters
	controls.eawColor = SetupSpinner(pb, PARAM_DENOISER_EAW_COLOR, IDC_DENOISER_EAW_COLOR, IDC_DENOISER_EAW_COLOR_S);
	controls.eawNormal = SetupSpinner(pb, PARAM_DENOISER_EAW_NORMAL, IDC_DENOISER_EAW_NORMAL, IDC_DENOISER_EAW_NORMAL_S);
	controls.eawDepth = SetupSpinner(pb, PARAM_DENOISER_EAW_DEPTH, IDC_DENOISER_EAW_DEPTH, IDC_DENOISER_EAW_DEPTH_S);
	controls.eawObjectId = SetupSpinner(pb, PARAM_DENOISER_EAW_OBJECTID, IDC_DENOISER_EAW_OBJECTID, IDC_DENOISER_EAW_OBJECTID_S);

	// fill in 
	std::unordered_map<int, std::pair<ISpinnerControl*, Parameter>> paramById
	{
		{ IDC_DENOISER_BILATERAL_RADIUS_S, { controls.bilateralRadius, PARAM_DENOISER_BILATERAL_RADIUS } },
		{ IDC_DENOISER_LWR_SAMPLES_S,      { controls.lwrSamples, PARAM_DENOISER_LWR_SAMPLES } },
		{ IDC_DENOISER_LWR_RADIUS_S,       { controls.lwrRadius, PARAM_DENOISER_LWR_RADIUS } },
		{ IDC_DENOISER_LWR_BANDWIDTH_S,    { controls.lwrBandwidth, PARAM_DENOISER_LWR_BANDWIDTH } },
		{ IDC_DENOISER_EAW_COLOR_S,        { controls.eawColor, PARAM_DENOISER_EAW_COLOR } },
		{ IDC_DENOISER_EAW_NORMAL_S,       { controls.eawNormal, PARAM_DENOISER_EAW_NORMAL } },
		{ IDC_DENOISER_EAW_DEPTH_S,        { controls.eawDepth, PARAM_DENOISER_EAW_DEPTH } },
		{ IDC_DENOISER_EAW_OBJECTID_S,     { controls.eawObjectId, PARAM_DENOISER_EAW_OBJECTID } },
	};

	mParamById = std::move(paramById);

	mIsReady = true;
}

void FireRenderParamDlg::CDenoiserSettings::DestroyDialog()
{
	if (mIsReady)
	{
		mIsReady = false;

		ReleaseISpinner(controls.bilateralRadius);
		ReleaseISpinner(controls.lwrSamples);
		ReleaseISpinner(controls.lwrRadius);
		ReleaseISpinner(controls.lwrBandwidth);
		ReleaseISpinner(controls.eawColor);
		ReleaseISpinner(controls.eawNormal);
		ReleaseISpinner(controls.eawDepth);
		ReleaseISpinner(controls.eawObjectId);

		mParamById.clear();
	}
}

INT_PTR FireRenderParamDlg::CDenoiserSettings::DlgProc(UINT msg, WPARAM wParam, LPARAM lParam)
{
	static const std::unordered_map<int, DenoiserType> typeById = 
	{
		{ IDC_DENOISER_BILATERAL, DenoiserBilateral },
		{ IDC_DENOISER_LWR, DenoiserLwr },
		{ IDC_DENOISER_EAW, DenoiserEaw },
	};

	IParamBlock2* pb = mOwner->renderer->GetParamBlock(0);

	switch (msg)
	{
		case WM_COMMAND:
		{
			int idButton = (int) LOWORD(wParam);

			switch (HIWORD(wParam))
			{
				case BN_CLICKED:
				{
					switch (idButton)
					{
					case IDC_DENOISER_ENABLE:
						if (mIsReady)
						{
							BOOL value = BOOL(IsDlgButtonChecked(mHwnd, IDC_DENOISER_ENABLE));
							BOOL res = pb->SetValue(PARAM_DENOISER_ENABLED, 0, value);
							FASSERT(res);
						}
						break;

					case IDC_DENOISER_BILATERAL:
					case IDC_DENOISER_LWR:
					case IDC_DENOISER_EAW:
						if (mIsReady)
						{
							BOOL res = pb->SetValue(PARAM_DENOISER_TYPE, 0, typeById.at(idButton));
							FASSERT(res);
						}
						break;
					}
				}
			}
		}

		case CC_SPINNER_CHANGE:
		{
			int idSpinner = (int) LOWORD(wParam);

			switch (idSpinner)
			{
			case IDC_DENOISER_BILATERAL_RADIUS:
			case IDC_DENOISER_LWR_SAMPLES:
			case IDC_DENOISER_LWR_RADIUS:
			case IDC_DENOISER_LWR_BANDWIDTH:
			case IDC_DENOISER_EAW_COLOR:
			case IDC_DENOISER_EAW_NORMAL:
			case IDC_DENOISER_EAW_DEPTH:
			case IDC_DENOISER_EAW_OBJECTID:
				if (mIsReady)
				{
					CommitSpinnerToParam(pb, idSpinner);
				}
				break;

			case IDC_DENOISER_BILATERAL_RADIUS_S:
			case IDC_DENOISER_LWR_SAMPLES_S:
			case IDC_DENOISER_LWR_RADIUS_S:
				if (mIsReady)
				{
					int value = mParamById.at(idSpinner).first->GetIVal();
					BOOL res = pb->SetValue(mParamById.at(idSpinner).second, 0, value);
					FASSERT(res);
				}
				break;

			case IDC_DENOISER_LWR_BANDWIDTH_S:
			case IDC_DENOISER_EAW_COLOR_S:
			case IDC_DENOISER_EAW_NORMAL_S:
			case IDC_DENOISER_EAW_DEPTH_S:
			case IDC_DENOISER_EAW_OBJECTID_S:
				if (mIsReady)
				{
					float value = mParamById.at(idSpinner).first->GetFVal();
					BOOL res = pb->SetValue(mParamById.at(idSpinner).second, 0, value);
					FASSERT(res);
				}
				break;
			}
		}

	}

	return TRUE;
}

void FireRenderParamDlg::CDenoiserSettings::setupUIFromData()
{
}

void FireRenderParamDlg::DeleteThis()
{
	mScripts.DestroyDialog();
	mAdvancedSettings.DestroyDialog();
	mAntialiasSettings.DestroyDialog();
	mBackgroundSettings.DestroyDialog();
	mTonemapSetting.DestroyDialog();
	mCameraSetting.DestroyDialog();
	mDenoiserSettings.DestroyDialog();
	mQualitySettings.DestroyDialog();
	mHardwareSetting.DestroyDialog();
	mGeneralSettings.DestroyDialog();

	mScripts.DeleteRollout();
	mAdvancedSettings.DeleteRollout();
	mAntialiasSettings.DeleteRollout();
	mBackgroundSettings.DeleteRollout();
	mTonemapSetting.DeleteRollout();
	mCameraSetting.DeleteRollout();
	mDenoiserSettings.DeleteRollout();
	mQualitySettings.DeleteRollout();
	mHardwareSetting.DeleteRollout();
	mGeneralSettings.DeleteRollout();

	delete this;
}


//////////////////////////////////////////////////////////////////////////////
// CROLLOUT CLASS
//

CRollout::CRollout()
{
	mIsReady = false;
	mTabId = NULL;
}

CRollout::~CRollout()
{
}

void CRollout::DeleteRollout()
{
	if (mHwnd)
	{
		if(mTabId)
			mRenderParms->DeleteTabRollupPage(*mTabId, mHwnd);
		else
			mRenderParms->DeleteRollupPage(mHwnd);

		DLSetWindowLongPtr<CRollout*>(mHwnd, 0);
		mHwnd = 0;
		mIsReady = false;
	}
}

void CRollout::Init(IRendParams* ir, int tmpl, const MCHAR *title, FireRenderParamDlg *owner, bool closed, const Class_ID* tabId)
{
	mName = title;

	mOwner = owner;
	mRenderParms = ir;
	mTabId = tabId;

	if (owner->isReadOnly)
		closed = true;

	HWND tmp = 0;

	if (tabId)
		tmp = ir->AddTabRollupPage(*tabId, fireRenderHInstance, MAKEINTRESOURCE(tmpl), &CRollout::HandleMessage, title, (LPARAM)this, closed ? APPENDROLL_CLOSED : 0);
	else
		tmp = ir->AddRollupPage(fireRenderHInstance, MAKEINTRESOURCE(tmpl), &CRollout::HandleMessage, title, (LPARAM)this, closed ? APPENDROLL_CLOSED : 0);

	FASSERT(mHwnd == tmp);

	if (owner->isReadOnly)
		DisableAllControls();
}

void CRollout::InitDialog()
{
}

void CRollout::DestroyDialog()
{
	mIsReady = false;
}

INT_PTR CALLBACK CRollout::HandleMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	CRollout *roll = DLGetWindowLongPtr<CRollout*>(hWnd);
	
	switch (msg)
	{
	case WM_INITDIALOG:
	{
		FASSERT(lParam);
		roll = reinterpret_cast<CRollout*>(lParam);

		DLSetWindowLongPtr<CRollout*>(hWnd, roll);

		roll->mHwnd = hWnd;

		roll->InitDialog();
	}
	break;

	case WM_CLOSE:
	case WM_DESTROY:
	{
		if (roll)
		{
			roll->mHwnd = 0;
			DLSetWindowLongPtr<CRollout*>(hWnd, 0);
		}
	}
	break;
	
	default:
	{
		if (roll && roll->mHwnd)
			roll->DlgProc(msg, wParam, lParam);
	}
	break;
	}

	return FALSE;
}

void CRollout::SetControlTooltip(int controlId, const MCHAR* tooltip)
{
	HWND wnd = GetDlgItem(mHwnd, controlId);
	if (wnd) GetToolTipExtender().SetToolTip(wnd, tooltip);
}


//-----------------------------------------------------------------------------
// UI controls
//-----------------------------------------------------------------------------

// There's a class TexDADMgr in 3ds Max SDK, however it can work only with ParamDlg class (material editor).
// We're implementing universal DAD manager here.

class CTexmapDADMgr : public DADMgr
{
public:
	CTexmapDADMgr()
	{}

	void RegisterButton(CRollout::CTexmapButton* button)
	{
		mHwnd2Button.insert(std::make_pair(button->mButton->GetHwnd(), button));
	}
	void UnregisterButton(CRollout::CTexmapButton* button)
	{
		auto ii = mHwnd2Button.find(button->mButton->GetHwnd());
		if (ii != mHwnd2Button.end())
			mHwnd2Button.erase(button->mButton->GetHwnd());
	}

	CRollout::CTexmapButton* FindButton(HWND hwnd)
	{
		auto ii = mHwnd2Button.find(hwnd);
		if (ii == mHwnd2Button.end())
		{
			return NULL;
		}
		return ii->second;
	}

	// DADMgr interface
	virtual SClass_ID GetDragType(HWND hwnd, POINT p) override
	{
		CRollout::CTexmapButton* button = FindButton(hwnd);
		if (!button)
			return NULL;
		
		return button->GetValue() ? TEXMAP_CLASS_ID : NULL;
	}

	virtual BOOL OkToDrop(ReferenceTarget *dropThis, HWND hfrom, HWND hto, POINT p, SClass_ID type, BOOL isNew) override
	{
		if (hfrom == hto) return FALSE;
		if (type == TEXMAP_CLASS_ID) return TRUE;
		return FALSE;
	}

	virtual ReferenceTarget *GetInstance(HWND hwnd, POINT p, SClass_ID type) override
	{
		if (type != TEXMAP_CLASS_ID)
			return NULL;

		CRollout::CTexmapButton* button = FindButton(hwnd);
		if (!button)
			return NULL;
		
		return button->GetValue();
	}

	virtual void Drop(ReferenceTarget *dropThis, HWND hwnd, POINT p, SClass_ID type, DADMgr* srcMgr, BOOL bSrcClone) override
	{
		if (dropThis == NULL || dropThis->SuperClassID() == TEXMAP_CLASS_ID)
		{
			CRollout::CTexmapButton* button = FindButton(hwnd);
			if (button)
			{
				Texmap* map = static_cast<Texmap*>(dropThis);
				button->SetValue(map);
			}
		}
	}

private:
	std::map<HWND, CRollout::CTexmapButton*> mHwnd2Button; // to quickly find a custom button given a control handle
};

static CTexmapDADMgr TexmapDADMgr;

CRollout::CTexmapButton* CRollout::SetupTexmapButton(IParamBlock2* pb, const ParamID paramId, const int16_t controlIdEdit)
{
	HWND buttonWnd = GetDlgItem(mHwnd, controlIdEdit);
	FASSERT(buttonWnd);
	CTexmapButton *button = new CTexmapButton(this, buttonWnd, pb, paramId, controlIdEdit);
	return button;
}

CRollout::CTexmapButton::CTexmapButton(CRollout* owner, HWND wnd, IParamBlock2* pb, const ParamID paramId, const int16_t controlIdEdit)
	: mWnd(wnd)
	, mButton(GetICustButton(wnd))
	, mPb(pb)
	, mParamId(paramId)
	, mControlIdEdit(controlIdEdit)
	, mOwner(owner)
	, mValue(NULL)
{
	TexmapDADMgr.RegisterButton(this);
	mButton->SetDADMgr(&TexmapDADMgr);
	mButton->SetRightClickNotify(TRUE);

	mPb->GetValue(mParamId, 0, mValue, FOREVER);
	UpdateUI();
}

CRollout::CTexmapButton::~CTexmapButton()
{
	TexmapDADMgr.UnregisterButton(this);
	ReleaseICustButton(mButton);
}

void CRollout::CTexmapButton::SetValue(Texmap* value)
{
	BgManagerMax::TheManager.SetProperty(mParamId, value);

	mValue = value;
	mPb->SetValue(mParamId, 0, mValue);

	UpdateUI();
}

void CRollout::CTexmapButton::UpdateUI()
{
	// update text and tooltip
	if (mValue)
	{
		MSTR text = mValue->GetFullName();
		mButton->SetText(text);
		mButton->SetTooltip(true, text);
	}
	else
	{
		mButton->SetText(_T("..."));
		mButton->SetTooltip(false, NULL);
	}
}


IColorSwatch* CRollout::SetupSwatch(IParamBlock2* pb, const ParamID paramId, const int16_t controlIdEdit)
{
	Color col;
	BOOL res = pb->GetValue(paramId, GetCOREInterface()->GetTime(), col, FOREVER);
	FASSERT(res);
	
	HWND swatchControl = GetDlgItem(mHwnd, controlIdEdit);
	FASSERT(swatchControl);
	
	IColorSwatch* swatch = GetIColorSwatch(swatchControl);
	swatch->SetColor(col);
	
	return swatch;
}

IColorSwatch* CRollout::SetupSwatch(const ParamID paramId, const int16_t controlIdEdit)
{
	Color col;
	BOOL res = BgManagerMax::TheManager.GetProperty(paramId, col);
	FASSERT(res);

	HWND swatchControl = GetDlgItem(mHwnd, controlIdEdit);
	FASSERT(swatchControl);

	IColorSwatch* swatch = GetIColorSwatch(swatchControl);
	swatch->SetColor(col);

	return swatch;
}

ISpinnerControl* CRollout::SetupSpinner(IParamBlock2* pb, const ParamID paramId, const int16_t controlIdEdit, const int16_t controlIdSpin)
{
	const ParamDef& def = FIRE_MAX_PBDESC.GetParamDef(paramId);

	HWND textControl = GetDlgItem(mHwnd, controlIdEdit);
	HWND spinControl = GetDlgItem(mHwnd, controlIdSpin);
	FASSERT(textControl && spinControl && textControl != spinControl);
	ISpinnerControl* spinner = GetISpinner(spinControl);
	switch (int(def.type)) { // int and float spinners are set up differently
	case TYPE_RADIOBTN_INDEX:
	//case TYPE_TIMEVALUE:
	case TYPE_INT:
		spinner->LinkToEdit(textControl, EDITTYPE_INT);
		spinner->SetResetValue(def.def.i);
		spinner->SetLimits(def.range_low.i, def.range_high.i);
		spinner->SetAutoScale(TRUE);
		{
			int value;
			BOOL res = pb->GetValue(paramId, GetCOREInterface()->GetTime(), value, FOREVER);
			FASSERT(res);
			spinner->SetValue(value, FALSE);
			AssociateSpinnerInt(spinner, paramId, controlIdSpin);
		}
		break;
	//case TYPE_COLOR_CHANNEL:
	//case TYPE_PCNT_FRAC:
	//case TYPE_ANGLE:
	case TYPE_FLOAT:
	case TYPE_WORLD:
		spinner->LinkToEdit(textControl, (def.type == TYPE_FLOAT) ? EDITTYPE_FLOAT : EDITTYPE_UNIVERSE);
		spinner->SetResetValue(def.def.f);
		spinner->SetLimits(def.range_low.f, def.range_high.f);
		spinner->SetAutoScale(TRUE);
		{
			float value;
			BOOL res = pb->GetValue(paramId, GetCOREInterface()->GetTime(), value, FOREVER);
			FASSERT(res);
			spinner->SetValue(value, FALSE);
			AssociateSpinnerFloat(spinner, paramId, controlIdSpin);
		}
		break;
	default:
		STOP;
	}
	return spinner;
}

ISpinnerControl* CRollout::SetupSpinnerFloat(ManagerMaxBase &man, const ParamID paramId, const int16_t controlIdEdit, const int16_t controlIdSpin, const float &minVal, const float &maxVal)
{
	HWND textControl = GetDlgItem(mHwnd, controlIdEdit);
	HWND spinControl = GetDlgItem(mHwnd, controlIdSpin);
	FASSERT(textControl && spinControl && textControl != spinControl);
	ISpinnerControl* spinner = GetISpinner(spinControl);
	spinner->LinkToEdit(textControl, EDITTYPE_FLOAT);
	spinner->SetResetValue(0.f);
	spinner->SetLimits(minVal, maxVal);
	spinner->SetAutoScale(TRUE);
	float value;
	BOOL res = man.GetProperty(paramId, value);
	FASSERT(res);
	spinner->SetResetValue(value);
	spinner->SetValue(value, FALSE);
	AssociateSpinnerFloat(spinner, paramId, controlIdSpin);
	return spinner;
}

ISpinnerControl* CRollout::SetupSpinnerInt(ManagerMaxBase &man, const ParamID paramId, const int16_t controlIdEdit, const int16_t controlIdSpin, int minVal, int maxVal)
{
	HWND textControl = GetDlgItem(mHwnd, controlIdEdit);
	HWND spinControl = GetDlgItem(mHwnd, controlIdSpin);
	FASSERT(textControl && spinControl && textControl != spinControl);
	ISpinnerControl* spinner = GetISpinner(spinControl);
	spinner->LinkToEdit(textControl, EDITTYPE_INT);
	spinner->SetLimits(minVal, maxVal);
	spinner->SetAutoScale(TRUE);
	int value;
	BOOL res = man.GetProperty(paramId, value);
	FASSERT(res);
	spinner->SetValue(value, FALSE);
	spinner->SetResetValue(value);
	AssociateSpinnerInt(spinner, paramId, controlIdSpin);
	return spinner;
}

void CRollout::SetVisibleDlg(bool visible, const int16_t dlgID)
{
	HWND temp = GetDlgItem(mHwnd, dlgID);
	FASSERT(temp);
	EnableWindow(temp, visible);
}

void CRollout::AcceptParams(IParamBlock2* pb)
{

}

FIRERENDER_NAMESPACE_END
