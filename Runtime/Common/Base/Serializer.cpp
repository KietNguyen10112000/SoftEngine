#include "Serializer.h"

#include "SerializableDB.h"

NAMESPACE_BEGIN

Serializer::~Serializer()
{
	
}

Handle<Serializable> Serializer::Deserialize(const char* className)
{
	auto reader = ByteStreamRead(m_byteStream->CurRead(), m_byteStream->GetRemainReadSize());

	if (m_debug)
	{
		auto sign = reader.Get<size_t>();

		// reached this assertion means serialize and deserialize order mismatch
		assert(sign == DEBUG_SIGN);
	}

	size_t serializedSize = reader.Get<size_t>();

	auto& read = m_read;
	read.BeginReadFrom(m_byteStream->CurRead(), serializedSize);

	auto record = SerializableDB::Get()->GetSerializableRecord(className);
	auto ret = record.ctor();
	ret->Deserialize(this);
	return ret;
}

void Serializer::Reset(ByteStream* byteStream, bool debug)
{
	m_byteStream = byteStream;
	m_debug = debug;
}

void Serializer::Serialize(Serializable* object)
{
	m_byteStream->Put<size_t>(DEBUG_SIGN);
	auto idx = m_byteStream->Put<size_t>(0);

	auto size = m_byteStream->GetCurrentWriteSize();

	object->Serialize(this);

	auto serializedSize = m_byteStream->GetCurrentWriteSize() - size;

	m_byteStream->Set(idx, serializedSize);
}

NAMESPACE_END