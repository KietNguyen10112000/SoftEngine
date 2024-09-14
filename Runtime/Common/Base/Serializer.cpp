#include "Serializer.h"

#include "SerializableDB.h"
#include "FileSystem/FileUtils.h"

NAMESPACE_MEMORY_BEGIN
extern thread_local size_t g_heapId;
NAMESPACE_MEMORY_END

NAMESPACE_BEGIN

Serializer::Serializer(MODE mode) :  m_mode(mode)
{
}

Serializer::~Serializer()
{
	
}

//SerializableDB::Get()->GetSerializableRecord(className);

void Serializer::TrySerialize(Serializable* obj, SerializedRecord::TYPE type)
{
	assert(obj != nullptr);

	type = (SerializedRecord::TYPE)obj->GetMemoryType();

	auto& uuid = obj->GetUUID();
	
	auto it = m_serializedObjects.find(uuid);
	if (it != m_serializedObjects.end())
	{
		assert(it->second.type == type);
		return;
	}

	SerializedRecord record;
	record.type = type;

	{
		// indexing className

		String className = obj->GetClassName();
		auto classNameIt = m_classNameIds.find(className);

		if (classNameIt == m_classNameIds.end())
		{
			record.classNameIdx = m_classNames.size();
			m_classNames.push_back(className);
			m_classNameIds.insert({ className,record.classNameIdx });
		}
		else
		{
			record.classNameIdx = classNameIt->second;
		}
	}

	record.heapId = mheap::internal::GetHeapIdOfMemoryBlock(dynamic_cast<void*>(obj));

	switch (m_mode)
	{
	case Serializer::MODE_BINARY: {
		record.idx = m_binaries.size();

		auto& back = m_binaries.emplace_back();
		back.uuid = uuid;
		back.stream = std::make_shared<ByteStream>();
		back.record = record;

		m_serializedObjects.insert({ uuid,record });
		obj->SerializeToBinary(this, *m_binaries[record.idx].stream);
		break;
	}
	case Serializer::MODE_JSON: {
		record.idx = m_jsons.size();

		auto& back = m_jsons.emplace_back();
		back.uuid = uuid;
		back.j = std::make_shared<json>();
		back.record = record;

		m_serializedObjects.insert({ uuid,record });
		obj->SerializeToJson(this, *m_jsons[record.idx].j);
		break;
	}
	default:
		assert(0);
		break;
	}
}

void Serializer::TrySerializeRC(ResourceBase* rc)
{
	auto& uuid = rc->GetUUID();
	auto it = m_usedResources.find(uuid);
	if (it != m_usedResources.end())
	{
		return;
	}

	SerializedResourceRecord record;
	record.type = SERIALIZABLE_MEM_RESOURCE;
	{
		// indexing className

		String className = rc->GetClassName();
		auto classNameIt = m_classNameIds.find(className);

		if (classNameIt == m_classNameIds.end())
		{
			/*if (className == "class soft::AnimMotion")
			{
				String s = "class soft::AnimMotion";
				String s2 = rc->GetClassName();

				std::map<String, ID> testMap;
				testMap.insert({ "class soft::AnimMotion", 0 });
				auto testIt = testMap.find(className);

				int x = 3;
			}*/

			record.classNameIdx = m_classNames.size();
			m_classNames.push_back(className);
			m_classNameIds.insert({ className,record.classNameIdx });
		}
		else
		{
			record.classNameIdx = classNameIt->second;
		}
	}

	record.resource = rc->GetSelfResource();

	switch (m_mode)
	{
	case Serializer::MODE_BINARY: {
		record.idx = m_binaries.size();
		m_binaries.push_back({ uuid,std::move(std::make_unique<ByteStream>()),record.ToBase()});
		m_usedResources.insert({ uuid,record });
		m_serializedObjects.insert({ uuid,record.ToBase()});
		rc->SerializeExtDataToBinary(this, *m_binaries[record.idx].stream);
		break;
	}
	case Serializer::MODE_JSON: {
		record.idx = m_jsons.size();
		m_jsons.push_back({ uuid,std::move(std::make_unique<json>()),record.ToBase() });
		m_usedResources.insert({ uuid,record });
		m_serializedObjects.insert({ uuid,record.ToBase() });
		rc->SerializeExtDataToJson(this, *m_jsons[record.idx].j);
		break;
	}
	default:
		assert(0);
		break;
	}
}

