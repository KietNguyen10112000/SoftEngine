#pragma once

#include "Common/Actions/ActionBase.h"

#include "Math/Math.h"

#include "PhysicsQueryFilterCallback.h"

NAMESPACE_BEGIN

class PhysicsSystem;

class ActionPhysicsQuery : public ActionBase
{
	friend class PhysicsSystem;
	size_t m_activeIteration = 0;

protected:
	PhysicsSystem* m_system = nullptr;

public:
	ActionPhysicsQuery(PhysicsSystem* sys, size_t activeIteration);
	inline virtual ~ActionPhysicsQuery() {};

private:
	// Inherited via ActionBase
	RETURN_CODE Update(float dt) override;

protected:
	virtual void CallCallback() = 0;
	virtual void ExecuteQuery() = 0;

};

NAMESPACE_END