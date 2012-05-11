#include "stdafx.h"
#include "ShellContextMenu.h"

#define S_OLDPROC		_T("CShellContextMenu_OldWndProc")
#define S_CONTEXTMENU	_T("CShellContextMenu_ContextMenu")

CShellContextMenu theSHMenuFactory;

CShellContextMenu::CShellContextMenu()
	: m_psfFolder(NULL), m_pidlArray(NULL), m_hMenu(NULL)
{
}

CShellContextMenu::~CShellContextMenu()
{
	// free all allocated datas
	if (m_psfFolder && m_bDelete)
		m_psfFolder->Release();

	m_psfFolder = NULL;

	FreePIDLArray(m_pidlArray);

	if (m_hMenu)
		DestroyMenu(m_hMenu);
}

// this functions determines which version of IContextMenu is avaibale for those objects (always the highest one)
// and returns that interface
BOOL CShellContextMenu::GetContextMenu (void ** ppContextMenu, int & iMenuType)
{
	*ppContextMenu = NULL;
	LPCONTEXTMENU icm1 = NULL;

	if (!m_psfFolder)
		return FALSE;
	// first we retrieve the normal IContextMenu interface (every object should have it)
	m_psfFolder->GetUIObjectOf (NULL, m_nItems, (LPCITEMIDLIST *) m_pidlArray, IID_IContextMenu, NULL, (void**) &icm1);

	if (icm1)
	{	// since we got an IContextMenu interface we can now obtain the higher version interfaces via that
		if (icm1->QueryInterface (IID_IContextMenu3, ppContextMenu) == NOERROR)
			iMenuType = 3;
		else if (icm1->QueryInterface (IID_IContextMenu2, ppContextMenu) == NOERROR)
			iMenuType = 2;

		if (*ppContextMenu) 
			icm1->Release(); // we can now release version 1 interface, cause we got a higher one
		else 
		{	
			iMenuType = 1;
			*ppContextMenu = icm1;	// since no higher versions were found
		}							// redirect ppContextMenu to version 1 interface
	}
	else
		return (FALSE);	// something went wrong
	
	return (TRUE); // success
}


LRESULT CALLBACK CShellContextMenu::HookWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	LPSCONTEXTMENU pCMenu = (LPSCONTEXTMENU) GetProp(hWnd, S_CONTEXTMENU);

	switch (message) { 
		case WM_MENUCHAR:	// only supported by IContextMenu3
			if (pCMenu->iMenuVer == 3)
			{
				LRESULT lResult = 0;
				pCMenu->pContextMenu3->HandleMenuMsg2(message, wParam, lParam, &lResult);
				return (lResult);
			}
			break;
		case WM_DRAWITEM:
		case WM_MEASUREITEM:
			if (wParam) 
				break; // if wParam != 0 then the message is not menu-related
		case WM_INITMENUPOPUP:
			if (pCMenu->iMenuVer == 2)
				pCMenu->pContextMenu2->HandleMenuMsg(message, wParam, lParam);
			else if (pCMenu->iMenuVer == 3)
				pCMenu->pContextMenu3->HandleMenuMsg(message, wParam, lParam);
			return (message == WM_INITMENUPOPUP ? FALSE : TRUE); // inform caller that we handled WM_INITPOPUPMENU by ourself
		default:
			break;
	}

	// call original WndProc of window to prevent undefined bevhaviour of window
	return ::CallWindowProc((WNDPROC) GetProp(hWnd, S_OLDPROC), hWnd, message, wParam, lParam);
}


