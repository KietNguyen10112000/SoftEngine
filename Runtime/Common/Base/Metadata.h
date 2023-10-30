#pragma once

#include "Core/TypeDef.h"
#include "Core/Structures/String.h"

#include "Core/Structures/Managed/Array.h"

#include "Math/Math.h"

#include "Variant.h"

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

	ID m_hasTag = 0;

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

	UnknownAddress(size_t tag) :
#ifdef _DEBUG_METADATA
		m_classIndex(INVALID_ID),
#endif
		m_ptr((void*)tag)
	{
		m_hasTag = 1;
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

	bool Is(size_t tag) const
	{
		return (size_t)m_ptr == tag && m_hasTag != 0;
	}
};

using SetAccessor = void (*)(const Variant& input, UnknownAddress& var, Serializable* instance);
using GetAccessor = Variant (*)(UnknownAddress& var, Serializable* instance);

class API Accessor
{
private:
	const char* m_name = nullptr;
	UnknownAddress m_var = {};
	SetAccessor m_setter = nullptr;
	GetAccessor m_getter = nullptr;
	Serializable* m_instance = nullptr;

public:
	Accessor() {};

	Accessor(
		const char* name,
		const UnknownAddress& var, 
		SetAccessor setter, 
		GetAccessor getter, 
		Serializable* instance
	) : m_name(name), m_var(var), m_setter(setter), m_getter(getter), m_instance(instance) {};

	template<bool DIRECT_MODIFY_VAR = true, typename T>
	inline static Accessor For(const char* name, T& var, Serializable* instance)
	{
		return Accessor(
			name,
			&var,

			[](const Variant& input, UnknownAddress& var, Serializable* instance) -> void
			{
				if constexpr (DIRECT_MODIFY_VAR)
				{
					var.As<T>() = input.As<T>();
				}
			},

			[](UnknownAddress& var, Serializable* instance) -> Variant
			{
				Variant ret = Variant::Of<T>();
				ret.As<T>() = var.As<T>();
				return ret;
			},

			instance
		);
	}

public:
	void Set(const Variant& input);

	inline Variant Get()
	{
		return m_getter(m_var, m_instance);
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
	/*static Accessor For(const char* name, double& var, Serializable* instance)
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
	static Accessor For(const char* name, Transform& transform, Serializable* instance);*/

};


class ClassMetadata : Traceable<ClassMetadata>
{
private:
	const char* m_className = nullptr;
	Serializable* m_instance = nullptr;

	std::vector<Accessor> m_properties;

	// class contains class
	Array<Handle<ClassMetadata>> m_subClasses;
	std::vector<const char*> m_subClassesPropertyNames;

	size_t m_visited = 0;

	TRACEABLE_FRIEND();
	void Trace(Tracer* tracer)
	{
		tracer->Trace(m_subClasses);
	}

public:
	//ClassMetadata() {};

	ClassMetadata(const char* className, Serializable* instance) 
		: m_className(className), m_instance(instance) {};

private:
	template<bool RECURSIVE = true, typename Fn>
	void ForEachPropertiesImpl(Fn fn, const char* name)
	{
		SetVisited(true);

		for (auto& a : m_properties)
		{
			// param: (current class type, current class instance name, property name)
			fn(this, a.GetName(), a);
		}

		if constexpr (RECURSIVE)
		{
			size_t i = 0;
			for (auto& c : m_subClasses)
			{
				if (!c->IsVisited()) 
				{
					c->ForEachPropertiesImpl<RECURSIVE>(fn, m_subClassesPropertyNames[i]);
				}
				i++;
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
		//assert(accessor.Var().Ptr() == (void*)m_instance);
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

	void AddProperty(const char* name, const Handle<ClassMetadata>& subClass)
	{
		if (subClass.Get() == nullptr) return;

		m_subClasses.Push(subClass);
		m_subClassesPropertyNames.push_back(name);
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
	void ForEachProperties(Fn fn, const char* currentIntanceName = "")
	{
		ForEachPropertiesImpl<RECURSIVE>(fn, currentIntanceName);

		if constexpr (RECURSIVE)
		{
			UnsetVisited();
		}
	}
};

NAMESPACE_END