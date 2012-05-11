#include "stdafx.h"
#include "cmdmap.h"

const _sCommand CMD_MAP[] =
{
	{ID_REPLACE_PLAY,				_T("Replace Playlist And Play")},
	{ID_REPLACE,					_T("Replace Playlist")},
	{0, NULL}, //separator
	{ID_ADD_PLAY,					_T("Add to Playlist And Play")},
	{ID_ADD,						_T("Add to Playlist")},
	{0, NULL}, //separator
	{ID_NEW_PLAY,					_T("Create New Playlist And Play")},
	{ID_NEW,						_T("Create New Playlist")},
	{0, NULL}, //separator
	{ID_ADD_FAVO,					_T("Add to Favorites")},
	{ID_SELECT_NOWPLAYING,			_T("Goto Now Playing")},
	{ID_PREFERENCE,					_T("Preferences")},
	{0, NULL}, //separator
	{ID_REVEAL,						_T("Open in Explorer")},
	{ID_DELETE_FILES,				_T("Delete")},
	{ID_RENAME_FILE,				_T("Rename")},
//////////////////////////////////////////////////////////////////
	{ID_MENU_LAST, NULL},
//////////////////////////////////////////////////////////////////
	{ID_MARK_THIS,					_T("Toggle Mark")},
	{ID_MARK_ALL,					_T("Mark All Children")},
	{ID_MARK_ALL_SIBLING,			_T("Mark All Sibling")},
	{ID_MARK_TO_HERE,				_T("Mark to Here")},
	{ID_MARK_CLEAR,					_T("Unmark All Children")},
	{ID_MARK_CLEAR_SIBLING,			_T("Unmark All Sibling")},
	{ID_MARK_CLEAR_ALL,				_T("Unmark All")},
//////////////////////////////////////////////////////////////////
	{ID_MARK_END, NULL},
//////////////////////////////////////////////////////////////////
	{ID_REPLACE_PLAY_REC,			_T("Replace Playlist And Play (recursive)")},
	{ID_REPLACE_REC,				_T("Replace Playlist (recursive)")},
	{ID_ADD_PLAY_REC,				_T("Add to Playlist And Play (recursive)")},
	{ID_ADD_REC,					_T("Add to Playlist (recursive)")},
	{ID_NEW_PLAY_REC,				_T("Create New Playlist And Play (recursive)")},
	{ID_NEW_REC,					_T("Create New Playlist (recursive)")},
	{ID_UPDATE_TEMP_PL_REC,			_T("Show content in 'Explorer View' playlist. (recursive)")},
	{ID_UPDATE_TEMP_PL,				_T("Show content in 'Explorer View' playlist.")},
	{ID_UPDATE_TEMP_PL_PLAY_REC,	_T("Show content in 'Explorer View' playlist and play. (recursive)")},
	{ID_UPDATE_TEMP_PL_PLAY,		_T("Show content in 'Explorer View' playlist and play.")},
	{ID_REPLACE_DEF_PLAY,			_T("Replace Default Playlist And Play")},
	{ID_REPLACE_DEF,				_T("Replace Default Playlist")},
	{ID_ADD_DEF_PLAY,				_T("Add to Default Playlist And Play")},
	{ID_ADD_DEF,					_T("Add to Default Playlist")},
	{ID_REPLACE_DEF_PLAY_REC,		_T("Replace Default Playlist And Play (recursive)")},
	{ID_REPLACE_DEF_REC,			_T("Replace Default Playlist (recursive)")},
	{ID_ADD_DEF_PLAY_REC,			_T("Add to Default Playlist And Play (recursive)")},
	{ID_ADD_DEF_REC,				_T("Add to Default Playlist (recursive)")},
	{ID_TOGGLE_EXPAND,				_T("Toggle expanded/collapsed.")},
	{ID_COLLAPSE_ALL,				_T("Collapse all nodes.")},
	{ID_RESET_TREE,					_T("Reset the panel.")}
};

const int g_iCmdCount = sizeof(CMD_MAP) / sizeof(_sCommand);
int g_iValidCmdCount = -1;

int FindCmd(int id)
{
	int i;
	for (i = 0;i < g_iCmdCount;i++)
		if (CMD_MAP[i].id == id) {
			if (CMD_MAP[i].desc)
				return i;
			else
				return -1;
		}
	return -1;
}

int FindCmd(LPCTSTR desc)
{
	int i;
	for (i = 0;i < g_iCmdCount;i++)
		if (CMD_MAP[i].desc && _tcscmp(CMD_MAP[i].desc, desc) == 0)
			return i;
	return -1;
}

int GetValidCmdCount()
{
	if (g_iValidCmdCount == -1) {
		int i;
		g_iValidCmdCount = 0;
		for (i = 0;i < g_iCmdCount;i++)
			if (CMD_MAP[i].desc)
				g_iValidCmdCount++;
	}
	return g_iValidCmdCount;
}

int cmdcmp(const void *c1, const void *c2)
{
	return _tcsicmp((*(_sCommand **) c1)->desc, (*(_sCommand **) c2)->desc);
}
