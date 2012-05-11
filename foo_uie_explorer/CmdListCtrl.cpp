#include "stdafx.h"
#include "cmdlistctrl.h"
#include "foo_uie_explorer.h"
#include "preference.h"

CmdItem *CmdItem::m_CmdList[MAX_CMD] = {0};
int CmdItem::m_iCmdCount = 0;

bool CmdItem::PerformCommand(CFileName &fnItem)
{
	TCHAR sParam[65535], sWD[65535];
	INT WS;

	sParam[0] = _T('\0');
	sWD[0] = _T('\0');

	ReplaceVars(GetParam(), sParam, ARRSIZE(sParam), fnItem, true);
	ReplaceVars(GetWorkPath(), sWD, ARRSIZE(sWD), fnItem);

	switch (m_iWindowSize) {
		case CMD_WS_NORMAL:
			WS = SW_SHOWNORMAL;
			break;
		case CMD_WS_MAX:
			WS = SW_SHOWMAXIMIZED;
			break;
		case CMD_WS_MIN:
			WS = SW_SHOWMINIMIZED;
			break;
	}

	ShellExecute(NULL, NULL, GetApp(), sParam, sWD, WS);

	return false;
}

void CmdItem::ReplaceVars(LPCTSTR in, LPTSTR out, UINT cch, CFileName &fnItem, bool bQuote/*= false*/)
{
	LPTSTR ptr;
	LPCTSTR ext, title, fn, fp, loc, outStart = out, outEnd = out + cch - 1;

	fn = fnItem.GetName();
	fp = fnItem.GetFullPath();
	loc = fnItem.GetLocation();

	if (fnItem.IsFile()) {
		ext = fnItem.GetType();
		title = fnItem.GetTitle();
	}
	else {
		ext = _T("");
		title = fn;
	}

	//avoid access NULL
	fn = SAFESTR(fn);
	fp = SAFESTR(fp);
	loc = SAFESTR(loc);
	ext = SAFESTR(ext);
	title = SAFESTR(title);

#define RPTHEVAR(v, q) {\
	in++;\
	ptr = (LPTSTR)(v);\
	if (q && out < outEnd) *out++ = _T('"');\
	while ((out < outEnd) && (*out = *ptr)) {\
		out++;\
		ptr++;\
	}\
	if (q) {\
		if (out > outStart && *(out - 1) == _T('\\'))\
			*out--;\
		if (out < outEnd)\
			*out++ = _T('"');\
	}\
}

	while (*in) {
		if (*in == _T(':')) {
			in++;
			switch(*in) {
				case 'D':
				case 'd':
					RPTHEVAR(loc, bQuote);
					continue;
				case 'P':
				case 'p':
					RPTHEVAR(fp, bQuote);
					continue;
				case 'T':
				case 't':
					RPTHEVAR(title, bQuote);
					continue;
				case 'N':
				case 'n':
					RPTHEVAR(fn, bQuote);
					continue;
				case 'E':
				case 'e':
					RPTHEVAR(ext, false);
					continue;
				case ':':
					break;
				default:
					in--;
					break;
			}
		}
		if (out < outEnd)
			*out++ = *in++;
	}

#undef RPTHEVAR

	*out = _T('\0');
}

CmdItem **CmdItem::GetCmdList(bool bReload/* = true*/)
{
	if (!bReload && m_CmdList)
		return m_CmdList;

	pfc::stringcvt::string_os_from_utf8 cmdlist(prefs::sCmdList);
	LPTSTR pBegin = (LPTSTR) cmdlist.get_ptr(), pEnd;
	CmdItem *cItem = NULL;
	int i = 0;

	FreeList();
	m_iCmdCount = 0;

	for (pEnd = pBegin;*pEnd;pEnd++) {
		if (*pEnd == _T('|')) {
			*pEnd = _T('\0');
			switch (i % 5) {
				case 0:
					{
						cItem = new CmdItem;
						m_CmdList[m_iCmdCount] = cItem;
						m_iCmdCount++;
						cItem->SetWindowSize(*pBegin - _T('0'));
					}
					break;
				case 1:
					cItem->SetDesc(pBegin);
					break;
				case 2:
					cItem->SetApp(pBegin);
					break;
				case 3:
					cItem->SetParam(pBegin);
					break;
				case 4:
					cItem->SetWorkPath(pBegin);
					i = -1;
					break;
			}
			pBegin = pEnd + 1;
			i++;
		}
	}

	m_CmdList[m_iCmdCount] = NULL;

	return m_CmdList;
}

void CmdItem::FreeList()
{
	int i;
	for (i = 0;i < m_iCmdCount;i++)
		if (m_CmdList[i]) {
			delete m_CmdList[i];
			m_CmdList[i] = NULL;
		}
}

////////////////////////////////////////////////////////////////////////

CmdListCtrl::CmdListCtrl()
{
}

CmdListCtrl::~CmdListCtrl()
{
}

