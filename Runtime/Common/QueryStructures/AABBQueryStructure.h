#pragma once

#include "Core/TypeDef.h"

#include "Math/Math.h"

#include "AABBQuerySession.h"

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
	virtual void Clear() = 0;

	// child class must implement with mediator pattern
	virtual AABBQuerySession* NewSession() = 0;
	virtual void DeleteSession(AABBQuerySession* session) = 0;

public:
	// thread-safe methods
	// output contains what u added by Add()
	virtual void QueryAABox(const AABox& aabox, AABBQuerySession* session) = 0;
	virtual void QuerySphere(const Sphere& sphere, AABBQuerySession* session) = 0;
	virtual void QueryBox(const Box& box, AABBQuerySession* session) = 0;
	virtual void QueryFrustum(const Frustum& frustum, AABBQuerySession* session) = 0;
	virtual void Query(AABBQueryTester* tester, AABBQuerySession* session) = 0;

};


NAMESPACE_END