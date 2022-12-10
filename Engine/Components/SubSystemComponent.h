#pragma once

#include "Core/TypeDef.h"

NAMESPACE_BEGIN

template <typename T>
class SubSystemComponent
{
protected:
	friend class GameObject;

	static size_t s_id;

};

class Renderer;
API SubSystemComponent<Renderer>;

NAMESPACE_END