void CmdListCtrl::Init()
{
	CmdListCtrlBase::Init();

	LVCOLUMN col = {0};
	col.mask = LVCF_FMT | LVCF_ORDER | LVCF_SUBITEM | LVCF_TEXT | LVCF_WIDTH;
	col.fmt = LVCFMT_LEFT;

#define InsCol(i, s, w) {col.iSubItem = (i);col.iOrder = (i);col.pszText = (s);col.cx = (w);ListView_InsertColumn(m_hWnd, (i), &col);}

	InsCol(0, _T("Description"), 120);
	InsCol(1, _T("Application"), 200);
	InsCol(2, _T("Parameters"), 120);
	InsCol(3, _T("Work Path"), 200);
	InsCol(4, _T("Window Size"), 120);

#undef InsCol

	pfc::stringcvt::string_os_from_utf8 cmdlist(prefs::sCmdList);
	LPTSTR pBegin = (LPTSTR) cmdlist.get_ptr(), pEnd;
	CmdItem *cItem;
	int i = 0;

	for (pEnd = pBegin;*pEnd;pEnd++) {
		if (*pEnd == _T('|')) {
			*pEnd = _T('\0');
			switch (i % 5) {
				case 0:
					{
						cItem = new CmdItem;
						cItem->SetWindowSize(*pBegin - _T('0'));
						InsertItem(cItem);
					}
					break;
				case 1:
					cItem->SetDesc(pBegin);
					break;
				case 2:
					cItem->SetApp(pBegin);
					break;
				case 3:
					cItem->SetParam(pBegin);
					break;
				case 4:
					cItem->SetWorkPath(pBegin);
					i = -1;
					break;
			}
			pBegin = pEnd + 1;
			i++;
		}
	}
}

int CmdListCtrl::InsertItem(CmdItem *cItem, int nItem/* = -1*/)
{
	LVITEM lvItem = {0};
	int count = ListView_GetItemCount(m_hWnd);

	if (count >= MAX_CMD) {
		MessageBox(m_hWnd, _T("Can not support more command line shortcuts!"), _T(APPNAME), MB_OK | MB_ICONERROR);
		return -1;
	}

	if (nItem == -1)
		nItem = count + 1;

	lvItem.mask = LVIF_PARAM | LVIF_TEXT;
	lvItem.iItem = nItem;
	lvItem.iSubItem = 0;
	lvItem.pszText = LPSTR_TEXTCALLBACK;
	lvItem.lParam = (LPARAM) cItem;

	return ListView_InsertItem(m_hWnd, &lvItem);
}

bool CmdListCtrl::SetItem(const CmdItem &cItem, int nItem)
{
	CmdItem *pItem;

	pItem = GetItemData(nItem);
	*pItem = cItem;

	ListView_Update(m_hWnd, nItem);

	return true;
}

int CALLBACK CmdListCtrl::CmpFun(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	CmdItem *c1 = (CmdItem *) lParam1, *c2 = (CmdItem *) lParam2;
	int iCol = (int) (lParamSort >> 1);
	bool bAsc = ((lParamSort & 0x00000001L) != 0);
	int result;

	switch (iCol) {
		case 0:
			result = _tcsicmp(c1->GetDesc(), c2->GetDesc());
			break;
		case 1:
			result = _tcsicmp(c1->GetApp(), c2->GetApp());
			break;
		case 2:
			result = _tcsicmp(c1->GetParam(), c2->GetParam());
			break;
		case 3:
			result = _tcsicmp(c1->GetWorkPath(), c2->GetWorkPath());
			break;
		case 4:
			result = _tcsicmp(c1->GetWindowSizeStr(), c2->GetWindowSizeStr());
			break;
	}

	if (!bAsc)
		result = -result;

	return result;
}

void CmdListCtrl::OnGetDispInfo(NMLVDISPINFO *lpDisp)
{
	CmdItem *cmdItem = (CmdItem *) lpDisp->item.lParam;

	if (lpDisp->item.mask & LVIF_TEXT) {
		switch (lpDisp->item.iSubItem) {
			case 0:
				lpDisp->item.pszText = (LPTSTR) cmdItem->GetDesc();
				break;
			case 1:
				lpDisp->item.pszText = (LPTSTR) cmdItem->GetApp();
				break;
			case 2:
				lpDisp->item.pszText = (LPTSTR) cmdItem->GetParam();
				break;
			case 3:
				lpDisp->item.pszText = (LPTSTR) cmdItem->GetWorkPath();
				break;
			case 4:
				lpDisp->item.pszText = (LPTSTR) cmdItem->GetWindowSizeStr();
				break;
		}
	}
}

BOOL CmdListCtrl::OnNotify(WPARAM wParam, LPARAM lParam)
{
	LPNMHDR lpNMHDR = (LPNMHDR) lParam;

	switch (lpNMHDR->code) {
		case LVN_GETDISPINFO:
			OnGetDispInfo((NMLVDISPINFO *) lParam);
			break;
		case LVN_DELETEITEM:
			{
				CmdItem *cItem = GetItemData(((LPNMLISTVIEW) lParam)->iItem);
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
					CmdItem *cItem = GetItemData(pnmv->iItem);
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

LRESULT CmdListCtrl::DefWindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
	switch (Msg) {
		case WM_DESTROY:
			{
				int i, count = ListView_GetItemCount(m_hWnd);
				CmdItem *cItem;
				pfc::string8 cmdlist;
				char wsize[3] = " |";

//				cmdlist.prealloc(EX_MAX_PATH);
//				cmdlist.set_mem_logic(mem_block::ALLOC_FAST_DONTGODOWN); //we deallocate it soon. :)

				for (i = 0;i < count;i++) {
					cItem = GetItemData(i);
					wsize[0] = (char) (cItem->GetWindowSize() + '0');
					cmdlist.add_string(wsize);
					cmdlist.add_string(pfc::stringcvt::string_utf8_from_os(cItem->GetDesc()));
					cmdlist.add_char('|');
					cmdlist.add_string(pfc::stringcvt::string_utf8_from_os(cItem->GetApp()));
					cmdlist.add_char('|');
					cmdlist.add_string(pfc::stringcvt::string_utf8_from_os(cItem->GetParam()));
					cmdlist.add_char('|');
					cmdlist.add_string(pfc::stringcvt::string_utf8_from_os(cItem->GetWorkPath()));
					cmdlist.add_char('|');
				}

				prefs::sCmdList = cmdlist;
			}
			break;
	}

	return CmdListCtrlBase::DefWindowProc(Msg, wParam, lParam);
}
