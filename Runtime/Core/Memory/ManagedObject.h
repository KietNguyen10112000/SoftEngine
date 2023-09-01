#pragma once

#include "Trace.h"

#include "ManagedHandle.h"
#include "ManagedLocalScope.h"

NAMESPACE_MEMORY_BEGIN

template <typename T>
class LivableOnStackObject {};

template <typename T>
class ManagedLocalObject
{
public:
	class Object : public ManagedHandle, public T
	{
	private:
		size_t m_idx = -1;

	public:
		Object(const T& v)
		{
			static_assert(std::is_base_of_v<Traceable<T>, T>, "Type check!");
			static_assert(std::is_base_of_v<LivableOnStackObject<T>, T>, "Type check!");

			ManagedHandle::traceTable = Traceable<T>::GetTraceTable();

			auto _this = This();
			// shallow copy
			memcpy(_this, &v, sizeof(T));
			//reinterpret_cast<T&>(*This()) = v;
			m_idx = ManagedLocalScope::Add(_this);
		}

		~Object()
		{
			if (m_idx != -1) ManagedLocalScope::DecrementRefAndAutoRemove(m_idx);
		}

	private:
		inline auto This()
		{
			return ManagedHandle::GetUsableMemAddress();
		}

	};

};


template <typename T>
using LObject = typename ManagedLocalObject<T>::Object;


NAMESPACE_MEMORY_END