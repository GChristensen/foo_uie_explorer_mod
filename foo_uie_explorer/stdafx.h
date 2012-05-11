#pragma once

#define _WIN32_WINNT	0x0501

#include <shlwapi.h>
#include <winuser.h>
#include <limits.h>
#include <shlobj.h>
#include <atlbase.h>
#include <atlstr.h>
#include <atlcoll.h>
#include <io.h>

#include "../SDK/foobar2000.h"
#include "../helpers/helpers.h"
#include "../columns_ui-sdk/ui_extension.h"

#include "../foo_kulibs/common.h"
#include "../foo_kulibs/columns_ui.h"
#include "../foo_kulibs/cfg_resetable.h"
#include "../foo_kulibs/window.h"

#define EX_MAX_PATH 1024

#define SAFESTR(s) ((s) ? (s) : _T(""))

#define INRANGE(v, m, M) ((v) >= m && v <= (M))

#define APPNAME "foo_uie_explorer"
#define PPGNAME "Explorer Tree Panel"

#define ISDOTS(s) ((s)[0] == '.' && ((s)[1] == 0 || (s)[1] == '.' && (s)[2] == 0))

class LastErrorMsg
{
public:
	LastErrorMsg() : lpErrMsgBuffer(NULL) { FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, GetLastError(), 0, (LPTSTR) &lpErrMsgBuffer, 0, NULL); }
	~LastErrorMsg() { if (lpErrMsgBuffer) LocalFree(lpErrMsgBuffer); }
	operator LPCTSTR() {return (LPCTSTR) lpErrMsgBuffer;}
private:
	LPVOID lpErrMsgBuffer;
};
