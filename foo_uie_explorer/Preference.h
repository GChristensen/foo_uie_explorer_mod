#pragma once

template <class T> class cfg_array;

#include "cmdmap.h"

namespace prefs
{
	extern cfg_int_resetable iStartup;
	extern cfg_string_resetable sStartupPath;
	extern cfg_string_resetable sLastPath;
	extern cfg_bool_resetable bShowLines;
	extern cfg_bool_resetable bShowIcons;
	extern cfg_bool_resetable bShowAddBar;
	extern cfg_bool_resetable bShowHScroll;
	extern cfg_bool_resetable bShowTooltip;
	extern cfg_bool_resetable bForceSort;
	extern cfg_int_resetable iShowTypeMode;
	extern cfg_string_resetable sShowType;
	extern cfg_bool_resetable bShowHiddenFiles;
	extern cfg_bool_resetable bShowFileExtension;
	extern cfg_int_resetable iTreeFrame;
	extern cfg_int_resetable iAdBarFrame;
	extern cfg_int_resetable iNodeHeight;
	extern cfg_int_resetable iMinHeight;
	extern cfg_string_resetable sHideDrive;
	extern cfg_array<_sKeyData> aShortcuts;
	extern cfg_string_resetable sCmdList;
	extern cfg_bool_resetable bGrpCmd;
	extern cfg_string_resetable sFavoList;
	extern cfg_int_resetable iShowFavoAsRoot;
	extern cfg_bool_resetable bGrpFavo;
	extern cfg_string_resetable sFilterType;
	extern cfg_bool_resetable bRecursive;
	extern cfg_bool_resetable bExpand;
	extern cfg_bool_resetable bSendDef;
	extern cfg_string_resetable sDefPL;
}

template <class T>
class cfg_array : public cfg_var
{
public:
	explicit cfg_array(const GUID & p_guid) : cfg_var(p_guid), m_pBuffer(NULL), m_iCount(0) {}
	virtual ~cfg_array() {if (m_pBuffer) free(m_pBuffer);}
	bool SetArray(const T *pArray, int iCount)
	{
		if (pArray == m_pBuffer)
			return false;

		m_iCount = iCount;
		if (m_pBuffer)
			free(m_pBuffer);
		if (m_iCount > 0) {
			m_pBuffer = (T *) malloc(sizeof(T) * m_iCount);
			memcpy(m_pBuffer, pArray, m_iCount * sizeof(T));
		}
		else
			m_pBuffer = NULL;

		return true;
	}
	bool Attach(T *pArray, int iCount) //intended to attach the arrays allocated by 'malloc' only.
	{
		if (pArray == m_pBuffer)
			return false;

		m_iCount = iCount;
		if (m_pBuffer)
			free(m_pBuffer);
		if (m_iCount > 0)
			m_pBuffer = pArray;
		else
			m_pBuffer = NULL;

		return true;
	}
	T* Detach()
	{
		T *pOldBuffer = m_pBuffer;
		m_iCount = 0;
		m_pBuffer = NULL;
		return pOldBuffer;
	}
	operator T*() { return m_pBuffer; }
	int GetCount() { return m_iCount; }
protected:
	virtual void get_data_raw(stream_writer * p_stream,abort_callback & p_abort)
	{
		if (m_iCount > 0 && m_pBuffer)
			p_stream->write(m_pBuffer, m_iCount * sizeof(T), p_abort);
	}
	virtual void set_data_raw(stream_reader * p_stream,t_size p_sizehint,abort_callback & p_abort)
	{
		m_iCount = p_sizehint / sizeof(T);
		if (m_pBuffer)
			free(m_pBuffer);
		if (m_iCount > 0) {
			m_pBuffer = (T *) malloc(sizeof(T) * m_iCount);
			p_stream->read(m_pBuffer, m_iCount * sizeof(T), p_abort);
		}
		else
			m_pBuffer = NULL;
	}

	T *m_pBuffer;
	int m_iCount;
};

//COLORREF to string
class color_string
{
public:
	explicit color_string(const int clr)
	{
		sprintf(buffer, "%02X%02X%02X", GetRValue(clr), GetGValue(clr), GetBValue(clr));
	}
	operator const char *()
	{
		return buffer;
	}
private:
	char buffer[7];
};

//string to COLORREF
int string_color(const char *clr);

class int_string
{
public:
	int_string(const int i, int radix = 10)
	{
		_itot(i, buffer, radix);
	}
	operator LPCTSTR()
	{
		return buffer;
	}
private:
	TCHAR buffer[12];
};
