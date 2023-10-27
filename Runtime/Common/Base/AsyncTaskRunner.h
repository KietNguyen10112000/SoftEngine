#pragma once

#include "Core/Structures/Managed/Function.h"
#include "Core/Structures/Managed/Array.h"
#include "Core/Structures/Managed/ConcurrentList.h"

NAMESPACE_BEGIN

class AsyncTaskRunner : Traceable<AsyncTaskRunner>
{
public:
	ConcurrentList<Handle<FunctionBase>> m_functions;

protected:
	TRACEABLE_FRIEND();
	inline void Trace(Tracer* tracer)
	{
		tracer->Trace(m_functions);
	}

	inline void Flush()
	{
		m_functions.ForEach([](Handle<FunctionBase>& func) { func->Invoke(); });
		m_functions.Clear();
	}

public:
	// thread-safe, use this function to do scripts communication
	template <typename Fn, typename... Args>
	inline auto RunAsync(Fn fn, Args&&... args)
	{
		auto ret = MakeAsyncFunction(fn, std::forward<Args>(args)...);
		m_functions.Add(ret);
		return ret;
	}

};

NAMESPACE_END