#pragma once

#define CheckThisDlgButton(id, chk) CheckDlgButton(m_hWnd, (id), (chk) ? BST_CHECKED : BST_UNCHECKED)

inline BOOL EnableDlgItem(HWND hDlg, UINT uId, BOOL bEnable)
{
	HWND hDlgItem = GetDlgItem(hDlg, uId);
	ASSERT(hDlgItem);
	return EnableWindow(hDlgItem, bEnable);
}

#define EnableThisDlgItem(id) EnableDlgItem(m_hWnd, (id), TRUE)
#define DisableThisDlgItem(id) EnableDlgItem(m_hWnd, (id), FALSE)

class PPgBase : public DialogBase
{
public:
	PPgBase() {}
	virtual ~PPgBase() {}
	virtual HWND Create(HWND hParent) = 0;
	virtual LPCTSTR GetName() const = 0;
	virtual void Reset() = 0;
};
