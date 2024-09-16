#pragma once

#include "ActionPhysicsQuery.h"

#include "Math/Math.h"

#include "PhysicsQueryFilterCallback.h"

#include <functional>

NAMESPACE_BEGIN

class PhysicsSystem;

class ActionPhysicsOverlap : public ActionPhysicsQuery
{
private:
	friend class PhysicsSystem;

	using OverlapResultCallback = std::function<void(const ActionPhysicsOverlap*, const PhysicsOverlapResult&)>;

	OverlapResultCallback m_callback = nullptr;

	SharedPtr<PhysicsShape> m_shape = nullptr;
	Vec3 m_startPosition;
	Quaternion m_startRotation;
	SharedPtr<PhysicsQueryFilterCallback> m_filter = nullptr;

	PhysicsOverlapResult m_result;
	bool m_queryStatus = false;
	bool m_executedQuery = false;

	ActionPhysicsOverlap(PhysicsSystem* sys, size_t activeIteration);

	inline static SharedPtr<ActionPhysicsOverlap> Create(PhysicsSystem* sys, size_t activeIteration)
	{
		struct make_shared_enabler : public ActionPhysicsOverlap 
		{
			inline make_shared_enabler(PhysicsSystem* sys, size_t activeIteration) : ActionPhysicsOverlap(sys, activeIteration) {};
		};
		return std::make_shared<make_shared_enabler>(sys, activeIteration);
	}

	virtual void CallCallback() override;
	virtual void ExecuteQuery() override;

public:
	inline auto GetTransform() const
	{
		Transform ret = {};
		ret.Position() = m_startPosition;
		ret.Rotation() = m_startRotation;
		return ret;
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