#include "stdafx.h"
#include "ppgother.h"
#include "resource.h"
#include "preference.h"
#include "others.h"

PPgOther::PPgOther()
{
}

PPgOther::~PPgOther()
{
}

HWND PPgOther::Create(HWND hParent)
{
	return DialogBase::Create(hParent, IDD_PPG_OTHER);
}


LRESULT PPgOther::DefWindowProc(UINT msg, WPARAM wp, LPARAM lp)
{	
	switch (msg) {
		case WM_INITDIALOG:
			{
				uSetDlgItemText(m_hWnd, IDC_EDT_FILTER, prefs::sFilterType);
				uSetDlgItemText(m_hWnd, IDC_EDT_DEF_PL, prefs::sDefPL);

				CheckThisDlgButton(IDC_CHK_RECUR, prefs::bRecursive);
				CheckThisDlgButton(IDC_CHK_EXPAND, prefs::bExpand);
				CheckThisDlgButton(IDC_CHK_DEFAULT, prefs::bSendDef);
			}
			break;
		case WM_COMMAND:
			switch (HIWORD(wp)) {
				case 0:
					switch (LOWORD(wp)) {
						case IDC_CHK_RECUR:
							prefs::bRecursive = !!SendMessage((HWND) lp, BM_GETCHECK, 0, 0);
							break;
						case IDC_CHK_EXPAND:
							prefs::bExpand = !!SendMessage((HWND) lp, BM_GETCHECK, 0, 0);
							break;
						case IDC_CHK_DEFAULT:
							prefs::bSendDef = !!SendMessage((HWND) lp, BM_GETCHECK, 0, 0);
							break;
					}
					break;
				case EN_KILLFOCUS:
					switch(LOWORD(wp)) {
						case IDC_EDT_FILTER:
							prefs::sFilterType = string_utf8_from_window((HWND) lp);
							DirTreeCtrl::UpdateFilterString();
							break;
						case IDC_EDT_DEF_PL:
							prefs::sDefPL = string_utf8_from_window((HWND) lp);
							break;
					}
					break;
			}
			break;
	}


	return DialogBase::DefWindowProc(msg, wp, lp);
}

void PPgOther::Reset()
{
	prefs::bExpand.reset();
	prefs::bRecursive.reset();
	prefs::bSendDef.reset();
	prefs::sDefPL.reset();
	prefs::sFilterType.reset();
}
