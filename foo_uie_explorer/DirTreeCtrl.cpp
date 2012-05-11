#include "stdafx.h"
#include "DirTreeCtrl.h"
#include "FavoListCtrl.h"
#include "Preference.h"
#include "cmdmap.h"
#include "resource.h"
#include "ActListCtrl.h"
#include "CmdListCtrl.h"
#include "PreferenceDlg.h"
#include "others.h"
#include "MemDC.h"
#include "ShellContextMenu.h"
#include "CuiAppearance.h"

class ThreadParam
{
public:
	ThreadParam() : m_hThread(NULL) {}
	virtual ~ThreadParam() { if (m_hThread) CloseHandle(m_hThread); }
	virtual DWORD DoThreadOp() = 0;
	DWORD StartThread(DWORD dwPriority = THREAD_PRIORITY_NORMAL)
	{
		DWORD dwThreadId;

		m_hThread = CreateThread(NULL, 0, ThreadProc, (LPVOID) this, 0, &dwThreadId);
		if (dwPriority != THREAD_PRIORITY_NORMAL)
			SetThreadPriority(m_hThread, dwPriority);

		return dwThreadId;
	}

	HANDLE m_hThread;
private:
	static DWORD WINAPI ThreadProc(LPVOID pThis)
	{
		return ((ThreadParam *) pThis)->DoThreadOp();
	}
};

class DirInsThreadParam : public CKuCriticalSection
{
public:
	DirInsThreadParam()
		: m_hParentItem(NULL), m_sParentPath(NULL), m_pFnList(NULL), m_bIsValid(true), m_pParent(NULL)
	{}
	virtual ~DirInsThreadParam()
	{
		if (m_sParentPath)
			free(m_sParentPath);
		if (m_pFnList) {
			if (!IsValid()) {
				for (size_t i = 0;i < m_pFnList->GetCount();i++)
					delete m_pFnList->GetAt(i);
			}
			delete m_pFnList;
		}
	}

	bool IsValid()
	{
		CKuAutoCriticalSection theLocker(this);
		return m_bIsValid;
	}
	void Invalidate()
	{
		CKuAutoCriticalSection theLocker(this);
		if (m_bIsValid) {
			m_bIsValid = false;
			m_pParent = NULL;
			m_hParentItem = NULL;
		}
	}

	virtual DWORD DoThreadOp() { return 0; }

	CAtlArray<CFileName *> *m_pFnList;
	HTREEITEM m_hParentItem;
	LPTSTR m_sParentPath;
	CFileName *m_pParent;
private:
	bool m_bIsValid;
};

class DirInsThread : public ThreadParam
{
public:
	explicit DirInsThread(DirTreeCtrl *pDirTree)
		: m_pDirTree(pDirTree)
	{
	}
	virtual ~DirInsThread()
	{
	}

	DWORD DoThreadOp()
	{
		while (WaitForSingleObject(m_pDirTree->m_hEvent, INFINITE) == WAIT_OBJECT_0) {
			while (true) {
				DirInsThreadParam *pParam = NULL;
				{
					CKuAutoCriticalSection theLocker(&m_pDirTree->m_lock);
					if (m_pDirTree->m_bQuit)
						goto EndOfThread;
					if (m_pDirTree->m_InsQueue.GetCount() > 0) {
						pParam = (DirInsThreadParam *) m_pDirTree->m_InsQueue.RemoveHead();
						if (!pParam)
							break;
					}
					else
						break;
					if (!pParam)
						break;
				}

				if (m_pDirTree->IsQuitting()) {
					m_pDirTree->m_InsQueue.AddHead((ThreadParam *) pParam);
					goto EndOfThread;
				}

				if (pParam->IsValid())
					pParam->m_pFnList = m_pDirTree->InsertChildren(pParam->m_sParentPath);

				PostMessage(m_pDirTree->m_hWnd, WMU_INSERT, (WPARAM) pParam, 0);
			}
		}
EndOfThread:
		SetEvent(m_pDirTree->m_hEvent);
		return 0;
	}

private:
	DirTreeCtrl *m_pDirTree;
};

class PlayThreadParam : public ThreadParam
{
public:
	PlayThreadParam()
		: m_hWnd(NULL), m_bRecur(false), m_bReplace(true), m_bFilter(true), m_bDefault(false),
		m_pPathList(NULL), m_uPathCount(NULL), m_sName(NULL), m_iCommand(0)
	{}
	~PlayThreadParam()
	{
		if (m_sName)
			delete [] m_sName;
		if (m_pPathList)
			free(m_pPathList);
		m_PlayList.for_each(str_free);
	}
	void CreatePlaylist()
	{
		ParseCommand();

		if (m_pPathList) {
			UINT i;
			LPCTSTR psPath;
			for (i = 0;i < m_uPathCount;i++)
				if (m_pPathList[i]) {
					psPath = FindRealPath(m_pPathList[i]);

					PathRemoveBackslash(m_pPathList[i]);

					if (!PathIsDirectory(m_pPathList[i]))
						m_PlayList.add_item(strdup(pfc::stringcvt::string_utf8_from_os(psPath)));
					else {
						PathAddBackslash(m_pPathList[i]);
						DirTreeCtrl::CreatePlaylist(m_PlayList, pfc::stringcvt::string_utf8_from_os(psPath), m_bRecur, m_bFilter);
					}
				}
		}
	}
	bool SendPlaylist()
	{
		static_api_ptr_t<playlist_manager> ps;
		static_api_ptr_t<playback_control> pc;

		const int iCount = ps->activeplaylist_get_item_count();
		t_size idx;

		if (m_bDefault)
			ps->set_active_playlist(ps->find_or_create_playlist(prefs::sDefPL, INFINITE));

		if (m_iParsedCommand == ID_NEW_PLAY || m_iParsedCommand == ID_NEW) {
			idx = ps->create_playlist(pfc::stringcvt::string_utf8_from_os(m_sName), INFINITE, INFINITE);
			if (idx == INFINITE)
				return false;
			ps->set_active_playlist(idx);
		}
		else if (m_iParsedCommand == ID_UPDATE_TEMP_PL || m_iParsedCommand == ID_UPDATE_TEMP_PL_PLAY) {
			if ((idx = ps->find_or_create_playlist("Explorer View", INFINITE)) == INFINITE)
				return false;
			ps->set_active_playlist(idx);
		}

		if (m_bReplace)
			ps->activeplaylist_clear();
		ps->activeplaylist_add_locations(m_PlayList, true, m_hWnd);

		if (m_iParsedCommand == ID_ADD_PLAY) {
			ps->reset_playing_playlist();
			ps->activeplaylist_execute_default_action(iCount);
		}
		else if (m_iParsedCommand == ID_REPLACE_PLAY || m_iParsedCommand == ID_NEW_PLAY || m_iParsedCommand == ID_UPDATE_TEMP_PL_PLAY) {
			ps->reset_playing_playlist();
			pc->start(playback_control::track_command_next);
		}

		return true;
	}
	DWORD DoThreadOp()
	{
		SetThreadName(GetCurrentThreadId(), "Create Playlist");

		CreatePlaylist();

		PostMessage(m_hWnd, WMU_SEND_PLAYLIST, (WPARAM) this, 0);
		return 0;
	}
	LPCTSTR GetClassName() const
	{
		return _T("PlayThreadParam");
	}

	HWND m_hWnd;
	LPTSTR *m_pPathList;
	UINT m_uPathCount;
	LPTSTR m_sName;
	int m_iCommand;
	bool m_bFilter;
private:
	void ParseCommand()
	{
		m_iParsedCommand = m_iCommand;
		m_bRecur = INRANGE(m_iParsedCommand, ID_RECUR_START + 1, ID_RECUR_END - 1);

		if (m_bRecur)
			m_iParsedCommand -= RECUR_SHIFT;

		if (INRANGE(m_iParsedCommand, ID_DEFAULT_START + 1, ID_DEFAULT_END - 1)) {
			m_iParsedCommand -= DEFAULT_SHIFT;
			m_bDefault = true;
		}

		m_bReplace = !(m_iParsedCommand == ID_ADD_PLAY || m_iParsedCommand == ID_ADD);
	}
	static void str_free(const char *ptr) { free((void *) ptr); }

	pfc::ptr_list_t<const char> m_PlayList;
	int m_iParsedCommand;
	bool m_bRecur;
	bool m_bReplace;
	bool m_bDefault;
};

LPTSTR *DirTreeCtrl::m_MaskStrList = NULL;
LPTSTR DirTreeCtrl::m_sMaskStr = NULL;
UINT DirTreeCtrl::m_uMaskCount = 0;
LPTSTR *DirTreeCtrl::m_FilterStrList = NULL;
LPTSTR DirTreeCtrl::m_sFilterStr = NULL;
UINT DirTreeCtrl::m_uFilterCount = 0;

HWND DirTreeCtrl::Create(HWND hParent)
{
	m_bQuit = false;
	m_hParent = hParent;
	m_hWnd = CreateWindowEx(0, WC_TREEVIEW, _T("Explorer Tree"),
		((prefs::iTreeFrame & 0x00010000) ? WS_BORDER : 0) | TVS_HASBUTTONS | TVS_HASLINES | TVS_LINESATROOT | TVS_SHOWSELALWAYS | TVS_EDITLABELS | WS_CHILD | WS_VISIBLE, 0, 0, 0, 0,
		hParent, HMENU(IDC_EXPLORER), core_api::get_my_instance(), NULL);
	SetWindowLongPtr(m_hWnd, GWLP_USERDATA, (LONG_PTR) this);
	m_TreeProc = (WNDPROC) SetWindowLongPtr(m_hWnd, GWLP_WNDPROC, (LONG)((INT_PTR) OnHook));

	m_hEditWnd = CreateWindowEx(WS_EX_TRANSPARENT, WC_EDIT, _T(""), ((prefs::iAdBarFrame & 0x00010000) ? WS_BORDER : 0) | WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 0, 0, 0, 0, m_hParent, HMENU(IDC_PATHEDIT), core_api::get_my_instance(), NULL);
	SetWindowLongPtr(m_hEditWnd, GWLP_USERDATA, (LONG_PTR) this);
	m_EditProc = (WNDPROC) SetWindowLongPtr(m_hEditWnd, GWLP_WNDPROC, (LONG)((INT_PTR) OnEditHook));

	m_hBtnWnd = CreateWindowEx(0, WC_BUTTON, _T("6"), WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 0, 0, 0, 0, m_hParent, HMENU(IDC_BTN_FAVO), core_api::get_my_instance(), NULL);

	Init();

	return m_hWnd;
}