//Warning: You should create only one shell menu for a window at a time.
CShellMenu *CShellContextMenu::OpenShellMenu(HWND hWnd, UINT uIdMin, UINT uIdMax)
{
	ASSERT(uIdMin < uIdMax);
	ASSERT(hWnd);

	int iMenuType = 0;	// to know which version of IContextMenu is supported
	LPCONTEXTMENU pContextMenu;

	if (!GetContextMenu ((void**) &pContextMenu, iMenuType))
		return NULL;

	CShellMenu *pShellMenu = new CShellMenu(hWnd, uIdMin, uIdMax);
	pShellMenu->m_ContextMenu.iMenuVer = iMenuType;
	pShellMenu->m_ContextMenu.pContextMenu = pContextMenu;
	pShellMenu->m_hMenu = CreatePopupMenu();

	// lets fill the our popupmenu
	pShellMenu->m_ContextMenu.pContextMenu->QueryContextMenu(pShellMenu->m_hMenu, GetMenuItemCount(pShellMenu->m_hMenu), pShellMenu->m_uIdMin, pShellMenu->m_uIdMax, CMF_NORMAL | CMF_EXPLORE);
 
	// subclass window to handle menurelated messages in CShellContextMenu
	if (iMenuType > 1)	// only subclass if its version 2 or 3
	{
		pShellMenu->m_WndProc = (WNDPROC) SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR) HookWndProc);
		SetProp(hWnd, S_OLDPROC, (HANDLE) pShellMenu->m_WndProc);
		SetProp(hWnd, S_CONTEXTMENU, (HANDLE) &pShellMenu->m_ContextMenu);
	}

	return pShellMenu;
}

bool CShellContextMenu::CloseShellMenu(CShellMenu *pShellMenu, UINT idCommand, bool bInvoke/* = true*/)
{
	if (pShellMenu) {
		if (bInvoke && idCommand >= pShellMenu->m_uIdMin && idCommand <= pShellMenu->m_uIdMax) {
			InvokeCommand(pShellMenu->m_ContextMenu.pContextMenu, idCommand - pShellMenu->m_uIdMin);
			bInvoke = false;
		}

		if (pShellMenu->m_WndProc) {// unsubclass
			SetWindowLongPtr(pShellMenu->m_hWnd, GWLP_WNDPROC, (LONG_PTR) pShellMenu->m_WndProc);
			RemoveProp(pShellMenu->m_hWnd, S_OLDPROC);
			RemoveProp(pShellMenu->m_hWnd, S_CONTEXTMENU);
		}

		if (bInvoke)
			SendMessage(pShellMenu->m_hWnd, WM_COMMAND, idCommand, 0);

		delete pShellMenu;

		return true;
	}

	return false;
}

UINT CShellContextMenu::ShowContextMenu(HWND hWnd, const POINT &pt, UINT uIdMin, UINT uIdMax)
{
	ASSERT(hWnd);
	ASSERT(uIdMin < uIdMax);

	SCONTEXTMENU SContextMenu;

	if (!GetContextMenu((void**) &SContextMenu.pContextMenu, SContextMenu.iMenuVer))
		return 0; // something went wrong

	if (!m_hMenu)
	{
		DestroyMenu(m_hMenu);
		m_hMenu = CreatePopupMenu();
	}

	// lets fill the our popupmenu  
	SContextMenu.pContextMenu->QueryContextMenu(m_hMenu, GetMenuItemCount(m_hMenu), uIdMin, uIdMax, CMF_NORMAL | CMF_EXPLORE);
 
	// subclass window to handle menurelated messages in CShellContextMenu 
	WNDPROC OldWndProc;
	if (SContextMenu.iMenuVer > 1)	// only subclass if its version 2 or 3
	{
		OldWndProc = (WNDPROC) SetWindowLongPtr (hWnd, GWLP_WNDPROC, (LONG_PTR) HookWndProc);
		SetProp(hWnd, S_OLDPROC, (HANDLE) OldWndProc);
		SetProp(hWnd, S_CONTEXTMENU, (HANDLE) &SContextMenu);
	}
	else
		OldWndProc = NULL;

	UINT idCommand = ::TrackPopupMenu(m_hMenu, TPM_RETURNCMD | TPM_LEFTALIGN, pt.x, pt.y, 0, hWnd, NULL);

	if (OldWndProc) {// unsubclass
		SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR) OldWndProc);
		RemoveProp(hWnd, S_OLDPROC);
		RemoveProp(hWnd, S_CONTEXTMENU);
	}

	if (idCommand >= uIdMin && idCommand <= uIdMax)	// see if returned idCommand belongs to shell menu entries
	{
		InvokeCommand(SContextMenu.pContextMenu, idCommand - uIdMin);	// execute related command
		idCommand = 0;
	}

	SContextMenu.pContextMenu->Release();

	return idCommand;
}


