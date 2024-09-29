#pragma once

#include "MainSystem/Physics/PhysicsSystem.h"

NAMESPACE_BEGIN

class Script;

class API ScriptPhysicsInterface
{
private:
	friend class Script;

	Script* m_script = nullptr;

	ScriptPhysicsInterface(Script* script);

public:
	using SweepResultCallback = PhysicsSystem::SweepResultCallback;
	// see PhysicsSystem::Sweep
	SharedPtr<ActionBase> Sweep(
		const SweepResultCallback& callback,
		const PhysicsShape* shape,
		const Transform& startTransform,
		const Vec3& distance,
		const SharedPtr<PhysicsQueryFilterCallback>& filter = nullptr
	);

	using OverlapResultCallback = PhysicsSystem::OverlapResultCallback;
	SharedPtr<ActionBase> Overlap(
		const OverlapResultCallback& callback,
		const PhysicsShape* shape,
		const Transform& transform,
		const SharedPtr<PhysicsQueryFilterCallback>& filter = nullptr
	);

public:
	using QueryPrevCheckCallback = PhysicsSystem::QueryPrevCheckCallback;
	// see PhysicsSystem::BeginSerialQuery
	ID BeginSerialQuery(const QueryPrevCheckCallback& prevCheckCallback);
	void EndSerialQuery(ID serialQueryID);

	SharedPtr<ActionPhysicsQuery> SerialSweep(
		ID serialQueryID,
		const SweepResultCallback& callback,
		const PhysicsShape* shape,
		const Transform& startTransform,
		const Vec3& distance,
		const SharedPtr<PhysicsQueryFilterCallback>& filter = nullptr
	);

	SharedPtr<ActionPhysicsQuery> SerialOverlap(
		ID serialQueryID,
		const OverlapResultCallback& callback,
		const PhysicsShape* shape,
		const Transform& transform,
		const SharedPtr<PhysicsQueryFilterCallback>& filter = nullptr
	);

public:
	using QueryCallback = PhysicsSystem::QueryCallback;
	// see PhysicsSystem::Query
	void Query(const QueryCallback& callback);

};

NAMESPACE_END