void DirTreeCtrl::Init()
{
	// Extra initializing is postoned
	if (core_api::is_initializing()) {
		PostMessage(m_hWnd, WMU_INIT, 0, 0);
		return;
	}

	if (globals::hSymbolFont)
		SetWindowFont(m_hBtnWnd, globals::hSymbolFont, TRUE);

	BulidFavoMenu();

	//Drag 'n' Drop --->
	SetTargetWindow(m_hWnd);
	RegisterDragDrop(m_hWnd, this);

	FORMATETC fmt;

	fmt.cfFormat = CF_HDROP;
	fmt.ptd = NULL;
	fmt.dwAspect = DVASPECT_CONTENT;
	fmt.lindex = -1;
	fmt.tymed = TYMED_HGLOBAL;

	CIDropTarget::AddSuportedFormat(fmt);
	//Drag 'n' Drop <---


	UpdateColors(false);
	SetItemHeight();

	{//load icons --->
		SHFILEINFO shFinfo;
		// Get the system image list using a "path" which is available on all systems.
		m_hImgList = (HIMAGELIST) SHGetFileInfo(_T("."), 0, &shFinfo, sizeof(SHFILEINFO), SHGFI_SYSICONINDEX | SHGFI_SMALLICON);
		if (m_hImgList)
		{
			ImageList_SetBkColor(m_hImgList, CLR_NONE); //Does this cause any problem?

			// here is the modification which allows to customize the folder icon look

			TCHAR *point = NULL;
			TCHAR module_name[MAX_PATH];
			GetModuleFileName(NULL, module_name, MAX_PATH);

			size_t module_name_len = lstrlen(module_name);

			// icon path
			TCHAR icon_path[MAX_PATH];
			_tcscpy(icon_path, module_name);

			point = _tcsrchr(icon_path, _T('\\'));
			*(++point) = NULL;			
			_tcscpy(point, _T("folder.ico"));

			DWORD hAttrs = GetFileAttributes(icon_path);

			if (hAttrs != INVALID_FILE_ATTRIBUTES)
			{
				HANDLE hIcon = LoadImage(NULL, icon_path, IMAGE_ICON, 0, 0, LR_LOADFROMFILE | LR_SHARED);

				ImageList_ReplaceIcon(m_hImgList, shFinfo.iIcon, (HICON)hIcon);

				SHGetFileInfo(_T("."), 0, &shFinfo, sizeof(SHFILEINFO), SHGFI_SYSICONINDEX | SHGFI_SMALLICON | SHGFI_OPENICON);
				ImageList_ReplaceIcon(m_hImgList, shFinfo.iIcon, (HICON)hIcon);
			}
		}
	}//load icons <---

	UpdateStyles(false);
	UpdateFonts(false);

	SendMessage(m_hEditWnd, EM_SETLIMITTEXT, EX_MAX_PATH - 1, 0);

	UpdateMaskString();
	UpdateFilterString();

	for (int i = 0;i < DIR_INS_THREADS;i++) {
		if (!m_pInsThreads[i]) {
			m_pInsThreads[i] = new DirInsThread(this);
			m_pInsThreads[i]->StartThread();
		}
	}

	InitInsert();
}

void DirTreeCtrl::InitInsert()
{
	TreeView_DeleteAllItems(m_hWnd);

	DWORD i;
	TCHAR sDrives[DRIVELEN];

	sDrives[0] = _T('\0');
	m_hLastFavo = TVI_FIRST;

	const DWORD len = GetLogicalDriveStrings(DRIVELEN, sDrives);
	DWORD disks = len >> 2;
	HTREEITEM hItem;

	SHFILEINFO shInfo;
	int iIcon;
	CFileName *fnItem;
	LPTSTR pDrive;
	pfc::stringcvt::string_os_from_utf8 sHide(prefs::sHideDrive);

	if (prefs::iShowFavoAsRoot == 1) {
		FavoItem **favoList = FavoItem::GetFavoList();
		int count = FavoItem::GetListCount();

		for (i = 0;i < (DWORD) count;i++)
			InsertFavorite(favoList[i]);
	}

	for (i = 0;i < disks;i++) {
		pDrive = sDrives + (i << 2);
		// *pDrive is lowercase on wine
		if (_tcschr(sHide, _istlower(*pDrive) ? _toupper(*pDrive) : *pDrive) || !PathFileExists(pDrive))
			continue;
		shInfo.szDisplayName[0] = _T('\0');
		SHGetFileInfo(pDrive, 0, &shInfo, sizeof(SHFILEINFO), SHGFI_ICON | SHGFI_SMALLICON);
		iIcon = shInfo.iIcon;
		DestroyIcon(shInfo.hIcon);
		SHGetFileInfo(pDrive, 0, &shInfo, sizeof(SHFILEINFO), SHGFI_ICON | SHGFI_OPENICON | SHGFI_SMALLICON | SHGFI_DISPLAYNAME);
		DestroyIcon(shInfo.hIcon);

		fnItem = new CFileName(pDrive, CFN_DRIVE);
		fnItem->m_iImage = iIcon;
		fnItem->m_iSelImage = shInfo.iIcon;
		fnItem->SetDispName(shInfo.szDisplayName);
		hItem = InsertItem(fnItem);

		InsertChildren(hItem, true);
	}

	if (prefs::iShowFavoAsRoot == 2) {
		m_hLastFavo = TVI_LAST;
		FavoItem **favoList = FavoItem::GetFavoList();
		int count = FavoItem::GetListCount();

		for (i = 0;i < (DWORD) count;i++)
			InsertFavorite(favoList[i]);
	}

	switch (prefs::iStartup) {
		case IDC_ST_LAST:
		case IDC_ST_USER:
			{
				pfc::stringcvt::string_os_from_utf8 wPath((prefs::iStartup == IDC_ST_LAST) ? prefs::sLastPath : prefs::sStartupPath);
				if (wPath.length()) {
					SelectByPath(wPath);
					break;
				}
			}
			//no break here
		default:
			{
				HTREEITEM hItem = TreeView_GetRoot(m_hWnd);
				if (hItem) {
					TreeView_SelectItem(m_hWnd, hItem);
				}
			}
			break;
	}
}

void DirTreeCtrl::Refresh(HTREEITEM hParent/* = TVI_ROOT*/)
{
	HTREEITEM hItem = (hParent == TVI_ROOT) ? TreeView_GetRoot(m_hWnd) : TreeView_GetChild(m_hWnd, hParent);

	for (;hItem;hItem = TreeView_GetNextSibling(m_hWnd, hItem)) {
		if (TreeView_GetItemState(m_hWnd, hItem, TVIS_EXPANDED))
			Refresh(hItem);
		InsertChildren(hItem, true);
	}
}

HTREEITEM DirTreeCtrl::InsertFavorite(FavoItem *fvItem)
{
	if (prefs::iShowFavoAsRoot == 0 || !PathFileExists(fvItem->GetPath()))
		return NULL;

	SHFILEINFO shInfo = {0};
	int iIcon = 0;
	CFileName *fnItem;

	shInfo.szDisplayName[0] = _T('\0');
	SHGetFileInfo(fvItem->GetPath(), 0, &shInfo, sizeof(SHFILEINFO), SHGFI_ICON | SHGFI_SMALLICON);
	iIcon = shInfo.iIcon;
	DestroyIcon(shInfo.hIcon);
	SHGetFileInfo(fvItem->GetPath(), 0, &shInfo, sizeof(SHFILEINFO), SHGFI_ICON | SHGFI_OPENICON | SHGFI_SMALLICON | SHGFI_DISPLAYNAME);
	DestroyIcon(shInfo.hIcon);

	fnItem = new CFileName(fvItem->GetPath(), CFN_FAVORITE);
	fnItem->SetDispName(fvItem->GetDesc());
	fnItem->m_iImage = iIcon;
	fnItem->m_iSelImage = shInfo.iIcon;

	m_hLastFavo = InsertItem(fnItem, TVI_ROOT, m_hLastFavo);
	InsertChildren(m_hLastFavo, true);

	return m_hLastFavo;
}

HTREEITEM DirTreeCtrl::InsertItem(CFileName *data, HTREEITEM hParent/* = TVI_ROOT*/, HTREEITEM hInsertAfter/* = TVI_LAST*/)
{
	ASSERT(data);

	TVINSERTSTRUCT tvin;
	tvin.hParent = hParent;
	tvin.hInsertAfter = hInsertAfter;
	tvin.item.mask = TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM | TVIF_TEXT;
	tvin.item.lParam = (LPARAM) data;
	tvin.item.iImage = I_IMAGECALLBACK;
	tvin.item.iSelectedImage = I_IMAGECALLBACK;
	tvin.item.pszText = LPSTR_TEXTCALLBACK;

	return TreeView_InsertItem(m_hWnd, &tvin);
}

LPCTSTR DirTreeCtrl::GetFullPath(HTREEITEM hItem, LPTSTR buffer, bool bWithFileName/* = true*/) const
{
	LPTSTR lpEnd = buffer;

	return GetFullPath(hItem, buffer, bWithFileName, lpEnd);
}

LPCTSTR DirTreeCtrl::GetFullPath(HTREEITEM hItem, LPTSTR buffer, bool bWithFileName, LPTSTR &pEnd) const
{
	if (!hItem)
		return NULL;

	HTREEITEM hParent;

	CFileName *fnItem = GetItemData(hItem);
	LPTSTR pSource = (LPTSTR) fnItem->GetName();

	if (hParent = TreeView_GetParent(m_hWnd, hItem)) {
		GetFullPath(hParent, buffer, true, pEnd);

		if (fnItem->IsFile() && !bWithFileName)
			return buffer;
	}

	strcpy_getend(pEnd, (LPCTSTR &) pSource);

	if (fnItem->IsFolder()) {
		*pEnd++ = _T('\\');
		*pEnd = _T('\0');
	}

	return buffer;
}

LPTSTR DirTreeCtrl::GetPrefixFullPath(HTREEITEM hItem, LPTSTR buffer, LPTSTR &sRealPath, bool bWithFileName/* = true*/)
{
	if (!GetFullPath(hItem, buffer + 6, bWithFileName)) {
		sRealPath = NULL;
		return NULL;
	}
	if (PathIsUNC(buffer + 6)) {
		PREFIX_UNC(buffer);
		sRealPath = buffer + 6;
		return buffer;
	}
	else {
		PREFIX_LONGPATH(buffer + 2);
		sRealPath = buffer + 6;
		return buffer + 2;
	}
}

void DirTreeCtrl::InsertChildren(HTREEITEM hParent, bool bThread /* = true */)
{
	CFileName *fnItem = GetItemData(hParent);

	if (fnItem->IsFile())
		return;

	LPTSTR theBuffer = (LPTSTR) malloc(sizeof(TCHAR) * EX_MAX_PATH);

	GetFullPath(hParent, theBuffer);

	if (bThread) {
		if (!fnItem->m_pInsParam) {
			DirInsThreadParam *pParam = new DirInsThreadParam;
			pParam->m_sParentPath = theBuffer;
			pParam->m_hParentItem = hParent;
			pParam->m_pParent = fnItem;
			fnItem->m_pInsParam = pParam;
			Queue(pParam);
		}
		else
			free(theBuffer);
	}
	else {
		CAutoPtr<CAtlArray<CFileName *>> pList(InsertChildren(theBuffer));
		InsertChildren(hParent, pList);
		free(theBuffer);
	}
}