void Serializer::TryDeserialize(const UUID& uuid, Handle<Serializable>* output0, Serializable** output1, SharedPtr<Serializable>* output2)
{
	auto it = m_serializedObjects.find(uuid);
	if (it == m_serializedObjects.end())
	{
		assert(0); // read from file first before deserialize
		//return;
	}

Begin:
	auto& record = it->second;
	auto& handle = m_deserializedObjects[record.idx];
	if (handle)
	{
		//assert(output0 != nullptr);
		//assert(output1 == nullptr);
		assert(output2 == nullptr);

		if (output0)
			*output0 = handle;

		if (output1)
			*output1 = handle;

		return;
	}

	auto& rawOrShared = m_rawOrSharedDeserializedObjects[record.idx];
	if (rawOrShared.shared)
	{
		assert(output0 == nullptr);
		//assert(output1 == nullptr);
		//assert(output2 != nullptr);
		assert(rawOrShared.raw == nullptr);

		if (output2)
			*output2 = rawOrShared.shared;

		if (output1)
			*output1 = rawOrShared.shared.get();

		return;
	}
	
	if (rawOrShared.raw)
	{
		assert(output0 == nullptr);
		assert(output1 != nullptr);
		assert(output2 == nullptr);
		assert(rawOrShared.shared.get() == nullptr);

		*output1 = rawOrShared.raw;
		return;
	}

	// this object isn't deserialized
	auto dbRecord = SerializableDB::Get()->GetSerializableRecord(m_classNames[record.classNameIdx].c_str());
	if (dbRecord.name.empty())
	{
		assert(0 && "Register Serializable class to SerializableDB in SerializableList.h");
		*((int*)nullptr); // must crash here
	}

	SerializedRecord::TYPE memType = SerializedRecord::HANDLE;
	memType = (SerializedRecord::TYPE)dbRecord.memType;

	assert(memType == record.type);

	Serializable* obj = nullptr;
	if (memType == SerializedRecord::HANDLE)
	{
		byte old = g_heapId;//mheap::internal::GetCurrentHeapId();
		if (record.heapId != old)
		{
			//mheap::internal::SetHeapId(mheap::internal::HEAP_ID::ID(record.heapId));
			g_heapId = record.heapId;
		}

		auto h = dbRecord.ctor();
		m_deserializedObjects[record.idx] = h;
		obj = h;

		if (record.heapId != old)
		{
			g_heapId = old;
		}
	}

	if (memType == SerializedRecord::RAW)
	{
		auto raw = dbRecord.ctorRaw();
		m_rawOrSharedDeserializedObjects[record.idx].raw = raw;
		obj = raw;
	}

	if (memType == SerializedRecord::SHARED)
	{
		auto shared = dbRecord.ctorShared();
		m_rawOrSharedDeserializedObjects[record.idx].shared = shared;
		obj = shared.get();
	}

	switch (m_mode)
	{
	case Serializer::MODE_BINARY: {
		obj->DeserializeFromBinary(this, *m_binaries[record.idx].stream);
		break;
	}
	case Serializer::MODE_JSON: {
		obj->DeserializeFromJson(this, *m_jsons[record.idx].j);
		break;
	}
	default:
		assert(0);
		break;
	}

	if (m_readFlags & READ_FLAG_UNIQUE_UUID)
	{
		((UUID&)obj->m_UUID) = uuid;
	}

	goto Begin;
}

void Serializer::TryDeserializeRC(const UUID& uuid, Resource<ResourceBase>* output0)
{
	auto it = m_usedResources.find(uuid);
	if (it == m_usedResources.end())
	{
		assert(0); // read from file first before deserialize
		//return;
	}

	*output0 = it->second.resource;
}

