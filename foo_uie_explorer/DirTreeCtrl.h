#pragma once

#include "filename.h"
#include "ExMenu.h"
#include "DragDropImpl.h"

#define PERFORM_GLOBAL(op) if (g_instance.get_static_instance().m_hWnd) g_instance.get_static_instance().m_DirTree.op;

#define IDC_EXPLORER		4000
#define IDC_PATHEDIT		4001
#define IDC_BTN_FAVO		4002

#define DRIVELEN ((26 << 2) + 1)
#define ISFULLPATH(s) (_istalpha(s[0]) && s[1] == _T(':') || s[0] == _T('\\') && s[1] == _T('\\'))

#define PREFIX_LONGPATH(s) { (s)[0] = _T('\\'); (s)[1] = _T('\\'); (s)[2] = _T('?'); (s)[3] = _T('\\'); }
#define PREFIX_UNC(s) { PREFIX_LONGPATH((s)); (s)[4] = _T('U'); (s)[5] = _T('N'); (s)[6] = _T('C'); (s)[7] = _T('\\');}

#define DIR_INS_THREADS 4

class FavoItem;
class CShellMenu;
class ThreadParam;

class DirTreeCtrl : public CIDropTarget
{
	friend class DirInsThread;
public:
	DirTreeCtrl()
		: m_hParent(0), m_hImgList(0), m_hEditItem(0), m_hEditBkBrush(0), m_hWnd(0), m_hEditWnd(0),
		m_hBtnWnd(0), m_TreeProc(0), m_EditProc(0), m_hTreeFont(0), m_hEditFont(0), m_pShellMenu(NULL),
		m_sDragFile(NULL), m_hDragItem(NULL), m_hPrevSel(NULL), m_hEvent(NULL), m_bQuit(false)
	{
		m_sEditName[0] = _T('\0');
		memset(m_pInsThreads, 0, sizeof(DirInsThread *) * DIR_INS_THREADS);
		m_hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	}
	virtual ~DirTreeCtrl()
	{
		if (m_sDragFile)
			free(m_sDragFile);
		if (m_hEvent)
			CloseHandle(m_hEvent);
	}

	HWND Create(HWND hParent);
	void Init();
	void InitInsert();
	inline void Refresh(HTREEITEM hParent = TVI_ROOT);

	inline HTREEITEM InsertItem(CFileName *data, HTREEITEM hParent = TVI_ROOT, HTREEITEM hInsertAfter = TVI_LAST);
	HTREEITEM InsertFavorite(FavoItem *fvItem);

	void InsertChildren(HTREEITEM hParent, bool bThread = true);
	CAtlArray<CFileName *> *InsertChildren(LPTSTR sParentPath);
	inline void DeleteChildren(HTREEITEM hParent);

	HTREEITEM FindInSibling(HTREEITEM hItem, LPCTSTR sLabel, LPTSTR *pRoot = NULL) const;
	inline int FindInSibling(HTREEITEM hItem1, HTREEITEM hItem2) const;

	BOOL Expand(HTREEITEM hItem, UINT nCode)
	{
		ASSERT(hItem);
		ASSERT(m_hWnd);
		//reset the state to make sure the content will be updated.
		TreeView_SetItemState(m_hWnd, hItem, 0, TVIS_EXPANDEDONCE);
		return TreeView_Expand(m_hWnd, hItem, nCode);
	}
	HTREEITEM SelectByPath(LPCTSTR sPath);
	HTREEITEM SelectByPath()
	{
		TCHAR sPath[EX_MAX_PATH];
		GetWindowText(m_hEditWnd, sPath, EX_MAX_PATH);
		return SelectByPath(sPath);
	}
	HTREEITEM GetSelectedItem() const
	{
		return TreeView_GetSelection(m_hWnd);
	}
	inline HTREEITEM IsCursorOnItem() const;

	enum MARKTYPE {
		MARKED,
		UNMARKED,
		TOGGLE
	};

	inline void MarkItem(HTREEITEM hItem, MARKTYPE op = TOGGLE);
	size_t GetMarkedCount()
	{
		return m_MarkItems.GetCount();
	}
	bool IsMarked(HTREEITEM hItem) const
	{
		ASSERT(hItem);
		ASSERT(GetItemData(hItem));
		return GetItemData(hItem)->m_bMarked;
	}
	DWORD GetMarkedPathsLength();
	void GetMarkedPaths(LPTSTR buffer);
	void RemoveMarkedItem(HTREEITEM hItem)
	{
		for (size_t i = 0;i < m_MarkItems.GetCount();i++) {
			if (m_MarkItems[i] == hItem) {
				m_MarkItems.RemoveAt(i);
				break;
			}
		}
	}

	inline CFileName *GetItemData(HTREEITEM hItem) const;

