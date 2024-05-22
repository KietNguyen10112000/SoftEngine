#pragma once

#include "Metadata.h"

#include "Core/Memory/SmartPointers.h"

#include "../Stream/ByteStream.h"

#include "Common/Base/AsyncTaskRunnerRaw.h"

#include "UUID/UUID.h"
#include "JSON/JSON.h"

#include "Resources/Resource.h"

NAMESPACE_BEGIN

class API Serializer
{
public:
	enum MODE
	{
		MODE_BINARY,
		MODE_JSON
	};

private:
	constexpr static size_t DEBUG_SIGN = 0xffeeddffaaffccbb;

	struct SerializedRecord
	{
		enum TYPE
		{
			HANDLE	= 0,
			MANAGED = 0,
			SHARED	= 1,
			RAW		= 2
		};

		ID idx;
		uint32_t classNameIdx;
		uint16_t type;
		uint16_t heapId = 0;
	};

	struct SerializedResourceRecord : public SerializedRecord
	{
		Resource<ResourceBase> resource;

		inline const SerializedRecord& ToBase() const
		{
			return *this;
		}
	};

	struct DeserializedPtr
	{
		SharedPtr<Serializable> shared = nullptr;
		Serializable* raw = nullptr;
	};

	struct SerializedBinary
	{
		UUID uuid;
		UniquePtr<ByteStream> stream;
		SerializedRecord record;
	};

	struct SerializedJson
	{
		UUID uuid;
		UniquePtr<json> j;
		SerializedRecord record;
	};

	//ByteStream* m_byteStream = nullptr;

	//ByteStreamRead m_read;

	//// to convert from Debug version to Release version
	//bool m_debug;
	//bool m_padd[3];

	//std::map<ID, void*> m_IDMap;

	//std::map<void*, void*> m_addressMap;

	//raw::AsyncTaskRunner<Serializer> m_asyncTaskRunner;

	std::vector<SerializedBinary> m_binaries;
	std::vector<SerializedJson> m_jsons;
	std::map<UUID, SerializedRecord> m_serializedObjects;

	Array<Handle<Serializable>> m_deserializedObjects;
	std::vector<DeserializedPtr> m_rawOrSharedDeserializedObjects;

	Array<Handle<Serializable>> m_clonedObjects;
	std::vector<DeserializedPtr> m_rawOrSharedCloneObjects;
	std::map<UUID, SerializedRecord> m_clonedObjectIds;

	std::map<String, ID> m_classNameIds;
	std::vector<String> m_classNames;

	const MODE m_mode = MODE::MODE_JSON;

	std::vector<UUID> m_rootUUIDs;

	std::map<UUID, SerializedResourceRecord> m_usedResources;

	byte m_stableValuesMap[256] = {};

private:
	TRACEABLE_FRIEND();
	inline void Trace(Tracer* tracer)
	{
		tracer->Trace(m_deserializedObjects);
		tracer->Trace(m_clonedObjects);
	}

public:
	Serializer(MODE mode = MODE::MODE_JSON);
	~Serializer();

private:
	void TrySerialize(
		Serializable* obj,
		SerializedRecord::TYPE type
	);

	void TrySerializeRC(
		ResourceBase* rc
	);

	void TryDeserialize(
		const UUID& uuid, 
		Handle<Serializable>* output0,
		Serializable** output1,
		SharedPtr<Serializable>* output2
	);

	void TryDeserializeRC(
		const UUID& uuid,
		Resource<ResourceBase>* output0
	);

	void TryClone(
		Serializable* obj,
		Handle<Serializable>* output0,
		Serializable** output1,
		SharedPtr<Serializable>* output2
	);

	void WriteToFileJson(const String& path);
	void WriteToFileBinary(const String& path);

	void ReadFromFileJson(const String& path);
	void ReadFromFileBinary(const String& path);

	void SetStableValuesMap(byte* map);

public:
	template <typename T>
	Handle<T> Clone(const Handle<T>& obj)
	{
		Handle<Serializable> ret;
		TryClone(obj, &ret, nullptr, nullptr);
		return DynamicCast<T>(ret);
	}

