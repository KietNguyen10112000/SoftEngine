#pragma once

#include "Core/TypeDef.h"

#include "Core/Structures/STD/STDContainers.h"
#include "Math/Math.h"

NAMESPACE_BEGIN

class AABBQueryTester;

class AABBQueryStructure
{
public:
	inline virtual ~AABBQueryStructure() {};

public:
	// non thread-safe methods
	virtual ID Add(const AABox& aabb, void* userPtr) = 0;
	// remove what returned by Add()
	virtual void Remove(ID id) = 0;

public:
	// thread-safe methods
	// output contains what u added by Add()
	virtual void QueryAABox(const AABox& aabox, std::Vector<void*>& output) = 0;
	virtual void QuerySphere(const Sphere& sphere, std::Vector<void*>& output) = 0;
	virtual void QueryBox(const Box& box, std::Vector<void*>& output) = 0;
	virtual void Query(AABBQueryTester* tester, std::Vector<void*>& output) = 0;
};


NAMESPACE_END