#pragma once

#include "ppgbase.h"

class PPgOther : public PPgBase
{
public:
	PPgOther();
	virtual ~PPgOther();

	virtual HWND Create(HWND hParent);
	virtual LPCTSTR GetName() const
	{
		return _T("Other");
	}
	virtual void Reset();
private:
	virtual LRESULT DefWindowProc(UINT msg, WPARAM wp, LPARAM lp);
};
