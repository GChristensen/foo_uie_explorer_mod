#pragma once

#include "actlistctrl.h"
#include "xhotkeyctrl.h"
#include "ppgbase.h"

class PPgActions : public PPgBase
{
public:
	PPgActions();
	virtual ~PPgActions();

	virtual HWND Create(HWND hParent);
	virtual LPCTSTR GetName() const
	{
		return _T("Actions");
	}
	virtual void Reset();

	int GetCheckedRadioButton(int idFirst, int idLast);
	int SetSelectedCommand(int cmd);
	int GetSelectedCommand();
	void SetControls(int keys, int cmd = -1);

	ActListCtrl m_ActList;
	XHotKeyCtrl m_HotKey;
private:
	virtual LRESULT DefWindowProc(UINT msg, WPARAM wp, LPARAM lp);
};
