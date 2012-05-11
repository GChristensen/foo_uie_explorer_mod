#pragma once

#include "others.h"

#define MAX_FAVO	100

class FavoItem
{
public:
	FavoItem()
		: m_sPath(NULL), m_sDesc(NULL)
	{
	}

	FavoItem(const FavoItem &cItem)
		: m_sPath(NULL), m_sDesc(NULL)
	{
		*this = cItem;
	}

	~FavoItem()
	{
		if (m_sDesc)
			free(m_sDesc);
		if (m_sPath)
			free(m_sPath);
	}

	LPCTSTR GetDesc() const
	{
		return SAFESTR(m_sDesc);
	}

	LPCTSTR GetPath() const
	{
		return SAFESTR(m_sPath);
	}

	void SetDesc(LPCTSTR str)
	{
		if (m_sDesc)
			free(m_sDesc);
		m_sDesc = GetPrefString(str);
	}

	void SetPath(LPCTSTR str);

	FavoItem& operator=(const FavoItem &cItem)
	{
		if (this != &cItem) {
			SetDesc(cItem.GetDesc());
			SetPath(cItem.GetPath());
		}

		return *this;
	}

	static FavoItem **GetFavoList(bool bReload = true);
	static void SaveFavoList(bool bFlushOnEnd = false);
	static int GetListCount() { return m_iFavoCount; }
	static void FreeList();
private:
	static FavoItem *m_FavoList[MAX_FAVO];
	static int m_iFavoCount;
	LPTSTR m_sPath;
	LPTSTR m_sDesc;
};

class FavoListCtrl : public ListCtrlBase<FavoItem>
{
	typedef ListCtrlBase<FavoItem> FavoListCtrlBase;
public:
	FavoListCtrl();
	virtual ~FavoListCtrl();

	int InsertItem(FavoItem *cItem, int nItem = -1);
	bool SetItem(const FavoItem &cItem, int nItem);

	inline void OnGetDispInfo(NMLVDISPINFO *lpDisp);
	BOOL OnNotify(WPARAM wParam, LPARAM lParam);

private:
	static int CALLBACK CmpFun(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);

//overrides
public:
	virtual void Init();

	virtual BOOL SortItems(int iCol = -1)
	{
		return ListCtrlBase<FavoItem>::SortItems(iCol, CmpFun);
	}
protected:
	virtual LRESULT DefWindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);
};
