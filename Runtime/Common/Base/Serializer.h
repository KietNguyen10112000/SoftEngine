#pragma once

#include "Metadata.h"

#include "../Stream/ByteStream.h"


NAMESPACE_BEGIN

class API Serializer
{
private:
	constexpr static size_t DEBUG_SIGN = 0xffeeddffaaffccbb;

	ByteStream* m_byteStream = nullptr;

	ByteStreamRead m_read;

	// to convert from Debug version to Release version
	bool m_debug;
	bool m_padd[3];

	std::map<ID, void*> m_IDMap;

	std::set<void*, void*> m_addressMap;

public:
	~Serializer();

private:
	Handle<Serializable> Deserialize(const char* className);

public:
	void Reset(ByteStream* byteStream, bool debug = true);

	void Serialize(Serializable* object);

	template <typename SerializableType>
	Handle<SerializableType> Deserialize()
	{
		auto ret = Deserialize(SerializableType::___GetClassName());
		return DynamicCast<SerializableType>(ret);
	}

	inline auto& GetWriteStream()
	{
		return *m_byteStream;
	}

	inline auto GetReadStream()
	{
		return m_read;
	}

	// to store shared resource, each serializable has a UUID, from which, we can detect whenever shared resource is serialized/deserialized
	inline auto& GetIDMap()
	{
		return m_IDMap;
	}

	inline auto& GetAddressMap()
	{
		return m_addressMap;
	}

};

NAMESPACE_END