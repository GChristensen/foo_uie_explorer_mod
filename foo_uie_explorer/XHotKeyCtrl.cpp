#include "stdafx.h"
#include "xhotkeyctrl.h"

LRESULT XHotKeyCtrl::DefWindowProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
	switch (Msg) {
		case WM_KEYDOWN:
		case WM_SYSKEYDOWN:
			{
				int mod = MAKECIMOD(GetKeyState(VK_CONTROL) & 0x8000, GetKeyState(VK_MENU) & 0x8000,
					GetKeyState(VK_SHIFT) & 0x8000, (GetKeyState(VK_LWIN) & 0x8000) || (GetKeyState(VK_RWIN) & 0x8000));
				SetKeys(mod | wParam);
			}
		case WM_KEYUP:
		case WM_CHAR:
		case WM_DEADCHAR:
		case WM_SYSKEYUP:
		case WM_SYSCHAR:
		case WM_SYSDEADCHAR:
			return 0;
	}

	return WindowBase::DefWindowProc(Msg, wParam, lParam);
}
