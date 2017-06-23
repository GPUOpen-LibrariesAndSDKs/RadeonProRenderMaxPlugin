/*********************************************************************************************************************************
* Radeon ProRender for 3ds Max plugin
* Copyright (C) 2017 AMD
* All Rights Reserved
*
* 3ds Max-related part of implementation of the automatic testing tool
*********************************************************************************************************************************/
#include <max.h>
#include <utilapi.h>
#include <iparamb2.h>
#include "plugin\ClassDescs.h"
#include "Testing.h"
#include "Resource.h"
#include "utils\Utils.h"

FIRERENDER_NAMESPACE_BEGIN;

AutoTestingClassDesc autoTestingClassDesc;

static INT_PTR CALLBACK dlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

/// Quick & dirty implementation - we will make the testing implementation a singleton
Tester tester;

/// Plugin class implementation so that 3ds Max recognizes this tool as a plugin
class TestingPlugin : public UtilityObj {
public:
    HWND hPanel = NULL;

    /// Shows up a dialog for the plugin containing the "start" button
    void BeginEditParams(Interface *ip, IUtil *iu) override {
        hPanel = ip->AddRollupPage(fireRenderHInstance, MAKEINTRESOURCE(IDD_AUTOTESTING), dlgProc, _T("Radeon ProRender Auto-Tester"), (LPARAM) this);
    }

    /// Deletes the dialog
    void EndEditParams(Interface *ip, IUtil *iu) override {
        ip->DeleteRollupPage(hPanel);
        hPanel = NULL;
    }
    void DeleteThis() override {
        delete this;
    }
};

void* AutoTestingClassDesc::Create(BOOL loading) {
    return new TestingPlugin;
}


static INT_PTR CALLBACK dlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {

    static bool running = false;
    static ICustEdit* filter = NULL;
    static ISpinnerControl* passes = NULL;
    static ISpinnerControl* repeat = NULL;
    switch (msg) {
    case WM_INITDIALOG: {

        HWND spinHwnd = GetDlgItem(hWnd, IDC_AUTOTEST_PASSES_S);
        FASSERT(spinHwnd != NULL);
        passes = GetISpinner(spinHwnd);
        passes->LinkToEdit(GetDlgItem(hWnd, IDC_AUTOTEST_PASSES), EDITTYPE_INT);
        passes->SetResetValue(1);
        passes->SetLimits(1, INT_MAX, TRUE);
        passes->SetAutoScale(TRUE);
        passes->SetValue(1, TRUE);
        
        spinHwnd = GetDlgItem(hWnd, IDC_AUTOTEST_REPEAT_S);
        FASSERT(spinHwnd != NULL);
        repeat = GetISpinner(spinHwnd);
        repeat->LinkToEdit(GetDlgItem(hWnd, IDC_AUTOTEST_REPEAT), EDITTYPE_INT);
        repeat->SetResetValue(1);
        repeat->SetLimits(1, INT_MAX, TRUE);
        repeat->SetAutoScale(TRUE);
        repeat->SetValue(1, TRUE);

        SetDlgItemText(hWnd, IDC_AUTOTEST_PROGRESS, _T("[inactive]"));
        filter = GetICustEdit(GetDlgItem(hWnd, IDC_AUTOTEST_FILTER));
        SetTimer(hWnd, 1, 500, NULL);
        break;
    }

    case WM_DESTROY:
        ReleaseICustEdit(filter);
        ReleaseISpinner(passes);
        ReleaseISpinner(repeat);
        filter = NULL;
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDC_AUTOTEST_GO:
            // User clicked on the "Go" button - if there is no testing already going on, prompt for folder with tests and run 
            // them. If the testing is already in progress, the button press will stop it.
            if (!running) {
                running = true;
                MSTR tmp;
                filter->GetText(tmp);
                const int passesVal = passes->GetIVal();
                const int repeatVal = repeat->GetIVal();

                MCHAR buffer[8000]; // the 3ds Max "ChooseDirectory" method unfortunately cannot specify size of the buffer
                std::fill(buffer, buffer + 8000, '\0');
                GetCOREInterface()->ChooseDirectory(GetCOREInterface()->GetMAXHWnd(), _T("Select directory with tests"), buffer, NULL);

                for (int i = 0; i < repeatVal; ++i) { // optionally do stress-testing by repeating all tests n-times
                    tester.renderTests(ToAscii(buffer), ToAscii(tmp.data()), passesVal);
                }
                SetDlgItemText(hWnd, IDC_AUTOTEST_PROGRESS, _T("[inactive]"));
                running = false;
            } else {
                tester.stopRender();
            }
            break;
        }
        break;
    case WM_TIMER:
        if (running) { //periodically update the status text of testing
            SetDlgItemText(hWnd, IDC_AUTOTEST_PROGRESS, ToUnicode(tester.progress()).c_str());
        }
        break;
    default:
        return FALSE;
    }
    return TRUE;
}


FIRERENDER_NAMESPACE_END;