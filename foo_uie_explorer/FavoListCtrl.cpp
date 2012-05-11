#include "stdafx.h"
#include "favolistctrl.h"
#include "cmdmap.h"
#include "preference.h"
#include "foo_uie_explorer.h"

FavoItem *FavoItem::m_FavoList[MAX_FAVO];
int FavoItem::m_iFavoCount = 0;

void FavoItem::SetPath(LPCTSTR str)
{
	if (m_sPath)
		free(m_sPath);

	size_t len = _tcslen(str);

	if (str[len - 1] != _T('\\')) {
		CString temp;
		temp.Format(_T("%s\\"), str);
		m_sPath = GetPrefString(temp);
	}
	else
		m_sPath = GetPrefString(str);
}

FavoItem **FavoItem::GetFavoList(bool bReload/* = true*/)
{
	if (!bReload && m_FavoList)
		return m_FavoList;

	pfc::stringcvt::string_os_from_utf8 favolist(prefs::sFavoList);
	LPTSTR pBegin = (LPTSTR) favolist.get_ptr(), pEnd;
	FavoItem *cItem = NULL;
	int i = 0;

	FreeList();
	m_iFavoCount = 0;

	for (pEnd = pBegin;*pEnd;pEnd++) {
		if (*pEnd == _T('|')) {
			*pEnd = _T('\0');
			switch (i % 2) {
				case 0:
					{
						cItem = new FavoItem;
						m_FavoList[m_iFavoCount] = cItem;
						m_iFavoCount++;
						cItem->SetDesc(pBegin);
					}
					break;
				case 1:
					cItem->SetPath(pBegin);
					i = -1;
					break;
			}
			pBegin = pEnd + 1;
			i++;
		}
	}

	m_FavoList[m_iFavoCount] = NULL;

	return m_FavoList;
}

void FavoItem::SaveFavoList(bool bFlushOnEnd/* = false*/)
{
	int n;

	pfc::string8 favolist;

	for (n = 0;n < m_iFavoCount;n++) {
		if (m_FavoList[n]) {
			favolist.add_string(pfc::stringcvt::string_utf8_from_os(m_FavoList[n]->GetDesc()));
			favolist.add_char('|');
			favolist.add_string(pfc::stringcvt::string_utf8_from_os(m_FavoList[n]->GetPath()));
			favolist.add_char('|');
		}
	}

	if (strcmp(prefs::sFavoList, favolist)) {
		prefs::sFavoList = favolist;
		PERFORM_GLOBAL(BulidFavoMenu());
		if (bFlushOnEnd)
			PERFORM_GLOBAL(InitInsert());
	}
}

void FavoItem::FreeList()
{
	int i;
	for (i = 0;i < m_iFavoCount;i++)
		if (m_FavoList[i]) {
			delete m_FavoList[i];
			m_FavoList[i] = NULL;
		}
}

////////////////////////////////////////////////////////////////////////////////////////////

FavoListCtrl::FavoListCtrl()
{
}

FavoListCtrl::~FavoListCtrl()
{
}

void FavoListCtrl::Init()
{
	FavoListCtrlBase::Init();

	LVCOLUMN col = {0};
	col.mask = LVCF_FMT | LVCF_ORDER | LVCF_SUBITEM | LVCF_TEXT | LVCF_WIDTH;
	col.fmt = LVCFMT_LEFT;

#define InsCol(i, s, w) {col.iSubItem = (i);col.iOrder = (i);col.pszText = (s);col.cx = (w);ListView_InsertColumn(m_hWnd, (i), &col);}

	InsCol(0, _T("Description"), 120);
	InsCol(1, _T("Folder"), 280);

#undef InsCol

	pfc::stringcvt::string_os_from_utf8 favolist(prefs::sFavoList);
	LPTSTR pBegin = (LPTSTR) favolist.get_ptr(), pEnd;
	FavoItem *cItem;
	int i = 0;

	for (pEnd = pBegin;*pEnd;pEnd++) {
		if (*pEnd == _T('|')) {
			*pEnd = _T('\0');
			switch (i % 2) {
				case 0:
					{
						cItem = new FavoItem;
						cItem->SetDesc(pBegin);
						InsertItem(cItem);
					}
					break;
				case 1:
					cItem->SetPath(pBegin);
					i = -1;
					break;
			}
			pBegin = pEnd + 1;
			i++;
		}
	}
}

