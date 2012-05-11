#pragma once

class ExMenu
{
public:
	ExMenu() : m_hMenu(0)
	{
	}

	virtual ~ExMenu()
	{
		DestroyMenu();
	}

	HMENU Attach(HMENU hMenu)
	{
		HMENU hOldMenu = m_hMenu;
		m_hMenu = hMenu;
		return hOldMenu;
	}

	HMENU Detach()
	{
		HMENU hOldMenu = m_hMenu;
		m_hMenu = 0;
		return hOldMenu;	
	}

	UINT GetMenuItemCount() const
	{
		return ::GetMenuItemCount(m_hMenu);
	}

	HMENU CreatePopupMenu()
	{
		if (m_hMenu)
			DestroyMenu();
		m_hMenu = ::CreatePopupMenu();
		return m_hMenu;
	}

	BOOL TrackPopupMenu(UINT uFlags, int x, int y, HWND hWnd = NULL, const LPRECT prcRect = NULL)
	{
		return ::TrackPopupMenu(m_hMenu, uFlags, x, y, 0, hWnd, prcRect);
	}

	DWORD CheckMenuItem(UINT uIDCheckItem, UINT uCheck)
	{
		return ::CheckMenuItem(m_hMenu, uIDCheckItem, uCheck);
	}

	BOOL CheckMenuRadioItem(UINT idFirst, UINT idLast, UINT idCheck, UINT uFlags = MF_BYCOMMAND)
	{
		return ::CheckMenuRadioItem(m_hMenu, idFirst, idLast, idCheck, uFlags);
	}

	void DestroyMenu()
	{
		if (m_hMenu)
			::DestroyMenu(Detach());
	}

	BOOL AppendMenu(UINT uFlags, UINT_PTR uIDNewItem = 0, LPCTSTR lpNewItem = NULL)
	{
		return ::AppendMenu(m_hMenu, uFlags, uIDNewItem, lpNewItem);
	}

	BOOL InsertMenu(UINT uPosition, UINT uFlags, UINT_PTR uIDNewItem = 0, LPCTSTR lpNewItem = NULL)
	{
		return ::InsertMenu(m_hMenu, uPosition, uFlags, uIDNewItem, lpNewItem);
	}

	BOOL RemoveMenu(UINT uPosition, UINT uFlags = MF_BYCOMMAND)
	{
		return ::RemoveMenu(m_hMenu, uPosition, uFlags);
	}

	BOOL DeleteMenu(UINT uPosition, UINT uFlags = MF_BYCOMMAND)
	{
		return ::DeleteMenu(m_hMenu, uPosition, uFlags);
	}

	BOOL InsertMenuItem(UINT uItem, BOOL fByPosition, LPCMENUITEMINFO lpmii)
	{
		return ::InsertMenuItem(m_hMenu, uItem, fByPosition, lpmii);
	}
	operator HMENU() const
	{
		return m_hMenu;
	}

	HMENU m_hMenu;
};
