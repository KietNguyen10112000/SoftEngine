#pragma once

#include "../Physics.h"

NAMESPACE_BEGIN

class Collider;

class RigidBody : public Physics
{
public:
	RigidBody(TYPE type, const SharedPtr<Collider>& collider) : Physics(type, collider) {};

};

NAMESPACE_END