#pragma once

#include "Core/Memory/Memory.h"
#include "Core/Pattern/Singleton.h"

#include "Common/Base/MainComponent.h"

NAMESPACE_BEGIN

class SerializableDB : public Singleton<SerializableDB>
{
public:
	using SerializableCtor = Handle<Serializable> (*)();

	struct SerializableRecord
	{
		ID COMPONENT_ID = INVALID_ID;
		String name;

		// managed memory, so just ctor is enough, no need dtor
		SerializableCtor ctor = nullptr;

		inline bool operator<(const SerializableRecord& another)
		{
			return name < another.name;
		}
	};

	struct SerializableRecordCMP
	{
		bool operator() (const SerializableRecord& a, const SerializableRecord& b) const {
			return a.name < b.name;
		}
	};

private:
	std::set<SerializableRecord, SerializableRecordCMP, STLNodeGlobalAllocator<SerializableRecord>> m_records;

private:
	void AddRecord(const SerializableRecord& record);
	void RemoveRecord(const char* name);

public:
	template <typename SerializableType>
	void Register()
	{
		// main component must have a default constructor
		static_assert(std::is_default_constructible<SerializableType>::value);
		static_assert(std::is_base_of_v<Serializable, SerializableType>);

		SerializableRecord record;

		if constexpr (std::is_base_of_v<MainComponent, SerializableType>)
		{
			record.COMPONENT_ID = SerializableType::COMPONENT_ID;
		}
		else
		{
			record.COMPONENT_ID = INVALID_ID;
		}

		record.name = SerializableType::___GetClassName();
		record.ctor = []() -> Handle<Serializable>
		{
			return mheap::New<SerializableType>();
		};

		AddRecord(record);
	}

	template <typename SerializableType>
	void Unregister()
	{
		RemoveRecord(SerializableType::___GetClassName());
	}

	inline SerializableRecord GetSerializableRecord(const char* className)
	{
		SerializableRecord record;
		record.name = className;
		
		auto it = m_records.find(record);
		if (it != m_records.end())
		{
			return *it;
		}

		return {};
	}

};

NAMESPACE_END