	//assume length of buffer is EX_MAX_PATH
	LPCTSTR GetFullPath(HTREEITEM hItem, LPTSTR buffer, bool bWithFileName = true) const;
	LPTSTR GetPrefixFullPath(HTREEITEM hItem, LPTSTR buffer, LPTSTR &sRealPath, bool bWithFileName = true);
	LPCTSTR GetLocation(HTREEITEM hItem, LPTSTR buffer) const
	{
		return GetFullPath(hItem, buffer, false);	
	}
	LPCTSTR GetSelectedPath(LPTSTR buffer) const
	{
		return GetFullPath(GetSelectedItem(), buffer);
	}
	LPCTSTR GetSelectedLocation(LPTSTR buffer) const
	{
		return GetFullPath(GetSelectedItem(), buffer, false);
	}
	inline LPCTSTR GetItemText(HTREEITEM hItem, LPTSTR buffer) const;


	BOOL OnNotify(LPNMHDR lpNMHDR);
	inline void OnSize(int iWidth, int iHeight) const;
	inline bool ParseShortcut(int keys);

	void UpdateColors(bool bRepaint = true);
	void UpdateStyles(bool bRepaint = true) const;
	void UpdateFonts(bool bRepaint = true);
	BOOL RedrawWindow()
	{ return ::RedrawWindow(m_hWnd, NULL, NULL, RDW_INVALIDATE | RDW_INTERNALPAINT | RDW_FRAME | RDW_ERASE | RDW_UPDATENOW); }

	void BulidMenu();
	void BulidFavoMenu();
	void ShowAddressBar() const;
	inline void SetItemHeight();

	static UINT ParseMaskString(LPTSTR *&sList, LPTSTR &sBuffer, /* UTF-8 */const char *sSource);

	operator HWND() const
	{
		return m_hWnd;
	}

	HWND m_hWnd;
	HWND m_hEditWnd;
	HWND m_hBtnWnd;

	HWND m_hParent;

	HBRUSH m_hEditBkBrush;

	virtual LRESULT DefWindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);
	virtual LRESULT EditWindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);

	static void UpdateMaskString();
	static void UpdateFilterString();
	static void ClearMask(bool bInterlocked = true);
	static void ClearFilter(bool bInterlocked = true);

	static LRESULT WINAPI OnHook(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
	static LRESULT WINAPI OnEditHook(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);

	static void CreatePlaylist(pfc::ptr_list_t<const char> &plist, /* UTF-8 */const char *uPath, bool bRecur, bool bFilter);
private:
	inline void OnExpanding(LPNMHDR lpNMHDR);
	inline void OnExpanded(LPNMHDR lpNMHDR);
	inline void OnSelectChanged(LPNMHDR lpNMHDR = NULL);
	inline void OnRClick(LPNMHDR lpNMHDR);
	inline void OnClick();
	inline void OnGetDispInfo(LPNMTVDISPINFO lpTvdi);
	void OnPlaybackCommand(int iCommand);
	void OnContextMenu(int x, int y);

	void InsertChildren(HTREEITEM hParent, CAtlArray<CFileName *> *pFnList);

	LPCTSTR GetFullPath(HTREEITEM hItem, LPTSTR buffer, bool bWithFileName, LPTSTR &pEnd) const;

	ExMenu m_ContextMenu;
	ExMenu m_FavoMenu;
	CShellMenu *m_pShellMenu;

	WNDPROC m_TreeProc;
	WNDPROC m_EditProc;
	HIMAGELIST m_hImgList;
	HFONT m_hTreeFont;
	HFONT m_hEditFont;
	static LPTSTR *m_MaskStrList;
	static LPTSTR m_sMaskStr;
	static UINT m_uMaskCount;
	static LPTSTR *m_FilterStrList;
	static LPTSTR m_sFilterStr;
	static UINT m_uFilterCount;
	TCHAR m_sEditName[EX_MAX_PATH];
	HTREEITEM m_hEditItem;
	HTREEITEM m_hLastFavo;
	HTREEITEM m_hPrevSel;
	CAtlArray<HTREEITEM> m_MarkItems;
public: //Drag 'n' Drop
	virtual bool OnDrop(FORMATETC *pFmtEtc, STGMEDIUM &medium, const POINTL &pt, DWORD *pdwEffect);
	virtual HRESULT STDMETHODCALLTYPE DragEnter(
		/* [unique][in] */ IDataObject __RPC_FAR *pDataObj,
		/* [in] */ DWORD grfKeyState,
		/* [in] */ POINTL pt,
		/* [out][in] */ DWORD __RPC_FAR *pdwEffect);

	virtual bool QueryDrop(DWORD grfKeyState, const POINTL &pt, LPDWORD pdwEffect);
private:
	LPTSTR m_sDragFile;
	HTREEITEM m_hDragItem;


	void Queue(DirInsThreadParam *pParam);
	void Quit();
	bool IsQuitting();

	CAtlList<ThreadParam *> m_InsQueue;
	HANDLE m_hEvent;
	bool m_bQuit;
	CKuCriticalSection m_lock;
	DirInsThread *m_pInsThreads[DIR_INS_THREADS];
};
