#pragma once

#include "Core/TypeDef.h"
#include "Core/Pattern/Singleton.h"

NAMESPACE_BEGIN

struct UUID
{
	int64_t part0;
	int64_t part1;

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