int FavoListCtrl::InsertItem(FavoItem *cItem, int nItem/* = -1*/)
{
	LVITEM lvItem = {0};
	int count = ListView_GetItemCount(m_hWnd);

	if (count >= MAX_FAVO) {
		MessageBox(m_hWnd, _T("Can not support more favorites!"), _T(APPNAME), MB_OK | MB_ICONERROR);
		return -1;
	}

	FavoItem *pItem;

	if (nItem == -1)
		nItem = count + 1;

	int i;
	for (i = 0;i < count;i++) {
		pItem = GetItemData(i);
		if (_tcsicmp(pItem->GetPath(), cItem->GetPath()) == 0) {
			ListView_SetItemState(m_hWnd, i, LVIS_SELECTED, LVIS_SELECTED);
			return -1;
		}
	}

	lvItem.mask = LVIF_PARAM | LVIF_TEXT;
	lvItem.iItem = nItem;
	lvItem.iSubItem = 0;
	lvItem.pszText = LPSTR_TEXTCALLBACK;
	lvItem.lParam = (LPARAM) cItem;

	return ListView_InsertItem(m_hWnd, &lvItem);
}

bool FavoListCtrl::SetItem(const FavoItem &cItem, int nItem)
{
	int i, count = ListView_GetItemCount(m_hWnd);
	FavoItem *pItem;

	for (i = 0;i < count;i++) {
		pItem = GetItemData(i);
		if (_tcsicmp(pItem->GetPath(), cItem.GetPath()) == 0 && i != nItem) {
			ListView_SetItemState(m_hWnd, i, LVIS_SELECTED, LVIS_SELECTED);
			return false;
		}
	}

	pItem = GetItemData(nItem);
	*pItem = cItem;

	ListView_Update(m_hWnd, nItem);

	return true;
}

int CALLBACK FavoListCtrl::CmpFun(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	FavoItem *c1 = (FavoItem *) lParam1, *c2 = (FavoItem *) lParam2;
	int iCol = (int) (lParamSort >> 1);
	bool bAsc = ((lParamSort & 0x00000001L) != 0);
	int result;

	switch (iCol) {
		case 0:
			result = _tcsicmp(c1->GetDesc(), c2->GetDesc());
			break;
		case 1:
			result = _tcsicmp(c1->GetPath(), c2->GetPath());
			break;
	}

	if (!bAsc)
		result = -result;

	return result;
}

void FavoListCtrl::OnGetDispInfo(NMLVDISPINFO *lpDisp)
{
	FavoItem *cmdItem = (FavoItem *) lpDisp->item.lParam;

	if (lpDisp->item.mask & LVIF_TEXT) {
		switch (lpDisp->item.iSubItem) {
			case 0:
				lpDisp->item.pszText = (LPTSTR) cmdItem->GetDesc();
				break;
			case 1:
				lpDisp->item.pszText = (LPTSTR) cmdItem->GetPath();
				break;
		}
	}
}

BOOL FavoListCtrl::OnNotify(WPARAM wParam, LPARAM lParam)
{
	LPNMHDR lpNMHDR = (LPNMHDR) lParam;

	switch (lpNMHDR->code) {
		case LVN_GETDISPINFO:
			OnGetDispInfo((NMLVDISPINFO *) lParam);
			break;
		case LVN_DELETEITEM:
			{
				FavoItem *cItem = GetItemData(((LPNMLISTVIEW) lParam)->iItem);
				if (cItem)
					delete cItem;
			}
			break;
		case LVN_ITEMCHANGED:
			{
				LPNMLISTVIEW pnmv = (LPNMLISTVIEW) lParam;
				if (pnmv->iItem != -1 && (pnmv->uChanged & LVIF_STATE) &&
					(pnmv->uNewState & LVIS_SELECTED) && !(pnmv->uOldState & LVIS_SELECTED))
				{
					FavoItem *cItem = GetItemData(pnmv->iItem);
					SendMessage(m_hParent, WMU_SETCTRLS, (WPARAM) cItem, 0);
				}
			}
			break;
		case LVN_COLUMNCLICK:
			{
				LPNMLISTVIEW pnmv = (LPNMLISTVIEW) lParam; 
				SortItems(pnmv->iSubItem);
			}
			break;
	}

	return FALSE;
}

LRESULT FavoListCtrl::DefWindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
	switch (Msg) {
		case WM_DESTROY:
			{
				int i, count = ListView_GetItemCount(m_hWnd);
				FavoItem *cItem;
				pfc::string8 favolist;

				for (i = 0;i < count;i++) {
					cItem = GetItemData(i);
					favolist.add_string(pfc::stringcvt::string_utf8_from_os(cItem->GetDesc()));
					favolist.add_char('|');
					favolist.add_string(pfc::stringcvt::string_utf8_from_os(cItem->GetPath()));
					favolist.add_char('|');
				}

				if (strcmp(prefs::sFavoList, favolist)) {
					prefs::sFavoList = favolist;
					PERFORM_GLOBAL(BulidFavoMenu());
					PERFORM_GLOBAL(InitInsert());
				}
			}
			break;
	}

	return FavoListCtrlBase::DefWindowProc(Msg, wParam, lParam);
}
