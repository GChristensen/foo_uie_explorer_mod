#pragma once

#include "ppgbase.h"
#include "exmenu.h"

class PPgDisplay : public PPgBase
{
public:
	PPgDisplay();
	virtual ~PPgDisplay();

	virtual HWND Create(HWND hParent);
	virtual LPCTSTR GetName() const
	{
		return _T("Display");
	}
	virtual void Reset();
private:
	virtual LRESULT DefWindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);
};
