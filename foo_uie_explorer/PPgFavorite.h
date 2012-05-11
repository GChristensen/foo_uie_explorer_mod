#pragma once

#include "ppgbase.h"
#include "favolistctrl.h"

class PPgFavorite : public PPgBase
{
public:
	PPgFavorite();
	virtual ~PPgFavorite();

	virtual HWND Create(HWND hParent);
	virtual LPCTSTR GetName() const
	{
		return _T("Favorites");
	}
	virtual void Reset();

	FavoListCtrl m_FavoList;
private:
	virtual LRESULT DefWindowProc(UINT msg, WPARAM wp, LPARAM lp);
};
