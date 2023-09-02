#pragma once

#include "Core/TypeDef.h"
#include "Core/Structures/String.h"

#include "Core/Structures/Managed/Array.h"

#include "Math/Math.h"

NAMESPACE_BEGIN

class Serializable;

#define _DEBUG_METADATA 1

#ifdef _DEBUG_METADATA

class API MetadataClassCounter
{
	template<typename T>
	friend class MetadataClassIndex;

	static ID s_count;
};

template <typename _Class>
class MetadataClassIndex
{
	friend class MetadataUtils;
	static ID GetIndex()
	{
		static ID index = MetadataClassCounter::s_count++;
		return index;
	}
};

class MetadataUtils
{
public:
	template<typename T>
	inline static ID GetClassId()
	{
		return MetadataClassIndex<T>::GetIndex();
	}

};

#endif // _DEBUG_METADATA

class UnknownAddress
{
private:
#ifdef _DEBUG_METADATA
	ID m_classIndex = INVALID_ID;
#endif // _DEBUG_METADATA

	void* m_ptr = nullptr;

public:
	UnknownAddress() {};

	template<typename T>
	UnknownAddress(T* ptr) : 
#ifdef _DEBUG_METADATA
		m_classIndex(MetadataUtils::GetClassId<T>()), 
#endif
		m_ptr((void*)ptr)
	{

	}

	bool HasDestination() const
	{
		return m_ptr != nullptr;
	}

	template<typename T>
	T& As()
	{
#ifdef _DEBUG_METADATA
		assert(MetadataUtils::GetClassId<T>() == m_classIndex);
#endif
		return *(T*)m_ptr;
	}

	void* Ptr() const
	{
		return m_ptr;
	}

	bool Is(void* p) const
	{
		return m_ptr == p;
	}
};

using SetAccessor = void (*)(const String& input, UnknownAddress& var, Serializable* instance);
using GetAccessor = String (*)(UnknownAddress& var, Serializable* instance);

class API Accessor
{
private:
	const char* m_name = nullptr;
	const char* m_editFormat = nullptr;
	UnknownAddress m_var = {};
	SetAccessor m_setter = nullptr;
	GetAccessor m_getter = nullptr;
	Serializable* m_instance = nullptr;

public:
	Accessor() {};

	Accessor(
		const char* name,
		const char* editFormat,
		const UnknownAddress& var, 
		SetAccessor setter, 
		GetAccessor getter, 
		Serializable* instance
	) : m_name(name), m_editFormat(editFormat), m_var(var), m_setter(setter), m_getter(getter), m_instance(instance) {};

	template<typename T>
	Accessor(const char* name, T& var, Serializable* instance) : m_name(name), m_var(&var), m_instance(instance)
	{
		static_assert(std::is_fundamental_v<T> == true);

		m_setter = [](const String& input, UnknownAddress& var, Serializable* instance) -> void
		{
			auto v = std::stod(input.c_str());
			var.As<T>() = static_cast<T>(v);
		};

		m_getter = [](UnknownAddress& var, Serializable* instance) -> String
		{
			return String::Format("{}", var.As<T>());
		};

		m_editFormat = "%number";
	}

public:
	void Set(const String& input);

	inline String Get()
	{
		return m_getter(m_var, m_instance);
	}

	inline auto GetEditFormat()
	{
		return m_editFormat;
	}

	inline auto GetName()
	{
		return m_name;
	}

	inline auto& Var() const
	{
		return m_var;
	}

public:
	static Accessor For(const char* name, float& var, Serializable* instance)
	{
		return Accessor(name, var, instance);
	}

	static Accessor For(const char* name, double& var, Serializable* instance)
	{
		return Accessor(name, var, instance);
	}

	static Accessor For(const char* name, int8_t& var, Serializable* instance)
	{
		return Accessor(name, var, instance);
	}

	static Accessor For(const char* name, int16_t& var, Serializable* instance)
	{
		return Accessor(name, var, instance);
	}

	static Accessor For(const char* name, int32_t& var, Serializable* instance)
	{
		return Accessor(name, var, instance);
	}

	static Accessor For(const char* name, int64_t& var, Serializable* instance)
	{
		return Accessor(name, var, instance);
	}

	static Accessor For(const char* name, size_t& var, Serializable* instance)
	{
		return Accessor(name, var, instance);
	}

	static Accessor For(const char* name, Vec3& vec, Serializable* instance);
	static Accessor For(const char* name, Quaternion& quad, Serializable* instance);
	static Accessor For(const char* name, Mat4& mat, Serializable* instance);
	static Accessor For(const char* name, Transform& transform, Serializable* instance);

};


class ClassMetadata : Traceable<ClassMetadata>
{
private:
	const char* m_className = nullptr;
	Serializable* m_instance = nullptr;

	std::vector<Accessor> m_properties;

	// class contains class
	Array<Handle<ClassMetadata>> m_subClasses;

	size_t m_visited = 0;

	TRACEABLE_FRIEND();
	void Trace(Tracer* tracer)
	{
		tracer->Trace(m_subClasses);
	}

public:
	ClassMetadata() {};

	ClassMetadata(const char* className, Serializable* instance) : m_className(className), m_instance(instance) {};

private:
	template<bool RECURSIVE = true, typename Fn>
	void ForEachPropertiesImpl(Fn fn)
	{
		SetVisited(true);

		for (auto& a : m_properties)
		{
			fn(this, a);
		}

		if constexpr (RECURSIVE)
		{
			for (auto& c : m_subClasses)
			{
				if (!c->IsVisited()) 
				{
					c->ForEachPropertiesImpl<RECURSIVE>(fn);
				}
			}
		}
	}

	void UnsetVisited()
	{
		SetVisited(false);
		for (auto& c : m_subClasses)
		{
			if (c->IsVisited())
			{
				c->UnsetVisited();
			}
		}
	}

public:
	void AddProperty(const Accessor& accessor)
	{
		assert(accessor.Var().Ptr() == (void*)m_instance);
		m_properties.push_back(accessor);
	}

	template<typename T>
	void AddProperty(T property, const char* name)
	{
		using _T = decltype(m_instance->*property);

		if constexpr (std::is_fundamental_v<_T>)
		{
			m_properties.push_back(Accessor(name, &(m_instance->*property), m_instance));
		}
		else
		{
			m_properties.push_back(Accessor::For(name, &(m_instance->*property), m_instance));
		}
	}

	void AddSubClass(const Handle<ClassMetadata>& subClass)
	{
		m_subClasses.Push(subClass);
	}

	bool IsVisited() const
	{
		return m_visited != 0;
	}

	void SetVisited(bool v)
	{
		m_visited = (size_t)v;
	}

	template<bool RECURSIVE = true, typename Fn>
	void ForEachProperties(Fn fn)
	{
		ForEachPropertiesImpl<RECURSIVE>(fn);

		if constexpr (RECURSIVE)
		{
			UnsetVisited();
		}
	}
};

NAMESPACE_END