CAtlArray<CFileName *> *DirTreeCtrl::InsertChildren(LPTSTR sParentPath)
{
	LPTSTR theEnd, thePath = sParentPath;
	HANDLE hFindFile;
	WIN32_FIND_DATA findData = {0};
	SHFILEINFO shInfo;
	int iIcon;

	if (IsQuitting() || !PathFileExists(sParentPath))
		return NULL;

	theEnd = _tcsrchr(sParentPath, _T('\\')) + 1;
	_tcscpy(theEnd, _T("*.*"));

	if ((hFindFile = FindFirstFile(sParentPath, &findData)) == INVALID_HANDLE_VALUE)
		return NULL;

	CAutoPtr<CFileName> fnTester(new CFileName);
	CFileName *fnItem;
	CAtlArray<CFileName *> *theList = new CAtlArray<CFileName *>, theFileList;
	bool bIsFolder, bIsFile, bShowAll, bInTypes;

	bShowAll = (prefs::iShowTypeMode == IDC_FILES_ALL);

	do {
		if (IsQuitting())
			break;

		if (!prefs::bShowHiddenFiles && (findData.dwFileAttributes & (FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM)))
			continue;

		bIsFile = !(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
		bIsFolder = (!bIsFile && !ISDOTS(findData.cFileName));

		if (bIsFile) {
			if (m_uMaskCount > 0) {
				fnTester->SetFileName(findData.cFileName);
				bInTypes = fnTester->InTheseTypes(m_MaskStrList, m_uMaskCount);
			}
			else
				bInTypes = false;
		}

		if (bIsFolder || (bIsFile && (bInTypes && !bShowAll 
			|| !bInTypes && bShowAll && input_entry::g_is_supported_path(pfc::stringcvt::string_utf8_from_os(findData.cFileName)))))
		{

			_tcscpy(theEnd, findData.cFileName);
			shInfo.szDisplayName[0] = _T('\0');
			SHGetFileInfo(thePath, 0, &shInfo, sizeof(SHFILEINFO), SHGFI_ICON | SHGFI_SMALLICON);
			iIcon = shInfo.iIcon;
			DestroyIcon(shInfo.hIcon);
			SHGetFileInfo(thePath, 0, &shInfo, sizeof(SHFILEINFO), SHGFI_ICON | SHGFI_OPENICON | SHGFI_SMALLICON);
			DestroyIcon(shInfo.hIcon);
			fnItem = new CFileName(findData.cFileName,
				FHRSD(bIsFolder,
				findData.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN,
				findData.dwFileAttributes & FILE_ATTRIBUTE_READONLY,
				findData.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM, 0));
			if (bIsFolder)
				theList->Add(fnItem);
			else
				theFileList.Add(fnItem);
			fnItem->m_iImage = iIcon;
			fnItem->m_iSelImage = shInfo.iIcon;
		}
	} while (FindNextFile(hFindFile, &findData));

	FindClose(hFindFile);

	if (theList->GetCount() == 0 && theFileList.GetCount() == 0) {
		delete theList;
		return NULL;
	}

	if (!IsQuitting() && prefs::bForceSort) {
		qsort(theFileList.GetData(), theFileList.GetCount(), sizeof(CFileName *), FnCompare);
		qsort(theList->GetData(), theList->GetCount(), sizeof(CFileName *), FnCompare); 
	}

	theList->Append(theFileList);

	return theList;
}

void DirTreeCtrl::InsertChildren(HTREEITEM hParent, CAtlArray<CFileName *> *pFnList)
{
	HTREEITEM hChild = TreeView_GetChild(m_hWnd, hParent);

	bool bRc, bExpanded = (TreeView_GetItemState(m_hWnd, hParent, TVIS_EXPANDED) != 0);
	RECT rc;
	bRc = TreeView_GetItemRect(m_hWnd, hParent, &rc, TRUE) != 0;

	if (pFnList) {
		CFileName *fnItem, *fnOldItem;
		int i, count = pFnList->GetCount();
		int cmp;
		TCHAR sPath[EX_MAX_PATH];
		HTREEITEM hPreChild = TVI_FIRST, hTemp;

		for (i = 0;i < count;i++) {
			fnItem = pFnList->GetAt(i);
			if (hChild) {
				fnOldItem = GetItemData(hChild);
				if (cmp = FnCompareFolderFirst(&fnItem, &fnOldItem)) {
					GetFullPath(hChild, sPath);
					if (cmp < 0) { //The new item doesn't exist in the old list. ==> Insert new one.
						hPreChild = InsertItem(fnItem, hParent, hPreChild);
						if (bExpanded)
							InsertChildren(hPreChild, true);
					}
					else {// (cmp > 0); Found an item which no longer exists in the new list. ==> Delete old one.
						hTemp = hChild;
						hChild = TreeView_GetNextSibling(m_hWnd, hChild);
						TreeView_DeleteItem(m_hWnd, hTemp);
						i--;
					}
				}
				else {// The item exists in both the old list and the new list. ==> Do not change the old list.
					delete fnItem;
					hPreChild = hChild;
					hChild = TreeView_GetNextSibling(m_hWnd, hPreChild);
				}
			}
			else {// (!hChild); The old list ran out. ==> Insert the items remaining in the new list.
				hPreChild = InsertItem(fnItem, hParent);
				if (bExpanded)
					InsertChildren(hPreChild, true);
			}
		}
		while (hChild) {// The new list ran out. ==> Delete the items remaining in the old list.
			hPreChild = hChild;
			hChild = TreeView_GetNextSibling(m_hWnd, hPreChild);
			TreeView_DeleteItem(m_hWnd, hPreChild);
		}
	}
	else {// (pFnList); There has no items in the new list. ==> Just delete all items in the old list.
		DeleteChildren(hParent);
		TreeView_SetItemState(m_hWnd, hParent, 0, TVIS_EXPANDED);
	}

	if (bRc) {//redraw expand button
		rc.right = rc.left;
		rc.left -= 40;
		if (rc.left < 0)
			rc.left = 0;

		InvalidateRect(m_hWnd, &rc, TRUE);
	}
}

void DirTreeCtrl::DeleteChildren(HTREEITEM hParent)
{
	HTREEITEM dchild, child = TreeView_GetChild(m_hWnd, hParent);

	while (child != NULL) {
		dchild = child;
		child = TreeView_GetNextSibling(m_hWnd, child);
		TreeView_DeleteItem(m_hWnd, dchild);
	}
}

HTREEITEM DirTreeCtrl::FindInSibling(HTREEITEM hItem, LPCTSTR sLabel, LPTSTR *pRoot/* = NULL*/) const
{
	CFileName *fnItem;

	for (;hItem;hItem = TreeView_GetNextSibling(m_hWnd, hItem)) {
		fnItem = GetItemData(hItem);
		if (fnItem->IsFavorite()) {
			if (pRoot) {
				size_t len = _tcslen(fnItem->GetName());
				if (_tcsnicmp(sLabel, fnItem->GetName(), len) == 0) {
					*pRoot = (LPTSTR) sLabel + (len - 1);
					return hItem;
				}
			}
			else
				continue;
		}
		else if (fnItem->IsDrive()) {
			if (_tcsnicmp(sLabel, fnItem->GetName(), 2) == 0)
				return hItem;
		}
		else if (_tcsicmp(sLabel, fnItem->GetName()) == 0)
			return hItem;
	}

	return NULL;
}

int DirTreeCtrl::FindInSibling(HTREEITEM hItem1, HTREEITEM hItem2) const
{
	HTREEITEM hItem;
	if (!hItem1 || !hItem2 || (hItem = TreeView_GetParent(m_hWnd, hItem1)) != TreeView_GetParent(m_hWnd, hItem2))
		return 0;
	for (hItem = TreeView_GetChild(m_hWnd, hItem);hItem;hItem = TreeView_GetNextSibling(m_hWnd, hItem)) {
		if (hItem == hItem1)
			return -1;
		else if (hItem == hItem2)
			return 1;
	}
	return 0;
}

HTREEITEM DirTreeCtrl::SelectByPath(LPCTSTR sPath)
{
	if (!sPath || !ISFULLPATH(sPath))
		return NULL;

	LPTSTR token = (LPTSTR) _tcschr(sPath, _T('\\'));
	LPCTSTR pStart = sPath;
	HTREEITEM hItem = TreeView_GetRoot(m_hWnd);
	bool bIsRoot = true;

	for (;token && hItem;token = _tcschr(token + 1, _T('\\'))) {
		if (*(token + 1) == _T('\0'))
			break;
		if (bIsRoot)
			hItem = FindInSibling(hItem, pStart, &token);
		else {
			*token = _T('\0');
			hItem = FindInSibling(hItem, pStart, NULL);
			*token = _T('\\');
		}
		pStart = token + 1;
		if (hItem) {
			TreeView_SelectItem(m_hWnd, hItem);
			InsertChildren(hItem, false);
			Expand(hItem, TVE_EXPAND);
			hItem = TreeView_GetChild(m_hWnd, hItem);
			bIsRoot = false;
		}
		else
			return NULL;
	}

	if (hItem) {
		if (token)
			*token = _T('\0');
		if (hItem = FindInSibling(hItem, pStart, NULL))
			TreeView_SelectItem(m_hWnd, hItem);
		if (token)
			*token = _T('\\');

		if (prefs::bExpand && (hItem == GetSelectedItem())) {
			InsertChildren(hItem, false);
			Expand(hItem, TVE_EXPAND);
		}

		TreeView_EnsureVisible(m_hWnd, hItem);
	}

	return hItem;
}

LPCTSTR DirTreeCtrl::GetItemText(HTREEITEM hItem, LPTSTR buffer) const
{
	if (!hItem || !buffer)
		return NULL;

	TVITEM tvItem = {0};

	tvItem.mask = TVIF_TEXT;
	tvItem.hItem = hItem;
	tvItem.cchTextMax = EX_MAX_PATH;
	tvItem.pszText = buffer;

	return TreeView_GetItem(m_hWnd, &tvItem) ? tvItem.pszText : NULL;
}

CFileName *DirTreeCtrl::GetItemData(HTREEITEM hItem) const
{
	if (!hItem)
		return NULL;

	TVITEM tvItem = {0};
	tvItem.mask = TVIF_PARAM;
	tvItem.hItem = hItem;
	TreeView_GetItem(m_hWnd, &tvItem);

	return (CFileName *)tvItem.lParam;
}

void DirTreeCtrl::OnSize(int iWidth, int iHeight) const
{
	if (prefs::bShowAddBar) {
		ShowWindow(m_hEditWnd, SW_SHOW);
		ShowWindow(m_hBtnWnd, SW_SHOW);
		MoveWindow(m_hEditWnd, 2, 2, iWidth - 26, 18, TRUE);
		MoveWindow(m_hBtnWnd, iWidth - 21, 1, 21, 20, TRUE);
		MoveWindow(m_hWnd, 0, 22, iWidth, iHeight - 22, TRUE);
		::RedrawWindow(m_hEditWnd, NULL, NULL, RDW_INVALIDATE | RDW_INTERNALPAINT | RDW_FRAME | RDW_ERASE | RDW_UPDATENOW);
	}
	else {
		ShowWindow(m_hEditWnd, SW_HIDE);
		ShowWindow(m_hBtnWnd, SW_HIDE);
		MoveWindow(m_hWnd, 0, 0, iWidth, iHeight, TRUE);
	}
}

void DirTreeCtrl::ShowAddressBar() const
{
	RECT rc;
	GetWindowRect(m_hParent, &rc);
	OnSize(rc.right - rc.left, rc.bottom - rc.top);
}

void DirTreeCtrl::UpdateFonts(bool bRepaint/* = true*/)
{
	HFONT hTreeFont = m_hTreeFont, hEditFont = m_hEditFont;

	m_hTreeFont = columns_ui::fonts::helper(CTreeFonts::class_guid).get_font();
	SetWindowFont(m_hWnd, m_hTreeFont, bRepaint);

	m_hEditFont = columns_ui::fonts::helper(CAddrBarFonts::class_guid).get_font();
	SetWindowFont(m_hEditWnd, m_hEditFont, bRepaint);

	if (hTreeFont)
		DeleteObject((HGDIOBJ) hTreeFont);
	if (hEditFont)
		DeleteObject((HGDIOBJ) hEditFont);
}

void DirTreeCtrl::UpdateColors(bool bRepaint/*= true*/)
{
	columns_ui::colours::helper treeColors(CTreeColors::class_guid);

	TreeView_SetTextColor(m_hWnd, treeColors.get_colour(columns_ui::colours::colour_text));
	TreeView_SetLineColor(m_hWnd, treeColors.get_colour(columns_ui::colours::colour_text));
	TreeView_SetBkColor(m_hWnd, treeColors.get_colour(columns_ui::colours::colour_background));

	columns_ui::colours::helper addrBarColors(CAddrBarColors::class_guid);

	if (m_hEditBkBrush) {
		DeleteObject(m_hEditBkBrush);
		m_hEditBkBrush = 0;
	}
	m_hEditBkBrush = CreateSolidBrush(addrBarColors.get_colour(columns_ui::colours::colour_background));

	if (bRepaint) {
		UpdateWindow(m_hWnd);
		::RedrawWindow(m_hEditWnd, NULL, NULL, RDW_INVALIDATE | RDW_INTERNALPAINT | RDW_FRAME | RDW_ERASE | RDW_UPDATENOW);
	}
}

void DirTreeCtrl::UpdateStyles(bool bRepaint/*= true*/) const
{
	TreeView_SetImageList(m_hWnd, prefs::bShowIcons ? m_hImgList : 0, TVSIL_NORMAL);

	LONG_PTR curStyle = GetWindowLongPtr(m_hWnd, GWL_STYLE);
	LONG_PTR curExStyle = GetWindowLongPtr(m_hWnd, GWL_EXSTYLE);

	prefs::bShowLines ? curStyle |= TVS_HASLINES : curStyle &= ~TVS_HASLINES;
	prefs::bShowHScroll ? curStyle &= ~TVS_NOHSCROLL : curStyle |= TVS_NOHSCROLL;
	prefs::bShowTooltip ? curStyle &= ~TVS_NOTOOLTIPS : curStyle |= TVS_NOTOOLTIPS;
	(prefs::iTreeFrame & 0x00010000) ? curStyle |= WS_BORDER : curStyle &= ~WS_BORDER;

	switch (LOWORD(prefs::iTreeFrame)) {
		case ID_TREE_NONE:
			curExStyle &= ~WS_EX_STATICEDGE;
			curExStyle &= ~WS_EX_CLIENTEDGE;
			break;
		case ID_TREE_SUNKEN:
			curExStyle &= ~WS_EX_STATICEDGE;
			curExStyle |= WS_EX_CLIENTEDGE;
			break;
		case ID_TREE_GREY:
			curExStyle |= WS_EX_STATICEDGE;
			curExStyle &= ~WS_EX_CLIENTEDGE;
			break;
	}

	SetWindowLongPtr(m_hWnd, GWL_STYLE, curStyle);
	SetWindowLongPtr(m_hWnd, GWL_EXSTYLE, curExStyle);

	curStyle = GetWindowLongPtr(m_hEditWnd, GWL_STYLE);
	curExStyle = GetWindowLongPtr(m_hEditWnd, GWL_EXSTYLE);

	(prefs::iAdBarFrame & 0x00010000) ? curStyle |= WS_BORDER : curStyle &= ~WS_BORDER;

	switch (LOWORD(prefs::iAdBarFrame)) {
		case ID_ADBAR_NONE:
			curExStyle &= ~WS_EX_STATICEDGE;
			curExStyle &= ~WS_EX_CLIENTEDGE;
			break;
		case ID_ADBAR_SUNKEN:
			curExStyle &= ~WS_EX_STATICEDGE;
			curExStyle |= WS_EX_CLIENTEDGE;
			break;
		case ID_ADBAR_GREY:
			curExStyle |= WS_EX_STATICEDGE;
			curExStyle &= ~WS_EX_CLIENTEDGE;
			break;
	}

	SetWindowLongPtr(m_hEditWnd, GWL_STYLE, curStyle);
	SetWindowLongPtr(m_hEditWnd, GWL_EXSTYLE, curExStyle);

	if (bRepaint) {
		SetWindowPos(m_hWnd, 0, 0, 0, 0, 0, SWP_DRAWFRAME | SWP_FRAMECHANGED | SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOSIZE | SWP_NOZORDER);
		SetWindowPos(m_hEditWnd, 0, 0, 0, 0, 0, SWP_DRAWFRAME | SWP_FRAMECHANGED | SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOSIZE | SWP_NOZORDER);
		UpdateWindow(m_hWnd);
	}
}

void DirTreeCtrl::SetItemHeight()
{
	TreeView_SetItemHeight(m_hWnd, (prefs::iNodeHeight == 0) ? -1 : prefs::iNodeHeight);
}

void DirTreeCtrl::UpdateMaskString()
{
	ClearMask(false);

	if (m_uMaskCount = ParseMaskString(m_MaskStrList, m_sMaskStr, prefs::sShowType))
		qsort(m_MaskStrList, m_uMaskCount, sizeof(LPTSTR), void_tcsicmp);
	else
		ClearMask(false);
}

void DirTreeCtrl::UpdateFilterString()
{
	ClearFilter(false);

	if (m_uFilterCount = ParseMaskString(m_FilterStrList, m_sFilterStr, prefs::sFilterType))
		qsort(m_FilterStrList, m_uFilterCount, sizeof(LPTSTR), void_tcsicmp);
	else
		ClearFilter(false);
}

UINT DirTreeCtrl::ParseMaskString(LPTSTR *&sList, LPTSTR &sBuffer, /* UTF-8 */const char *sSource)
{
	LPTSTR ptr;
	UINT count = 1, i = 0;

	sBuffer = _tcsdup(pfc::stringcvt::string_os_from_utf8(sSource));

	for (ptr = sBuffer;*ptr;ptr++)
		if (*ptr == _T('|'))
			count++;

	sList = (LPTSTR *) malloc(sizeof(LPTSTR) * (count + 1));
	sList[0] = sBuffer;

	for (ptr = sBuffer;*ptr;ptr++)
		if (*ptr == _T('|')) {
			*ptr = _T('\0');
			if (sList[i][0] != _T('\0') && IsValidExt(sList[i]))
				i++;
			sList[i] = ptr + 1;
		}
	sList[i + 1] = NULL;

	return (sList[i][0] != _T('\0') && IsValidExt(sList[i])) ? i + 1 : i;
}

void DirTreeCtrl::BulidMenu()
{
	int i, n, count;

	m_ContextMenu.CreatePopupMenu();

	for (i = 0;CMD_MAP[i].id < ID_MENU_LAST;i++)
		if (CMD_MAP[i].id == 0)
			m_ContextMenu.AppendMenu(MF_SEPARATOR);
		else
			m_ContextMenu.AppendMenu(MF_STRING, CMD_MAP[i].id, CMD_MAP[i].desc);

	n = i;

	{
		m_ContextMenu.AppendMenu(MF_SEPARATOR); n++;
		ExMenu markMenu;
		markMenu.CreatePopupMenu();
		for (i = i + 1;CMD_MAP[i].id < ID_MARK_END;i++)
			markMenu.AppendMenu(MF_STRING, CMD_MAP[i].id, CMD_MAP[i].desc);

		MENUITEMINFO mItem = {0};
		mItem.cbSize = sizeof(MENUITEMINFO);
		mItem.fMask = MIIM_SUBMENU | MIIM_STRING;
		mItem.dwTypeData = _T("Mark");
		mItem.hSubMenu = markMenu.Detach();
		m_ContextMenu.InsertMenuItem(n, MF_BYPOSITION, &mItem);
		n++;
	}

	{
		UINT uMarkedCount = GetMarkedCount(), uIdx;
		TCHAR sBuffer[EX_MAX_PATH];

		if (uMarkedCount == 0)
			theSHMenuFactory.SetObjects(GetSelectedPath(sBuffer));
		else {
			LPTSTR *pPathList = (LPTSTR *) malloc((sizeof(LPTSTR *) + sizeof(TCHAR) * EX_MAX_PATH) * uMarkedCount);
			for (uIdx = 0;uIdx < uMarkedCount;uIdx++) {
				pPathList[uIdx] = (LPTSTR)(pPathList + uMarkedCount) + (uIdx * EX_MAX_PATH);
				GetFullPath(m_MarkItems[uIdx], pPathList[uIdx]);
			}
			theSHMenuFactory.SetObjects(pPathList, uMarkedCount);
			free(pPathList);
		}
		if (m_pShellMenu)
			theSHMenuFactory.CloseShellMenu(m_pShellMenu, 0, false);
		if (m_pShellMenu = theSHMenuFactory.OpenShellMenu(m_hWnd, ID_SHELL_START, ID_SHELL_END)) {
			m_ContextMenu.AppendMenu(MF_SEPARATOR); n++;

			MENUITEMINFO mItem = {0};
			mItem.cbSize = sizeof(MENUITEMINFO);
			mItem.fMask = MIIM_SUBMENU | MIIM_STRING;
			mItem.dwTypeData = _T("Shell Menu");
			mItem.hSubMenu = m_pShellMenu->Detach();
			m_ContextMenu.InsertMenuItem(n, MF_BYPOSITION, &mItem);
			n++;
		}
	}

	CmdItem **cmdList = CmdItem::GetCmdList();
	count = CmdItem::GetListCount();

	if (count > 0) {
		m_ContextMenu.AppendMenu(MF_SEPARATOR); n++;

		if (prefs::bGrpCmd) {
			ExMenu cmdMenu;
			cmdMenu.CreatePopupMenu();
			for (i = 0;i < MAX_CMD && cmdList[i];i++)
				cmdMenu.AppendMenu(MF_STRING, ID_CMDLINE_START + i, cmdList[i]->GetDesc());
			MENUITEMINFO mItem = {0};
			mItem.cbSize = sizeof(MENUITEMINFO);
			mItem.fMask = MIIM_SUBMENU | MIIM_STRING;
			mItem.dwTypeData = _T("Shell Commands");
			mItem.hSubMenu = cmdMenu.Detach();
			m_ContextMenu.InsertMenuItem(n, MF_BYPOSITION, &mItem);
			n++;
		}
		else 
			for (i = 0;i < MAX_CMD && cmdList[i];i++) {
				m_ContextMenu.AppendMenu(MF_STRING, ID_CMDLINE_START + i, cmdList[i]->GetDesc());
				n++;
			}
	}

	FavoItem **favoList = FavoItem::GetFavoList();
	count = FavoItem::GetListCount();

	if (count > 0) {
		m_ContextMenu.AppendMenu(MF_SEPARATOR); n++;

		if (prefs::bGrpFavo) {
			ExMenu favoMenu;
			favoMenu.CreatePopupMenu();
			for (i = 0;i < MAX_FAVO && favoList[i];i++)
				favoMenu.AppendMenu(MF_STRING, ID_FAVO_START + i, favoList[i]->GetDesc());
			MENUITEMINFO mItem = {0};
			mItem.cbSize = sizeof(MENUITEMINFO);
			mItem.fMask = MIIM_SUBMENU | MIIM_STRING;
			mItem.dwTypeData = _T("Favorites");
			mItem.hSubMenu = favoMenu.Detach();
			m_ContextMenu.InsertMenuItem(n, MF_BYPOSITION, &mItem);
			n++;
		}
		else 
			for (i = 0;i < MAX_FAVO && cmdList[i];i++) {
				m_ContextMenu.AppendMenu(MF_STRING, ID_FAVO_START + i, favoList[i]->GetDesc());
				n++;
			}
	}
}

void DirTreeCtrl::BulidFavoMenu()
{
	if (m_FavoMenu)
		m_FavoMenu.DestroyMenu();

	m_FavoMenu.CreatePopupMenu();

	m_FavoMenu.AppendMenu(MF_STRING, ID_ADBAR_GO, _T("&Go"));
	m_FavoMenu.AppendMenu(MF_STRING, ID_SELECT_NOWPLAYING, _T("Goto &Now Playing"));

	int i, count;

	FavoItem **favoList = FavoItem::GetFavoList();
	count = FavoItem::GetListCount();

	if (count == 0)
		return;

	m_FavoMenu.AppendMenu(MF_SEPARATOR);

	for (i = 0;i < MAX_FAVO && favoList[i];i++)
		m_FavoMenu.AppendMenu(MF_STRING, ID_FAVO_START + i, favoList[i]->GetDesc());
}

void DirTreeCtrl::ClearMask(bool bInterlocked/* = true*/)
{
	if (m_sMaskStr) {
		free(m_sMaskStr);
		m_sMaskStr = NULL;
	}
	if (m_MaskStrList) {
		free(m_MaskStrList);
		m_MaskStrList = NULL;
	}
	m_uMaskCount = 0;
}

void DirTreeCtrl::ClearFilter(bool bInterlocked/* = true*/)
{
	if (m_sFilterStr) {
		free(m_sFilterStr);
		m_sFilterStr = NULL;
	}
	if (m_FilterStrList) {
		free(m_FilterStrList);
		m_FilterStrList = NULL;
	}
	m_uFilterCount = 0;
}

void DirTreeCtrl::OnExpanding(LPNMHDR lpNMHDR)
{
	LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(lpNMHDR);

	TreeView_SelectItem(m_hWnd, NULL); // force OnSelectChanged() to be called.
	TreeView_SelectItem(m_hWnd, pNMTreeView->itemNew.hItem);
}

void DirTreeCtrl::OnExpanded(LPNMHDR lpNMHDR)
{
	LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(lpNMHDR);

	HTREEITEM nowItem, childItem;

	nowItem = pNMTreeView->itemNew.hItem;

	if (pNMTreeView->action == TVE_EXPAND) {
		childItem = TreeView_GetChild(m_hWnd, nowItem);

		while (childItem != NULL) {
			InsertChildren(childItem, true);
			childItem = TreeView_GetNextSibling(m_hWnd, childItem);
		}

		InsertChildren(nowItem, true);
	}
}

void DirTreeCtrl::OnSelectChanged(LPNMHDR lpNMHDR/* = NULL*/)
{
	LPNMTREEVIEW pNMTreeView;
	HTREEITEM hItem;
	CFileName *fnItem;

	m_hPrevSel = NULL;

	if (lpNMHDR) {
		pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(lpNMHDR);
		if (!pNMTreeView->itemNew.hItem)
			return;
		hItem = pNMTreeView->itemNew.hItem;
		fnItem = (CFileName *) pNMTreeView->itemNew.lParam;
		m_hPrevSel = pNMTreeView->itemOld.hItem;
	}
	else if (hItem = GetSelectedItem())
		fnItem = GetItemData(hItem);
	else
		return;

	TCHAR sPath[EX_MAX_PATH];

	GetFullPath(hItem, sPath);

	if (!fnItem->IsRoot() && !PathFileExists(sPath)) {
		TreeView_DeleteItem(m_hWnd, hItem);
		return;
	}
	else if (!TreeView_GetChild(m_hWnd, hItem))
		InsertChildren(hItem, true);

	SetWindowText(m_hEditWnd, sPath);
	prefs::sLastPath = pfc::stringcvt::string_utf8_from_os(sPath);
}

void DirTreeCtrl::OnRClick(LPNMHDR lpNMHDR)
{
	OnClick();

	POINT pt;
	GetCursorPos(&pt);

	OnContextMenu(pt.x, pt.y);
}

void DirTreeCtrl::OnClick()
{
	HTREEITEM hItem;
	if (hItem = IsCursorOnItem())
		TreeView_SelectItem(m_hWnd, hItem);
	else
		SendMessage(m_hWnd, WM_COMMAND, ID_MARK_CLEAR_ALL, 0);
}

HTREEITEM DirTreeCtrl::IsCursorOnItem() const
{
	POINT cursor;
	GetCursorPos(&cursor);

	TVHITTESTINFO hitInfo = {0};

	hitInfo.pt = cursor;
	ScreenToClient(m_hWnd, &hitInfo.pt);
	TreeView_HitTest(m_hWnd, &hitInfo);

	return (hitInfo.flags & TVHT_ONITEM) ? hitInfo.hItem : NULL;
}

void DirTreeCtrl::OnGetDispInfo(LPNMTVDISPINFO lpTvdi)
{
	LPTVITEM lpItem = &lpTvdi->item;
	UINT mask = lpItem->mask;
	CFileName *data = (CFileName *) lpItem->lParam;

	if (!data)
		return;

	if (mask & TVIF_TEXT) // we do NOT delete this data soon, so just assign the pointer. See MSDN.
		lpItem->pszText = (LPTSTR) data->GetDispName();
	if (mask & TVIF_SELECTEDIMAGE)
		lpItem->iSelectedImage = data->m_iSelImage;
	if (mask & TVIF_IMAGE)
		lpItem->iImage = data->m_iImage;
}

bool DirTreeCtrl::ParseShortcut(int keys)
{
	_sKeyData *sc = prefs::aShortcuts;
	int i;

	if ((keys & CI_MC_MASK) && !IsCursorOnItem())
		return false;

	for (i = 0;i < prefs::aShortcuts.GetCount();i++)
		if (sc[i].key == keys) {
			SendMessage(m_hWnd, WM_COMMAND, (0x00010000L | sc[i].id), 0);
			return true;
		}

	return false;
}

void DirTreeCtrl::OnPlaybackCommand(int iCommand)
{
	if (!(INRANGE(iCommand, ID_PLAYLIST_START + 1, ID_PLAYLIST_END - 1) ||
		INRANGE(iCommand, ID_RECUR_START + 1, ID_RECUR_END - 1)))
		return;

	HTREEITEM hItem = GetSelectedItem();
	const unsigned int uMarkCount = GetMarkedCount();
	unsigned int i = 0, uPathCount = (uMarkCount > 0) ? uMarkCount : (hItem ? 1 : 0);

	if (uPathCount == 0)
		return;

	PlayThreadParam *pParam = new PlayThreadParam;
	pParam->m_hWnd = m_hWnd;
	pParam->m_sName = new TCHAR [EX_MAX_PATH];
	pParam->m_bFilter = (m_uFilterCount > 0);
	GetItemText((uMarkCount > 0) ? m_MarkItems[0] : hItem, pParam->m_sName);

	pParam->m_uPathCount = uPathCount;
	pParam->m_pPathList = (LPTSTR *) malloc((sizeof(LPTSTR *) + sizeof(TCHAR) * EX_MAX_PATH) * uPathCount);
	for (i = 0;i < uPathCount;i++) {
		pParam->m_pPathList[i] = (LPTSTR)(pParam->m_pPathList + uPathCount) + (i * EX_MAX_PATH);
		if (i == 0 && uMarkCount == 0)
			GetFullPath(hItem, pParam->m_pPathList[i]);
		else
			GetFullPath(m_MarkItems[i], pParam->m_pPathList[i]);
	}

	pParam->m_iCommand = iCommand;

	pParam->StartThread();
}

void DirTreeCtrl::CreatePlaylist(pfc::ptr_list_t<const char> &plist, /* UTF-8 */const char *uPath, bool bRecur, bool bFilter)
{
	char u8Path[EX_MAX_PATH];
	char *pEnd;
	const char *pFileName;
	uFindFile *uFF;

	strcpy(u8Path, uPath);
	pEnd = u8Path + strlen(uPath);
	strcpy(pEnd, "*.*");

	if (!(uFF = uFindFirstFile(u8Path)))
		return;

	CFileName *fnTester = new CFileName;
	bool bFiltered;

	do {
		bFiltered = false;
		pFileName = uFF->GetFileName();

		if (bRecur && uFF->IsDirectory() && !ISDOTS(pFileName)) {
			sprintf(pEnd, "%s\\", pFileName);
			CreatePlaylist(plist, u8Path, bRecur, bFilter);
		}
		else if (!uFF->IsDirectory()) {
			strcpy(pEnd, pFileName);
			if (bFilter) {
				fnTester->SetFileName(pfc::stringcvt::string_os_from_utf8(pFileName));
				if (fnTester->InTheseTypes(m_FilterStrList, m_uFilterCount))
					bFiltered = true;
			}
			if (!bFiltered && input_entry::g_is_supported_path(u8Path))
				plist.add_item(strdup(u8Path));
		}
	} while (uFF->FindNext());

	delete fnTester;
	delete uFF;
}

void DirTreeCtrl::Queue(DirInsThreadParam *pParam)
{
	{
		CKuAutoCriticalSection theLocker(&m_lock);
		m_InsQueue.AddTail((ThreadParam *) pParam);
	}
	SetEvent(m_hEvent);
}

void DirTreeCtrl::Quit()
{
	{
		CKuAutoCriticalSection theLocker(&m_lock);
		m_bQuit = true;
	}
	SetEvent(m_hEvent);
}

bool DirTreeCtrl::IsQuitting()
{
	CKuAutoCriticalSection theLocker(&m_lock);
	bool bRet = m_bQuit;
	return bRet;
}

DWORD DirTreeCtrl::GetMarkedPathsLength()
{
	DWORD uBufLen = 1, i, count = GetMarkedCount();
	HTREEITEM hItem;
	TCHAR sBuffer[EX_MAX_PATH];
	for (i = 0;i < count;i++) {
		hItem = m_MarkItems[i];
		if (TreeView_GetParent(m_hWnd, hItem) && GetFullPath(hItem, sBuffer)) {
			PathRemoveBackslash(sBuffer);
			uBufLen += (_tcslen(sBuffer) + 1);
		}
	}
	if (uBufLen == 1)
		return 0;
	return uBufLen;
}

void DirTreeCtrl::GetMarkedPaths(LPTSTR buffer)
{
	DWORD i, count = GetMarkedCount();
	HTREEITEM hItem;
	TCHAR sBuffer[EX_MAX_PATH];

	for (i = 0;i < count;i++) {
		hItem = m_MarkItems[i];
		if (TreeView_GetParent(m_hWnd, hItem) && GetFullPath(hItem, sBuffer)) {
			PathRemoveBackslash(sBuffer);
			buffer += (_stprintf(buffer, _T("%s"), sBuffer) + 1);
		}
	}
	*buffer = _T('\0');
}

void DirTreeCtrl::MarkItem(HTREEITEM hItem, MARKTYPE op/* = TOGGLE*/)
{
	if (!hItem)
		hItem = GetSelectedItem();

	bool bMarked = IsMarked(hItem);

	switch (op) {
		case TOGGLE:
			MarkItem(hItem, bMarked ? UNMARKED : MARKED);
			break;
		case MARKED:
			if (!bMarked) {
				GetItemData(hItem)->m_bMarked = true;
				m_MarkItems.Add(hItem);
				TreeView_SetItemState(m_hWnd, hItem, TVIS_BOLD, TVIS_BOLD);
			}
			break;
		case UNMARKED:
			if (bMarked) {
				GetItemData(hItem)->m_bMarked = false;
				RemoveMarkedItem(hItem);
				TreeView_SetItemState(m_hWnd, hItem, 0, TVIS_BOLD);
			}
			break;
	}
}

BOOL DirTreeCtrl::OnNotify(LPNMHDR lpNMHDR)
{
	switch (lpNMHDR->code) {
		case TVN_ITEMEXPANDING:
			OnExpanding(lpNMHDR);
			break;
		case TVN_ITEMEXPANDED:
			OnExpanded(lpNMHDR);
			break;
		case NM_CLICK:
			{
				OnClick();
				int mod = MAKECIMOD(GetKeyState(VK_CONTROL) & 0x8000, GetKeyState(VK_MENU) & 0x8000,
					GetKeyState(VK_SHIFT) & 0x8000, (GetKeyState(VK_LWIN) & 0x8000) || (GetKeyState(VK_RWIN) & 0x8000));
				if (ParseShortcut(mod | CI_MC_LCLICK))
					return TRUE;
			}
			break;
		case NM_RCLICK:
			OnRClick(lpNMHDR);
			return TRUE;
		case TVN_SELCHANGED:
			OnSelectChanged(lpNMHDR);
			break;
		case TVN_DELETEITEM:
			{
				LPNMTREEVIEW lpNMTV = (LPNMTREEVIEW) lpNMHDR;
				CFileName *fnItem = (CFileName *) lpNMTV->itemOld.lParam;
				if (fnItem) {
					if (fnItem->m_bMarked)
						RemoveMarkedItem(lpNMTV->itemOld.hItem);
					if (fnItem->m_pInsParam)
						fnItem->m_pInsParam->Invalidate();
					delete fnItem;
				}
				if (m_hLastFavo == lpNMTV->itemOld.hItem) {
					m_hLastFavo = TreeView_GetPrevSibling(m_hWnd, m_hLastFavo);
					if (!m_hLastFavo)
						m_hLastFavo = TVI_FIRST;
				}
			}
			break;
		case TVN_GETDISPINFO:
			OnGetDispInfo((LPNMTVDISPINFO) lpNMHDR);
			break;
		case TVN_BEGINLABELEDIT:
			{
				LPNMTVDISPINFO ptvdi = (LPNMTVDISPINFO) lpNMHDR;
				CFileName *fnItem = (CFileName *) ptvdi->item.lParam;
				if (fnItem->IsDrive())
					return TRUE;
				m_hEditItem = ptvdi->item.hItem;
				_tcscpy(m_sEditName, fnItem->IsFile() ? fnItem->GetName() : fnItem->GetDispName()) ;
			}
			break;
		case TVN_ENDLABELEDIT:
			{
				LPNMTVDISPINFO ptvdi = (LPNMTVDISPINFO) lpNMHDR;
				CFileName *fnItem = (CFileName *) ptvdi->item.lParam;
				BOOL bOK = FALSE;
			
				if (m_hEditItem && m_hEditItem == ptvdi->item.hItem && !fnItem->IsDrive() &&
					ptvdi->item.pszText && _tcscmp(m_sEditName, ptvdi->item.pszText))
				{
					if (fnItem->IsFavorite()) {
						FavoItem **fvList = FavoItem::GetFavoList();
						int i, count = FavoItem::GetListCount();

						for (i = 0;i < count;i++) {
							if (_tcsicmp(fvList[i]->GetPath(), fnItem->GetName()) == 0) {
								fvList[i]->SetDesc(ptvdi->item.pszText);
								fnItem->SetDispName(ptvdi->item.pszText);
								FavoItem::SaveFavoList();
								bOK = TRUE;
								break;
							}
						}
					}
					else {
						TCHAR sOld[EX_MAX_PATH], sNew[EX_MAX_PATH];
						GetFullPath(m_hEditItem, sOld);
						*fnItem = ptvdi->item.pszText;
						GetFullPath(m_hEditItem, sNew);
						if(MoveFile(sOld, sNew))
							bOK = TRUE;
					}
				}

				if (bOK)
					OnSelectChanged();

				m_hEditItem = 0;
				return bOK;
			}
			break;
		case TVN_BEGINDRAG:
		case TVN_BEGINRDRAG:
			{
				LPNMTREEVIEW pnmtv = (LPNMTREEVIEW) lpNMHDR;
				TCHAR sBuffer[EX_MAX_PATH + 1];
				const unsigned int count = GetMarkedCount();
				unsigned int uBufLen = 1;

				if (count > 0) {
					uBufLen = GetMarkedPathsLength();
					if (uBufLen == 0)
						break;
				}
				else {
					if (((CFileName *) pnmtv->itemNew.lParam)->IsRoot() ||
						GetFullPath(pnmtv->itemNew.hItem, sBuffer) == NULL)
						break;
					PathRemoveBackslash(sBuffer);
					uBufLen = _tcslen(sBuffer) + 2;
				}

				LPDROPFILES pDrop;
				HGLOBAL hMem;

				if ((hMem = GlobalAlloc(GHND, sizeof(DROPFILES) + uBufLen * sizeof(TCHAR))) == NULL)
					break;

				if ((pDrop = (LPDROPFILES) GlobalLock(hMem)) == NULL) {
					GlobalFree(hMem);
					break;
				}

				LPTSTR pFiles = (LPTSTR)(((INT_PTR) pDrop) + sizeof(DROPFILES));

				pDrop->pFiles = sizeof(DROPFILES);
#ifdef _UNICODE
				pDrop->fWide = TRUE;
#else
				pDrop->fWide = FALSE;
#endif
				if (count > 0)
					GetMarkedPaths(pFiles);
				else
					_tcscpy(pFiles, sBuffer);

				GlobalUnlock(hMem);

				CIDropSource* pdsrc = new CIDropSource;
				CIDataObject* pdobj = new CIDataObject(pdsrc);
				DWORD dwEffect;
				FORMATETC fmt;
				STGMEDIUM stg;

				fmt.cfFormat = CF_HDROP;
				fmt.ptd = NULL;
				fmt.dwAspect = DVASPECT_CONTENT;
				fmt.lindex = -1;
				fmt.tymed = TYMED_HGLOBAL;

				stg.tymed = TYMED_HGLOBAL;
				stg.pUnkForRelease = NULL;
				stg.hGlobal = hMem;

				pdobj->SetData(&fmt, &stg, TRUE);

				if (count == 0)
					m_hDragItem = pnmtv->itemNew.hItem;

				DoDragDrop(pdobj, pdsrc, DROPEFFECT_COPY | DROPEFFECT_MOVE, &dwEffect);

				m_hDragItem = NULL;

				pdsrc->Release();
				pdobj->Release();

				if (count > 0) {
					SendMessage(m_hWnd, WM_COMMAND, ID_MARK_CLEAR_ALL, 0);
					Refresh();
				}
				else if (!PathFileExists(sBuffer))
					TreeView_DeleteItem(m_hWnd, pnmtv->itemNew.hItem);	

				ASSERT(GlobalSize(hMem) == 0);
			}
			break;
	}

	return FALSE;
}

LRESULT DirTreeCtrl::DefWindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
	switch (Msg) {
		case WM_ERASEBKGND:
			return TRUE;
		case WM_PAINT:
			return CallWindowProc(m_TreeProc, m_hWnd, WM_PAINT, (WPARAM)(HDC) MemDC(m_hWnd), 0);
		case WM_MBUTTONUP:
			OnClick();
			//NO break
		case WM_LBUTTONDBLCLK:
			{
				int mod = MAKECIMOD(GetKeyState(VK_CONTROL) & 0x8000, GetKeyState(VK_MENU) & 0x8000,
					GetKeyState(VK_SHIFT) & 0x8000, (GetKeyState(VK_LWIN) & 0x8000) || (GetKeyState(VK_RWIN) & 0x8000));
				int mou;
				switch (Msg) {
					case WM_LBUTTONDBLCLK:
						mou = CI_MC_DBLCLICK;
						break;
					case WM_MBUTTONUP:
						mou = CI_MC_MCLICK;
						break;
				}
				if (ParseShortcut(mod | mou))
					return 0;
			}
			break;
		case WM_KEYDOWN:
		case WM_SYSKEYDOWN:
			switch (wParam) {
				case VK_RETURN:
					SelectByPath();
					return 0;
				default:
					{
						int mod = MAKECIMOD(GetKeyState(VK_CONTROL) & 0x8000, GetKeyState(VK_MENU) & 0x8000,
							GetKeyState(VK_SHIFT) & 0x8000, (GetKeyState(VK_LWIN) & 0x8000) || (GetKeyState(VK_RWIN) & 0x8000));
						if (ParseShortcut(mod | (int) wParam))
							return 0;
					}
					break;
			}
			break;
		case WM_CONTEXTMENU:
			{
				POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
				RECT rc;
				if (pt.x == -1 || pt.y == -1) {
					GetWindowRect(m_hWnd, &rc);
					pt.x = rc.left;
					pt.y = rc.top;
				}
				OnContextMenu(pt.x, pt.y);
			}
			return 0;
		case WM_DESTROY:
			TreeView_DeleteAllItems(m_hWnd); // *sometimes* the items are not delete automatically.
			if (m_hTreeFont) {
				DeleteObject(m_hTreeFont);
				m_hTreeFont = NULL;
			}
			RevokeDragDrop(m_hWnd);
			Quit();
			HANDLE hThreads[DIR_INS_THREADS];
			for (int i = 0;i < DIR_INS_THREADS;i++)
				hThreads[i] = m_pInsThreads[i]->m_hThread;
			WaitForMultipleObjects(DIR_INS_THREADS, hThreads, TRUE, INFINITE);
			{
				MSG msg;
				while (PeekMessage(&msg, m_hWnd, WMU_INSERT, WMU_INSERT, PM_REMOVE | PM_NOYIELD | PM_QS_POSTMESSAGE)) {
					DirInsThreadParam *pParam = (DirInsThreadParam *) msg.wParam;
					if (pParam) {
						pParam->Invalidate();
						delete pParam;
					}
				}
			}
			while (m_InsQueue.GetCount() > 0) {
				DirInsThreadParam *pParam = (DirInsThreadParam *) m_InsQueue.RemoveHead();
				if (pParam) {
					pParam->Invalidate();
					delete pParam;
				}
			}
			for (int i = 0;i < DIR_INS_THREADS;i++) {
				ASSERT(m_pInsThreads[i]);
				delete m_pInsThreads[i];
				m_pInsThreads[i] = NULL;
			}
			break;
		case WM_COMMAND:
			switch (HIWORD(wParam)) {
				case 0: //from menus or buttons.
				case 1: //from accelerators
					{
						int iCmd = LOWORD(wParam);
						switch (iCmd) {
							case ID_REVEAL:
								{
									TCHAR thePath[EX_MAX_PATH];
									GetSelectedLocation(thePath);
									ShellExecute(NULL, _T("explore"), thePath, NULL, thePath, SW_SHOWNORMAL);
								}
								return 0;
							case ID_TOGGLE_EXPAND:
								{
									HTREEITEM hItem;
									if (hItem = GetSelectedItem())
										Expand(hItem, TVE_TOGGLE);
								}
								return 0;
							case ID_DELETE_FILES:
								{
									HTREEITEM hItem;
									CFileName *fnItem;
									UINT uMarkedCount;
									const bool bRecycled = ((GetKeyState(VK_SHIFT) & 0x8000) == 0);

									{
										bool bIsChanged = false;
										FavoItem **fvList = FavoItem::GetFavoList();
										int i, count = FavoItem::GetListCount();
										UINT j = 0, uDelCount = 0;
										uMarkedCount = GetMarkedCount();
										hItem = (uMarkedCount > 0) ? m_MarkItems[0] : GetSelectedItem();
										fnItem = GetItemData(hItem);
										HTREEITEM *pDelItems = new HTREEITEM [(uMarkedCount > 0) ? uMarkedCount : 1];

										do {
											if (fnItem->IsFavorite())
												for (i = 0;i < count;i++) {
													if (fvList[i] && _tcsicmp(fvList[i]->GetPath(), fnItem->GetName()) == 0) {
														fvList[i] = NULL;
														bIsChanged = true;
														pDelItems[uDelCount++] = hItem;
														break;
													}
												}
											j++;
											if (j < uMarkedCount)
												fnItem = GetItemData(hItem = m_MarkItems[j]);
										} while(j < uMarkedCount);

										if (bIsChanged) {
											FavoItem::SaveFavoList();
											for (j = 0;j < uDelCount;j++)
												TreeView_DeleteItem(m_hWnd, pDelItems[j]);
											if (uMarkedCount == 0 || uMarkedCount == uDelCount) {
												delete [] pDelItems;
												return 0;
											}
										}
										delete [] pDelItems;
									}
									{
										hItem = GetSelectedItem();
										uMarkedCount = GetMarkedCount();
										LPTSTR buffer = NULL;
										SHFILEOPSTRUCT fp = {0};

										if (uMarkedCount > 0) {
											buffer = (LPTSTR) malloc(sizeof(TCHAR) * GetMarkedPathsLength());
											GetMarkedPaths(buffer);
										}
										else if (hItem) {
											buffer = (LPTSTR) malloc(sizeof(TCHAR) * EX_MAX_PATH);
											GetFullPath(hItem, buffer);
											PathRemoveBackslash(buffer);
										}
										else
											break;

										fp.wFunc = FO_DELETE;
										fp.hwnd = m_hWnd;
										fp.fFlags = (bRecycled ? FOF_ALLOWUNDO : 0);
										fp.pFrom = buffer;

										SHFileOperation(&fp);

										if (uMarkedCount == 0 && !PathFileExists(buffer))
											TreeView_DeleteItem(m_hWnd, hItem);
										else
											Refresh();

										free(buffer);
									}
								}
								return 0;
							case ID_RENAME_FILE:
								{
									HTREEITEM hItem = GetSelectedItem();
									if (hItem)
										TreeView_EditLabel(m_hWnd, hItem);
								}
								return 0;
							case ID_ADBAR_GO:
								SelectByPath();
								return 0;
							case ID_SELECT_NOWPLAYING:
								{
									static_api_ptr_t<playback_control> pc;
									metadb_handle_ptr hMeta;
									if (pc->get_now_playing(hMeta)) {
										const char *uPath = hMeta->get_path();
										if (strnicmp(uPath, "file://", 7) == 0) {
											SelectByPath(pfc::stringcvt::string_os_from_utf8(uPath + 7));
											return 0;
										}
									}
								}
								break;
							case ID_ADD_FAVO:
								{
									TCHAR sPath[EX_MAX_PATH];
									UINT uMarkedCount = GetMarkedCount(), i = 0;
									HTREEITEM hItem = (uMarkedCount > 0) ? m_MarkItems[0] : GetSelectedItem();

									if (!hItem)
										break;

									FavoListCtrl *pFavoWnd = &PreferenceDlg::m_ppgFavorite.m_FavoList;

									if (pFavoWnd->IsRunning()) { // the preferences page is opening
										FavoItem *cItem;
										do {
											GetFullPath(hItem, sPath);
											cItem = new FavoItem;
											cItem->SetDesc(GetItemData(hItem)->GetName());
											cItem->SetPath(sPath);
											pFavoWnd->InsertItem(cItem);
											InsertFavorite(cItem);
											i++;
											if (i < uMarkedCount)
												hItem = m_MarkItems[i];
										} while (i < uMarkedCount);
									}
									else {
										if (FavoItem::GetListCount() >= MAX_FAVO) {
											MessageBox(m_hWnd, _T("Can not support more favorites!"), _T(APPNAME), MB_OK | MB_ICONERROR);
											return 0;
										}
										pfc::string8 uPref;
										uPref.set_string(prefs::sFavoList);
										do {
											GetFullPath(hItem, sPath);
											uPref.add_string(pfc::stringcvt::string_utf8_from_os(GetItemData(hItem)->GetName()));
											uPref.add_char('|');
											uPref.add_string(pfc::stringcvt::string_utf8_from_os(sPath));
											uPref.add_char('|');
											i++;
											if (i < uMarkedCount)
												hItem = m_MarkItems[i];
										} while (i < uMarkedCount);
										prefs::sFavoList = uPref;
										BulidFavoMenu();
										FavoItem **favoList = FavoItem::GetFavoList(false);
										UINT favoCount = (UINT) FavoItem::GetListCount();
										for (i = favoCount - (uMarkedCount > 0 ? uMarkedCount : 1);i < favoCount;i++)
											InsertFavorite(favoList[i]);
									}
								}
								return 0;
							case ID_PREFERENCE:
								{
									static_api_ptr_t<ui_control> ui_c;
									ui_c->show_preferences(PreferenceDlg::m_GUID);
								}
								return 0;
							case ID_MARK_THIS:
								MarkItem(NULL);
								return 0;
							case ID_MARK_ALL:
							case ID_MARK_ALL_SIBLING:
							case ID_MARK_CLEAR:
							case ID_MARK_CLEAR_SIBLING:
								{
									HTREEITEM hItem;
									hItem = GetSelectedItem();
									DWORD uID = LOWORD(wParam);
									MARKTYPE op = (uID == ID_MARK_ALL || uID == ID_MARK_ALL_SIBLING) ? MARKED : UNMARKED;
									if (hItem) {
										if (uID == ID_MARK_ALL_SIBLING || uID == ID_MARK_CLEAR_SIBLING)
											hItem = TreeView_GetParent(m_hWnd, hItem);
										hItem = hItem ? TreeView_GetChild(m_hWnd, hItem) : TreeView_GetRoot(m_hWnd);
										for (;hItem;hItem = TreeView_GetNextSibling(m_hWnd, hItem))
											MarkItem(hItem, op);
									}
								}
								return 0;
							case ID_MARK_CLEAR_ALL:
								{
									HTREEITEM hItem;
									unsigned int i, count = GetMarkedCount();
									for (i = 0;i < count;i++) {
										hItem = m_MarkItems[i];
										TreeView_SetItemState(m_hWnd, hItem, 0, TVIS_BOLD);
										GetItemData(hItem)->m_bMarked = false;
									}
									m_MarkItems.RemoveAll();
								}
								return 0;
							case ID_MARK_TO_HERE:
								{
									HTREEITEM hItem = GetSelectedItem();
									int cmp;
									if (cmp = FindInSibling(hItem, m_hPrevSel)) {
										if (cmp > 0)
											for (;hItem;hItem = TreeView_GetPrevSibling(m_hWnd, hItem)) {
												MarkItem(hItem, MARKED);
												if (hItem == m_hPrevSel)
													break;
											}
										else
											for (;hItem;hItem = TreeView_GetNextSibling(m_hWnd, hItem)) {
												MarkItem(hItem, MARKED);
												if (hItem == m_hPrevSel)
													break;
											}
									}
								}
								return 0;
							case ID_COLLAPSE_ALL:
								{
									HTREEITEM hItem;

									for (hItem = TreeView_GetRoot(m_hWnd);hItem;hItem = TreeView_GetNextSibling(m_hWnd, hItem))
										Expand(hItem, TVE_COLLAPSE);
								}
								return 0;
							case ID_RESET_TREE:
								InitInsert();
								return 0;
							default:
								{
									int CmdID = LOWORD(wParam);

									if (INRANGE(CmdID, ID_CMDLINE_START, ID_CMDLINE_END)) {
										CmdItem **cList = CmdItem::GetCmdList();
										CFileName *fnItem = GetItemData(GetSelectedItem());
										if (fnItem && cList[CmdID - ID_CMDLINE_START]) {
											TCHAR buf[EX_MAX_PATH];
											fnItem->SetLocation(GetSelectedLocation(buf));
											cList[CmdID - ID_CMDLINE_START]->PerformCommand(*fnItem);
										}
										return 0;
									}

									if (INRANGE(CmdID, ID_FAVO_START, ID_FAVO_END)) {
										FavoItem **favoList = FavoItem::GetFavoList();
										if (favoList[CmdID - ID_FAVO_START])
											SelectByPath(favoList[CmdID - ID_FAVO_START]->GetPath());
										return 0;
									}

									if (HIWORD(wParam) == 0) {//from menu
										bool bRecur = (prefs::bRecursive != 0);
										bool bShift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
										bool bDef = (prefs::bSendDef != 0);
										bool bCtrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
										if (bRecur != bShift)
											iCmd += RECUR_SHIFT;
										if (bDef != bCtrl)
											iCmd += DEFAULT_SHIFT;
									}
									OnPlaybackCommand(iCmd);
								}
								return 0;
						}
					}
			}
			break;
		case WMU_INIT:
			Init();
			return 0;
		case WMU_INSERT:
			{
				ASSERT(wParam);

				DirInsThreadParam *pParam = (DirInsThreadParam *) wParam;

				if (pParam->IsValid()) {
					ASSERT(pParam->m_hParentItem);
					ASSERT(pParam->m_pParent);
					ASSERT(pParam->m_pParent == GetItemData(pParam->m_hParentItem));
					ASSERT(pParam == pParam->m_pParent->m_pInsParam);

					InsertChildren(pParam->m_hParentItem, pParam->m_pFnList);
					pParam->m_pParent->m_pInsParam = NULL;
				}

				delete pParam;
			}
			break;
		case WMU_SEND_PLAYLIST:
			{
				ASSERT(wParam);

				PlayThreadParam *pParam = (PlayThreadParam *) wParam;
				HTREEITEM hItem = GetSelectedItem();
				pParam->SendPlaylist();
				SetFocus(m_hWnd);
				if (hItem)
					TreeView_SelectItem(m_hWnd, hItem);
				delete pParam;
			}
			break;
	}

	return CallWindowProc(m_TreeProc, m_hWnd, Msg, wParam, lParam);
}

