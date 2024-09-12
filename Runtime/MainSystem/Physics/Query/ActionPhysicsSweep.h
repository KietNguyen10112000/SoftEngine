#pragma once

#include "ActionPhysicsQuery.h"

#include "Math/Math.h"

#include "PhysicsQueryFilterCallback.h"

#include <functional>

NAMESPACE_BEGIN

class PhysicsSystem;

class ActionPhysicsSweep : public ActionPhysicsQuery
{
private:
	friend class PhysicsSystem;

	using SweepResultCallback = std::function<void(const ActionPhysicsSweep*, const PhysicsSweepResult&)>;

	SweepResultCallback m_callback = nullptr;

	SharedPtr<PhysicsShape> m_shape = nullptr;
	Vec3 m_startPosition;
	Quaternion m_startRotation;
	Vec3 m_sweepDistance;
	SharedPtr<PhysicsQueryFilterCallback> m_filter = nullptr;

	PhysicsSweepResult m_result;
	bool m_queryStatus = false;
	bool m_executedQuery = false;

	ActionPhysicsSweep(PhysicsSystem* sys, size_t activeIteration);

	inline static SharedPtr<ActionPhysicsSweep> Create(PhysicsSystem* sys, size_t activeIteration)
	{
		struct make_shared_enabler : public ActionPhysicsSweep 
		{
			inline make_shared_enabler(PhysicsSystem* sys, size_t activeIteration) : ActionPhysicsSweep(sys, activeIteration) {};
		};
		return std::make_shared<make_shared_enabler>(sys, activeIteration);
	}

	virtual void CallCallback() override;
	virtual void ExecuteQuery() override;

public:
	inline auto GetStartTransform() const
	{
		Transform ret = {};
		ret.Position() = m_startPosition;
		ret.Rotation() = m_startRotation;
		return ret;
	}

	inline auto GetSweepDistance() const
	{
		return m_sweepDistance;
	}

	inline bool Status() const
	{
		return m_queryStatus;
	}

	inline bool HitOrTouchAnything() const
	{
		return m_queryStatus;
	}

};

NAMESPACE_END