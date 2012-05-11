#pragma once

typedef struct tagContextMenu {
	int iMenuVer;
	union {
		LPCONTEXTMENU pContextMenu;
		LPCONTEXTMENU2 pContextMenu2;
		LPCONTEXTMENU3 pContextMenu3;
	};
} SCONTEXTMENU, *LPSCONTEXTMENU;

class CShellMenu
{
	friend class CShellContextMenu;
public:
	HMENU GetMenu() const
	{
		return m_hMenu;
	}
	HMENU Detach()
	{
		HMENU hOldMenu = m_hMenu;
		m_hMenu = 0;
		return hOldMenu;
	}
	UINT GetMaxId() const
	{
		return m_uIdMax;
	}
	UINT GetMinId() const
	{
		return m_uIdMin;
	}
private:
	CShellMenu(HWND hWnd, UINT uIdMin, UINT uIdMax) //only create instances in CShellContextMenu.
		: m_hWnd(hWnd), m_WndProc(0), m_uIdMin(uIdMin), m_uIdMax(uIdMax), m_hMenu(NULL)
	{
		ZeroMemory(&m_ContextMenu, sizeof(SCONTEXTMENU));
	}
	virtual ~CShellMenu() //only destroy in CShellContextMenu.
	{
		if (m_ContextMenu.pContextMenu)
			m_ContextMenu.pContextMenu->Release();
		if (m_hMenu)
			DestroyMenu(m_hMenu);
	}

	HMENU m_hMenu;

	SCONTEXTMENU m_ContextMenu;
	UINT m_uIdMin;
	UINT m_uIdMax;
	HWND m_hWnd;
	WNDPROC m_WndProc;
};

class CShellContextMenu
{
public:
	CShellContextMenu();
	virtual ~CShellContextMenu();

	HMENU GetMenu();


	//Warning: You should create only one shell menu for a window at a time.
	CShellMenu *OpenShellMenu(HWND hWnd, UINT uIdMin, UINT uIdMax);
	//When 'bInvoke' is false, 'idCommand' is ignored.
	bool CloseShellMenu(CShellMenu *pShellMenu, UINT idCommand, bool bInvoke = true);

	void SetObjects (IShellFolder * psfFolder, LPITEMIDLIST pidlItem);
	void SetObjects (IShellFolder * psfFolder, LPITEMIDLIST * pidlArray, UINT nItemCount);
	void SetObjects (LPCTSTR strObject);
	void SetObjects (const LPTSTR *strArray, UINT nItemCount);
	void SetObjects (LPITEMIDLIST pidl);

	UINT ShowContextMenu (HWND hWnd, const POINT &pt, UINT uIdMin, UINT uIdMax);

private:
	int m_nItems;
	bool m_bDelete;
	HMENU m_hMenu;
	IShellFolder *m_psfFolder;
	LPITEMIDLIST *m_pidlArray;

	static void InvokeCommand (LPCONTEXTMENU pContextMenu, UINT idCommand);
	BOOL GetContextMenu (void ** ppContextMenu, int & iMenuType);
	HRESULT SHBindToParentEx (LPCITEMIDLIST pidl, REFIID riid, VOID **ppv, LPCITEMIDLIST *ppidlLast);
	static LRESULT CALLBACK HookWndProc (HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	void FreePIDLArray (LPITEMIDLIST * pidlArray);
	LPITEMIDLIST CopyPIDL (LPCITEMIDLIST pidl, int cb = -1);
	UINT GetPIDLSize (LPCITEMIDLIST pidl);
	LPBYTE GetPIDLPos (LPCITEMIDLIST pidl, int nPos);
	int GetPIDLCount (LPCITEMIDLIST pidl);
};

extern CShellContextMenu theSHMenuFactory;
