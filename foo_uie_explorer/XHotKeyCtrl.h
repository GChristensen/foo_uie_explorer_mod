#pragma once

#include "actlistctrl.h"

class XHotKeyCtrl :	public WindowBase
{
public:
	XHotKeyCtrl() : m_iKeys(0) {}
	virtual ~XHotKeyCtrl() {}

	int GetKeys() const
	{
		return m_iKeys;
	}

	bool SetKeys(const int iKeys)
	{
		m_iKeys = iKeys;
		TCHAR sBuffer[50];
		LPCTSTR sKeyDesc;

		if (sKeyDesc = ActItem::SetKeyString(sBuffer, m_iKeys))
			SetWindowText(m_hWnd, sKeyDesc);
		else {
			SetWindowText(m_hWnd, _T(""));
			m_iKeys = 0;
			return false;
		}
		return true;
	}

protected:
	int m_iKeys;

	virtual LRESULT DefWindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);
};
