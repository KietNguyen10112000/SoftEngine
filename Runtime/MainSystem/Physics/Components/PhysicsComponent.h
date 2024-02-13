#pragma once

#include "MainSystem/MainSystemInfo.h"
#include "Common/Base/MainComponent.h"

#include "PHYSICS_TYPE.h"
#include "PHYSICS_FLAG.h"

#include "Math/Math.h"

#include "Scene/DeferredBuffer.h"

#include "../Collision/Collision.h"

//#include <bitset>

namespace physx
{
	class PxActor;
}

NAMESPACE_BEGIN

struct PhysicsCollisionResult
{
	// collision.Read() is collisions that currenly be on this PhysicsComponent
	DeferredBuffer<Collision, 3> collision;

	size_t lastActiveIterationCount = 0;

	// callback: void (const SharedPtr<CollisionContactPair>& contact);
	template <typename Fn>
	inline void ForEachCollisionBegin(Fn callback)
	{
		auto& _collision = *collision.Read();
		auto count = _collision.collisionBeginCount;

		for (size_t i = 0; i < count; i++)
		{
			callback(_collision.contactPairs[i]);
		}
	}

	// callback: void (const SharedPtr<CollisionContactPair>& contact);
	template <typename Fn>
	inline void ForEachCollisionEnd(Fn callback)
	{
		auto& _prevCollision = *(collision.Buffers()[(collision.GetWriteIdx() + 3 - 2) % 3]);
		auto& _curCollision = *collision.Read();

		for (auto idx : _curCollision.collisionEnd)
		{
			callback(_prevCollision.contactPairs[idx]);
		}
	}

	// callback: void (const SharedPtr<CollisionContactPair>& contact);
	template <typename Fn>
	inline void ForEachCollision(Fn callback)
	{
		auto& _collision = *collision.Read();
		for (auto& e : _collision.contactPairs)
		{
			callback(e);
		}
	}

	inline auto GetCollisionBeginCount()
	{
		return collision.Read()->collisionBeginCount;
	}

	inline auto GetCollisionEndCount()
	{
		return collision.Read()->collisionEnd.size();
	}

	inline auto GetCollisionCount()
	{
		return collision.Read()->contactPairs.size();
	}

	inline void Clear()
	{
		auto buffers = collision.Buffers();
		for (size_t i = 0; i < 3; i++)
		{
			buffers[i].Clear();
		}
	}
};

class API PhysicsComponent : public MainComponent
{
private:
	friend class GameObject;
	friend class PhysXSimulationCallback;
	friend class CharacterControllerHitCallback;
	MAIN_SYSTEM_FRIEND_CLASSES();
	constexpr static ID COMPONENT_ID = MainSystemInfo::PHYSICS_ID;

private:
	size_t m_physicsFlag = 0;

protected:
	physx::PxActor* m_pxActor = nullptr;

	PhysicsCollisionResult* m_collisionResult = nullptr;

	bool isInPrevFrame[8] = {};

public:
	virtual ~PhysicsComponent();

	virtual PHYSICS_TYPE GetPhysicsType() const = 0;

	inline virtual void OnDrawDebug() {};

protected:
	virtual void OnPhysicsTransformChanged() = 0;

	// called before PhysX fetchResults, use PhysicsSystem::ScheduleUpdate() to schedule update
	inline virtual void OnUpdate(float dt) {};

	// called after PhysX fetchResults, use PhysicsSystem::SchedulePostUpdate() to schedule post update
	inline virtual void OnPostUpdate(float dt) {};

	inline size_t& UpdateId()
	{
		return m_doubleBVHId[0].bvhId;
	}

	inline bool& IsUpdateIdRemoved()
	{
		return *(bool*)&m_doubleBVHId[0].ulistId;
	}

	inline size_t& PostUpdateId()
	{
		return m_doubleBVHId[1].bvhId;
	}

	inline bool& IsPostUpdateIdRemoved()
	{
		return *(bool*)&m_doubleBVHId[1].ulistId;
	}

	bool HasCollisionBegin();
	bool HasCollisionEnd();
	bool HasCollision();

	inline auto* GetCollision()
	{
		return m_collisionResult->collision.Read();
	}
	
public:
	void SetPhysicsFlag(PHYSICS_FLAG flag, bool value);

	inline bool HasPhysicsFlag(PHYSICS_FLAG flag) const
	{
		return 0 != (m_physicsFlag & flag);
	}

};

NAMESPACE_END