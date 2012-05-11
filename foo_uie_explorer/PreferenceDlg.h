#pragma once

#include "PPgDisplay.h"
#include "PPgDisplay2.h"
#include "PPgActions.h"
#include "PPgCommand.h"
#include "PPgFavorite.h"
#include "PPgOther.h"

class PreferenceDlg : public preferences_page, public DialogBase
{
public:
	PreferenceDlg();
	virtual ~PreferenceDlg();

	static PPgDisplay m_ppgDisplay;
	static PPgDisplay2 m_ppgDisplay2;
	static PPgActions m_ppgActions;
	static PPgCommand m_ppgCommand;
	static PPgFavorite m_ppgFavorite;
	static PPgOther m_ppgOther;

	void InsertTab(PPgBase *PPgDlg);

//overrides
	HWND create(HWND hParent);
	const char * get_name() {return PPGNAME;}
	GUID get_guid() { return m_GUID; }
	GUID get_parent_guid() { return g_guid_columns_ui_extensions_branch; }
	bool reset_query() { return true; }
	void reset() { m_PPgDlgs[m_iCurWnd]->Reset(); }

	static const GUID m_GUID;
private:
	virtual LRESULT DefWindowProc(UINT msg, WPARAM wp, LPARAM lp);
	static PPgBase *m_PPgDlgs[];
	static int m_iCurWnd;
};
