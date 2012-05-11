#include "stdafx.h"
#include "ppgactions.h"
#include "resource.h"
#include "preference.h"

PPgActions::PPgActions()
{
}

PPgActions::~PPgActions()
{
}

HWND PPgActions::Create(HWND hParent)
{
	return DialogBase::Create(hParent, IDD_PPG_ACTION);
}

LRESULT PPgActions::DefWindowProc(UINT msg, WPARAM wp, LPARAM lp)
{	
	switch (msg) {
		case WM_INITDIALOG:
			{
				m_ActList.m_hWnd = GetDlgItem(m_hWnd, IDC_SC_LIST);
				m_ActList.InitBinding(m_hWnd);
				m_ActList.Init();

				m_HotKey.m_hWnd = GetDlgItem(m_hWnd, IDC_EDT_HOTKEY);
				m_HotKey.InitBinding(m_hWnd);

				{
					int i, n = GetValidCmdCount();
					HWND hCBox = GetDlgItem(m_hWnd, IDC_SC_SELECT);
					_sCommand **cmdArray = new _sCommand * [n];
					
					n = 0;
					for (i = 0;i < g_iCmdCount;i++)
						if (CMD_MAP[i].desc) {
							cmdArray[n] = (_sCommand *) &CMD_MAP[i];
							n++;
						}

					qsort(cmdArray, n, sizeof(_sCommand *), cmdcmp);

					for (i = 0;i < n;i++) {
						SendMessage(hCBox, CB_INSERTSTRING, i, (LPARAM) cmdArray[i]->desc);
						SendMessage(hCBox, CB_SETITEMDATA, i, (LPARAM) cmdArray[i]);
					}

					delete [] cmdArray;

					SendMessage(hCBox, CB_SETCURSEL, 0, 0);
				}

				CheckRadioButton(m_hWnd, IDC_LCLICK, IDC_MCLICK, IDC_LDBLCLICK);
			}
			break;
		case WM_NOTIFY:
			if (((LPNMHDR) lp)->idFrom == IDC_SC_LIST && m_ActList.OnNotify(wp, lp))
				return TRUE;
			break;
		case WM_COMMAND:
			switch (HIWORD(wp)) {
				case 0:
					switch (LOWORD(wp)) {
						case IDC_MOUSE_ADD:
						case IDC_MOUSE_REPLACE:
							{
								int mEvent = GetCheckedRadioButton(IDC_LCLICK, IDC_MCLICK);
								if (mEvent) {
									mEvent -= IDC_LCLICK;
									mEvent++;
									int mod = MAKECIMOD(
										(IsDlgButtonChecked(m_hWnd, IDC_CHK_CTRLKEY) == BST_CHECKED),
										(IsDlgButtonChecked(m_hWnd, IDC_CHK_ALTKEY) == BST_CHECKED),
										(IsDlgButtonChecked(m_hWnd, IDC_CHK_SHIFTKEY) == BST_CHECKED),
										(IsDlgButtonChecked(m_hWnd, IDC_CHK_WINKEY) == BST_CHECKED) );
									if (LOWORD(wp) == IDC_MOUSE_ADD) {
										ActItem *cItem = new ActItem(MAKECIKEY(0, mEvent, 0) | mod, GetSelectedCommand());
										m_ActList.InsertItem(cItem);
									}
									else {
										int sel = m_ActList.GetFirstSelItem();
										if (sel != -1) {
											ActItem cItem(MAKECIKEY(0, mEvent, 0) | mod, GetSelectedCommand());
											m_ActList.SetItem(cItem, sel);
										}
									}
								}
							}
							break;
						case IDC_KEY_ADD:
						case IDC_KEY_REPLACE:
							{
								int keys = m_HotKey.GetKeys();
								if (keys == 0)
									break;

								if (LOWORD(wp) == IDC_KEY_ADD) {
									ActItem *cItem = new ActItem(keys, GetSelectedCommand());
									m_ActList.InsertItem(cItem);
								}
								else {
									int sel = m_ActList.GetFirstSelItem();
									if (sel != -1) {
										ActItem cItem(keys, GetSelectedCommand());
										m_ActList.SetItem(cItem, sel);
									}
								}
							}
							break;
						case IDC_BTN_DELETE:
							m_ActList.DeleteSelItem();
							break;
					}
			}
			break;
		case WMU_SETCTRLS:
			SetControls((int) wp, (int) lp);
			break;
	}

	return DialogBase::DefWindowProc(msg, wp, lp);
}

int PPgActions::GetCheckedRadioButton(int idFirst, int idLast)
{
	for (;idFirst <= idLast;idFirst++)
		if (IsDlgButtonChecked(m_hWnd, idFirst) == BST_CHECKED)
			return idFirst;
	return 0;
}

int PPgActions::SetSelectedCommand(int cmd)
{
	HWND hCBox = GetDlgItem(m_hWnd, IDC_SC_SELECT);
	int i, count = GetValidCmdCount();
	_sCommand *data;
	for (i = 0;i < count;i++) {
		data = (_sCommand *) SendMessage(hCBox, CB_GETITEMDATA, i, 0);
		if (data->id == cmd) {
			SendMessage(hCBox, CB_SETCURSEL, i, 0);
			return i;
		}
	}
	return -1;
}

int PPgActions::GetSelectedCommand()
{
	HWND hCBox = GetDlgItem(m_hWnd, IDC_SC_SELECT);
	int sel = (int) SendMessage(hCBox, CB_GETCURSEL, 0, 0);
	_sCommand *data = (_sCommand *) SendMessage(hCBox, CB_GETITEMDATA, sel, 0);
	return data->id;
}

void PPgActions::SetControls(int keys, int cmd/* = -1*/)
{
	int temp;

	if (keys & CI_MC_MASK) {
		CheckThisDlgButton(IDC_CHK_CTRLKEY, keys & CI_MK_CTRL);
		CheckThisDlgButton(IDC_CHK_ALTKEY, keys & CI_MK_ALT);
		CheckThisDlgButton(IDC_CHK_SHIFTKEY, keys & CI_MK_SHIFT);
		CheckThisDlgButton(IDC_CHK_WINKEY, keys & CI_MK_WIN);
		temp = ((keys & CI_MC_MASK) >> 16) + IDC_LCLICK - 1;
		if (INRANGE(temp, IDC_LCLICK, IDC_MCLICK))
			CheckRadioButton(m_hWnd, IDC_LCLICK, IDC_MCLICK, temp);
	}
	else
		m_HotKey.SetKeys(keys);

	if (cmd != -1)
		SetSelectedCommand(cmd);
}

void PPgActions::Reset()
{
	prefs::aShortcuts.SetArray(NULL, 0);
}
