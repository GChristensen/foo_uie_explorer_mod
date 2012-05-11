#pragma once

template<typename B, typename T>
class cfg_resetable_t : public B
{
public:
	cfg_resetable_t(const GUID & p_guid, T defaultval) : B(p_guid, defaultval), m_defaultval(defaultval) {}
	void reset() { *this = m_defaultval; }
	const T& default_value() const { return m_defaultval; }

	using B::operator=;
protected:
	T m_defaultval;
};

typedef cfg_resetable_t<cfg_int, t_int32> cfg_int_resetable;
typedef cfg_resetable_t<cfg_uint, t_uint32> cfg_uint_resetable;
typedef cfg_resetable_t<cfg_guid, GUID> cfg_guid_resetable;
typedef cfg_resetable_t<cfg_bool, bool> cfg_bool_resetable;
typedef cfg_resetable_t<cfg_string, const char *> cfg_string_resetable;

template<typename T>
class cfg_strcut_resetable_t : public cfg_struct_t<T>
{
	typedef cfg_struct_t<T> B;
public:
	cfg_strcut_resetable_t(const GUID & p_guid, const T & defaultval) : B(p_guid, defaultval), m_defaultval(defaultval) {}
	cfg_strcut_resetable_t(const GUID & p_guid, int filler) : B(p_guid, filler) { memset(&m_defaultval, filler, sizeof(T)); }
	void reset() { *this = m_defaultval; }
	const T& default_value() const { return m_defaultval; }

	using B::operator=;
protected:
	T m_defaultval;
};
