#pragma once

#include "ppgbase.h"
#include "exmenu.h"

class PPgDisplay2 :	public PPgBase
{
public:
	PPgDisplay2();
	virtual ~PPgDisplay2();

	virtual HWND Create(HWND hParent);
	virtual LPCTSTR GetName() const
	{
		return _T("Display2");
	}
	virtual void Reset();
private:
	ExMenu m_StyleMenu;
	virtual LRESULT DefWindowProc(UINT msg, WPARAM wp, LPARAM lp);
};
