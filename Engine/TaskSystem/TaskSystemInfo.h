#pragma once

#include "Core/TypeDef.h"

NAMESPACE_BEGIN

class TaskSystemInfo
{
public:
	constexpr static size_t WORKERS_COUNT = 6;
	constexpr static size_t QUEUE_CAPACITY = 4 * KB;

};

NAMESPACE_END