#pragma once

#include "Common/Actions/ActionBase.h"

#include "Math/Math.h"

#include "PhysicsQueryFilterCallback.h"

NAMESPACE_BEGIN

class PhysicsSystem;

class ActionPhysicsSweep : public ActionBase
{
private:
	friend class PhysicsSystem;

	using SweepResultCallback = std::function<void(const ActionPhysicsSweep*, const PhysicsSweepResult&)>;

	size_t m_activeIteration = 0;
	PhysicsSystem* m_system = nullptr;

	SweepResultCallback m_callback;

	Vec3 m_startPosition;
	Quaternion m_startRotation;
	Vec3 m_sweepDistance;
	SharedPtr<PhysicsQueryFilterCallback> m_filter = nullptr;

	PhysicsSweepResult m_result;
	bool m_queryStatus = false;

	ActionPhysicsSweep(PhysicsSystem* sys, size_t activeIteration);

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

protected:
	// Inherited via ActionBase
	RETURN_CODE Update(float dt) override;

};

NAMESPACE_END