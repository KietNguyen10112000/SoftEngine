#pragma once

#include "Core/TypeDef.h"

#include <iostream>
#include <atomic>
#include <map>
#include <string>
#include <cstdlib>
#include <sstream>

#include "Core/Memory/Memory.h"

NAMESPACE_BEGIN

// faster compare
class String
{
protected:
	using char_type = char;

	class Header
	{
	public:
		std::atomic<size_t> m_refCount = { 0 };
		size_t m_length = 0;
		size_t m_hash = 0;

	public:
		inline Header* Acquire()
		{
			++m_refCount;
			return this;
		};

		inline bool Release()
		{
			return (--m_refCount) == 0;
		};
	};

	mutable Header* m_header = 0;

public:
	String()
	{
	};

	String(const char* s)
	{
		*this = s;
	};

	String(const String& s)
	{
		*this = s;
	};

	~String()
	{
		Reset();
	};

protected:
	inline static void* Allocate(size_t n)
	{
		return rheap::malloc(n);
	}

	inline static void Deallocate(void* p)
	{
		rheap::free(p);
	}

	inline void Add(const char_type* l, size_t lLen, const char_type* r, size_t rLen)
	{
		auto len = lLen + rLen;

		auto nBytes = (len + 1) * sizeof(char_type);
		m_header = (Header*)Allocate(sizeof(Header) + nBytes);

		m_header->m_length = len;
		m_header->m_refCount = 1;

		auto buf = (char_type*)(m_header + 1);
		::memcpy(buf, l, lLen);
		::memcpy(buf + lLen, r, rLen);
		buf[len] = 0;

		m_header->m_hash = Hash(c_str());
	};

public:
	static size_t Hash(const char_type* str)
	{
		constexpr size_t p = 31; // or 53
		constexpr size_t m = 1e9 + 9;
		size_t hash_value = 0;
		size_t p_pow = 1;

		char_type* s = (char_type*)str;
		char_type c = *s;
		while (c)
		{
			hash_value = (hash_value + (c - 'a' + 1) * p_pow) % m;
			p_pow = (p_pow * p) % m;
			c = *(++s);
		}
		return hash_value;
	};

public:
	inline const char_type* c_str() const
	{
		return m_header ? (char*)(m_header + 1) : 0;
	};

	inline size_t length() const
	{
		return m_header->m_length;
	};

	inline size_t hash() const
	{
		return m_header->m_hash;
	};

	friend std::ostream& operator<<(std::ostream& out, const String& str)
	{
		auto cstr = str.c_str();
		if (cstr)
		{
			out << cstr;
		}
		return out;
	}

	inline void Reset()
	{
		if (m_header && m_header->Release())
		{
			Deallocate(m_header);
			m_header = 0;
		}
	};

	inline String& operator=(const String& r)
	{
		Reset();

		if (r.m_header)
		{
			m_header = r.m_header->Acquire();
		}

		return *this;
	};

	inline String& operator=(const char_type* r)
	{
		Reset();

		auto length = strlen(r);
		auto nBytes = (length + 1) * sizeof(char_type);
		m_header = (Header*)Allocate(sizeof(Header) + nBytes);

		m_header->m_length = length;
		m_header->m_refCount = 1;
		m_header->m_hash = Hash(r);

		memcpy(m_header + 1, r, nBytes);

		return *this;
	};

	inline friend String operator+(const String& l, const String& r)
	{
		if (r.length() == 0) return l;
		if (l.length() == 0) return r;

		String ret;
		ret.Add(l.c_str(), l.length(), r.c_str(), r.length());
		return ret;
	};

	inline friend String operator+(const String& s, const char_type* cstr)
	{
		if (cstr == 0) return s;
		if (s.length() == 0) return cstr;

		String ret;
		ret.Add(s.c_str(), s.length(), cstr, strlen(cstr));
		return ret;
	};

