#pragma once

#include "Core/Structures/Managed/Function.h"
#include "Core/Structures/Managed/Array.h"

NAMESPACE_BEGIN

class AsyncTaskRunner : Traceable<AsyncTaskRunner>
{
public:
	Array<Handle<FunctionBase>> m_functions;

protected:
	TRACEABLE_FRIEND();
	inline void Trace(Tracer* tracer)
	{
		tracer->Trace(m_functions);
	}

	inline void Flush()
	{
		for (auto& func : m_functions)
		{
			func->Invoke();
		}
		m_functions.Clear();
	}

public:
	template <typename Fn, typename... Args>
	inline auto RunAsync(Fn fn, Args&&... args)
	{
		auto ret = MakeAsyncFunction(fn, std::forward<Args>(args)...);
		m_functions.Push(ret);
		return ret;
	}

};

NAMESPACE_END