	template <typename T>
	T* Clone(T* obj)
	{
		Serializable* ret = nullptr;
		TryClone(obj, nullptr, &ret, nullptr);
		return dynamic_cast<T*>(ret);
	}

	template <typename T>
	SharedPtr<T> Clone(SharedPtr<T> obj)
	{
		SharedPtr<Serializable> ret;
		TryClone(obj.get(), nullptr, nullptr, &ret);
		return std::dynamic_pointer_cast<T>(ret);
	}

	template <typename T>
	UUID Serialize(const Handle<T>& obj)
	{
		static_assert(std::is_base_of_v<Serializable, T>);

		TrySerialize(obj, SerializedRecord::HANDLE);
		return obj->GetUUID();
	}

	template <typename T>
	UUID Serialize(const SharedPtr<T>& obj)
	{
		static_assert(std::is_base_of_v<Serializable, T>);

		TrySerialize(obj.get(), SerializedRecord::SHARED);
		return obj->GetUUID();
	}

	template <typename T>
	UUID Serialize(T* obj)
	{
		static_assert(std::is_base_of_v<ResourceBase, T> || std::is_base_of_v<Serializable, T>);

		if constexpr (std::is_base_of_v<ResourceBase, T>)
		{
			TrySerializeRC(obj);
		}
		else
		{
			TrySerialize(obj, SerializedRecord::RAW);
		}

		return obj->GetUUID();
	}

	template <typename T>
	UUID Serialize(const Resource<T>& rc)
	{
		TrySerializeRC(rc);
		return rc->GetUUID();
	}

	template <typename T>
	Handle<T> Deserialize(const UUID& uuid)
	{
		static_assert(std::is_base_of_v<Serializable, T>);

		Handle<Serializable> ret = nullptr;
		TryDeserialize(uuid, &ret, nullptr, nullptr);
		return DynamicCast<T>(ret);
	}

	template <typename T>
	void Deserialize(const UUID& uuid, Handle<T>& output)
	{
		static_assert(std::is_base_of_v<Serializable, T>);

		Handle<Serializable> ret;
		TryDeserialize(uuid, &ret, nullptr, nullptr);
		output = DynamicCast<T>(ret);
	}

	template <typename T>
	void Deserialize(const UUID& uuid, T*& output)
	{
		static_assert(std::is_base_of_v<ResourceBase, T> || std::is_base_of_v<Serializable, T>);

		if constexpr (std::is_base_of_v<ResourceBase, T>)
		{
			Resource<ResourceBase> ret;
			TryDeserializeRC(uuid, &ret);
			output = resource::StaticCast<T>(ret);
		}
		else
		{
			Serializable* ret = nullptr;
			TryDeserialize(uuid, nullptr, &ret, nullptr);
			output = dynamic_cast<T*>(ret);
		}
	}

	template <typename T>
	void Deserialize(const UUID& uuid, SharedPtr<T>& output)
	{
		static_assert(std::is_base_of_v<Serializable, T>);

		SharedPtr<Serializable> ret;
		TryDeserialize(uuid, nullptr, nullptr, &ret);
		output = std::dynamic_pointer_cast<T>(ret);
	}

	template <typename T>
	void Deserialize(const UUID& uuid, Resource<T>& output)
	{
		static_assert(std::is_base_of_v<ResourceBase, T>);

		Resource<ResourceBase> ret;
		TryDeserializeRC(uuid, &ret);
		output = resource::StaticCast<T>(ret);
	}

public:
	void WriteToFile(const String& path);
	void ReadFromFile(const String& path);

	void SetRootUUID(const UUID& uuid, ID id = 0);

	inline const auto& GetRootUUID(ID id = 0) const
	{
		return m_rootUUIDs[id];
	}

	template <typename T> 
	inline static auto CloneObject(const T& v)
	{
		Serializer s = {};
		return s.Clone(v);
	}

};

NAMESPACE_END