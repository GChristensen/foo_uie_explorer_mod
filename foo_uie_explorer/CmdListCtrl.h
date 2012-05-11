#pragma once

#include "cmdmap.h"
#include "filename.h"
#include "others.h"

#define MAX_CMD			100

#define CMD_WS_NORMAL	0
#define CMD_WS_MAX		1
#define CMD_WS_MIN		2

class CmdItem
{
public:
	CmdItem()
		: m_sDesc(NULL), m_sParam(NULL), m_sApp(NULL), m_sWorkPath(NULL), m_iWindowSize(0)
	{
	}

	CmdItem(const CmdItem &cItem)
		: m_sDesc(NULL), m_sParam(NULL), m_sApp(NULL), m_sWorkPath(NULL), m_iWindowSize(0)
	{
		*this = cItem;
	}

	~CmdItem()
	{
		if (m_sDesc)
			free(m_sDesc);
		if (m_sParam)
			free(m_sParam);
		if (m_sApp)
			free(m_sApp);
		if (m_sWorkPath)
			free(m_sWorkPath);
	}

	void SetDesc(LPCTSTR str)
	{
		if (m_sDesc)
			free(m_sDesc);
		m_sDesc = GetPrefString(str);
	}

	void SetApp(LPCTSTR str)
	{
		if (m_sApp)
			free(m_sApp);
		m_sApp = GetPrefString(str);
	}

	void SetParam(LPCTSTR str)
	{
		if (m_sParam)
			free(m_sParam);
		m_sParam = GetPrefString(str);
	}

	void SetWorkPath(LPCTSTR str)
	{
		if (m_sWorkPath)
			free(m_sWorkPath);
		m_sWorkPath = GetPrefString(str);
	}

	void SetWindowSize(int ws)
	{
		if (!INRANGE(ws, CMD_WS_NORMAL, CMD_WS_MIN))
			ws = 0;
		m_iWindowSize = ws;
	}

	LPCTSTR GetDesc() const
	{
		return SAFESTR(m_sDesc);
	}

	LPCTSTR GetApp() const
	{
		return SAFESTR(m_sApp);
	}

	LPCTSTR GetParam() const
	{
		return SAFESTR(m_sParam);
	}

	LPCTSTR GetWorkPath() const
	{
		return SAFESTR(m_sWorkPath);
	}

	int GetWindowSize() const
	{
		return m_iWindowSize;
	}

	LPCTSTR GetWindowSizeStr() const
	{
		switch (m_iWindowSize) {
			case CMD_WS_NORMAL:
				return _T("Normal");
			case CMD_WS_MAX:
				return _T("Maximized");
			case CMD_WS_MIN:
				return _T("Minimized");
		}

		return _T(""); //should never happen
	}

	CmdItem& operator=(const CmdItem &cItem)
	{
		if (this != &cItem) {
			SetDesc(cItem.GetDesc());
			SetApp(cItem.GetApp());
			SetParam(cItem.GetParam());
			SetWorkPath(cItem.GetWorkPath());
			SetWindowSize(cItem.GetWindowSize());
		}

		return *this;
	}

	bool PerformCommand(CFileName &fnItem);

	static CmdItem **GetCmdList(bool bReload = true);
	static int GetListCount() { return m_iCmdCount; }
	static void FreeList();
private:
	void ReplaceVars(LPCTSTR in, LPTSTR out, UINT cch, CFileName &fnItem, bool bQuote = false);
	static CmdItem *m_CmdList[MAX_CMD];
	static int m_iCmdCount;
	int m_iWindowSize;
	LPTSTR m_sDesc;
	LPTSTR m_sApp;
	LPTSTR m_sParam;
	LPTSTR m_sWorkPath;
};

class CmdListCtrl : public ListCtrlBase<CmdItem>
{
	typedef ListCtrlBase<CmdItem> CmdListCtrlBase;
public:
	CmdListCtrl();
	virtual ~CmdListCtrl();

	int InsertItem(CmdItem *cItem, int nItem = -1);
	bool SetItem(const CmdItem &cItem, int nItem);

	inline void OnGetDispInfo(NMLVDISPINFO *lpDisp);
	BOOL OnNotify(WPARAM wParam, LPARAM lParam);

private:
	static int CALLBACK CmpFun(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);

//overrides
public:
	virtual void Init();

	virtual BOOL SortItems(int iCol = -1)
	{
		return ListCtrlBase<CmdItem>::SortItems(iCol, CmpFun);
	}
protected:
	virtual LRESULT DefWindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);
};
