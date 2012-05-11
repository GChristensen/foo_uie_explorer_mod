#include "stdafx.h"
#include "actlistctrl.h"
#include "preference.h"

ActItem::ActItem()
{
	m_iTriggerKey = 0;
	id = 0;
	desc = NULL;
	_tcscpy(m_sKeyDesc, _T("   Invalid"));
}

LPCTSTR ActItem::SetKeyString(LPTSTR outStr, int keys)
{
	LPTSTR str = outStr;
	LPCTSTR keyName, result = outStr + 3;

	_tcscpy(outStr, _T("   Invalid"));

	if (keys & CI_MK_CTRL)
		str += (_stprintf(str, _T("   Ctrl")));
	if (keys & CI_MK_ALT)
		str += (_stprintf(str, _T(" + Alt")));
	if (keys & CI_MK_SHIFT)
		str += (_stprintf(str, _T(" + Shift")));
	if (keys & CI_MK_WIN)
		str += (_stprintf(str, _T(" + Win")));

	if (keys & CI_MC_MASK) {
		switch (keys & CI_MC_MASK) {
			case CI_MC_LCLICK:
				str += (_stprintf(str, _T(" + Left-click")));
				break;
			case CI_MC_DBLCLICK:
				str += (_stprintf(str, _T(" + Double-click")));
				break;
			case CI_MC_MCLICK:
				str += (_stprintf(str, _T(" + Mid-click")));
				break;
			default:
				_tcscpy(outStr, _T("   Invalid"));
				result = NULL;
				break;
		}
	}
	else if (keys & CI_KB_MASK) {
		WORD vKey = LOWORD(keys);
		if (INRANGE(vKey, _T('0'), _T('Z')) && vKey != 0x40)
			str += (_stprintf(str, _T(" + %c"), vKey));
		else if (vKey != VK_DECIMAL && vKey != VK_SEPARATOR && INRANGE(vKey, VK_NUMPAD0, VK_DIVIDE) || vKey == VK_OEM_NEC_EQUAL) {
			TCHAR ch;
			if (vKey <= VK_NUMPAD9)
				ch = vKey - VK_NUMPAD0 + _T('0');
			else {
				switch (vKey) {
					case VK_MULTIPLY:
						ch = _T('*');
						break;
					case VK_ADD:
						ch = _T('+');
						break;
					case VK_SUBTRACT:
						ch = _T('-');
						break;
					case VK_DIVIDE:
						ch = _T('/');
						break;
					case VK_OEM_NEC_EQUAL:
						ch = _T('=');
						break;
				}
			}
			str += (_stprintf(str, _T(" + Num %c"), ch));
		}
		else if (INRANGE(vKey, VK_F1, VK_F24))
			str += (_stprintf(str, _T(" + F%d"), vKey - VK_F1 + 1));
		else {
			switch (vKey) {
				case VK_DECIMAL:
					keyName = _T("Num Del");
					break;
				case VK_NUMLOCK:
					keyName = _T("Num Lock");
					break;
				case VK_SCROLL:
					keyName = _T("Scroll Lock");
					break;
				case VK_OEM_1:
					keyName = _T(";");
					break;
				case VK_OEM_PLUS:
					keyName = _T("+");
					break;
				case VK_OEM_COMMA:
					keyName = _T(",");
					break;
				case VK_OEM_MINUS:
					keyName = _T("-");
					break;
				case VK_OEM_PERIOD:
					keyName = _T(".");
					break;
				case VK_OEM_2:
					keyName = _T("/");
					break;
				case VK_OEM_3:
					keyName = _T("`");
					break;
				case VK_OEM_4:
					keyName = _T("[");
					break;
				case VK_OEM_5:
					keyName = _T("\\");
					break;
				case VK_OEM_6:
					keyName = _T("]");
					break;
				case VK_OEM_7:
					keyName = _T("'");
					break;
				case VK_RETURN:
					keyName = _T("Enter");
					break;
				case VK_ESCAPE:
					keyName = _T("Esc");
					break;
				case VK_SPACE:
					keyName = _T("Space");
					break;
				case VK_PRIOR:
					keyName = _T("PgUp");
					break;
				case VK_NEXT:
					keyName = _T("PgDown");
					break;
				case VK_END:
					keyName = _T("End");
					break;
				case VK_HOME:
					keyName = _T("Home");
					break;
				case VK_LEFT:
					keyName = _T("Left");
					break;
				case VK_UP:
					keyName = _T("Up");
					break;
				case VK_RIGHT:
					keyName = _T("Right");
					break;
				case VK_DOWN:
					keyName = _T("Down");
					break;
				case VK_INSERT:
					keyName = _T("Insert");
					break;
				case VK_DELETE:
					keyName = _T("Delete");
					break;
				default:
					keyName = _T("Unknown");
					result = NULL;
					break;
			}
			str += (_stprintf(str, _T(" + %s"), keyName));
		}
	}
	else {
		_tcscpy(outStr, _T("   Invalid"));
		result = NULL;
	}

	return result;
}


