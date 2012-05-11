#pragma once

#include "DirTreeCtrl.h"

class CTreeColors : public columns_ui::colours::client
{
public:
	CTreeColors() {}
	virtual ~CTreeColors() {}

	virtual const GUID & get_client_guid() const { return class_guid; }
	virtual void get_name (pfc::string_base & p_out) const { p_out = "Explorer: Directory Tree"; }

	virtual t_size get_supported_colours() const
	{ return columns_ui::colours::colour_flag_text | columns_ui::colours::colour_flag_background; }
	virtual t_size get_supported_bools() const { return 0; }
	virtual bool get_themes_supported() const { return false; }
	virtual void on_colour_changed(t_size mask) const { PERFORM_GLOBAL( UpdateColors() ); }
	virtual void on_bool_changed(t_size mask) const {}

	const static GUID class_guid;
};
// {ACBF0E7F-FD01-48f1-ACDE-9708E0A177E7}
DECLARE_CLASS_GUID(CTreeColors,
0xacbf0e7f, 0xfd01, 0x48f1, 0xac, 0xde, 0x97, 0x8, 0xe0, 0xa1, 0x77, 0xe7);

class CTreeFonts : public columns_ui::fonts::client
{
public:
	CTreeFonts() {}
	virtual ~CTreeFonts() {}

	virtual const GUID & get_client_guid() const { return class_guid; }
	virtual void get_name (pfc::string_base & p_out) const { p_out = "Explorer: Directory Tree"; }

	virtual columns_ui::fonts::font_type_t get_default_font_type() const { return columns_ui::fonts::font_type_items; }
	virtual void on_font_changed() const { PERFORM_GLOBAL( UpdateFonts() ); }

	const static GUID class_guid;
};
// {BB7BADB2-976D-4191-A1B0-08389258E61B}
DECLARE_CLASS_GUID(CTreeFonts, 
0xbb7badb2, 0x976d, 0x4191, 0xa1, 0xb0, 0x8, 0x38, 0x92, 0x58, 0xe6, 0x1b);

class CAddrBarColors : public columns_ui::colours::client
{
public:
	CAddrBarColors() {}
	virtual ~CAddrBarColors() {}

	virtual const GUID & get_client_guid() const { return class_guid; }
	virtual void get_name (pfc::string_base & p_out) const { p_out = "Explorer: Address Bar"; }

	virtual t_size get_supported_colours() const
	{ return columns_ui::colours::colour_flag_text | columns_ui::colours::colour_flag_background; }
	virtual t_size get_supported_bools() const { return 0; }
	virtual bool get_themes_supported() const { return false; }
	virtual void on_colour_changed(t_size mask) const { PERFORM_GLOBAL( UpdateColors() ); }
	virtual void on_bool_changed(t_size mask) const {}

	const static GUID class_guid;
};
// {296E24C5-4ED1-444d-AE70-0895E582310A}
DECLARE_CLASS_GUID(CAddrBarColors,
0x296e24c5, 0x4ed1, 0x444d, 0xae, 0x70, 0x8, 0x95, 0xe5, 0x82, 0x31, 0xa);

class CAddrBarFonts : public columns_ui::fonts::client
{
public:
	CAddrBarFonts() {}
	virtual ~CAddrBarFonts() {}

	virtual const GUID & get_client_guid() const { return class_guid; }
	virtual void get_name (pfc::string_base & p_out) const { p_out = "Explorer: Address Bar"; }

	virtual columns_ui::fonts::font_type_t get_default_font_type() const { return columns_ui::fonts::font_type_items; }
	virtual void on_font_changed() const { PERFORM_GLOBAL( UpdateFonts() ); }

	const static GUID class_guid;
};
// {4DA6C319-DA1B-4f57-9530-E828A3CF939E}
DECLARE_CLASS_GUID(CAddrBarFonts, 
0x4da6c319, 0xda1b, 0x4f57, 0x95, 0x30, 0xe8, 0x28, 0xa3, 0xcf, 0x93, 0x9e);