LRESULT DirTreeCtrl::EditWindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
	switch (Msg) {
		case WM_SYSKEYDOWN:
		case WM_KEYDOWN:
			switch (wParam) {
				case VK_RETURN:
					SelectByPath();
					return FALSE;
			}
			break;
		case WM_COMMAND:
			switch (HIWORD(wParam)) {
				case 0:
				case 1:
					switch (LOWORD(wParam)) {
						case IDC_BTN_FAVO:
							{
								RECT rc;
								GetWindowRect(m_hBtnWnd, &rc);
								m_FavoMenu.TrackPopupMenu(TPM_RIGHTALIGN | TPM_RIGHTBUTTON, rc.right, rc.bottom, m_hWnd);
							}
							return 0;
					}
					break;
			}
		case WM_DESTROY:
			if (m_hEditBkBrush)
				DeleteObject(m_hEditBkBrush);
			if (m_hEditFont)
				DeleteObject(m_hEditFont);
			m_hEditBkBrush = 0;
			m_hEditFont = 0;
			break;
	}

	return CallWindowProc(m_EditProc, m_hEditWnd, Msg, wParam, lParam);
}

LRESULT WINAPI DirTreeCtrl::OnHook(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	DirTreeCtrl *pThis;

	//retrieve pointer to the class
	pThis = reinterpret_cast<DirTreeCtrl*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

	if (pThis)
		return pThis->DefWindowProc(Msg, wParam, lParam);

	return ::DefWindowProc(hWnd, Msg, wParam, lParam);
}