void Serializer::TryClone(Serializable* obj, Handle<Serializable>* output0, Serializable** output1, SharedPtr<Serializable>* output2)
{
	assert(obj != nullptr);

	auto& uuid = obj->GetUUID();
	auto it = m_clonedObjectIds.find(uuid);
	ID idx = INVALID_ID;

	if (it == m_clonedObjectIds.end())
	{
		auto dbRecord = SerializableDB::Get()->GetSerializableRecord(obj->GetClassName());
		if (dbRecord.name.empty())
		{
			assert(0 && "Register Serializable class to SerializableDB in SerializableList.h");
			*((int*)nullptr); // must crash here
		}

		auto memType = (SerializedRecord::TYPE)obj->GetMemoryType();
		assert(memType == (SerializedRecord::TYPE)dbRecord.memType);

		Serializable* newObj = nullptr;
		if (memType == SerializedRecord::HANDLE)
		{
			auto h = dbRecord.ctor();

			idx = m_rawOrSharedCloneObjects.size();
			m_clonedObjects.Push(h);
			m_rawOrSharedCloneObjects.push_back({});

			newObj = h;
		}

		if (memType == SerializedRecord::RAW)
		{
			auto raw = dbRecord.ctorRaw();

			idx = m_rawOrSharedCloneObjects.size();
			m_clonedObjects.Push(nullptr);
			m_rawOrSharedCloneObjects.push_back({ nullptr,raw });

			newObj = raw;
		}

		if (memType == SerializedRecord::SHARED)
		{
			auto shared = dbRecord.ctorShared();
			
			idx = m_rawOrSharedCloneObjects.size();
			m_clonedObjects.Push(nullptr);
			m_rawOrSharedCloneObjects.push_back({ shared,nullptr });

			newObj = shared.get();
		}

		//ID classNameIdx = INVALID_ID;
		//{
		//	// indexing className

		//	String className = obj->GetClassName();
		//	auto classNameIt = m_classNameIds.find(className);

		//	if (classNameIt == m_classNameIds.end())
		//	{
		//		classNameIdx = m_classNames.size();
		//		m_classNames.push_back(className);
		//		m_classNameIds.insert({ className,classNameIdx });
		//	}
		//	else
		//	{
		//		classNameIdx = classNameIt->second;
		//	}
		//}

		m_clonedObjectIds.insert({ uuid,{ idx,(uint32_t)0,(uint16_t)memType,(uint16_t)0 } });

		newObj->CloneFrom(this, obj);
	}
	else
	{
		idx = it->second.idx;
	}

	auto& handle = m_clonedObjects[idx];
	if (handle)
	{
		//assert(output0 != nullptr);
		//assert(output1 == nullptr);
		assert(output2 == nullptr);

		if (output0)
			*output0 = handle;

		if (output1)
			*output1 = handle;

		return;
	}

	auto& rawOrShared = m_rawOrSharedCloneObjects[idx];
	if (rawOrShared.shared)
	{
		assert(output0 == nullptr);
		//assert(output1 == nullptr);
		//assert(output2 != nullptr);
		assert(rawOrShared.raw == nullptr);

		if (output2)
			*output2 = rawOrShared.shared;

		if (output1)
			*output1 = rawOrShared.shared.get();

		return;
	}

	if (rawOrShared.raw)
	{
		assert(output0 == nullptr);
		assert(output1 != nullptr);
		assert(output2 == nullptr);
		assert(rawOrShared.shared.get() == nullptr);

		*output1 = rawOrShared.raw;
		return;
	}

	assert(0);
}

void Serializer::WriteToJson(json& j)
{
	json meta;
	{
		auto arr = json::array();

		for (auto& className : m_classNames)
		{
			arr.push_back(className.c_str());
		}

		meta["UsedClassNames"] = arr;
	}

	meta["RootUUIDs"] = m_rootUUIDs;
	j["Meta"] = meta;
	
	{
		auto arr = json::array();
		auto count = m_jsons.size();
		for (size_t i = 0; i < count; i++)
		{
			auto& v = m_jsons[i];
			json& data = *v.j;
			json j1;
			j1["UUID"]			= v.uuid;
			j1["MemType"]		= v.record.type;

			if (v.record.type == SerializedRecord::HANDLE)
			{
				j1["MemHeapId"] = v.record.heapId;
			}

			j1["ClassName"]		= m_classNames[v.record.classNameIdx];
			j1["Data"]			= data;
			arr.push_back(j1);
		}
		j["Objects"] = arr;
	}

	{
		auto arr = json::array();
		for (auto& [key, value] : m_usedResources)
		{
			json j1;
			j1["UUID"] = key;
			j1["Path"] = value.resource->GetPath();
			arr.push_back(j1);
		}
		j["UsedResources"] = arr;
	}
}

void Serializer::WriteToFileJson(const String& path, json& j)
{
	auto str = j.dump(2);
	FileUtils::WriteFile(path.c_str(), str.c_str(), str.length());
}

void Serializer::WriteToFileBinary(const String& path)
{
}

