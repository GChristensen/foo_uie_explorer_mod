#include "stdafx.h"
#include "foo_uie_explorer.h"
#include "CmdListCtrl.h"
#include "FavoListCtrl.h"
#include "others.h"
#include "Preference.h"
#include "CuiAppearance.h"

DECLARE_COMPONENT_VERSION(
	"Explorer Tree",
	"2.4.8",
	"A directive tree view like Windows Explorer.\n\n"
	"Author: Kai-Chieh Ku <kjackie@gmail.com>\n"
	__DATE__ " " __TIME__
)

const LPCTSTR ExplorerTree::m_sClassName = _T("{C566583D-080F-48ce-BA4D-CABFAAB06A40}");

// {C566583D-080F-48ce-BA4D-CABFAAB06A40}
const GUID ExplorerTree::m_GuidExtension = 
{ 0xc566583d, 0x80f, 0x48ce, { 0xba, 0x4d, 0xca, 0xbf, 0xaa, 0xb0, 0x6a, 0x40 } };


EXTERN_C BOOL APIENTRY DllMain(HANDLE hModule, DWORD dwReason, LPVOID lpReserved)
{
	switch (dwReason) {
		case DLL_PROCESS_ATTACH:
			_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF); // detect memory leaks
			break;
		case DLL_PROCESS_DETACH:
			break;
		case DLL_THREAD_ATTACH:
			break;
		case DLL_THREAD_DETACH:
			break;
	}

    return TRUE;
}

class CInitQuit : public initquit
{
	virtual void on_init()
	{
		try {
			static_api_ptr_t<columns_ui::control> ctrl;
		} catch (exception_service_not_found e) {
			MessageBox(core_api::get_main_window(),
				_T("You don't have (proper version of) Columns UI installed.\n")
				_T("foo_uie_explorer can only work with Columns UI.\n")
				_T("You have to activate Columns UI after you install it, too."),
				_T("foo_uie_explorer"), MB_ICONERROR);
			return;
		} catch (exception_service_duplicated e) {}
		globals::hSymbolFont = CreatePointFont(120, _T("Marlett"));
	}
	virtual void on_quit()
	{
		if (globals::hSymbolFont)
			DeleteObject((HGDIOBJ) globals::hSymbolFont);
		CmdItem::FreeList();
		FavoItem::FreeList();
		DirTreeCtrl::ClearMask();
		DirTreeCtrl::ClearFilter();
	}
};

static initquit_factory_t<CInitQuit> g_CInitQuit_factory;

LRESULT ExplorerTree::on_message(HWND wnd,UINT msg,WPARAM wp,LPARAM lp)
{
	switch (msg) {
		case WM_CREATE:
			m_hWnd = wnd;
			m_DirTree.Create(m_hWnd);
			break;
		case WM_GETMINMAXINFO:
			{
				LPMINMAXINFO pMmInfo = (LPMINMAXINFO) lp;
				pMmInfo->ptMinTrackSize.y = prefs::iMinHeight;
			}
			return 0;
		case WM_SIZE:
			m_DirTree.OnSize(LOWORD(lp), HIWORD(lp));
			break;
		case WM_NOTIFYFORMAT:
#ifdef UNICODE
			return NFR_UNICODE;
#else
			return NFR_ANSI;
#endif
		case WM_NOTIFY:
			if (((LPNMHDR) lp)->idFrom == IDC_EXPLORER && m_DirTree.OnNotify((LPNMHDR) lp))
				return TRUE;
			break;
		case WM_COMMAND:
			if (wp == IDC_BTN_FAVO)
				SendMessage(m_DirTree.m_hEditWnd, WM_COMMAND, IDC_BTN_FAVO, 0);
			break;
		case WM_CTLCOLOREDIT:
			if (m_DirTree.m_hEditWnd == (HWND) lp) {
				HDC hDC = (HDC) wp;

				columns_ui::colours::helper addrBarColors(CAddrBarColors::class_guid);

				SetTextColor(hDC, addrBarColors.get_colour(columns_ui::colours::colour_text));
				SetBkColor(hDC, addrBarColors.get_colour(columns_ui::colours::colour_background));

				return (LRESULT) m_DirTree.m_hEditBkBrush;
			}
			break;
		case WM_DESTROY:
			m_hWnd = NULL;
			break;
	}

	return ::DefWindowProc(wnd, msg, wp, lp);
}

void ExplorerTree::UpdateHeight()
{
	if (m_hWnd)
		get_host()->on_size_limit_change(m_hWnd, uie::size_limit_minimum_height);
}

uie::window_factory_single<ExplorerTree> g_instance;

static service_factory_single_t<CTreeColors> CTreeColors_factory;
static service_factory_single_t<CTreeFonts> CTreeFonts_factory;
static service_factory_single_t<CAddrBarColors> CAddrBarColors_factory;
static service_factory_single_t<CAddrBarFonts> CAddrBarFonts_factory;