void CShellContextMenu::InvokeCommand (LPCONTEXTMENU pContextMenu, UINT idCommand)
{
	CMINVOKECOMMANDINFO cmi = {0};
	cmi.cbSize = sizeof (CMINVOKECOMMANDINFO);
	cmi.lpVerb = (LPSTR) MAKEINTRESOURCE (idCommand);
	cmi.nShow = SW_SHOWNORMAL;
	
	pContextMenu->InvokeCommand (&cmi);
}


void CShellContextMenu::SetObjects(LPCTSTR strObject)
{
	// only one object is passed
	
	SetObjects ((const LPTSTR *) &strObject, 1);	// and pass it to SetObjects (CStringArray &strArray)
								// for further processing
}


void CShellContextMenu::SetObjects(const LPTSTR *strArray, UINT nItemCount)
{
	ASSERT(nItemCount > 0);
	ASSERT(strArray);

	// free all allocated datas
	if (m_psfFolder && m_bDelete)
		m_psfFolder->Release ();
	m_psfFolder = NULL;
	FreePIDLArray (m_pidlArray);
	m_pidlArray = NULL;
	
	// get IShellFolder interface of Desktop (root of shell namespace)
	IShellFolder * psfDesktop = NULL;
	SHGetDesktopFolder (&psfDesktop);	// needed to obtain full qualified pidl

	// ParseDisplayName creates a PIDL from a file system path relative to the IShellFolder interface
	// but since we use the Desktop as our interface and the Desktop is the namespace root
	// that means that it's a fully qualified PIDL, which is what we need
	LPITEMIDLIST pidl = NULL;
	
#ifndef _UNICODE
	OLECHAR * olePath = NULL;
	int iLen = strlen(strArray[0]);
	olePath = (OLECHAR *) alloca((iLen + 1) * sizeof (OLECHAR));
	MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, strArray[0], -1, olePath, iLen + 1);	
	psfDesktop->ParseDisplayName (NULL, 0, olePath, NULL, &pidl, NULL);
#else
	psfDesktop->ParseDisplayName (NULL, 0, (LPOLESTR) strArray[0], NULL, &pidl, NULL);
#endif

	// now we need the parent IShellFolder interface of pidl, and the relative PIDL to that interface
	LPITEMIDLIST pidlItem = NULL;	// relative pidl
	SHBindToParentEx (pidl, IID_IShellFolder, (void **) &m_psfFolder, NULL);
	free (pidlItem);
	// get interface to IMalloc (need to free the PIDLs allocated by the shell functions)
	LPMALLOC lpMalloc = NULL;
	SHGetMalloc (&lpMalloc);
	lpMalloc->Free (pidl);

	// now we have the IShellFolder interface to the parent folder specified in the first element in strArray
	// since we assume that all objects are in the same folder (as it's stated in the MSDN)
	// we now have the IShellFolder interface to every objects parent folder
	
	IShellFolder * psfFolder = NULL;
	m_nItems = nItemCount;
	for (UINT i = 0; i < nItemCount; i++)
	{
#ifndef _UNICODE
		iLen = strlen(strArray[i]);
		olePath = (OLECHAR *) alloca((iLen + 1) * sizeof (OLECHAR));
		MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, strArray[i], -1, olePath, iLen + 1);	
		psfDesktop->ParseDisplayName (NULL, 0, olePath, NULL, &pidl, NULL);
#else
		psfDesktop->ParseDisplayName (NULL, 0, (LPOLESTR) strArray[i], NULL, &pidl, NULL);