LRESULT WINAPI DirTreeCtrl::OnEditHook(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	DirTreeCtrl *pThis;

	//retrieve pointer to the class
	pThis = reinterpret_cast<DirTreeCtrl*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

	if (pThis)
		return pThis->EditWindowProc(Msg, wParam, lParam);

	return ::DefWindowProc(hWnd, Msg, wParam, lParam);
}

HRESULT STDMETHODCALLTYPE DirTreeCtrl::DragEnter(
    /* [unique][in] */ IDataObject __RPC_FAR *pDataObj,
    /* [in] */ DWORD grfKeyState,
    /* [in] */ POINTL pt,
    /* [out][in] */ DWORD __RPC_FAR *pdwEffect)
{
	HRESULT hResult = CIDropTarget::DragEnter(pDataObj, grfKeyState, pt, pdwEffect);

	if (m_bAllowDrop && hResult == S_OK && m_pSupportedFrmt) {
		STGMEDIUM medium;
		if (pDataObj->GetData(m_pSupportedFrmt, &medium) == S_OK) {
			HDROP hDrop;
			if ((hDrop = (HDROP) GlobalLock(medium.hGlobal)) == NULL)
				return hResult;

			UINT cch = DragQueryFile(hDrop, 0, NULL, 0);

			if (m_sDragFile)
				free(m_sDragFile);
			m_sDragFile = (LPTSTR) malloc(sizeof(TCHAR) * (cch + 1));

			DragQueryFile(hDrop, 0, m_sDragFile, cch + 1);

			GlobalUnlock(medium.hGlobal);
		}
	}

	return hResult;
}

