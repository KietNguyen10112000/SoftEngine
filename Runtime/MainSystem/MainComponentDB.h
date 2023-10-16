#pragma once

#include "Core/Memory/Memory.h"
#include "Core/Pattern/Singleton.h"

#include "Common/Base/MainComponent.h"

NAMESPACE_BEGIN

class MainComponentDB : public Singleton<MainComponentDB>
{
public:
	using ComponentCtor = Handle<MainComponent> (*)();

	struct ComponentRecord
	{
		String name;

		// managed memory, so just ctor is enough, no need dtor
		ComponentCtor ctor;

		inline bool operator<(const ComponentRecord& another)
		{
			return name < another.name;
		}
	};

	struct ComponentRecordCMP
	{
		bool operator() (const ComponentRecord& a, const ComponentRecord& b) const {
			return a.name < b.name;
		}
	};

private:
	std::set<ComponentRecord, ComponentRecordCMP, STLNodeGlobalAllocator<ComponentRecord>> m_compRecords;

private:
	void AddRecord(const ComponentRecord& record);
	void RemoveRecord(const char* name);

public:
	template <typename Component>
	void RegisterComponent()
	{
		// main component must have a default constructor
		static_assert(std::is_default_constructible<Component>::value);

		ComponentRecord record;
		record.name = Component::___GetClassName();
		record.ctor = []() -> Handle<MainComponent>
		{
			return mheap::New<Component>();
		};

		AddRecord(record);
	}

	template <typename Component>
	void UnregisterComponent()
	{
		RemoveRecord(Component::___GetClassName());
	}

};

NAMESPACE_END