#pragma once

#include "Core/TypeDef.h"

#include "Core/Pattern/Singleton.h"

NAMESPACE_BEGIN


class Engine : public Singleton<Engine>
{
public:
	void Loop();

};


NAMESPACE_END