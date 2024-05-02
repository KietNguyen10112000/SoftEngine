#include "Serializer.h"

#include "SerializableDB.h"
#include "FileSystem/FileUtils.h"

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
			record.idx = m_classNames.size();
			m_classNames.push_back(className);
			m_classNameIds.insert({ className,record.idx });
		}
		else
		{
			record.idx = classNameIt->second;
		}
	}

	switch (m_mode)
	{
	case Serializer::MODE_BINARY: {
		record.idx = m_binaries.size();
		m_binaries.push_back({ uuid,std::move(std::make_unique<ByteStream>()),record });
		obj->SerializeToBinary(this, *m_binaries[record.idx].stream);
		break;
	}
	case Serializer::MODE_JSON: {
		record.idx = m_jsons.size();
		m_jsons.push_back({ uuid,std::move(std::make_unique<json>()),record });
		obj->SerializeToJson(this, *m_jsons[record.idx].j);
		break;
	}
	default:
		assert(0);
		break;
	}

	m_serializedObjects.insert({ uuid,record });
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
		assert(output0 != nullptr);
		assert(output1 == nullptr);
		assert(output2 == nullptr);
		*output0 = handle;
		return;
	}
	
	auto& rawOrShared = m_rawOrSharedDeserializedObjects[record.idx];
	if (rawOrShared.raw)
	{
		assert(output0 == nullptr);
		assert(output1 != nullptr);
		assert(output2 == nullptr);
		assert(rawOrShared.shared.get() == nullptr);
		*output1 = rawOrShared.raw;
		return;
	}

	if (rawOrShared.shared)
	{
		assert(output0 == nullptr);
		assert(output1 == nullptr);
		assert(output2 != nullptr);
		assert(rawOrShared.raw == nullptr);
		*output2 = rawOrShared.shared;
		return;
	}

	// this object isn't deserialized
	SerializedRecord::TYPE memType = SerializedRecord::HANDLE;
	if (output0)
	{
		memType = SerializedRecord::HANDLE;
	}

	if (output1)
	{
		memType = SerializedRecord::RAW;
	}

	if (output2)
	{
		memType = SerializedRecord::SHARED;
	}

	assert(memType == record.type);

	Serializable* obj = nullptr;
	auto dbRecord = SerializableDB::Get()->GetSerializableRecord(m_classNames[record.classNameIdx].c_str());
	if (dbRecord.name.empty())
	{
		assert(0 && "Register Serializable class to SerializableDB in SerializableList.h");
		*((int*)nullptr); // must crash here
	}

	if (memType == SerializedRecord::HANDLE)
	{
		auto h = dbRecord.ctor();
		m_deserializedObjects[record.idx] = h;
		obj = h;
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

	goto Begin;
}

void Serializer::WriteToFileJson(const String& path)
{
	json j;

	json meta;
	{
		auto arr = json::array();

		for (auto& className : m_classNames)
		{
			arr.push_back(className.c_str());
		}

		meta["UsedClassNames"] = arr;
	}

	meta["RootUUID"] = m_rootUUID;
	j["Meta"] = meta;
	
	{
		auto arr = json::array();
		auto count = m_jsons.size();
		for (size_t i = 0; i < count; i++)
		{
			auto& v = m_jsons[count];
			json& data = *v.j;
			json j1;
			j1["UUID"]			= v.uuid;
			j1["MemType"]		= v.record.type;
			j1["ClassNameId"]	= v.record.classNameIdx;
			j1["Data"]			= data;
			arr.push_back(j1);
		}
		j["Objects"] = arr;
	}

	auto str = j.dump();

	FileUtils::WriteFile(path.c_str(), str.c_str(), str.length());
}

void Serializer::WriteToFileBinary(const String& path)
{
}

void Serializer::ReadFromFileJson(const String& path)
{
	byte* buffer; size_t fileSize;
	FileUtils::ReadFile(path, buffer, fileSize);

	json j = json::parse((const char*)buffer);

	auto& meta = j["Meta"];
	{
		auto& arr = meta["UsedClassNames"];
		auto count = arr.size();
		for (size_t i = 0; i < count; i++)
		{
			m_classNames.push_back(arr[i].dump().c_str());
			m_classNameIds.insert({ m_classNames.back(),i });
		}
	}
	m_rootUUID = meta["RootUUID"];

	{
		auto& arr = j["Objects"];
		auto count = arr.size();
		for (size_t i = 0; i < count; i++)
		{
			auto& j1 = arr[i];
			json& data = j1["Data"];

			SerializedRecord record;
			record.idx			= i;
			record.classNameIdx = j1["ClassNameId"];
			record.type			= j1["MemType"];

			UUID uuid = j1["UUID"];
			m_serializedObjects.insert({ uuid,record });

			m_jsons.push_back({ uuid,std::move(std::make_unique<json>(data)),record });
		}

		m_deserializedObjects.Resize(count);
		m_rawOrSharedDeserializedObjects.resize(count);
	}

	FileUtils::FreeBuffer(buffer);
}

void Serializer::ReadFromFileBinary(const String& path)
{
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
		WriteToFileJson(path);
		break;
	}
	default:
		assert(0);
		break;
	}
}

void Serializer::ReadFromFile(const String& path)
{
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

void Serializer::SetRootUUID(const UUID& uuid)
{
	m_rootUUID = uuid;
}

NAMESPACE_END