#pragma once

#include "foo_uie_explorer.h"

namespace globals
{
	extern HFONT hSymbolFont;
}

COLORREF ChooseColor(COLORREF clr, HWND hWnd = 0);
LOGFONT ChooseFont(const LOGFONT &lf, HWND hWnd = 0);
bool ChooseFolder(LPTSTR folder, HWND hWnd = 0);
HFONT CreatePointFont(int nPointSize, LPCTSTR lpszFaceName, HDC hDC = 0);

LPTSTR GetPrefString(LPCTSTR str);

inline LPCTSTR FindRealPath(LPCTSTR sPath)
{
	return _tcsncmp(sPath, _T("\\\\?\\"), 4) ? sPath : sPath + 4;
}

inline LPTSTR strcpy_getend(LPTSTR &dest, LPCTSTR &src)
{
	ASSERT(dest);
	ASSERT(src);

	while (*dest = *src) {
		dest++;
		src++;
	}

	return dest;
}

typedef struct tagTHREADNAME_INFO
{
   DWORD dwType; // must be 0x1000
   LPCSTR szName; // pointer to name (in user addr space)
   DWORD dwThreadID; // thread ID (-1=caller thread)
   DWORD dwFlags; // reserved for future use, must be zero
} THREADNAME_INFO;

#ifdef _DEBUG
void SetThreadName(DWORD dwThreadID, LPCSTR szThreadName);
#else
#define SetThreadName(a, b) ((void) 0)
#endif

#ifndef ARRSIZE
#define ARRSIZE(arr) (sizeof(arr) / sizeof(arr[0]))
#endif

