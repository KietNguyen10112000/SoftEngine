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
			HANDLE,
			SHARED,
			RAW
		};

		ID idx;
		uint32_t classNameIdx;
		uint32_t type;
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

	std::map<String, ID> m_classNameIds;
	std::vector<String> m_classNames;

	const MODE m_mode = MODE::MODE_JSON;

	UUID m_rootUUID = {};

	std::vector<Resource<ResourceBase>> m_resourceHolder;

private:
	TRACEABLE_FRIEND();
	inline void Trace(Tracer* tracer)
	{
		tracer->Trace(m_deserializedObjects);
	}

public:
	Serializer(MODE mode = MODE::MODE_JSON);
	~Serializer();

private:
	void TrySerialize(
		Serializable* obj,
		SerializedRecord::TYPE type
	);

	void TryDeserialize(
		const UUID& uuid, 
		Handle<Serializable>* output0,
		Serializable** output1,
		SharedPtr<Serializable>* output2
	);

	void WriteToFileJson(const String& path);
	void WriteToFileBinary(const String& path);

	void ReadFromFileJson(const String& path);
	void ReadFromFileBinary(const String& path);

public:
	template <typename T>
	Handle<T> Clone(const Handle<T>& obj)
	{
		/*static_assert(std::is_base_of_v<Serializable, T>);

		auto ret = DynamicCast<T>(obj->_MakeInstance());
		ret->CloneFrom(this, obj);
		return ret;*/
		return nullptr;
	}

	template <typename T>
	T* Clone(T* obj)
	{
		/*static_assert(std::is_base_of_v<Serializable, T>);

		auto ret = dynamic_cast<T>(obj->_MakeInstanceRaw());
		ret->CloneFrom(this, obj);
		return ret;*/
		return nullptr;
	}

	template <typename T>
	SharedPtr<T> Clone(SharedPtr<T> obj)
	{
		/*static_assert(std::is_base_of_v<Serializable, T>);

		auto ret = std::dynamic_pointer_cast<T>(obj->_MakeInstanceShared());
		ret->CloneFrom(this, obj.get());
		return ret;*/
		return nullptr;
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
		static_assert(std::is_base_of_v<Serializable, T>);

		TrySerialize(obj, SerializedRecord::RAW);
		return obj->GetUUID();
	}

	template <typename T>
	Handle<T> Deserialize(const UUID& uuid)
	{
		static_assert(std::is_base_of_v<Serializable, T>);

		Handle<T> ret = nullptr;
		TryDeserialize(uuid, &ret, nullptr, nullptr);
		return ret;
	}

	template <typename T>
	void Deserialize(const UUID& uuid, Handle<T>& output)
	{
		static_assert(std::is_base_of_v<Serializable, T>);

		TryDeserialize(uuid, &output, nullptr, nullptr);
	}

	template <typename T>
	void Deserialize(const UUID& uuid, T*& output)
	{
		static_assert(std::is_base_of_v<Serializable, T>);

		TryDeserialize(uuid, nullptr, &output, nullptr);
	}

	template <typename T>
	void Deserialize(const UUID& uuid, SharedPtr<T>& output)
	{
		static_assert(std::is_base_of_v<Serializable, T>);

		TryDeserialize(uuid, nullptr, nullptr, &output);
	}

public:
	void WriteToFile(const String& path);
	void ReadFromFile(const String& path);

	void SetRootUUID(const UUID& uuid);

	inline const auto& GetRootUUID() const
	{
		return m_rootUUID;
	}

	inline auto& GetResourceHolder()
	{
		return m_resourceHolder;
	}

};

NAMESPACE_END