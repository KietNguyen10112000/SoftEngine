#pragma once

#include "Core/TypeDef.h"

#include "Core/Structures/String.h"
#include "Core/Structures/STD/STDContainers.h"

NAMESPACE_BEGIN

class ByteStreamRead
{
protected:
	friend class Package;
	friend class PackageReceiver;
	friend class ByteStream;

	byte* m_beginRead = nullptr;
	byte* m_endRead = nullptr;
	byte* m_curRead = nullptr;

	/*inline void Initialize(byte* begin, size_t len)
	{
		m_begin		= begin;
		m_end		= begin + len;
		m_cur		= begin;
	}*/

public:
	inline ByteStreamRead() {};
	inline ByteStreamRead(byte* begin, size_t len)
	{
		m_beginRead = begin;
		m_endRead = begin + len;
		m_curRead = begin;
	}
	inline ByteStreamRead(ByteStream* stream);
	inline void BeginReadFrom(ByteStream* stream);

	inline void BeginReadFrom(byte* begin, size_t len)
	{
		m_beginRead = begin;
		m_endRead = begin + len;
		m_curRead = begin;
	}

	inline static ByteStreamRead From(ByteStream* stream)
	{
		return ByteStreamRead(stream);
	}

	inline static ByteStreamRead From(ByteStream& stream)
	{
		return ByteStreamRead(&stream);
	}

	inline byte*& BeginRead()
	{
		return m_beginRead;
	}

	inline byte*& EndRead()
	{
		return m_endRead;
	}

	inline byte*& CurRead()
	{
		return m_curRead;
	}

	inline size_t GetHeaderSize() const
	{
		return sizeof(uint32_t);
		//return 0;
	}

	inline void ResetRead()
	{
		CurRead() = BeginRead() + GetHeaderSize();
	}

#define BYTE_STREAM_ASSERT_NO_OVERFLOW(len) assert((m_curRead + len <= m_endRead) && "ByteStream overflow");

	inline const char* GetString()
	{
#ifdef _DEBUG
		bool valid = false;
		auto it = m_curRead;
		while (it != m_endRead)
		{
			if (*it == 0)
			{
				valid = true;
				break;
			}
			it++;
		}
		if (!valid)
		{
			assert(0 && "Invalid string");
		}
#endif // _DEBUG

		auto ret = (char*)m_curRead;
		m_curRead += strlen(ret) + 1;
		return ret;
	}

	template <typename T>
	inline T Get()
	{
		static_assert(std::is_const_v<T> == false, "Can not get const value");

		if constexpr (
			std::is_same_v<T, String>
			|| std::is_same_v<T, char*>)
		{
			return GetString();
		}

		BYTE_STREAM_ASSERT_NO_OVERFLOW(sizeof(T));
		auto p = (T*)m_curRead;
		m_curRead += sizeof(T);
		return *p;
	}

	template <typename T>
	inline void Pick(T& output)
	{
		output = Get<T>();
	}

	template <typename T>
	inline void Skip()
	{
		static_assert(std::is_const_v<T> == false, "Can not skip const value");

		if constexpr (
			std::is_same_v<T, String>
			|| std::is_same_v<T, char*>)
		{
			m_curRead += strlen((char*)m_curRead) + 1;
			return;
		}

		BYTE_STREAM_ASSERT_NO_OVERFLOW(sizeof(T));
		m_curRead += sizeof(T);
		return;
	}

	// return false if bufferSize is not enough
	template <typename T>
	inline bool GetArray(T* buffer, uint32_t bufferSize, uint32_t& outputSize)
	{
		auto savedCur = m_cur;
		auto count = Get<uint32_t>();

		if (count > bufferSize)
		{
			m_curRead = savedCur;
			return false;
		}

		for (uint32_t i = 0; i < count; i++)
		{
			buffer[i] = Get<T>();
		}
		outputSize = count;

		return true;
	}

	template <typename Vec>
	inline void GetSTLVector(Vec& output)
	{
		using T = typename std::decay<decltype(*output.begin())>::type;

		auto count = Get<uint32_t>();
		for (uint32_t i = 0; i < count; i++)
		{
			output.push_back(Get<T>());
		}
	}

	// return buffer that skipped
	inline byte* SkipBuffer(size_t bufferSize)
	{
		BYTE_STREAM_ASSERT_NO_OVERFLOW(bufferSize);
		auto ret = m_curRead;
		m_curRead += bufferSize;
		return ret;
	}

	inline void PickBuffer(byte* buffer, size_t bufferSize)
	{
		BYTE_STREAM_ASSERT_NO_OVERFLOW(bufferSize);

		std::memcpy(buffer, m_curRead, bufferSize);
		m_curRead += bufferSize;
	}

#undef BYTE_STREAM_ASSERT_NO_OVERFLOW

	inline bool IsEmpty()
	{
		assert(m_curRead <= m_endRead);
		return m_curRead == m_endRead;
	}

	inline size_t GetPayloadSize() const
	{
		return m_curRead - m_beginRead - sizeof(uint32_t);
	}

	inline size_t GetSize() const
	{
		return m_curRead - m_beginRead;
	}
};

class ByteStream : public ByteStreamRead
{
protected:
	friend class PackageSender;
	friend class FileSystem;
	friend class ByteStreamRead;

	std::vector<byte>	m_buffer;