#endif
		m_pidlArray = (LPITEMIDLIST *) realloc (m_pidlArray, (i + 1) * sizeof (LPITEMIDLIST));
		// get relative pidl via SHBindToParent
		SHBindToParentEx (pidl, IID_IShellFolder, (void **) &psfFolder, (LPCITEMIDLIST *) &pidlItem);
		m_pidlArray[i] = CopyPIDL (pidlItem);	// copy relative pidl to pidlArray
		free (pidlItem);
		lpMalloc->Free (pidl);		// free pidl allocated by ParseDisplayName
		if (psfFolder)
			psfFolder->Release ();
	}
	lpMalloc->Release ();
	psfDesktop->Release ();

	m_bDelete = true; // indicates that m_psfFolder should be deleted by CShellContextMenu
}


// only one full qualified PIDL has been passed
void CShellContextMenu::SetObjects(LPITEMIDLIST pidl)
{
	// free all allocated datas
	if (m_psfFolder && m_bDelete)
		m_psfFolder->Release ();
	m_psfFolder = NULL;
	FreePIDLArray (m_pidlArray);
	m_pidlArray = NULL;

		// full qualified PIDL is passed so we need
	// its parent IShellFolder interface and its relative PIDL to that
	LPITEMIDLIST pidlItem = NULL;
	SHBindToParent ((LPCITEMIDLIST) pidl, IID_IShellFolder, (void **) &m_psfFolder, (LPCITEMIDLIST *) &pidlItem);	

	m_pidlArray = (LPITEMIDLIST *) malloc (sizeof (LPITEMIDLIST));	// allocate ony for one elemnt
	m_pidlArray[0] = CopyPIDL (pidlItem);


	// now free pidlItem via IMalloc interface (but not m_psfFolder, that we need later
	LPMALLOC lpMalloc = NULL;
	SHGetMalloc (&lpMalloc);
	lpMalloc->Free (pidlItem);
	lpMalloc->Release();

	m_nItems = 1;
	m_bDelete = true; // indicates that m_psfFolder should be deleted by CShellContextMenu
}


// IShellFolder interface with a relative pidl has been passed
void CShellContextMenu::SetObjects(IShellFolder *psfFolder, LPITEMIDLIST pidlItem)
{
	// free all allocated datas
	if (m_psfFolder && m_bDelete)
		m_psfFolder->Release ();
	m_psfFolder = NULL;
	FreePIDLArray (m_pidlArray);
	m_pidlArray = NULL;

	m_psfFolder = psfFolder;

	m_pidlArray = (LPITEMIDLIST *) malloc (sizeof (LPITEMIDLIST));
	m_pidlArray[0] = CopyPIDL (pidlItem);
	
	m_nItems = 1;
	m_bDelete = false;	// indicates wheter m_psfFolder should be deleted by CShellContextMenu
}

void CShellContextMenu::SetObjects(IShellFolder * psfFolder, LPITEMIDLIST *pidlArray, UINT nItemCount)
{
	// free all allocated datas
	if (m_psfFolder && m_bDelete)
		m_psfFolder->Release ();
	m_psfFolder = NULL;
	FreePIDLArray (m_pidlArray);
	m_pidlArray = NULL;

	m_psfFolder = psfFolder;

	m_pidlArray = (LPITEMIDLIST *) malloc (nItemCount * sizeof (LPITEMIDLIST));

	for (UINT i = 0; i < nItemCount; i++)
		m_pidlArray[i] = CopyPIDL (pidlArray[i]);

	m_nItems = nItemCount;
	m_bDelete = false;	// indicates wheter m_psfFolder should be deleted by CShellContextMenu
}


void CShellContextMenu::FreePIDLArray(LPITEMIDLIST *pidlArray)
{
	if (!pidlArray)
		return;

	int iSize = _msize (pidlArray) / sizeof (LPITEMIDLIST);

	for (int i = 0; i < iSize; i++)
		free (pidlArray[i]);
	free (pidlArray);
}


