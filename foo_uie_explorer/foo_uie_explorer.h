#pragma once

#include "DirTreeCtrl.h"

class ExplorerTree : public uie::container_ui_extension
{
public:
	ExplorerTree() : m_hWnd(0)
	{
	}

	virtual ~ExplorerTree()
	{
	}

	operator HWND() const
	{
		return m_hWnd;
	}

	void UpdateHeight();

	HWND m_hWnd;
	DirTreeCtrl m_DirTree;
private:
	static const GUID m_GuidExtension;
	static const LPCTSTR m_sClassName;

//overrides
public:
	virtual class_data & get_class_data() const 
	{
		__implement_get_class_data(m_sClassName, false);
	}
	virtual const GUID & get_extension_guid() const
	{
		return m_GuidExtension;
	}

	virtual void get_name(pfc::string_base & out) const
	{
		out.set_string("Explorer Tree");
	}

	virtual void get_category(pfc::string_base & out) const
	{
		out.set_string("Panels");
	}
	virtual unsigned get_type() const
	{
		return uie::type_panel;
	}
private:
	virtual LRESULT on_message(HWND wnd, UINT msg, WPARAM wp, LPARAM lp);
};

extern uie::window_factory_single<ExplorerTree> g_instance;