	inline friend String operator+(const char_type* cstr, const String& s)
	{
		if (cstr == 0) return s;
		if (s.length() == 0) return cstr;

		String ret;
		ret.Add(cstr, strlen(cstr), s.c_str(), s.length());
		return ret;
	};

	// fast compare
	inline bool operator>(const String& r) const
	{
		if (hash() != r.hash()) return hash() > r.hash();
		return strcmp(c_str(), r.c_str()) > 0;
	};

	inline bool operator<(const String& r) const
	{
		if (hash() != r.hash()) return hash() < r.hash();
		return strcmp(c_str(), r.c_str()) < 0;
	};

	inline bool operator==(const String& r) const
	{
		if (hash() != r.hash()) return false;
		return strcmp(c_str(), r.c_str()) == 0;
	};

public:
	static String From(const char* v)
	{
		return v;
	}

	static String From(bool v)
	{
		return  v ? "true" : "false";
	}

	static String From(const String& str)
	{
		return str;
	}

	inline thread_local static std::ostringstream ss;

	constexpr static size_t N = 1080;
	constexpr static size_t FMT_N = 128;
	inline thread_local static char s_buf[N + FMT_N] = {};

	template <typename T>
	static String From(const T& v)
	{
		ss.clear();
		ss.seekp(0);
		ss << v << '\0';
		String ret = ss.str().c_str();
		return ret;
	};

	// fmt likes printf(fmt)
	template <typename T>
	static String FromFmt(const char* fmt, const T& v)
	{
		snprintf(s_buf, N, fmt, v);
		return s_buf;
	};

	struct Formatter
	{
		char* it = 0;
		char* snapshot = 0;
		char* buf = 0;

		template <typename T, typename... Args>
		String Format(const T& v, Args&&... args)
		{
			/*if constexpr (sizeof...(args) == 0)
			{
					return (buf + String::From(v));
			}*/
			auto c = *it;
			char* bracketBegin = 0;
			char* bracketEnd = 0;
			while (c != 0)
			{
				if (c == '{')
				{
					bracketBegin = it;
				}

				if (c == '}')
				{
					bracketEnd = it;
					break;
				}

				c = *(++it);
			}

			if (*it == 0)
			{
				return snapshot;
			}
			else
			{
				intmax_t len = bracketBegin - snapshot;

				//if (len < 0) std::cout << "error\n";

				::memcpy(buf, snapshot, len);
				buf[len] = '\0';
				snapshot = ++it;

				if constexpr (sizeof...(args) == 0)
				{
					if (bracketBegin + 1 == bracketEnd)
					{
						return buf + String::From(v) + it;
					}
					else
					{
						char* fmt = &s_buf[N];
						auto count = bracketEnd - bracketBegin - 1;
						::memcpy(fmt, bracketBegin + 1, count);
						*(fmt + count) = '\0';
						auto ret = buf + String::FromFmt(fmt, v) + it;
						return ret;
					}
				}

				if constexpr (sizeof...(args) != 0)
				{
					String s;
					if (bracketBegin + 1 == bracketEnd)
					{
						s = buf + String::From(v);
					}
					else
					{
						char* fmt = &s_buf[N];
						auto count = bracketEnd - bracketBegin - 1;
						::memcpy(fmt, bracketBegin + 1, count);
						*(fmt + count) = '\0';
						s = buf + String::FromFmt(fmt, v) + it;
					}

					return s + Format(std::forward<Args>(args)...);
				}
			}
		};
	};

	template <typename... Args>
	static String Format(const char* fmt, Args&&... args)
	{
		auto len = ::strlen(fmt) + 1;
		auto buf = (char*)Allocate(len);
		::memset(buf, 0, len);
		Formatter formatter = { (char*)fmt, (char*)fmt, buf };
		String ret = formatter.Format(std::forward<Args>(args)...);
		Deallocate(buf);
		return ret;
	};

};

NAMESPACE_END