bool ActItem::SetCommand(int _id)
{
	int idx = FindCmd(_id);

	if (idx == -1)
		return false;

	id = _id;
	desc = CMD_MAP[idx].desc;

	return true;
}

/////////////////////////////////////////////////////////////////////////////////////////

ActListCtrl::ActListCtrl()
{
}

ActListCtrl::~ActListCtrl()
{
}

void ActListCtrl::Init()
{
	ActListCtrlBase::Init();

	LVCOLUMN col = {0};
	col.mask = LVCF_FMT | LVCF_ORDER | LVCF_SUBITEM | LVCF_TEXT | LVCF_WIDTH;
	col.fmt = LVCFMT_LEFT;

#define InsCol(i, s, w) {col.iSubItem = (i);col.iOrder = (i);col.pszText = (s);col.cx = (w);ListView_InsertColumn(m_hWnd, (i), &col);}

	InsCol(0, _T("Keys"), 120);
	InsCol(1, _T("Action"), 300);

#undef InsCol

	{
		int i;
		ActItem *cItem;
		_sKeyData *keyArray = prefs::aShortcuts;
		for (i = 0;i < prefs::aShortcuts.GetCount();i++) {
			cItem = new ActItem(keyArray[i]);
			if (cItem->IsValid())
				InsertItem(cItem);
			else
				delete cItem;
		}
	}
}

int ActListCtrl::InsertItem(ActItem *cItem, int nItem/* = -1*/)
{
	LVITEM lvItem = {0};
	int i, count = ListView_GetItemCount(m_hWnd);
	ActItem *pItem;

	if (nItem == -1)
		nItem = count + 1;

	for (i = 0;i < count;i++) {
		pItem = GetItemData(i);
		if (pItem->GetKeys() == cItem->GetKeys()) {
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

bool ActListCtrl::SetItem(const ActItem &cItem, int nItem)
{
	int i, count = ListView_GetItemCount(m_hWnd);
	ActItem *pItem;

	for (i = 0;i < count;i++) {
		pItem = GetItemData(i);
		if (pItem->GetKeys() == cItem.GetKeys() && i != nItem) {
			ListView_SetItemState(m_hWnd, i, LVIS_SELECTED, LVIS_SELECTED);
			return false;
		}
	}

	pItem = GetItemData(nItem);
	*pItem = cItem;

	ListView_Update(m_hWnd, nItem);

	return true;
}

int CALLBACK ActListCtrl::CmpFun(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	ActItem *c1 = (ActItem *) lParam1, *c2 = (ActItem *) lParam2;
	int iCol = (int) (lParamSort >> 1);
	bool bAsc = ((lParamSort & 0x00000001L) != 0);
	int result;

	switch (iCol) {
		case 0:
			result = _tcsicmp(c1->GetKeyString(), c2->GetKeyString());
			break;
		case 1:
			result = _tcsicmp(c1->desc, c2->desc);
			break;
	}

	if (!bAsc)
		result = -result;

	return result;
}

void ActListCtrl::OnGetDispInfo(NMLVDISPINFO *lpDisp)
{
	ActItem *cmdItem = (ActItem *) lpDisp->item.lParam;

	if (lpDisp->item.mask & LVIF_TEXT) {
		switch (lpDisp->item.iSubItem) {
			case 0:
				lpDisp->item.pszText = (LPTSTR) cmdItem->GetKeyString();
				break;
			case 1:
				lpDisp->item.pszText = (LPTSTR) cmdItem->desc;
				break;
		}
	}
}

BOOL ActListCtrl::OnNotify(WPARAM wParam, LPARAM lParam)
{
	LPNMHDR lpNMHDR = (LPNMHDR) lParam;

	switch (lpNMHDR->code) {
		case LVN_GETDISPINFO:
			OnGetDispInfo((NMLVDISPINFO *) lParam);
			break;
		case LVN_DELETEITEM:
			{
				ActItem *cItem = GetItemData(((LPNMLISTVIEW) lParam)->iItem);
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
					ActItem *cItem = GetItemData(pnmv->iItem);
					SendMessage(m_hParent, WMU_SETCTRLS, cItem->GetKeys(), cItem->id);
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

LRESULT ActListCtrl::DefWindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
	switch (Msg) {
		case WM_DESTROY:
			{
				int i, count = ListView_GetItemCount(m_hWnd);
				_sKeyData *keyArray = (_sKeyData *) malloc(sizeof(_sKeyData) * count);
				for (i = 0;i < count;i++)
					keyArray[i] = GetItemData(i)->GetKeyData();
				prefs::aShortcuts.Attach(keyArray, count);
			}
			break;
	}

	return ActListCtrlBase::DefWindowProc(Msg, wParam, lParam);
}
