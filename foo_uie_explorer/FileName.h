#pragma once

#include "preference.h"

#define CFN_FOLDER		((uint16) 0x0001)
#define CFN_HIDDEN		((uint16) 0x0002)
#define CFN_READONLY	((uint16) 0x0004)
#define CFN_SYSTEM		((uint16) 0x0008)
#define CFN_DRIVE		((uint16) 0x0010)
#define CFN_FAVORITE	((uint16) 0x0020)
#define CFN_NOTFILE		(CFN_FOLDER | CFN_DRIVE | CFN_FAVORITE)
#define CFN_ROOT		(CFN_DRIVE | CFN_FAVORITE)

#define FHRSD(f, h, r, s, d) (((f) ? CFN_FOLDER : 0) | ((h) ? CFN_HIDDEN : 0) | \
		((r) ? CFN_READONLY : 0) | ((s) ? CFN_SYSTEM : 0) | ((d) ? CFN_DRIVE : 0))

typedef unsigned __int8		uint8;
typedef unsigned __int16	uint16;
typedef unsigned __int32	uint32;
typedef unsigned __int64	uint64;

class CFileName;
class DirInsThreadParam;

class CFileName
{
public:
	CFileName()
		: m_attribute(0), filename(NULL), title(NULL), ext(NULL), dispname(NULL), location(NULL), fullpath(NULL),
		m_iLastTitleCase(-1), m_iLastExtCase(-1), m_bMarked(false), m_pInsParam(NULL)
	{
	}

	CFileName(LPCTSTR fn, uint16 attr /* = 0 */)
		: m_attribute(0), filename(NULL), title(NULL), ext(NULL), dispname(NULL), location(NULL), fullpath(NULL),
		m_iLastTitleCase(-1), m_iLastExtCase(-1), m_bMarked(false), m_pInsParam(NULL)
	{
		SetFileName(fn, attr);
	}

	CFileName(CFileName &fn)
		: m_attribute(0), filename(NULL), title(NULL), ext(NULL), dispname(NULL), location(NULL), fullpath(NULL),
		m_iLastTitleCase(-1), m_iLastExtCase(-1), m_bMarked(false), m_pInsParam(NULL)
	{
		if (this != &fn)
			*this = fn;
	}

	virtual ~CFileName()
	{
		clear();
		if (dispname)
			free(dispname);
	}

	LPCTSTR GetType(const __int8 _case = 0);
	LPCTSTR GetTitle(const __int8 _case = 0);

	LPCTSTR GetName() const
	{
		return SAFESTR(filename);
	}

	LPCTSTR GetDispName()
	{
		if (dispname)
			return dispname;
		else {
			if (prefs::bShowFileExtension || IsFolder())
				return SAFESTR(filename);
			else
				return GetTitle();
		}
	}

	LPCTSTR GetLocation() const
	{
		if (IsFavorite())
			return SAFESTR(filename);
		return SAFESTR(location);
	}

	LPCTSTR GetFullPath();

	bool IsSystem() const
	{
		return ((m_attribute & CFN_SYSTEM) != 0);
	}

	bool IsReadOnly() const
	{
		return ((m_attribute & CFN_READONLY) != 0);
	}

	bool IsHidden() const
	{
		return ((m_attribute & CFN_HIDDEN) != 0);
	}

	bool IsFile() const
	{
		return ((m_attribute & CFN_NOTFILE) == 0);
	}

	bool IsFolder() const
	{
		return ((m_attribute & CFN_FOLDER) != 0);
	}

	bool IsDrive() const
	{
		return ((m_attribute & CFN_DRIVE) != 0);
	}

	bool IsFavorite() const
	{
		return ((m_attribute & CFN_FAVORITE) != 0);
	}

	bool IsRoot() const
	{
		return ((m_attribute & CFN_ROOT) != 0);
	}

	bool InTheseTypes(const LPTSTR *types); //linear search; types is a NULL-terminated string array.
	bool InTheseTypes(const LPTSTR *types, uint32 uCount); //binary search.

	void SetFileName(LPCTSTR fn)
	{
		if (!fn)
			return;
		clear();
		filename = _tcsdup(fn);
	}

	void SetFileName(LPCTSTR fn, uint16 attr)
	{
		SetFileName(fn);
		m_attribute = attr;
	}

	void SetDispName(LPCTSTR dn)
	{
		if (!dn)
			return;
		if (dispname)
			free(dispname);
		dispname = _tcsdup(dn);
	}

	void SetLocation(LPCTSTR loc)
	{
		if (location)
			free(location);
		location = _tcsdup(loc);
		if (fullpath) {
			free(fullpath);
			fullpath = NULL;
		}
	}

	operator LPCTSTR() const
	{
		return filename;
	}

	CFileName& operator =(CFileName &src)
	{
		if (this != &src)
			SetFileName(src, src.m_attribute);
		return *this;
	}

	CFileName& operator =(LPCTSTR src)
	{
		SetFileName(src);
		return *this;
	}

	virtual LPCTSTR GetClassName() const
	{
		return _T("CFileName");
	}

	void clear();

	uint16 m_attribute;
	int m_iImage;
	int m_iSelImage;
	bool m_bMarked;
	DirInsThreadParam *m_pInsParam;
private:
	__int8 m_iLastTitleCase;
	__int8 m_iLastExtCase;
	LPTSTR dispname;
	LPTSTR filename;
	LPTSTR title;
	LPTSTR ext;
	LPTSTR location;
	LPTSTR fullpath;
};

LPTSTR strToLower(LPTSTR dest, LPCTSTR src);
LPTSTR strToUpper(LPTSTR dest, LPCTSTR src);

inline int FnCompareFolderFirst(CFileName * const *fn1, CFileName * const *fn2)
{
	ASSERT(fn1);
	ASSERT(fn2);
	int res;
	if (res = ((*fn1)->IsFolder() == (*fn2)->IsFolder()) ? 0 : ((*fn1)->IsFolder() ? -1 : 1))
		return res;
	return _tcsicmp((*fn1)->GetName(), (*fn2)->GetName());
}

inline int FnCompare(const void *a, const void *b)
{
	if (!a || !b)
		return 0;
	return _tcsicmp((*((CFileName**)a))->GetName(), (*((CFileName**)b))->GetName());
}

inline int void_tcsicmp(const void *v1, const void *v2)
{
	ASSERT(v1);
	ASSERT(v2);
	return _tcsicmp(*((const TCHAR **) v1), *((const TCHAR **) v2));
}

inline bool IsValidExt(LPCTSTR ext)
{
	for (;*ext;ext++)
		if (_tcschr(_T("\\/*?:\"<>|."), *ext))
			return false;
	return true;
}
