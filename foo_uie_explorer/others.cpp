#include "stdafx.h"
#include "preference.h"
#include "others.h"

namespace globals
{
	HFONT hSymbolFont = 0;
}

COLORREF ChooseColor(COLORREF clr, HWND hWnd/*= 0*/)
{
	CHOOSECOLOR cc = {0};

	COLORREF clrarray[16];

	cc.lStructSize = sizeof(CHOOSECOLOR);
	cc.hwndOwner = hWnd;
	cc.rgbResult = clr;
	cc.Flags = CC_ANYCOLOR | CC_FULLOPEN | CC_RGBINIT;
	cc.lpCustColors = clrarray;

	return ::ChooseColor(&cc) ? cc.rgbResult : -1;
}

LOGFONT ChooseFont(const LOGFONT &lf, HWND hWnd/*= 0*/)
{
	CHOOSEFONT cf = {0};
	LOGFONT newlf = lf;

	cf.lStructSize = sizeof(CHOOSEFONT);
	cf.hwndOwner = hWnd;
	cf.lpLogFont = &newlf;
	cf.iPointSize = 100;
	cf.Flags = CF_INITTOLOGFONTSTRUCT | CF_NOVERTFONTS | CF_SCREENFONTS;
	cf.nFontType = SCREEN_FONTTYPE;

	return ::ChooseFont(&cf) ? newlf : lf;
}

bool ChooseFolder(LPTSTR folder, HWND hWnd/*= 0*/)
{
	LPMALLOC pMalloc;
	bool bOK = false;

	if (SHGetMalloc(&pMalloc) == NOERROR) {
		BROWSEINFO brInfo = {0};
		brInfo.hwndOwner = hWnd;
		brInfo.pszDisplayName = folder;
		brInfo.lpszTitle = _T("Please select a folder.");
		brInfo.ulFlags = BIF_USENEWUI;

		CoInitialize(NULL);

		LPITEMIDLIST pidl;
		if ((pidl = SHBrowseForFolder(&brInfo)) != NULL){
			if (SHGetPathFromIDList(pidl, folder))
				bOK = true;
			pMalloc->Free(pidl);
		}
		pMalloc->Release();

		CoUninitialize();
	}

	return bOK;
}

HFONT CreatePointFont(int nPointSize, LPCTSTR lpszFaceName, HDC hDC/* = 0*/)
{
	LOGFONT ft = {0};
	bool bRes = false;

	if (!hDC) {
		hDC = GetDC(NULL);
		bRes = true;
	}

	ft.lfCharSet = DEFAULT_CHARSET;
	ft.lfHeight = nPointSize;
	lstrcpyn(ft.lfFaceName, lpszFaceName, ARRSIZE(ft.lfFaceName));
	POINT pt;
	pt.y = GetDeviceCaps(hDC, LOGPIXELSY) * ft.lfHeight;
	pt.y /= 720;
	pt.x = 0;
	DPtoLP(hDC, &pt, 1);
	POINT ptOrg = { 0, 0 };
	DPtoLP(hDC, &ptOrg, 1);
	ft.lfHeight = -abs(pt.y - ptOrg.y);

	if (bRes)
		ReleaseDC(NULL, hDC);

	return CreateFontIndirect(&ft);
}

LPTSTR GetPrefString(LPCTSTR str)
{
	LPTSTR buf, ptr;

	buf = _tcsdup(str);

	for (ptr = buf;*ptr;ptr++)
		if (*ptr == _T('|'))
			*ptr = _T('_');

	return buf;
}

#ifdef _DEBUG
void SetThreadName(DWORD dwThreadID, LPCSTR szThreadName)
{
	THREADNAME_INFO info = {0x1000, szThreadName, dwThreadID, 0};

	__try
	{
		RaiseException(0x406D1388, 0, sizeof(THREADNAME_INFO) / sizeof(DWORD), (DWORD*) &info);
	} __except(EXCEPTION_CONTINUE_EXECUTION) {
	}
}
#endif