void DirTreeCtrl::OnContextMenu(int x, int y)
{
	if (GetKeyState(VK_CONTROL) & 0x8000)
		SendMessage(m_hParent, WM_CONTEXTMENU, 0, (LPARAM) ((DWORD) y) << 16 | ((WORD) x));
	else {
		BulidMenu();

		int iCommand = m_ContextMenu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON | TPM_RETURNCMD, x, y, m_hWnd);

		if (!theSHMenuFactory.CloseShellMenu(m_pShellMenu, iCommand)) //invoke command
			SendMessage(m_hWnd, WM_COMMAND, iCommand, 0); //the shell menu is failed to created. ==> send the command anyway.

		m_pShellMenu = NULL;

		m_ContextMenu.DestroyMenu();
	}
}

bool DirTreeCtrl::OnDrop(FORMATETC *pFmtEtc, STGMEDIUM &medium, const POINTL &pt, DWORD *pdwEffect)
{
	if (pFmtEtc->cfFormat == CF_HDROP && medium.tymed == TYMED_HGLOBAL && 
		(*pdwEffect & (DROPEFFECT_COPY | DROPEFFECT_MOVE)))
	{
		LPDROPFILES pDrop;
		if ((pDrop = (LPDROPFILES) GlobalLock(medium.hGlobal)) == NULL)
			goto NOPROCESS;

		TVHITTESTINFO hitInfo = {0};
		CFileName *pFileName;
		hitInfo.pt.x = pt.x;
		hitInfo.pt.y = pt.y;
		ScreenToClient(m_hWnd, &hitInfo.pt);
		TreeView_HitTest(m_hWnd, &hitInfo);

		if (!(hitInfo.flags & TVHT_ONITEM) ||
			(pFileName = GetItemData(hitInfo.hItem)) != NULL && pFileName->IsFile())
		{
			GlobalUnlock(medium.hGlobal);
			goto NOPROCESS;
		}

		SHFILEOPSTRUCT shFileOp = {0};
		TCHAR sBuffer[EX_MAX_PATH + 1] = {0};

		shFileOp.hwnd = m_hWnd;
		shFileOp.wFunc = (*pdwEffect == DROPEFFECT_COPY) ? FO_COPY : FO_MOVE;
		shFileOp.pFrom = (LPCTSTR) (((INT_PTR) pDrop) + pDrop->pFiles);
		shFileOp.pTo = GetFullPath(hitInfo.hItem, sBuffer);
		shFileOp.fFlags = FOF_ALLOWUNDO;

		SHFileOperation(&shFileOp);

		GlobalUnlock(medium.hGlobal);

		InsertChildren(hitInfo.hItem, true);

		return true;
	}

NOPROCESS:
	*pdwEffect = DROPEFFECT_NONE;
	return false;
}

