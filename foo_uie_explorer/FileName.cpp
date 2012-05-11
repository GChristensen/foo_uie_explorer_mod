#include "stdafx.h"
#include "filename.h"

void CFileName::clear()
{
	if (filename) {
		free(filename);
		filename = NULL;
	}
	if (title) {
		free(title);
		title = NULL;
	}
	if (ext) {
		free(ext);
		ext = NULL;
	}
	if (location)
		free(location);
	if (fullpath)
		free(fullpath);
}

LPCTSTR CFileName::GetTitle(const __int8 _case /* = 0*/)
{
	if (!filename)
		return _T("");

	LPTSTR ptr = _tcsrchr(filename, _T('.'));

	if ((m_attribute & CFN_NOTFILE) || !ptr)
		return filename;

	if (title) {
		if (m_iLastTitleCase == _case)
			return title;
		free(title);
	}

	title = (LPTSTR) malloc(sizeof(TCHAR) * (_tcslen(filename) + 1));

	switch (_case) {
		case 1:
			strToUpper(title, filename);
			break;
		case -1:
			strToLower(title, filename);
			break;
		default:
			_tcscpy(title, filename);
			break;
	}
	ptr = _tcsrchr(title, _T('.'));
	*ptr = _T('\0');
	m_iLastTitleCase = _case;
	return title;
}

LPCTSTR CFileName::GetType(const __int8 _case /* = 0*/)
{
	if (!filename || (m_attribute & CFN_NOTFILE))
		return _T("");

	if (ext && _case == m_iLastExtCase)
		return ext;

	LPTSTR ptr = filename;
	ptr = _tcsrchr(ptr, _T('.'));
	if (ptr) {
		ptr++;
		if (ext)
			free(ext);
		ext = (LPTSTR) malloc(sizeof(TCHAR) * (_tcslen(ptr) + 1));
		switch (_case) {
			case 1:
				strToUpper(ext, ptr);
				break;
			case -1:
				strToLower(ext, ptr);
				break;
			default:
				_tcscpy(ext, ptr);
				break;
		}
		m_iLastExtCase = _case;
		return ext;
	}
	return _T("");
}

LPCTSTR CFileName::GetFullPath()
{
	if (IsFavorite())
		return SAFESTR(filename);
	if (!location)
		return NULL;
	if (fullpath)
		return fullpath;
	if (!IsFile())
		return location;

	fullpath = (LPTSTR) malloc((_tcslen(location) + _tcslen(filename) + 2) * sizeof(TCHAR));
	_stprintf(fullpath, _T("%s%s"), location, filename);

	return fullpath;
}

bool CFileName::InTheseTypes(const LPTSTR *types)
{
	if (!types || !*types || **types == '\0')
		return false;

	for (;*types;types++)
		if (**types != '\0' && _tcsicmp(GetType(), *types) == 0)
			return true;

	return false;
}

bool CFileName::InTheseTypes(const LPTSTR *types, uint32 uCount)
{
	if (uCount == 0 || !types || !*types || **types == '\0')
		return false;

	LPCTSTR ptr = GetType();

	return !!bsearch(&ptr, types, uCount, sizeof(LPTSTR), void_tcsicmp);
}

LPTSTR strToLower(LPTSTR dest, LPCTSTR src)
{
	LPTSTR start = dest;

	while (*dest = *src) {
		if (_istupper(*dest))
			*dest = _tolower(*dest);
		dest++;
		src++;
	}

	return start;
}

LPTSTR strToUpper(LPTSTR dest, LPCTSTR src)
{
	LPTSTR start = dest;

	while (*dest = *src) {
		if (_istlower(*dest))
			*dest = _toupper(*dest);
		dest++;
		src++;
	}

	return start;
}
