#include "stdafx.h"
#include "ppgdisplay2.h"
#include "resource.h"
#include "foo_uie_explorer.h"
#include "preference.h"
#include "others.h"

PPgDisplay2::PPgDisplay2()
{
}

PPgDisplay2::~PPgDisplay2()
{
}

HWND PPgDisplay2::Create(HWND hParent)
{
	return DialogBase::Create(hParent, IDD_PPG_DISPLAY2);
}

LRESULT PPgDisplay2::DefWindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
	switch (Msg) {
		case WM_INITDIALOG:
			{
				uSetDlgItemText(m_hWnd, IDC_HIDEDRIVE, prefs::sHideDrive);
				SetDlgItemText(m_hWnd, IDC_NODE_HEIGHT, int_string(prefs::iNodeHeight));
				SetDlgItemText(m_hWnd, IDC_MIN_HEIGHT, int_string(prefs::iMinHeight));

				SendDlgItemMessage(m_hWnd, IDC_HIDEDRIVE, EM_SETLIMITTEXT, 26, 0);
				SendDlgItemMessage(m_hWnd, IDC_NODE_HEIGHT, EM_SETLIMITTEXT, 4, 0);
				SendDlgItemMessage(m_hWnd, IDC_MIN_HEIGHT, EM_SETLIMITTEXT, 4, 0);
				SendDlgItemMessage(m_hWnd, IDC_EDT_TEXTCOLOR, EM_SETLIMITTEXT, 6, 0);
				SendDlgItemMessage(m_hWnd, IDC_EDT_BKCOLOR, EM_SETLIMITTEXT, 6, 0);
				SendDlgItemMessage(m_hWnd, IDC_EDT_LINECOLOR, EM_SETLIMITTEXT, 6, 0);
				SendDlgItemMessage(m_hWnd, IDC_EDT_ABTEXTCOLOR, EM_SETLIMITTEXT, 6, 0);
				SendDlgItemMessage(m_hWnd, IDC_EDT_ABBKCOLOR, EM_SETLIMITTEXT, 6, 0);

				m_StyleMenu.CreatePopupMenu();
				m_StyleMenu.AppendMenu(MF_STRING | MF_GRAYED, ID_TREE_STYLE, _T("Explorer Tree"));
				m_StyleMenu.AppendMenu(MF_STRING, ID_TREE_BORDER, _T("&Has Border"));
				m_StyleMenu.AppendMenu(MF_STRING, ID_TREE_NONE, _T("&None"));
				m_StyleMenu.AppendMenu(MF_STRING, ID_TREE_SUNKEN, _T("&Sunken"));
				m_StyleMenu.AppendMenu(MF_STRING, ID_TREE_GREY, _T("&Grey"));
				m_StyleMenu.AppendMenu(MF_SEPARATOR);
				m_StyleMenu.AppendMenu(MF_STRING | MF_GRAYED, ID_ADBAR_STYLE, _T("Address Bar"));
				m_StyleMenu.AppendMenu(MF_STRING, ID_ADBAR_BORDER, _T("&Has Border"));
				m_StyleMenu.AppendMenu(MF_STRING, ID_ADBAR_NONE, _T("&None"));
				m_StyleMenu.AppendMenu(MF_STRING, ID_ADBAR_SUNKEN, _T("&Sunken"));
				m_StyleMenu.AppendMenu(MF_STRING, ID_ADBAR_GREY, _T("&Grey"));
			}
			break;
		case WM_COMMAND:
			switch (HIWORD(wParam)) {
				case 0:
					switch (LOWORD(wParam)) {
						case IDC_BTN_STYLE:
							{
								RECT rc;
								HWND hBtn;
								hBtn = GetDlgItem(m_hWnd, IDC_BTN_STYLE);
								GetWindowRect(hBtn, &rc);

								m_StyleMenu.CheckMenuItem(ID_TREE_BORDER, MF_BYCOMMAND | ((prefs::iTreeFrame & 0x00010000) ? MF_CHECKED : MF_UNCHECKED));
								m_StyleMenu.CheckMenuItem(ID_ADBAR_BORDER, MF_BYCOMMAND | ((prefs::iAdBarFrame & 0x00010000) ? MF_CHECKED : MF_UNCHECKED));
								m_StyleMenu.CheckMenuRadioItem(ID_TREE_NONE, ID_TREE_GREY, LOWORD(prefs::iTreeFrame));
								m_StyleMenu.CheckMenuRadioItem(ID_ADBAR_NONE, ID_ADBAR_GREY, LOWORD(prefs::iAdBarFrame));

								DWORD cmd = m_StyleMenu.TrackPopupMenu(TPM_RIGHTALIGN | TPM_BOTTOMALIGN | TPM_RIGHTBUTTON | TPM_NONOTIFY | TPM_RETURNCMD, rc.right, rc.top, m_hWnd);
								if (INRANGE(cmd, ID_TREE_NONE, ID_TREE_GREY))
									prefs::iTreeFrame = (prefs::iTreeFrame & 0x00010000) | cmd;
								else if (INRANGE(cmd, ID_ADBAR_NONE, ID_ADBAR_GREY))
									prefs::iAdBarFrame = (prefs::iAdBarFrame & 0x00010000) | cmd;
								else if (cmd == ID_TREE_BORDER)
									prefs::iTreeFrame = prefs::iTreeFrame ^ 0x00010000;
								else if (cmd == ID_ADBAR_BORDER)
									prefs::iAdBarFrame = prefs::iAdBarFrame ^ 0x00010000;
								PERFORM_GLOBAL(UpdateStyles());
							}
							break;
					}
					break;
				case EN_KILLFOCUS:
					switch (LOWORD(wParam)) {
						case IDC_HIDEDRIVE:
							{
								char flags[26] = {0};
								TCHAR sDrive[27];
								int i, n = 0;
								LPTSTR ptr;
								GetDlgItemText(m_hWnd, IDC_HIDEDRIVE, sDrive, 27);
								for (ptr = sDrive;*ptr;ptr++)
									if (_istalpha(*ptr)) {
										if (_istlower(*ptr))
											*ptr = _toupper(*ptr);
										flags[*ptr - _T('A')] = 1;
									}
								for (i = 0;i < 26;i++)
									if (flags[i]) {
										sDrive[n] = i + _T('A');
										n++;
									}
								sDrive[n] = _T('\0');
								prefs::sHideDrive = pfc::stringcvt::string_utf8_from_os(sDrive);
								PERFORM_GLOBAL(InitInsert());
							}
							break;
						case IDC_NODE_HEIGHT:
							{
								TCHAR buffer[5];
								GetDlgItemText(m_hWnd, IDC_NODE_HEIGHT, buffer, 5);
								prefs::iNodeHeight = _ttoi(buffer);
								PERFORM_GLOBAL(SetItemHeight());
							}
							break;
					}					
					break;
				case EN_UPDATE:
					switch (LOWORD(wParam)) {
						case IDC_MIN_HEIGHT:
							{
								TCHAR buffer[5];
								GetDlgItemText(m_hWnd, IDC_MIN_HEIGHT, buffer, 5);
								prefs::iMinHeight = _ttoi(buffer);
								g_instance.get_static_instance().UpdateHeight();
							}
							break;
					}
					break;
			}
			break;
		case WM_DESTROY:
			m_StyleMenu.DestroyMenu();
			break;
	}

	return DialogBase::DefWindowProc(Msg, wParam, lParam);
}

void PPgDisplay2::Reset()
{
	prefs::iAdBarFrame.reset();
	prefs::iMinHeight.reset();
	prefs::iNodeHeight.reset();
	prefs::iTreeFrame.reset();
	prefs::sHideDrive.reset();

	PERFORM_GLOBAL(UpdateStyles());
	PERFORM_GLOBAL(SetItemHeight());
	PERFORM_GLOBAL(InitInsert());
}
