#include "stdafx.h"
#include "PPgDisplay.h"
#include "resource.h"
#include "cmdmap.h"
#include "foo_uie_explorer.h"
#include "Preference.h"
#include "others.h"

PPgDisplay::PPgDisplay()
{
}

PPgDisplay::~PPgDisplay()
{
}

HWND PPgDisplay::Create(HWND hParent)
{
	return DialogBase::Create(hParent, IDD_PPG_DISPLAY);
}

LRESULT PPgDisplay::DefWindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
	switch (Msg) {
		case WM_INITDIALOG:
			{
				uSetDlgItemText(m_hWnd, IDC_STARTPATH, prefs::sStartupPath);
				uSetDlgItemText(m_hWnd, IDC_SHOW_FILES, prefs::sShowType);

				SendDlgItemMessage(m_hWnd, IDC_STARTPATH, EM_SETLIMITTEXT, EX_MAX_PATH - 1, 0);

				CheckThisDlgButton(IDC_CHK_ADDBAR, prefs::bShowAddBar);
				CheckThisDlgButton(IDC_CHK_SHOWLINE, prefs::bShowLines);
				CheckThisDlgButton(IDC_CHK_SHOWICON, prefs::bShowIcons);
				CheckThisDlgButton(IDC_CHK_FORCESORT, prefs::bForceSort);
				CheckThisDlgButton(IDC_CHK_HSCROLL, prefs::bShowHScroll);
				CheckThisDlgButton(IDC_CHK_TOOLTIP, prefs::bShowTooltip);
				CheckThisDlgButton(IDC_CHK_SHOW_HIDDENFILES, prefs::bShowHiddenFiles);
				CheckThisDlgButton(IDC_CHK_SHOW_EXT, prefs::bShowFileExtension);

				if (prefs::iStartup == IDC_ST_NONE) {
					DisableThisDlgItem(IDC_STARTPATH);
					DisableThisDlgItem(IDC_BTN_SELPATH);
				}

				CheckRadioButton(m_hWnd, IDC_ST_NONE, IDC_ST_USER, prefs::iStartup);
				CheckRadioButton(m_hWnd, IDC_FILES_ALL, IDC_FILES_NONE, prefs::iShowTypeMode);
			}
			break;
		case WM_COMMAND:
			switch (HIWORD(wParam)) {
				case 0:
					switch (LOWORD(wParam)) {
						case IDC_CHK_ADDBAR:
							prefs::bShowAddBar = !!SendMessage((HWND) lParam, BM_GETCHECK, 0, 0);
							PERFORM_GLOBAL(ShowAddressBar());
							break;
						case IDC_CHK_SHOWLINE:
							prefs::bShowLines = !!SendMessage((HWND) lParam, BM_GETCHECK, 0, 0);
							PERFORM_GLOBAL(UpdateStyles());
							break;
						case IDC_CHK_SHOWICON:
							prefs::bShowIcons = !!SendMessage((HWND) lParam, BM_GETCHECK, 0, 0);
							PERFORM_GLOBAL(UpdateStyles());
							break;
						case IDC_CHK_FORCESORT:
							prefs::bForceSort = !!SendMessage((HWND) lParam, BM_GETCHECK, 0, 0);
							break;
						case IDC_CHK_SHOW_HIDDENFILES:
							prefs::bShowHiddenFiles = !!SendMessage((HWND) lParam, BM_GETCHECK, 0, 0);
							PERFORM_GLOBAL(InitInsert());
							break;
						case IDC_CHK_SHOW_EXT:
							prefs::bShowFileExtension = !!SendMessage((HWND) lParam, BM_GETCHECK, 0, 0);
							PERFORM_GLOBAL(RedrawWindow());
							break;
						case IDC_CHK_HSCROLL:
							prefs::bShowHScroll = !!SendMessage((HWND) lParam, BM_GETCHECK, 0, 0);
							MessageBox(m_hWnd, _T("The change does not effect until you restart the panel."), _T(APPNAME), MB_OK | MB_ICONINFORMATION);
							//It seems there is no method to change this style one the fly. :(
							break;
						case IDC_CHK_TOOLTIP:
							prefs::bShowTooltip = !!SendMessage((HWND) lParam, BM_GETCHECK, 0, 0);
							PERFORM_GLOBAL(UpdateStyles());
							break;
						case IDC_FILES_ALL:
						case IDC_FILES_NONE:
							prefs::iShowTypeMode = (int) wParam;
							break;
						case IDC_ST_NONE:
						case IDC_ST_LAST:
						case IDC_ST_USER:
							prefs::iStartup = (int) wParam;
							EnableDlgItem(m_hWnd, IDC_STARTPATH, prefs::iStartup != IDC_ST_NONE);
							EnableDlgItem(m_hWnd, IDC_BTN_SELPATH, prefs::iStartup != IDC_ST_NONE);
							break;
						case IDC_BTN_SELPATH:
							{
								TCHAR sBuffer[EX_MAX_PATH];

								if (ChooseFolder(sBuffer, m_hWnd)) {
									SetDlgItemText(m_hWnd, IDC_STARTPATH, sBuffer);
									prefs::sStartupPath = pfc::stringcvt::string_utf8_from_os(sBuffer);
								}
							}
							break;
					}
					break;
				case EN_KILLFOCUS:
					switch (LOWORD(wParam)) {
						case IDC_STARTPATH:
							{
								TCHAR sPath[EX_MAX_PATH];
								if (GetDlgItemText(m_hWnd, IDC_STARTPATH, sPath, EX_MAX_PATH) && ISFULLPATH(sPath))
									prefs::sStartupPath = pfc::stringcvt::string_utf8_from_os(sPath);
								else //error --> restore
									uSetDlgItemText(m_hWnd, IDC_STARTPATH, prefs::sStartupPath);
							}
							break;
						case IDC_SHOW_FILES:
							prefs::sShowType = string_utf8_from_window((HWND) lParam);
							DirTreeCtrl::UpdateMaskString();
							break;
					}					
					break;
			}
			break;
	}

	return DialogBase::DefWindowProc(Msg, wParam, lParam);
}

void PPgDisplay::Reset()
{
	prefs::bForceSort.reset();
	prefs::bShowAddBar.reset();
	prefs::bShowFileExtension.reset();
	prefs::bShowHiddenFiles.reset();
	prefs::bShowHScroll.reset();
	prefs::bShowIcons.reset();
	prefs::bShowLines.reset();
	prefs::bShowTooltip.reset();
	prefs::iShowTypeMode.reset();
	prefs::iStartup.reset();
	prefs::sShowType.reset();
	prefs::sStartupPath.reset();

	PERFORM_GLOBAL(ShowAddressBar());
	PERFORM_GLOBAL(UpdateStyles());
	PERFORM_GLOBAL(InitInsert());
}
