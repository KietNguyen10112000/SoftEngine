#pragma once
#include "Core/TypeDef.h"

#include "Core/Pattern/Singleton.h"

#include "API/API.h"

NAMESPACE_BEGIN

class Network : public Singleton<Network>
{
public:
	inline static void Initialize()
	{
		socketapi::Initialize();
	}

	inline static void Finalize()
	{
		socketapi::Finalize();
	}

};

NAMESPACE_END