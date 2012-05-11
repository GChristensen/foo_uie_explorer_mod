#include "stdafx.h"
#include "preferencedlg.h"
#include "resource.h"
#include "others.h"
#include "VisualStylesXP.h"

PPgDisplay PreferenceDlg::m_ppgDisplay;
PPgDisplay2 PreferenceDlg::m_ppgDisplay2;
PPgActions PreferenceDlg::m_ppgActions;
PPgCommand PreferenceDlg::m_ppgCommand;
PPgFavorite PreferenceDlg::m_ppgFavorite;
PPgOther PreferenceDlg::m_ppgOther;

PPgBase *PreferenceDlg::m_PPgDlgs[]  = {
	&PreferenceDlg::m_ppgDisplay,
	&PreferenceDlg::m_ppgDisplay2,
	&PreferenceDlg::m_ppgActions,
	&PreferenceDlg::m_ppgCommand,
	&PreferenceDlg::m_ppgFavorite,
	&PreferenceDlg::m_ppgOther
};
int PreferenceDlg::m_iCurWnd = 0;

// {6E59370A-9D49-4919-902D-439207F297C1}
const GUID PreferenceDlg::m_GUID = 
{ 0x6e59370a, 0x9d49, 0x4919, { 0x90, 0x2d, 0x43, 0x92, 0x7, 0xf2, 0x97, 0xc1 } };

PreferenceDlg::PreferenceDlg()
{
}

PreferenceDlg::~PreferenceDlg()
{
}

HWND PreferenceDlg::create(HWND hParent)
{
	return Create(hParent, IDD_PREFERENCE);
}

LRESULT PreferenceDlg::DefWindowProc(UINT msg, WPARAM wp, LPARAM lp)
{
	switch (msg) {
		case WM_INITDIALOG:
			{
				UINT i;

				for (i = 0;i < ARRSIZE(m_PPgDlgs);i++)
					InsertTab(m_PPgDlgs[i]);

				TabCtrl_SetCurSel(GetDlgItem(m_hWnd, IDC_PFR_TAB), m_iCurWnd);
				ShowWindow(*m_PPgDlgs[m_iCurWnd], SW_SHOW);
				SetFocus(*m_PPgDlgs[m_iCurWnd]);
			}
			break;
		case WM_NOTIFY:
			switch (((LPNMHDR) lp)->idFrom) {
				case IDC_PFR_TAB:
					switch (((LPNMHDR) lp)->code) {
					case TCN_SELCHANGE:
						{
							ShowWindow(*m_PPgDlgs[m_iCurWnd], SW_HIDE);
							m_iCurWnd = TabCtrl_GetCurSel(GetDlgItem(m_hWnd, IDC_PFR_TAB));
							ShowWindow(*m_PPgDlgs[m_iCurWnd], SW_SHOW);
						}
						break;
					}
					break;
			}
			break;
	}

	return DialogBase::DefWindowProc(msg, wp, lp);
}

void PreferenceDlg::InsertTab(PPgBase *PPgDlg)
{
	HWND hTab;
	TCITEM tcItem = {0};
	RECT rcTab;

	hTab = GetDlgItem(m_hWnd, IDC_PFR_TAB);
	
	tcItem.mask = TCIF_TEXT;
	tcItem.pszText = (LPTSTR) PPgDlg->GetName();
	TabCtrl_InsertItem(hTab, TabCtrl_GetItemCount(hTab), &tcItem);

	GetWindowRect(hTab, &rcTab);
	MapWindowPoints(HWND_DESKTOP, m_hWnd, (LPPOINT) &rcTab, 2);

	TabCtrl_AdjustRect(hTab, FALSE, &rcTab);

	PPgDlg->Create(m_hWnd);
	MoveWindow(*PPgDlg, rcTab.left, rcTab.top, rcTab.right - rcTab.left, rcTab.bottom - rcTab.top, TRUE);

	if (theStyleXP)
		theStyleXP.EnableThemeDialogTexture(*PPgDlg, ETDT_ENABLETAB);
}

static preferences_page_factory_t<PreferenceDlg> foo;
