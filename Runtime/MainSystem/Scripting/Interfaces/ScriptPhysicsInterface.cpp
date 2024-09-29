#include "ScriptPhysicsInterface.h"

#include "../Components/Script.h"

NAMESPACE_BEGIN

ScriptPhysicsInterface::ScriptPhysicsInterface(Script* script) : m_script(script)
{

}

SharedPtr<ActionBase> ScriptPhysicsInterface::Sweep(const SweepResultCallback& callback, 
	const PhysicsShape* shape, const Transform& startTransform, const Vec3& distance, 
	const SharedPtr<PhysicsQueryFilterCallback>& filter)
{
	auto physicsSystem = m_script->GetScene()->GetPhysicsSystem();
	return physicsSystem->Sweep(callback, shape, startTransform, distance, filter);
}

SharedPtr<ActionBase> ScriptPhysicsInterface::Overlap(const OverlapResultCallback& callback, const PhysicsShape* shape, const Transform& transform, const SharedPtr<PhysicsQueryFilterCallback>& filter)
{
	auto physicsSystem = m_script->GetScene()->GetPhysicsSystem();
	return physicsSystem->Overlap(callback, shape, transform, filter);
}

ID ScriptPhysicsInterface::BeginSerialQuery(const QueryPrevCheckCallback& prevCheckCallback)
{
	auto physicsSystem = m_script->GetScene()->GetPhysicsSystem();
	return physicsSystem->BeginSerialQuery(prevCheckCallback);
}

void ScriptPhysicsInterface::EndSerialQuery(ID serialQueryID)
{
	auto physicsSystem = m_script->GetScene()->GetPhysicsSystem();
	return physicsSystem->EndSerialQuery(serialQueryID);
}

SharedPtr<ActionPhysicsQuery> ScriptPhysicsInterface::SerialSweep(
	ID serialQueryID, 
	const SweepResultCallback& callback, 
	const PhysicsShape* shape, 
	const Transform& startTransform, 
	const Vec3& distance, 
	const SharedPtr<PhysicsQueryFilterCallback>& filter)
{
	auto physicsSystem = m_script->GetScene()->GetPhysicsSystem();
	return physicsSystem->SerialSweep(serialQueryID, callback, shape, startTransform, distance, filter);
}

SharedPtr<ActionPhysicsQuery> ScriptPhysicsInterface::SerialOverlap(ID serialQueryID, const OverlapResultCallback& callback, const PhysicsShape* shape, const Transform& transform, const SharedPtr<PhysicsQueryFilterCallback>& filter)
{
	auto physicsSystem = m_script->GetScene()->GetPhysicsSystem();
	return physicsSystem->SerialOverlap(serialQueryID, callback, shape, transform, filter);
}

void ScriptPhysicsInterface::Query(const QueryCallback& callback)
{
	auto physicsSystem = m_script->GetScene()->GetPhysicsSystem();
	return physicsSystem->Query(callback, &m_script->m_lock);
}

NAMESPACE_END