bool DirTreeCtrl::QueryDrop(DWORD grfKeyState, const POINTL &pt, LPDWORD pdwEffect)
{
#pragma warning(disable : 4533)

	DWORD dwOKEffects = *pdwEffect; 

	if (!m_bAllowDrop || !m_sDragFile)
		goto RETURN_FALSE;

	TVHITTESTINFO hitInfo = {0};
	TCHAR sBuffer[EX_MAX_PATH + 1];
	bool bSameDrive;

	hitInfo.pt.x = pt.x;
	hitInfo.pt.y = pt.y;

	ScreenToClient(m_hWnd, &hitInfo.pt);

	if (INRANGE(hitInfo.pt.y, 0, 20)) {
		HTREEITEM hFirstItem = TreeView_GetFirstVisible(m_hWnd), hItem;
		if (hFirstItem) {
			if (hItem = TreeView_GetPrevSibling(m_hWnd, hFirstItem))
				TreeView_EnsureVisible(m_hWnd, hItem);
			else if (hItem = TreeView_GetParent(m_hWnd, hFirstItem))
				TreeView_EnsureVisible(m_hWnd, hItem);
		}
	}

	TreeView_HitTest(m_hWnd, &hitInfo);

	if (hitInfo.flags & TVHT_ONITEM && m_hDragItem != hitInfo.hItem) {
		TreeView_SelectItem(m_hWnd, hitInfo.hItem);
		CFileName *pFileName = GetItemData(hitInfo.hItem);
		if (pFileName->IsFile())
			goto RETURN_FALSE;
		GetFullPath(hitInfo.hItem, sBuffer);
		bSameDrive = (_tcsnicmp(sBuffer, FindRealPath(m_sDragFile), 2) == 0);
	}
	else
		goto RETURN_FALSE;

 	*pdwEffect = (grfKeyState & MK_CONTROL) ?
				 ( (grfKeyState & MK_SHIFT) ? DROPEFFECT_MOVE : DROPEFFECT_COPY ):
				 ( (grfKeyState & MK_SHIFT) ? DROPEFFECT_MOVE : 0 );
	if (*pdwEffect == 0) {
		if (bSameDrive) {
			if (DROPEFFECT_MOVE & dwOKEffects)
				*pdwEffect = DROPEFFECT_MOVE; 
			else if (DROPEFFECT_COPY & dwOKEffects)
				*pdwEffect = DROPEFFECT_COPY;
			else
				*pdwEffect = DROPEFFECT_NONE;
		}
		else {
			if (DROPEFFECT_COPY & dwOKEffects)
				*pdwEffect = DROPEFFECT_COPY;
			else if (DROPEFFECT_MOVE & dwOKEffects)
				*pdwEffect = DROPEFFECT_MOVE;
			else
				*pdwEffect = DROPEFFECT_NONE;
		}
	}
	else {
	   if(!(*pdwEffect & dwOKEffects))
		  *pdwEffect = DROPEFFECT_NONE;
	}

	return (DROPEFFECT_NONE != *pdwEffect);

RETURN_FALSE:
	*pdwEffect = DROPEFFECT_NONE;
	return false;
}
