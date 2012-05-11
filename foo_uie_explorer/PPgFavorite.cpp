#include "stdafx.h"
#include "ppgfavorite.h"
#include "resource.h"
#include "preference.h"
#include "others.h"
#include "foo_uie_explorer.h"

PPgFavorite::PPgFavorite()
{
}

PPgFavorite::~PPgFavorite()
{
}

HWND PPgFavorite::Create(HWND hParent)
{
	return DialogBase::Create(hParent, IDD_PPG_FAVORITE);
}

LRESULT PPgFavorite::DefWindowProc(UINT msg, WPARAM wp, LPARAM lp)
{	
	switch (msg) {
		case WM_INITDIALOG:
			{
				m_FavoList.m_hWnd = GetDlgItem(m_hWnd, IDC_FAVO_LIST);
				m_FavoList.InitBinding(m_hWnd);
				m_FavoList.Init();

				SendDlgItemMessage(m_hWnd, IDC_EDT_DESC, EM_SETLIMITTEXT, 128, 0);
				SendDlgItemMessage(m_hWnd, IDC_EDT_FOLDER, EM_SETLIMITTEXT, EX_MAX_PATH - 1, 0);

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

				CheckDlgButton(m_hWnd, IDC_CHK_SHOWINTREE, prefs::iShowFavoAsRoot); //3-state
				CheckThisDlgButton(IDC_CHK_SHOWINGRP, prefs::bGrpFavo);
			}
			break;
		case WM_NOTIFY:
			if (((LPNMHDR) lp)->idFrom == IDC_FAVO_LIST && m_FavoList.OnNotify(wp, lp))
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
								FavoItem *cItem = new FavoItem;
								try {
									GetDlgItemText(m_hWnd, IDC_EDT_DESC, buffer, EX_MAX_PATH);
									if (buffer[0] == _T('\0'))
                                        throw 1;
									cItem->SetDesc(buffer);
									GetDlgItemText(m_hWnd, IDC_EDT_FOLDER, buffer, EX_MAX_PATH);
									if (buffer[0] == _T('\0'))
                                        throw 2;
									cItem->SetPath(buffer);

									if (LOWORD(wp) == IDC_BTN_ADD) {
										m_FavoList.InsertItem(cItem);
									}
									else {
										int sel = m_FavoList.GetFirstSelItem();
										if (sel != -1)
											m_FavoList.SetItem(*cItem, sel);
										
										delete cItem;
									}

								} catch (const int ex) {
									switch (ex) {
										case 1:
											MessageBox(m_hWnd, _T("The description field can not be empty!"), _T(APPNAME), MB_OK | MB_ICONERROR);
											break;
										case 2:
											MessageBox(m_hWnd, _T("The folder field can not be empty!"), _T(APPNAME), MB_OK | MB_ICONERROR);
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
							m_FavoList.DeleteSelItem();
							break;
						case IDC_CHK_SHOWINTREE:
							prefs::iShowFavoAsRoot = (int) SendMessage((HWND) lp, BM_GETCHECK, 0, 0);
							PERFORM_GLOBAL(InitInsert());
							break;
						case IDC_CHK_SHOWINGRP:
							prefs::bGrpFavo = !!SendMessage((HWND) lp, BM_GETCHECK, 0, 0);
							break;
						case IDC_BTN_UP:
							{
								int sel = m_FavoList.GetFirstSelItem();
								m_FavoList.SwapItem(sel, sel - 1);
							}
							break;
						case IDC_BTN_DOWN:
							{
								int sel = m_FavoList.GetFirstSelItem();
								m_FavoList.SwapItem(sel, sel + 1);
							}
							break;
						case IDC_BTN_SELFOLDER:
							{
								TCHAR sFolder[EX_MAX_PATH];
								if (ChooseFolder(sFolder, m_hWnd))
									SetDlgItemText(m_hWnd, IDC_EDT_FOLDER, sFolder);
							}
							break;
					}
					break;
			}
			break;
		case WMU_SETCTRLS:
			{
				FavoItem *cItem = (FavoItem *) wp;
				SetDlgItemText(m_hWnd, IDC_EDT_DESC, cItem->GetDesc());
				SetDlgItemText(m_hWnd, IDC_EDT_FOLDER, cItem->GetPath());
			}
			break;
	}

	return DialogBase::DefWindowProc(msg, wp, lp);
}

void PPgFavorite::Reset()
{
	prefs::bGrpFavo.reset();
	prefs::iShowFavoAsRoot.reset();
	prefs::sFavoList.reset();

	PERFORM_GLOBAL(BulidFavoMenu());
	PERFORM_GLOBAL(InitInsert());
}