void Serializer::ReadFromJson(const json& j)
{
	auto& meta = j["Meta"];
	{
		auto& arr = meta["UsedClassNames"];
		auto count = arr.size();
		for (size_t i = 0; i < count; i++)
		{
			m_classNames.push_back(arr[i].get<std::string>().c_str());
			m_classNameIds.insert({ m_classNames.back(),i });
		}
	}
	m_rootUUIDs = meta["RootUUIDs"];

	{
		auto& arr = j["Objects"];
		auto count = arr.size();
		for (size_t i = 0; i < count; i++)
		{
			auto& j1 = arr[i];
			const json& data = j1["Data"];

			SerializedRecord record;
			record.idx			= i;
			record.classNameIdx = uint32_t(m_classNameIds[j1["ClassName"]]);
			record.type			= j1["MemType"];

			if (record.type == SerializedRecord::HANDLE)
			{
				record.heapId = j1["MemHeapId"];
			}

			UUID uuid = j1["UUID"];
			m_serializedObjects.insert({ uuid,record });

			m_jsons.push_back({ uuid,std::move(std::make_unique<json>(data)),record });
		}

		m_deserializedObjects.Resize(count);
		m_rawOrSharedDeserializedObjects.resize(count);
	}

	{
		auto& arr = j["UsedResources"];
		auto count = arr.size();
		for (int64_t i = count - 1; i != -1; i--)
		{
			auto& j1 = arr[i];
			UUID uuid = j1["UUID"];
			String path = j1["Path"];

			assert(m_serializedObjects.find(uuid) != m_serializedObjects.end());

			auto& sRecord = m_serializedObjects[uuid];
			auto dbRecord = SerializableDB::Get()->GetSerializableRecord(m_classNames[sRecord.classNameIdx].c_str());
			if (dbRecord.name.empty())
			{
				assert(0 && "Register Serializable class to SerializableDB in SerializableList.h");
				*((int*)nullptr); // must crash here
			}

			SerializedResourceRecord record;
			record.idx			= sRecord.idx;
			record.classNameIdx = sRecord.classNameIdx;
			record.type			= sRecord.type;
			record.resource		= dbRecord.ctorResource(path);
			m_usedResources.insert({ uuid,record });
		}

		for (auto& [key, value] : m_usedResources)
		{
			value.resource->DeserializeExtDataFromJson(this, *m_jsons[value.idx].j);
		}
	}

}

void Serializer::ReadFromFileJson(const String& path)
{
	byte* buffer; size_t fileSize;
	FileUtils::ReadFile(path, buffer, fileSize);
	buffer[fileSize] = '\0';

	json j = json::parse((const char*)buffer);
	ReadFromJson(j);

	FileUtils::FreeBuffer(buffer);
}

void Serializer::ReadFromFileBinary(const String& path)
{
}

void Serializer::SetStableValuesMap(byte* map)
{
	if (map)
	{
		std::memcpy(m_stableValuesMap, map, sizeof(m_stableValuesMap));
	}
	else
	{
		for (size_t i = 0; i < sizeof(m_stableValuesMap); i++)
		{
			m_stableValuesMap[i] = (byte)i;
		}
	}
}

void Serializer::WriteToFile(const String& path)
{
	switch (m_mode)
	{
	case Serializer::MODE_BINARY: {
		WriteToFileBinary(path);
		break;
	}
	case Serializer::MODE_JSON: {
		json j;
		WriteToJson(j);
		WriteToFileJson(path, j);
		break;
	}
	default:
		assert(0);
		break;
	}
}

void Serializer::ReadFromFile(const String& _path, READ_FLAG flags)
{
	m_readFlags = flags;

	auto path = FileSystem::Get()->FindAbsoluteFilePath(_path);

	switch (m_mode)
	{
	case Serializer::MODE_BINARY: {
		ReadFromFileBinary(path);
		break;
	}
	case Serializer::MODE_JSON: {
		ReadFromFileJson(path);
		break;
	}
	default:
		assert(0);
		break;
	}
}

void Serializer::WriteToJsonBuffer(json& buffer)
{
	assert(m_mode == MODE_JSON);
	WriteToJson(buffer);
}

void Serializer::ReadFromJsonBuffer(const json& buffer, READ_FLAG flags)
{
	assert(m_mode == MODE_JSON);
	m_readFlags = flags;
	ReadFromJson(buffer);
}

void Serializer::SetRootUUID(const UUID& uuid, ID id)
{
	if (id >= m_rootUUIDs.size())
	{
		m_rootUUIDs.resize(id + 1);
	}
	m_rootUUIDs[id] = uuid;
}

NAMESPACE_END