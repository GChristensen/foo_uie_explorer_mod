#pragma once

#include "cmdmap.h"

#define CI_MK_MASK		0xFF000000L
#define	CI_MK_CTRL		0x01000000L
#define CI_MK_ALT		0x02000000L
#define CI_MK_SHIFT		0x04000000L
#define CI_MK_WIN		0x08000000L

#define CI_MC_MASK		0x00FF0000L
#define CI_MC_LCLICK	0x00010000L
#define CI_MC_DBLCLICK	0x00020000L
#define CI_MC_MCLICK	0x00030000L

#define CI_KB_MASK		0x0000FFFFL

#define MAKECIMOD(c, a, s, w)		(((c) ? CI_MK_CTRL : 0) | ((a) ? CI_MK_ALT : 0) | ((s) ? CI_MK_SHIFT : 0) | ((w) ? CI_MK_WIN : 0))
#define MAKECIKEY(mod, mou, key)	((((DWORD) (mod)) << 24) | ((((DWORD) (mou)) << 16) & CI_MC_MASK) | (((DWORD) (key)) & CI_KB_MASK))

class ActItem : public __sCommand
{
public:
	ActItem();

	ActItem(int key, int _id)
	{
		SetKeys(key);
		SetCommand(_id);
	}

	ActItem(const _sKeyData &keydata)
	{
		*this = keydata;
	}
	~ActItem() {}

	bool IsMouse() const
	{
		return ((m_iTriggerKey & CI_MC_MASK) != 0);
	}

	bool IsKeyboard() const
	{
		return ((m_iTriggerKey & CI_KB_MASK) != 0);
	}

	bool IsValid() const
	{
		return (FindCmd(id) != -1);
	}

	_sKeyData GetKeyData() const
	{
		_sKeyData keydata = {id, m_iTriggerKey};
		return keydata;
	}

	LPCTSTR GetKeyString()
	{
		return m_sKeyDesc + 3;
	}

	int GetKeys() const
	{
		return m_iTriggerKey;
	}

	void SetKeys(int key)
	{
		m_iTriggerKey = key;
		SetKeyString();
	}

	bool SetCommand(int _id);

	ActItem& operator =(const ActItem &cmdItem)
	{
		if (this != &cmdItem) {
			id = cmdItem.id;
			SetKeys(cmdItem.m_iTriggerKey);
			desc = cmdItem.desc;
		}
		return *this;
	}

	ActItem& operator =(const _sKeyData &keydata)
	{
		SetKeys(keydata.key);
		SetCommand(keydata.id);

		return *this;
	}

	static LPCTSTR SetKeyString(LPTSTR outStr, int keys);
private:
	void SetKeyString()
	{
		SetKeyString(m_sKeyDesc, m_iTriggerKey);
	}

	int m_iTriggerKey;
	TCHAR m_sKeyDesc[50];
};

class ActListCtrl : public ListCtrlBase<ActItem>
{
	typedef ListCtrlBase<ActItem> ActListCtrlBase;
public:
	ActListCtrl();
	virtual ~ActListCtrl();

	int InsertItem(ActItem *cItem, int nItem = -1);
	bool SetItem(const ActItem &cItem, int nItem);

	inline void OnGetDispInfo(NMLVDISPINFO *lpDisp);
	BOOL OnNotify(WPARAM wParam, LPARAM lParam);

private:
	static int CALLBACK CmpFun(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);

//overrides
public:
	virtual void Init();

	virtual BOOL SortItems(int iCol = -1)
	{
		return ListCtrlBase<ActItem>::SortItems(iCol, CmpFun);
	}
protected:
	virtual LRESULT DefWindowProc(UINT Msg, WPARAM wParam, LPARAM lParam);
};
