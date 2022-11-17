#pragma once

#include <atomic>

#include "Core/TypeDef.h"

NAMESPACE_BEGIN

class ThreadID
{
private:
	API static size_t _Get();
	API static void Finalize();

	struct Finalizer
	{
		inline ~Finalizer()
		{
			Finalize();
		}
	};

	inline static thread_local size_t s_id = -1;// _Get();
	inline static thread_local Finalizer s_finalizer;

public:
	inline static void InitializeForThisThread()
	{
		if (s_id == -1) s_id = _Get();
	}

public:
	inline static size_t Get()
	{
		return s_id;
	}

};

NAMESPACE_END