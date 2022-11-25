#pragma once

#include <atomic>
//#include <unordered_map>

#include "Core/TypeDef.h"


NAMESPACE_BEGIN

class ThreadID
{
private:
	API static size_t _Get();
	API static void Finalize();

	inline static thread_local size_t s_id = -1;// _Get();

	//API static std::unordered_map<size_t, size_t> s_idMap;

public:
	inline static void InitializeForThisThread()
	{
		if (s_id == -1) s_id = _Get();
	}

	inline static void FinalizeForThisThread()
	{
		Finalize();
	}

public:
	inline static size_t Get()
	{
		return s_id;
	}

};

NAMESPACE_END