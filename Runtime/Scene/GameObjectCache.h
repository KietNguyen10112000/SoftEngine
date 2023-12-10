#pragma once

#include "Common/Utils/GenericDictionary.h"

NAMESPACE_BEGIN

class GameObjectCache
{
private:
	GenericDictionary m_dict;

	TRACEABLE_FRIEND();
	inline void Trace(Tracer* tracer)
	{
		tracer->Trace(m_dict);
	}

public:
	inline Handle<GameObject> Get(String name)
	{
		return m_dict.Get<GameObject>(name);
	}

	inline void Store(String name, const Handle<GameObject>& obj)
	{
		m_dict.Store(name, obj);
	}

	inline void Remove(String name)
	{
		m_dict.Remove(name);
	}
};

NAMESPACE_END