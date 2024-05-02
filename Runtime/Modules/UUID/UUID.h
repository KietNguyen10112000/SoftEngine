#pragma once

#include "Core/TypeDef.h"
#include "Core/Pattern/Singleton.h"
#include "Core/Structures/String.h"

#include "Core/Memory/Memory.h"

#include <sstream>
#include <iomanip>

NAMESPACE_BEGIN

struct UUID
{
	int64_t part0 = 0;
	int64_t part1 = 0;

	inline bool operator>(const UUID& another) const
	{
		return (part0 == another.part0) ? (part1 > another.part1) : (part0 > another.part0);
	}

	inline bool operator<(const UUID& another) const
	{
		return (part0 == another.part0) ? (part1 < another.part1) : (part0 < another.part0);
	}

	inline bool operator==(const UUID& another) const
	{
		return (part0 == another.part0) && (part1 == another.part1);
	}

	inline String ToHexString() const
	{
		std::stringstream stream;
		stream //<< "0x"
			<< std::setfill('0') << std::setw(32)
			<< std::hex << part0 << part1;
		return stream.str().c_str();
	}

	inline static UUID FromHexString(const String& str)
	{
		UUID ret;

		std::string part0(("0x" + str.SubString(0, 16)).c_str());
		std::string part1(("0x" + str.SubString(16, 16)).c_str());

		{
			std::istringstream iss(part0);
			iss >> std::hex >> ret.part0;
		}

		{
			std::istringstream iss(part1);
			iss >> std::hex >> ret.part1;
		}

		return ret;
	}
};

// generate uuid v1
API class UUIDGenerator : public Singleton<UUIDGenerator>
{
private:
	int64_t m_MACAddress;

public:
	UUIDGenerator();

	UUID GetUUID();
};

NAMESPACE_END