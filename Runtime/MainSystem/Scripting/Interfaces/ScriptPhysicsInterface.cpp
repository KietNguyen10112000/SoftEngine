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

void ScriptPhysicsInterface::Query(const QueryCallback& callback)
{
	auto physicsSystem = m_script->GetScene()->GetPhysicsSystem();
	return physicsSystem->Query(callback, &m_script->m_lock);
}

NAMESPACE_END