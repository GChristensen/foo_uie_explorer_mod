#pragma once

#include "ppgbase.h"
#include "cmdlistctrl.h"

class PPgCommand : public PPgBase
{
public:
	PPgCommand();
	virtual ~PPgCommand();

	virtual HWND Create(HWND hParent);
	virtual LPCTSTR GetName() const
	{
		return _T("Commands");
	}
	virtual void Reset();

	CmdListCtrl m_CmdList;
private:
	virtual LRESULT DefWindowProc(UINT msg, WPARAM wp, LPARAM lp);
};