	// m_endRead is CurWrite

	byte* m_endWrite = nullptr;

	inline auto& CurWrite()
	{
		return m_endRead;
	}

	inline auto& BeginWrite()
	{
		return m_beginRead;
	}

	inline auto& EndWrite()
	{
		return m_endWrite;
	}

	inline void GrowthBy(size_t size)
	{
		//auto ret = m_end - m_cur;
		if (m_endWrite - CurWrite() >= size)
		{
			return;
		}

		auto newSize = m_buffer.size() + size;
		m_buffer.resize(newSize);

		CurRead() = m_buffer.data() + (CurRead() - BeginRead());
		CurWrite() = m_buffer.data() + (CurWrite() - BeginWrite());
		BeginWrite() = m_buffer.data();
		EndWrite() = BeginWrite() + newSize;
	}

	inline bool SetHeaderSize(uint32_t size)
	{
		if (*(uint32_t*)BeginWrite() != 0) return false;

		*(uint32_t*)BeginWrite() = size;
		return true;
	}

	inline uint32_t PackSize()
	{
		auto size = CurWrite() - BeginWrite() - sizeof(uint32_t);
		assert(size != 0);
		if (!SetHeaderSize(size)) return CurWrite() - BeginWrite();
		Put<uint32_t>(size);
		return CurWrite() - BeginWrite();
	}

public:
	inline ByteStream()
	{
		GrowthBy(sizeof(uint32_t));

		// put placeholder for package size
		Put<uint32_t>(0);
	}

	inline ByteStream(size_t initSize)
	{
		GrowthBy(initSize + sizeof(uint32_t));

		// put placeholder for package size
		Put<uint32_t>(0);
	}

	inline void Resize(size_t newSize)
	{
		GrowthBy(newSize - m_buffer.size() + GetHeaderSize());
	}

	//// must be called before put anything
	//inline void Initialize(size_t initSize)
	//{
	//	GrowthBy(initSize + sizeof(uint32_t));

	//	// put placeholder for package size
	//	Put<uint32_t>(0);
	//}

	/*inline void Initialize(byte* begin, byte* cur)
	{
		size_t len = cur - begin;
		GrowthBy(len);
		std::memcpy(m_begin, begin, len);
		m_cur = m_begin + (cur - begin);
	}*/

	inline size_t PutString(size_t len, const char* str)
	{
		//len++;
		GrowthBy(len + 1);
		::memcpy(CurWrite(), str, len);
		CurWrite()[len] = 0;

		size_t ret = CurWrite() - BeginWrite();
		CurWrite() += len + 1;
		return ret;
	}

	inline auto Put(const char* str)
	{
		return PutString(::strlen(str), str);
	}

	template <typename T>
	inline auto Put(const T& v)
	{
		static_assert((!std::is_pointer_v<T>) || std::is_trivial_v<T> 
			|| std::is_same_v<T, String> || std::is_same_v<T, char*>,
			"Can not put non-trivial type");

		if constexpr (std::is_same_v<T, String>)
		{
			return PutString(v.length(), v.c_str());
		}

		if constexpr (std::is_same_v<T, const char*> || std::is_same_v<T, char*> || std::is_same_v<T, char[]>)
		{
			return PutString(::strlen(v), v);
		}

		GrowthBy(sizeof(T));
		auto p = (T*)CurWrite();
		size_t ret = CurWrite() - BeginWrite();
		CurWrite() += sizeof(T);
		*p = v;
		return ret;
	}

	// allowed up 4B elements
	template <typename T>
	inline auto PutArray(const T* arr, uint32_t count)
	{
		Put<uint32_t>(count);
		for (uint32_t i = 0; i < count; i++)
		{
			Put<T>(arr[i]);
		}
	}

	template <typename STLVector>
	inline auto PutSTLVector(STLVector& iterable)
	{
		Put<uint32_t>(std::distance(iterable.begin(), iterable.end()));
		for (auto& v : iterable)
		{
			Put(v);
		}
	}

	inline void ResetWrite()
	{
		CurWrite() = BeginWrite() + GetHeaderSize();
	}

	template <typename T>
	inline void Set(size_t idx, const T& v)
	{
		auto p = (T*)&BeginWrite()[idx];
		*p = v;
	}

	inline void Merge(ByteStream& another)
	{
		auto growthSize = another.CurWrite() - another.BeginWrite() - sizeof(uint32_t);

		if (growthSize == 0) return;

		GrowthBy(growthSize);

		memcpy(CurWrite(), another.BeginWrite() + sizeof(uint32_t), growthSize);
		CurWrite() += growthSize;
	}

	inline void Pack()
	{
		PackSize();
	}

	inline byte* PutBuffer(byte* buffer, size_t bufferSize)
	{
		GrowthBy(bufferSize);
		std::memcpy(CurWrite(), buffer, bufferSize);

		auto ret = CurWrite();
		CurWrite() += bufferSize;
		return ret;
	}
};

ByteStreamRead::ByteStreamRead(ByteStream* stream)
{
	BeginReadFrom(stream);
}

void ByteStreamRead::BeginReadFrom(ByteStream* stream)
{
	m_beginRead = stream->BeginWrite();
	m_endRead = stream->CurWrite();
	m_curRead = m_beginRead + stream->GetHeaderSize();
}

NAMESPACE_END