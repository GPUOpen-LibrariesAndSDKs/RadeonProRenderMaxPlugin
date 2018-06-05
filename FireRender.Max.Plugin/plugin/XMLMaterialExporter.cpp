/*********************************************************************************************************************************
* Radeon ProRender for 3ds Max plugin
* Copyright (c) 2017 AMD
* All Rights Reserved
*
* Additional plug-in to export (individually or  in bulk) materials to Radeon ProRender XML files
*********************************************************************************************************************************/

#include "XMLMaterialExporter.h"
#include <maxscript/maxscript.h>
#include <maxscript/mxsPlugin/mxsPlugin.h>
#include <xref/iXrefMaterial.h>
#include <shaders.h>
#include "resource.h"
#include <vector>
#include <set>

extern bool exportMat(Mtl *max_mat, INode* node, const std::wstring &path);

FIRERENDER_NAMESPACE_BEGIN;

namespace
{
	FireRenderClassDesc_MaterialExport fireRenderMaterialExporterClassDesc;
};

//////////////////////////////////////////////////////////////////////////////
//

FireRenderClassDesc_MaterialExport *IFRMaterialExporter::GetClassDesc()
{
	return &fireRenderMaterialExporterClassDesc;
}

class FRMaterialExporter : public SceneExport, public IFRMaterialExporter
{
public:
	std::wstring OUTDIR;

	FRMaterialExporter()
	{
	}

	~FRMaterialExporter()
	{
	}

	// Number of extensions supported
	int ExtCount() override
	{
		return 1;
	}

	// Extension #n
	const TCHAR *Ext(int n) override
	{
		return _T("XML");
	}

	// Long ASCII description
	const TCHAR *LongDesc() override
	{
		return _T("Radeon ProRender Material File");
	}

	// Short ASCII description
	const TCHAR *ShortDesc() override
	{
		return _T("Radeon ProRender Materials");
	}

	// ASCII Author name
	const TCHAR *AuthorName() override
	{
		return _T("AMD");
	}

	// ASCII Copyright message
	const TCHAR *CopyrightMessage() override
	{
		return _T("(C) AMD");
	}

	// Other message #1
	const TCHAR *OtherMessage1() override
	{
		return _T("");
	}

	// Other message #2
	const TCHAR *OtherMessage2() override
	{
		return _T("");
	}

	// Version number * 100 (i.e. v3.01 = 301)
	unsigned int Version() override
	{
		return 100;
	}

	// Show DLL's "About..." box
	void ShowAbout(HWND hWnd) override
	{
	}

	// Do the actual export
	int DoExport(const MCHAR *name, ExpInterface *ei, Interface *i, BOOL suppressPrompts = FALSE, DWORD options = 0) override;
};

//////////////////////////////////////////////////////////////////////////////
//


/*






// AFAIK - this  FireRenderClassDesc_MaterialExport  is not supported by plugin anymore
// the good way to export material is to use the exportFrMat_cf script.
// PM me on Slack (Richard Ge) if need to be discussed







*/






void* FireRenderClassDesc_MaterialExport::Create(BOOL loading)
{
	return new FRMaterialExporter;
}

namespace
{
	std::wstring safeFilename(const std::wstring &ff)
	{
		std::wstring f = ff;
		for (auto &c : f)
		{
			if (c == '#') c = '_';
			else if (c == '$') c = '_';
			else if (c == '[') c =  '_';
			else if (c == ']') c =  '_';
			else if (c == '{') c =  '_';
			else if (c == '}') c =  '_';
			else if (c == '+') c =  '_';
			else if (c == '%') c =  '_';
			else if (c == '!') c =  '_';
			else if (c == '`') c =  '_';
			else if (c == '&') c =  '_';
			else if (c == '*') c =  '_';
			else if (c == '‘') c =  '_';
			else if (c == '|') c =  '_';
			else if (c == '?') c =  '_';
			else if (c == '\"') c = '_';
			else if (c == '=') c = '_';
			else if (c == '/') c = '_';
			else if (c == '\\') c = '_';
			else if (c == ':') c = '_';
			else if (c == '@') c = '_';
		}
		return f;
	}

	class SelectDlgData
	{
	public:
		std::set<std::wstring> matLibsInStore;
		std::vector<std::wstring> matLibs;

		class LibEntry
		{
		public:
			int matLibIndex; // index into the matLibs array
			int materialIndex; // index of the material, inside the library
		};

		FRMaterialExporter *exporter;

		std::vector<LibEntry *> entries;
		
		~SelectDlgData()
		{
			for (auto ee : entries)
				delete ee;
		}
		
	};

