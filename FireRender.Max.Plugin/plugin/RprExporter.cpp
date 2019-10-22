#include "RprExporter.h"
#include "Common.h"
#include "FireRenderer.h"
#include "PRManager.h"
#include "ClassDescs.h"
#include "RprLoadStore.h"

FIRERENDER_NAMESPACE_BEGIN

class RprExporterClassDesc :public ClassDesc
{
public:
	int				IsPublic() { return 1; }
	void*			Create(BOOL loading = FALSE) { return new RprExporter; }
	const TCHAR*	ClassName() { return _T("RprExporter"); }
	SClass_ID		SuperClassID() { return SCENE_EXPORT_CLASS_ID; }
	Class_ID		ClassID() { return FireRender::EXPORTER_MAX_CLASSID; }
	const TCHAR*	Category() { return _T(""); }
};


namespace
{
	MCHAR * k_rprExtention = _T("RPR");

	MCHAR * k_rprLongDesc = _T("Radeon ProRender Scene File");

	MCHAR * k_rprShortDesc = _T("Radeon ProRender Scene");

	MCHAR * k_rprAuthorName = _T("AMD");

	MCHAR * k_rprCopyrightMessage = _T("(C) AMD");
}

static RprExporterClassDesc RprExporterDesc;

ClassDesc * RprExporter::GetClassDesc()
{
	return &RprExporterDesc;
}

const MCHAR * RprExporter::Ext(int n)
{
	return k_rprExtention;
}

const MCHAR * RprExporter::LongDesc()
{
	return k_rprLongDesc;
}

const MCHAR * RprExporter::ShortDesc()
{
	return k_rprShortDesc;
}

const MCHAR * RprExporter::AuthorName()
{
	return k_rprAuthorName;
}

const MCHAR * RprExporter::CopyrightMessage()
{
	return k_rprCopyrightMessage;
}

const MCHAR * RprExporter::OtherMessage1()
{
	return  _T("");
}

const MCHAR * RprExporter::OtherMessage2()
{
	return _T("");
}

INT_PTR CALLBACK ExportDlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
	{
		SetFocus(GetDlgItem(hWnd, IDOK));

		SendDlgItemMessage(hWnd, IDC_RADIO_SINGLE_FRAME, BM_SETCHECK, 1, 0);

		// setup compression combo
		std::vector<std::pair<const wchar_t*, std::uint32_t>> comboboxItemsDesc =
		{
			{ L"None", 0 },
			{ L"Level 1", RPRLOADSTORE_EXPORTFLAG_COMPRESS_IMAGE_LEVEL_1 },
			{ L"Level 2", RPRLOADSTORE_EXPORTFLAG_COMPRESS_IMAGE_LEVEL_2 },
			{ L"Level 3", RPRLOADSTORE_EXPORTFLAG_COMPRESS_IMAGE_LEVEL_2 | RPRLOADSTORE_EXPORTFLAG_COMPRESS_FLOAT_TO_HALF_NORMALS | RPRLOADSTORE_EXPORTFLAG_COMPRESS_FLOAT_TO_HALF_UV }
		};

		HWND combo = GetDlgItem(hWnd, IDC_EXPORT_COMRESSION);

		for (size_t i = 0; i < comboboxItemsDesc.size(); i++)
		{
			ComboBox_AddString(combo, comboboxItemsDesc[i].first);
			ComboBox_SetItemData(combo, i, comboboxItemsDesc[i].second);
		}

		ComboBox_SetCurSel(combo, 1);

		break;
	}

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_RADIO_SINGLE_FRAME:
		{
			switch (HIWORD(wParam))
			{
			case BN_CLICKED:
				
				break;
			}
			return FALSE;
		}

		case IDC_RADIO_SEQUENCE_OF_FRAMES:
		{
			switch (HIWORD(wParam))
			{
			case BN_CLICKED:				
				break;
			}
			return FALSE;
		}

		case IDOK:
		{
			PRManagerMax& prManager = PRManagerMax::TheManager;
			prManager.EnableUseExternalFiles(SendDlgItemMessage(hWnd, IDC_EXPORT_USEEXPTERNAL_FILES, BM_GETCHECK, 0, 0));
			prManager.SetIsExportCurrentFrame(SendDlgItemMessage(hWnd, IDC_RADIO_SINGLE_FRAME, BM_GETCHECK, 1, 0));

			{
				HWND combo = GetDlgItem(hWnd, IDC_EXPORT_COMRESSION);
				int selection = ComboBox_GetCurSel(combo);
				std::uint32_t itemData = ComboBox_GetItemData(combo, selection);
				prManager.SetExportCompressionFlags(itemData);
			}

			EndDialog(hWnd, wParam);
			return TRUE;
		}

		case IDCANCEL:
			EndDialog(hWnd, wParam);
			return TRUE;

		default:
			return FALSE;
		}

	case WM_CLOSE:
		EndDialog(hWnd, wParam);
		return TRUE;
	}

	return FALSE;
}

int RprExporter::DoExport(const MCHAR * name, ExpInterface * ei, Interface * i, BOOL suppressPrompts, DWORD options)
{
	Interface11* ip = GetCOREInterface11();
	
	if (!ip)
	{
		return FALSE;
	}

	Renderer* renderer = ip->GetCurrentRenderer();
	FireRenderClassDesc firerendererDesc;

	if (!(renderer->ClassID() == firerendererDesc.ClassID() && renderer->SuperClassID() == firerendererDesc.SuperClassID()))
	{
		MessageBox(GetCOREInterface()->GetMAXHWnd(), _T("You are exporting a Radeon ProRender Scene configuration.\n\n"
														"\tIt is required that you use Radeon ProRender as the active renderer.\n"
														"\tRenderer selection can be changed from the Render Setup dialog.\n")
														, _T("Radeon ProRender exporting warning"), MB_OK);
		return FALSE;
	}

	if (IDOK == DialogBoxParam(FireRender::fireRenderHInstance, MAKEINTRESOURCE(IDD_FIRERENDER_EXPORT), GetCOREInterface()->GetMAXHWnd(), ExportDlgProc, NULL))
	{
		Interface11* ip = GetCOREInterface11();
		
		if (!ip)
		{
			return FALSE;
		}

		PRManagerMax & prManager = PRManagerMax::TheManager;
		prManager.EnableExportState(true);
		prManager.SetExportName(name);

		BOOL isRenderFrameWindow = ip->GetRendShowVFB();
		ip->SetRendShowVFB(FALSE);

		if (prManager.GetIsExportCurrentFrame())
		{
			int currentRenderType = ip->GetRendTimeType();
			ip->SetRendTimeType(REND_TIMESINGLE);
			ip->QuickRender();
			ip->SetRendTimeType(currentRenderType);
		}
		else
		{
			ip->QuickRender();
		}

		ip->SetRendShowVFB(isRenderFrameWindow);

		prManager.EnableExportState(false);
	}
	
	return TRUE;
}

FIRERENDER_NAMESPACE_END