LPITEMIDLIST CShellContextMenu::CopyPIDL (LPCITEMIDLIST pidl, int cb)
{
	if (cb == -1)
		cb = GetPIDLSize (pidl); // Calculate size of list.

    LPITEMIDLIST pidlRet = (LPITEMIDLIST) calloc (cb + sizeof (USHORT), sizeof (BYTE));
    if (pidlRet)
		CopyMemory(pidlRet, pidl, cb);

    return (pidlRet);
}


UINT CShellContextMenu::GetPIDLSize (LPCITEMIDLIST pidl)
{  
	if (!pidl) 
		return 0;
	int nSize = 0;
	LPITEMIDLIST pidlTemp = (LPITEMIDLIST) pidl;
	while (pidlTemp->mkid.cb)
	{
		nSize += pidlTemp->mkid.cb;
		pidlTemp = (LPITEMIDLIST) (((LPBYTE) pidlTemp) + pidlTemp->mkid.cb);
	}
	return nSize;
}

HMENU CShellContextMenu::GetMenu()
{
	if (!m_hMenu)
		m_hMenu = CreatePopupMenu(); // create the popupmenu (its empty)
	return m_hMenu;
}


// this is workaround function for the Shell API Function SHBindToParent
// SHBindToParent is not available under Win95/98
HRESULT CShellContextMenu::SHBindToParentEx (LPCITEMIDLIST pidl, REFIID riid, VOID **ppv, LPCITEMIDLIST *ppidlLast)
{
	HRESULT hr = 0;
	if (!pidl || !ppv)
		return E_POINTER;
	
	int nCount = GetPIDLCount (pidl);
	if (nCount == 0)	// desktop pidl of invalid pidl
		return E_POINTER;

	IShellFolder * psfDesktop = NULL;
	SHGetDesktopFolder (&psfDesktop);
	if (nCount == 1)	// desktop pidl
	{
		if ((hr = psfDesktop->QueryInterface(riid, ppv)) == S_OK)
		{
			if (ppidlLast) 
				*ppidlLast = CopyPIDL (pidl);
		}
		psfDesktop->Release ();
		return hr;
	}

	LPBYTE pRel = GetPIDLPos (pidl, nCount - 1);
	LPITEMIDLIST pidlParent = NULL;
	pidlParent = CopyPIDL (pidl, pRel - (LPBYTE) pidl);
	IShellFolder * psfFolder = NULL;
	
	if ((hr = psfDesktop->BindToObject (pidlParent, NULL, IID_IShellFolder, (void **) &psfFolder)) != S_OK)
	{
		free (pidlParent);
		psfDesktop->Release ();
		return hr;
	}
	if ((hr = psfFolder->QueryInterface (riid, ppv)) == S_OK)
	{
		if (ppidlLast)
			*ppidlLast = CopyPIDL ((LPCITEMIDLIST) pRel);
	}
	free (pidlParent);
	psfFolder->Release ();
	psfDesktop->Release ();
	return hr;
}


LPBYTE CShellContextMenu::GetPIDLPos (LPCITEMIDLIST pidl, int nPos)
{
	if (!pidl)
		return 0;
	int nCount = 0;
	
	BYTE * pCur = (BYTE *) pidl;
	while (((LPCITEMIDLIST) pCur)->mkid.cb)
	{
		if (nCount == nPos)
			return pCur;
		nCount++;
		pCur += ((LPCITEMIDLIST) pCur)->mkid.cb;	// + sizeof(pidl->mkid.cb);
	}
	if (nCount == nPos) 
		return pCur;
	return NULL;
}


int CShellContextMenu::GetPIDLCount (LPCITEMIDLIST pidl)
{
	if (!pidl)
		return 0;

	int nCount = 0;
	BYTE*  pCur = (BYTE *) pidl;
	while (((LPCITEMIDLIST) pCur)->mkid.cb)
	{
		nCount++;
		pCur += ((LPCITEMIDLIST) pCur)->mkid.cb;
	}
	return nCount;
}