	INT_PTR CALLBACK SelectDlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		switch (message)
		{
			case WM_INITDIALOG:
			{
				SetWindowLongPtr(hWnd, GWLP_USERDATA, lParam);
				auto ip = GetCOREInterface();

				// scene materials
				auto sceneMats = ip->GetSceneMtls();
				for (int i = 0; i < sceneMats->Count(); i++)
				{
					auto mat = (*sceneMats)[i];
					if (mat)
					{
						HWND hmats = GetDlgItem(hWnd, IDC_SCENEMATS);
						std::wstring display = mat->GetName();
						MSTR cname;
						mat->GetClassName(cname);
						display += L" [";
						display += cname + L"]";
						int pos = SendMessage(hmats, LB_ADDSTRING, 0, (LPARAM)display.c_str());
						SendMessage(hmats, LB_SETITEMDATA, pos, (LPARAM)mat);
					}
				}

				// material editor
				// from docs: slot in the Materials Editor(a value in the range 0 to 23)
				for (int i = 0; i < 24; i++)
				{
					auto mat = ip->GetMtlSlot(i);
					if (mat)
					{
						HWND hmats = GetDlgItem(hWnd, IDC_MEMATS);
						std::wstring display = mat->GetName();
						MSTR cname;
						mat->GetClassName(cname);
						display += L" [";
						display += cname + L"]";
						int pos = SendMessage(hmats, LB_ADDSTRING, 0, (LPARAM)display.c_str());
						SendMessage(hmats, LB_SETITEMDATA, pos, (LPARAM)mat);
					}
				}
			}
			break;

			case WM_COMMAND:
				switch (LOWORD(wParam))
				{
					case IDOK:
					{
						SelectDlgData *data = reinterpret_cast<SelectDlgData*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
						FRMaterialExporter *exp = data->exporter;

						std::vector<Mtl*> toExport;
						HWND hmats = GetDlgItem(hWnd, IDC_SCENEMATS);
						int items = SendMessage(hmats, LB_GETCOUNT, 0, 0);
						for (int i = 0; i < items; i++)
						{
							if (SendMessage(hmats, LB_GETSEL, i, 0))
							{
								Mtl *m = reinterpret_cast<Mtl *>(SendMessage(hmats, LB_GETITEMDATA, i, 0));
								if (m)
									toExport.push_back(m);
							}
						}
						hmats = GetDlgItem(hWnd, IDC_MEMATS);
						items = SendMessage(hmats, LB_GETCOUNT, 0, 0);
						for (int i = 0; i < items; i++)
						{
							if (SendMessage(hmats, LB_GETSEL, i, 0))
							{
								Mtl *m = reinterpret_cast<Mtl *>(SendMessage(hmats, LB_GETITEMDATA, i, 0));
								if (m)
									toExport.push_back(m);
							}
						}
						
						for (auto m : toExport)
						{
							std::wstring safefname = safeFilename(std::wstring(m->GetName()));
							std::wstring mat_folder = exp->OUTDIR + safefname;
							bool exists = false;
							bool notFound = false;
							int res = _wmkdir(mat_folder.c_str());
							if (res != 0)
							{
								exists = (errno == EEXIST);
								notFound = (errno == ENOENT);
							}
							if (!notFound)
							{
							std::wstring mat_name = mat_folder + L"\\" + safefname + L".xml";
							
							// AFAIK - this  FireRenderClassDesc_MaterialExport  is not supported by plugin anymore
							// the good way to export material is to use the exportFrMat_cf script.
							// PM me on Slack (Richard Ge) if need to be discussed
							////// exportMat(m, mat_name);
						}
					}

						int lastLib = -1;
						MtlBaseLib lib;
						hmats = GetDlgItem(hWnd, IDC_MLMATS);
						items = SendMessage(hmats, LB_GETCOUNT, 0, 0);
						for (int i = 0; i < items; i++)
						{
							if (SendMessage(hmats, LB_GETSEL, i, 0))
							{
								SelectDlgData::LibEntry *entry = reinterpret_cast<SelectDlgData::LibEntry *>(SendMessage(hmats, LB_GETITEMDATA, i, 0));

								if (lastLib != entry->matLibIndex)
								{
									if (lastLib != -1)
										lib.DeleteAll();
									lastLib = entry->matLibIndex;
									const std::wstring &path = data->matLibs[entry->matLibIndex];
									GetCOREInterface()->LoadMaterialLib(path.c_str(), &lib);
								}

								auto mh = lib[entry->materialIndex];
								if (mh)
								{
									auto  m = dynamic_cast<Mtl*>(mh);
									if (m)
									{
										std::wstring safefname = safeFilename(std::wstring(m->GetName()));
										std::wstring mat_folder = exp->OUTDIR + safefname;
										bool exists = false;
										bool notFound = false;
										int res = _wmkdir(mat_folder.c_str());
										if (res != 0)
										{
											exists = (errno == EEXIST);
											notFound = (errno == ENOENT);
										}
										if (!notFound)
										{
											std::wstring mat_name = mat_folder + L"\\" + safefname + L".xml";
											
											// AFAIK - this  FireRenderClassDesc_MaterialExport  is not supported by plugin anymore
											// the good way to export material is to use the exportFrMat_cf script.
											// PM me on Slack (Richard Ge) if need to be discussed
											//exportMat(m, mat_name);
										}
									}

								}
							}
						}
						if (lastLib != -1)
							lib.DeleteAll();
					}
					case IDCANCEL:
						return EndDialog(hWnd, wParam);

					case IDC_EXPORTMATLIB:
					{
						SelectDlgData *data = reinterpret_cast<SelectDlgData*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

						WCHAR *buffer = new WCHAR[8192 + 2];
						memset(buffer, 0, 8192 + 2);

						OPENFILENAME of;
						memset(&of, 0, sizeof(OPENFILENAME));
						of.lStructSize = sizeof(OPENFILENAME);
						of.hwndOwner = GetCOREInterface()->GetMAXHWnd();
						of.hInstance = fireRenderHInstance;
						of.lpstrFilter = L"Material Libraries\0*.mat\0\0";
						of.lpstrFile = buffer;
						of.nMaxFile = 8192;
						of.lpstrTitle = L"Open one or more material libraries";
						of.Flags = OFN_ALLOWMULTISELECT | OFN_FILEMUSTEXIST | OFN_EXPLORER;
						
						if (GetOpenFileName(&of))
						{
							std::wstring directory = buffer;
							directory += '\\';
							std::vector<std::wstring> files;
							
							std::wstring::size_type i = of.nFileOffset;
							std::wstring temp;

							while (1)
							{
								if (buffer[i] == '\0')
								{
									if (!temp.empty())
										files.push_back(temp);
									temp.clear();
									if (buffer[i + 1] == '\0')
										break;
								}
								else
									temp += buffer[i];
								i++;
							}

							for (auto cc : files)
							{
								std::wstring path = directory + cc;

								if (data->matLibsInStore.find(path) != data->matLibsInStore.end())
									continue;
								data->matLibsInStore.insert(path);

								int thisLibrary = data->matLibs.size();
								data->matLibs.push_back(path);
																
								MtlBaseLib lib;
								GetCOREInterface()->LoadMaterialLib(path.c_str(), &lib);
								
								int num = lib.Count();
								for (int i = 0; i < num; i++)
								{
									auto mat = lib[i];
									if (mat)
									{
										SelectDlgData::LibEntry *entry = new SelectDlgData::LibEntry;
										entry->matLibIndex = thisLibrary;
										entry->materialIndex = i;
										data->entries.push_back(entry);
																				
										HWND hmats = GetDlgItem(hWnd, IDC_MLMATS);
										std::wstring display = mat->GetName();
										MSTR cname;
										mat->GetClassName(cname);
										display += L" [";
										display += cname + L"]";
										int pos = SendMessage(hmats, LB_ADDSTRING, 0, (LPARAM)display.c_str());
										SendMessage(hmats, LB_SETITEMDATA, pos, (LPARAM)entry);
									}
								}
								
								lib.DeleteAll();
							}
						}
						else
						{
							auto err = CommDlgExtendedError();
							if (err != 0)
							{
								//CDERR_DIALOGFAILURE
								//CDERR_MEMALLOCFAILURE
								//FNERR_BUFFERTOOSMALL
								//FNERR_INVALIDFILENAME
							}
						}
						delete[] buffer;
					}
					break;
				}
			break;

			case WM_CLOSE:
				return EndDialog(hWnd, 0);
		}
		return FALSE;
	}
};

// Do the actual export
int FRMaterialExporter::DoExport(const MCHAR *name, ExpInterface *ei, Interface *i, BOOL suppressPrompts, DWORD options)
{
	OUTDIR = name;
	std::wstring::size_type pos = OUTDIR.find_last_of('\\');
	if (pos == std::wstring::npos)
		pos = OUTDIR.find_last_of('/');
	if (pos != std::wstring::npos)
	{
		OUTDIR = OUTDIR.substr(0, pos + 1);
	}
	
	SelectDlgData data;
	data.exporter = this;

	if (DialogBoxParam(FireRender::fireRenderHInstance, MAKEINTRESOURCE(IDD_XMLMATERIALEXPORTER), GetCOREInterface()->GetMAXHWnd(), SelectDlgProc, (LPARAM)&data) == IDOK)
	{

	}

	return IMPEXP_SUCCESS;
}

FIRERENDER_NAMESPACE_END;
