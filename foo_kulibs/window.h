#pragma once

class WindowBase
{
public:
	WindowBase() : m_hWnd(0), m_hParent(0), m_WndProc(0), m_bIsRunning(false) {}
	virtual ~WindowBase() {}

	void InitBinding(HWND hParent)
	{
		m_hParent = hParent;
		SetWindowLongPtr(m_hWnd, GWLP_USERDATA, (LONG)((INT_PTR) this));
		m_WndProc = (WNDPROC) SetWindowLongPtr(m_hWnd, GWLP_WNDPROC, (LONG)((INT_PTR) WndProc));
		m_bIsRunning = true;
	}

	bool IsRunning() const
	{
		return m_bIsRunning;
	}

	virtual bool IsDialog() const
	{
		return false;
	}

	operator HWND() const
	{
		return m_hWnd;
	}

	HWND m_hWnd;
	HWND m_hParent;

protected:
	virtual LRESULT DefWindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
	{
		switch (Msg) {
			case WM_INITDIALOG:
			case WM_CREATE:
				m_bIsRunning = true;
				break;
			case WM_DESTROY:
				m_bIsRunning = false;
				break;
		}

		return (IsDialog() || !m_WndProc) ? FALSE : CallWindowProc(m_WndProc, m_hWnd, Msg, wParam, lParam);
	}
	static LRESULT WINAPI WndProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
	{
		WindowBase *pThis = NULL;
		LRESULT res;

		pThis = reinterpret_cast<WindowBase*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

		res = pThis ? pThis->DefWindowProc(Msg, wParam, lParam) : ::DefWindowProc(hWnd, Msg, wParam, lParam);

		return res;
	}
	WNDPROC m_WndProc;
	bool m_bIsRunning;
};

class DialogBase : public WindowBase
{
public:
	DialogBase() {}
	virtual ~DialogBase() {}
	virtual HWND Create(HWND hParent, UINT idResource)
	{
		m_hParent = hParent;
		CreateDialogParam(core_api::get_my_instance(), MAKEINTRESOURCE(idResource), hParent, (DLGPROC) DlgProc, (LPARAM) this);
		m_bIsRunning = true;

		return m_hWnd;
	}
	virtual bool IsDialog() const
	{
		return true;
	}
protected:
	static LRESULT WINAPI DlgProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
	{
		DialogBase *pThis = NULL;
		LRESULT res;

		if (Msg == WM_INITDIALOG) {
			pThis = reinterpret_cast<DialogBase*>(lParam);
			pThis->m_hWnd = hWnd;
			SetWindowLongPtr(hWnd, DWLP_USER, (LONG)((INT_PTR) pThis));
		}
		else
			pThis = reinterpret_cast<DialogBase*>(GetWindowLongPtr(hWnd, DWLP_USER));

		res = pThis ? pThis->DefWindowProc(Msg, wParam, lParam) : FALSE;

		return res;
	}
};

template <class ItemT>
class ListCtrlBase : public WindowBase
{
public:
	ListCtrlBase() : m_hHeader(0), m_bSortAsc(true), m_iSortCol(0)
	{
	}

	virtual ~ListCtrlBase()
	{
	}

	virtual void Init()
	{
		m_hHeader = ListView_GetHeader(m_hWnd);
		ListView_SetExtendedListViewStyleEx(m_hWnd, LVS_EX_FULLROWSELECT | LVS_EX_HEADERDRAGDROP, LVS_EX_FULLROWSELECT | LVS_EX_HEADERDRAGDROP);
	}

	inline ItemT *GetItemData(int nItem)
	{
		LVITEM lvItem = {0};

		lvItem.mask = LVIF_PARAM;
		lvItem.iItem = nItem;

		ListView_GetItem(m_hWnd, &lvItem);

		return (ItemT *) lvItem.lParam;
	}

	void DeleteSelItem()
	{
		int sel = GetFirstSelItem();
		if (sel != -1) {
			int count = ListView_GetItemCount(m_hWnd);
			if (count > 1)
				ListView_SetItemState(m_hWnd, (sel != count - 1) ? sel + 1 : sel - 1, LVIS_SELECTED, LVIS_SELECTED);
			ListView_DeleteItem(m_hWnd, sel);
		}
	}

	void SwapItem(int iItem1, int iItem2)
	{
		int count = ListView_GetItemCount(m_hWnd);

		if (iItem1 < 0 || iItem1 >= count || iItem2 < 0 || iItem2 >= count)
			return;

		ItemT *cItem1;
		int state1;
		LVITEM lvItem = {0};

		lvItem.mask = LVIF_PARAM | LVIF_STATE;
		lvItem.iItem = iItem1;
		lvItem.stateMask = LVIS_SELECTED | LVIS_FOCUSED;

		ListView_GetItem(m_hWnd, &lvItem);

		state1 = lvItem.state;
		cItem1 = (ItemT *) lvItem.lParam;

		lvItem.iItem = iItem2;

		ListView_GetItem(m_hWnd, &lvItem);

		lvItem.iItem = iItem1;

		ListView_SetItem(m_hWnd, &lvItem);

		lvItem.iItem = iItem2;
		lvItem.lParam = (LPARAM) cItem1;
		lvItem.state = state1;

		ListView_SetItem(m_hWnd, &lvItem);
	}

	void SetSortArrow(int iCol, bool bAsc)
	{
		HDITEM col;

		col.mask = HDI_FORMAT;
		col.fmt = HDF_STRING;
		
		Header_SetItem(m_hHeader, m_iSortCol, &col);

		col.fmt |= bAsc ? HDF_SORTUP : HDF_SORTDOWN;

		Header_SetItem(m_hHeader, iCol, &col);

		m_iSortCol = iCol;
	}

	virtual BOOL SortItems(int iCol, bool bAsc, PFNLVCOMPARE cmpfun)
	{
		BOOL sortRes = ListView_SortItems(m_hWnd, cmpfun, (iCol << 1) | (bAsc ? 1 : 0));

		SetSortArrow(iCol, bAsc);

		m_iSortCol = iCol;
		m_bSortAsc = bAsc;

		return sortRes;
	}

	virtual BOOL SortItems(int iCol, PFNLVCOMPARE cmpfun)
	{
		if (iCol < 0)
			return SortItems(m_iSortCol, m_bSortAsc, cmpfun);
		else if (iCol == m_iSortCol)
			return SortItems(iCol, !m_bSortAsc, cmpfun);
		else
			return SortItems(iCol, true, cmpfun);
	}

	int GetFirstSelItem()
	{
		return ListView_GetNextItem(m_hWnd, -1, LVNI_SELECTED);
	}

protected:
	virtual LRESULT DefWindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
	{
		switch (Msg) {
			case WM_KEYDOWN:
				switch (wParam) {
					case VK_DELETE:
						DeleteSelItem();
						break;
				}
				break;
		}

		return WindowBase::DefWindowProc(Msg, wParam, lParam);
	}

	HWND m_hHeader;
	bool m_bSortAsc;
	int m_iSortCol;
};
