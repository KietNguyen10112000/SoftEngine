#pragma once

#include <vector>

struct PhysicsMaterial
{
public:
	float mass = 0;
	float friction = 0.5;
	float restitution = 0.5;

};

struct CompoundPhysicsMaterial
{
public:
	PhysicsMaterial self;
	std::vector<PhysicsMaterial> childs;

};