#include "stdafx.h"
#include "ppgcommand.h"
#include "resource.h"
#include "preference.h"
#include "others.h"

PPgCommand::PPgCommand()
{
}

PPgCommand::~PPgCommand()
{
}

HWND PPgCommand::Create(HWND hParent)
{
	return DialogBase::Create(hParent, IDD_PPG_COMMAND);
}

LRESULT PPgCommand::DefWindowProc(UINT msg, WPARAM wp, LPARAM lp)
{	
	switch (msg) {
		case WM_INITDIALOG:
			{
				m_CmdList.m_hWnd = GetDlgItem(m_hWnd, IDC_CMD_LIST);
				m_CmdList.InitBinding(m_hWnd);
				m_CmdList.Init();

				{
					HWND hCBox = GetDlgItem(m_hWnd, IDC_SELECT_WS);
					SendMessage(hCBox, CB_INSERTSTRING, 0, (LPARAM) _T("Normal"));
					SendMessage(hCBox, CB_INSERTSTRING, 1, (LPARAM) _T("Maximized"));
					SendMessage(hCBox, CB_INSERTSTRING, 2, (LPARAM) _T("Minimized"));
					SendMessage(hCBox, CB_SETCURSEL, 0, 0);
				}

				SetDlgItemText(m_hWnd, IDC_EDT_PARAM, _T(":N"));
				SetDlgItemText(m_hWnd, IDC_EDT_WORKDIR, _T(":D"));

				SendDlgItemMessage(m_hWnd, IDC_EDT_DESC, EM_SETLIMITTEXT, 128, 0);
				SendDlgItemMessage(m_hWnd, IDC_EDT_APP, EM_SETLIMITTEXT, EX_MAX_PATH - 1, 0);
				SendDlgItemMessage(m_hWnd, IDC_EDT_PARAM, EM_SETLIMITTEXT, EX_MAX_PATH - 1, 0);
				SendDlgItemMessage(m_hWnd, IDC_EDT_WORKDIR, EM_SETLIMITTEXT, EX_MAX_PATH - 1, 0);

				{
					HWND hBtn;

					hBtn = GetDlgItem(m_hWnd, IDC_BTN_UP);
					if (globals::hSymbolFont) {
						SetWindowFont(hBtn, globals::hSymbolFont, TRUE);
						SetWindowText(hBtn, _T("5"));
					}
					else
						SetWindowText(hBtn, _T("Up"));

					hBtn = GetDlgItem(m_hWnd, IDC_BTN_DOWN);
					if (globals::hSymbolFont) {
						SetWindowFont(hBtn, globals::hSymbolFont, TRUE);
						SetWindowText(hBtn, _T("6"));
					}
					else
						SetWindowText(hBtn, _T("Down"));
				}

				CheckThisDlgButton(IDC_CHK_SHOWINGRP, prefs::bGrpCmd);
			}
			break;
		case WM_NOTIFY:
			if (((LPNMHDR) lp)->idFrom == IDC_CMD_LIST && m_CmdList.OnNotify(wp, lp))
				return TRUE;
			break;
		case WM_COMMAND:
			switch (HIWORD(wp)) {
				case 0:
					switch (LOWORD(wp)) {
						case IDC_BTN_ADD:
						case IDC_BTN_REPLACE:
							{
								TCHAR buffer[EX_MAX_PATH];
								CmdItem *cItem = new CmdItem;
								try {
									GetDlgItemText(m_hWnd, IDC_EDT_DESC, buffer, EX_MAX_PATH);
									if (buffer[0] == _T('\0'))
                                        throw 1;
									cItem->SetDesc(buffer);
									GetDlgItemText(m_hWnd, IDC_EDT_APP, buffer, EX_MAX_PATH);
									if (buffer[0] == _T('\0'))
                                        throw 2;
									cItem->SetApp(buffer);
									GetDlgItemText(m_hWnd, IDC_EDT_PARAM, buffer, EX_MAX_PATH);
									cItem->SetParam(buffer);
									GetDlgItemText(m_hWnd, IDC_EDT_WORKDIR, buffer, EX_MAX_PATH);
									cItem->SetWorkPath(buffer);

									cItem->SetWindowSize((int) SendDlgItemMessage(m_hWnd, IDC_SELECT_WS, CB_GETCURSEL, 0, 0));

									if (LOWORD(wp) == IDC_BTN_ADD) {
										m_CmdList.InsertItem(cItem);
									}
									else {
										int sel = m_CmdList.GetFirstSelItem();
										if (sel != -1)
											m_CmdList.SetItem(*cItem, sel);
										
										delete cItem;
									}

								} catch (const int ex) {
									switch (ex) {
										case 1:
											MessageBox(m_hWnd, _T("The description field can not be empty!"), _T(APPNAME), MB_OK | MB_ICONERROR);
											break;
										case 2:
											MessageBox(m_hWnd, _T("The application field can not be empty!"), _T(APPNAME), MB_OK | MB_ICONERROR);
											break;
										default:
											delete cItem;
											throw ex; //should never happen
									}
									delete cItem;
								}
							}
							break;
						case IDC_BTN_DELETE:
							m_CmdList.DeleteSelItem();
							break;
						case IDC_CHK_SHOWINGRP:
							prefs::bGrpCmd = !!SendMessage((HWND) lp, BM_GETCHECK, 0, 0);
							break;
						case IDC_BTN_UP:
							{
								int sel = m_CmdList.GetFirstSelItem();
								m_CmdList.SwapItem(sel, sel - 1);
							}
							break;
						case IDC_BTN_DOWN:
							{
								int sel = m_CmdList.GetFirstSelItem();
								m_CmdList.SwapItem(sel, sel + 1);
							}
							break;
						case IDC_BTN_SELAPP:
							{
								OPENFILENAME OFN = {0};
								TCHAR filter[100], filename[EX_MAX_PATH];
								LPTSTR ptr;

								GetDlgItemText(m_hWnd, IDC_EDT_APP, filename, EX_MAX_PATH);

								ptr = filter;

								ptr += (_stprintf(ptr, _T("Applications (*.exe)")) + 1);
								ptr += (_stprintf(ptr, _T("*.exe")) + 1);
								ptr += (_stprintf(ptr, _T("All Files (*.*)")) + 1);
								ptr += (_stprintf(ptr, _T("*.*")) + 1);
								*ptr = _T('\0');

								OFN.lStructSize = sizeof(OPENFILENAME);
								OFN.hwndOwner = m_hWnd;
								OFN.lpstrFilter = filter;
								OFN.lpstrFile = filename;
								OFN.nMaxFile = EX_MAX_PATH;
								OFN.Flags = OFN_ENABLESIZING | OFN_FORCESHOWHIDDEN | OFN_FILEMUSTEXIST;

								if (GetOpenFileName(&OFN))
									SetDlgItemText(m_hWnd, IDC_EDT_APP, filename);
							}
							break;
					}
					break;
			}
			break;
		case WMU_SETCTRLS:
			{
				CmdItem *cItem = (CmdItem *) wp;
				SetDlgItemText(m_hWnd, IDC_EDT_DESC, cItem->GetDesc());
				SetDlgItemText(m_hWnd, IDC_EDT_APP, cItem->GetApp());
				SetDlgItemText(m_hWnd, IDC_EDT_PARAM, cItem->GetParam());
				SetDlgItemText(m_hWnd, IDC_EDT_WORKDIR, cItem->GetWorkPath());
				SendDlgItemMessage(m_hWnd, IDC_SELECT_WS, CB_SETCURSEL, (WPARAM) cItem->GetWindowSize(), 0);
			}
			break;
	}

	return DialogBase::DefWindowProc(msg, wp, lp);
}

void PPgCommand::Reset()
{
	prefs::bExpand.reset();
	prefs::bGrpCmd.reset();
	prefs::bRecursive.reset();
	prefs::bSendDef.reset();
	prefs::sDefPL.reset();
	prefs::sFilterType.reset();
	prefs::sCmdList.reset();
}

