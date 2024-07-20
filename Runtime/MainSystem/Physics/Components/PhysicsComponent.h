#pragma once

#include "MainSystem/MainSystemInfo.h"
#include "Common/Base/MainComponent.h"

#include "PHYSICS_TYPE.h"
#include "PHYSICS_FLAG.h"

#include "Math/Math.h"

#include "Scene/DeferredBuffer.h"

#include "../Collision/Collision.h"

#include "../PhysicsClasses.h"

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

	// callback: void (const SharedPtr<CollisionContact>&, const SharedPtr<CollisionContactPair>& , const CollisionContactPoint&);
	template <typename Fn>
	inline void ForEachBeginContactPoints(Fn callback)
	{
		auto& _collision = *collision.Read();
		for (auto& contact : _collision.contacts)
		{
			auto& ids = contact->beginContactPairsIds;
			auto& pairs = contact->contactPairs;
			for (auto idx : ids)
			{
				auto& pair = pairs[idx];
				for (auto& point : pair->contactPoints)
				{
					callback(contact, pair, point);
				}
			}
		}
	}

	// callback: void (const SharedPtr<CollisionContact>&, const SharedPtr<CollisionContactPair>&);
	template <typename Fn>
	inline void ForEachBeginContactPairs(Fn callback)
	{
		auto& _collision = *collision.Read();
		for (auto& contact : _collision.contacts)
		{
			auto& ids = contact->beginContactPairsIds;
			auto& pairs = contact->contactPairs;
			for (auto idx : ids)
			{
				auto& pair = pairs[idx];
				callback(contact, pair);
			}
		}
	}

	// callback: void (const SharedPtr<CollisionContact>&, const SharedPtr<CollisionContactPair>&);
	template <typename Fn>
	inline void ForEachEndContactPairs(Fn callback)
	{
		auto& _collision = *collision.Read();

		for (auto& contact : _collision.endContacts)
		{
			auto& pairs = contact->contactPairs;
			for (auto& pair : pairs)
			{
				callback(contact, pair);
			}
		}

		for (auto& contact : _collision.contacts)
		{
			if (contact->oldCollisionContact == (void*)INVALID_ID)
			{
				continue;
			}

			auto& ids = contact->endContactPairsIds;
			auto& pairs = contact->oldCollisionContact->contactPairs;
			for (auto idx : ids)
			{
				auto& pair = pairs[idx];
				callback(contact, pair);
			}
		}
	}

	// callback: void (const SharedPtr<CollisionContact>&, const SharedPtr<CollisionContactPair>&);
	template <typename Fn>
	inline void ForEachContactPairs(Fn callback)
	{
		auto& _collision = *collision.Read();
		for (auto& contact : _collision.contacts)
		{
			for (auto& pair : contact->contactPairs)
			{
				//if (pair->contactPoints.size() != 0)
				assert(pair->contactPoints.size() != 0);

				callback(contact, pair);
			}
		}
	}

	// callback: void (const SharedPtr<CollisionContact>&)
	template <typename Fn>
	inline void ForEachBeginContacts(Fn callback)
	{
		auto& _collision = *collision.Read();
		for (auto& contact : _collision.contacts)
		{
			if (contact->oldCollisionContact == (void*)INVALID_ID)
			{
				callback(contact);
			}
		}
	}

	// callback: void (const SharedPtr<CollisionContact>&)
	template <typename Fn>
	inline void ForEachEndContacts(Fn callback)
	{
		auto& _collision = *collision.Read();
		for (auto& contact : _collision.endContacts)
		{
			callback(contact);
		}
	}

	// callback: void (const SharedPtr<CollisionContact>&)
	template <typename Fn>
	inline void ForEachContacts(Fn callback)
	{
		auto& _collision = *collision.Read();
		for (auto& contact : _collision.contacts)
		{
			callback(contact);
		}
	}

	inline auto GetBeginContactPairsCount()
	{
		return collision.Read()->beginContactPairsCount;
	}

	inline auto GetEndContactPairsCount()
	{
		return collision.Read()->endContactPairsCount;
	}

	inline auto GetContactPairsCount()
	{
		return collision.Read()->contactPairsCount;
	}

	inline auto GetContactPointsCount()
	{
		return collision.Read()->contactPointsCount;
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
	friend class PhysXSimulationFilterCallback;
	friend class CharacterControllerHitCallback;
	MAIN_SYSTEM_FRIEND_CLASSES();
	PHYSICS_FRIEND_CLASSES();
	constexpr static ID COMPONENT_ID = MainSystemInfo::PHYSICS_ID;

protected:
	struct ScheduleUpdateInfo
	{
		uint32_t prevUpdateId = uint32_t(INVALID_ID);
		bool isPrevUpdateIdRemoved = false;
		bool padd0[3];

		uint32_t updateId = uint32_t(INVALID_ID);
		bool isUpdateIdRemoved = false;
		bool padd1[3];

		uint32_t postUpdateId = uint32_t(INVALID_ID);
		bool isPostUpdateIdRemoved = false;
		bool padd2[3];
	};

private:
	size_t m_physicsFlag = 0;

	GameObject* m_lastGameObject = nullptr;

protected:
	physx::PxActor* m_pxActor = nullptr;

	PhysicsCollisionResult* m_collisionResult = nullptr;

	uint32_t m_refContactIdx[8] = {
		(uint32_t)INVALID_ID,
		(uint32_t)INVALID_ID,
		(uint32_t)INVALID_ID,
		(uint32_t)INVALID_ID,

		(uint32_t)INVALID_ID,
		(uint32_t)INVALID_ID,
		(uint32_t)INVALID_ID,
		(uint32_t)INVALID_ID,
	};

public:
	virtual ~PhysicsComponent();

	virtual PHYSICS_TYPE GetPhysicsType() const = 0;

protected:
	virtual void OnPhysicsTransformChanged() = 0;

	inline virtual void OnPrevUpdate(float dt) {};

	// called before PhysX fetchResults, use PhysicsSystem::ScheduleUpdate() to schedule update
	inline virtual void OnUpdate(float dt) {};

	// called after PhysX fetchResults, use PhysicsSystem::SchedulePostUpdate() to schedule post update
	inline virtual void OnPostUpdate(float dt) {};

	inline virtual void OnPhysicsFlagSetted(PHYSICS_FLAG flag, bool value) {};

	inline auto& ScheduleUpdateInfo()
	{
		return *(struct ScheduleUpdateInfo*)&m_doubleBVHId[0];
	}

	inline uint32_t& PrevUpdateId()
	{
		return ScheduleUpdateInfo().prevUpdateId;
	}

	inline bool& IsPrevUpdateIdRemoved()
	{
		return ScheduleUpdateInfo().isPrevUpdateIdRemoved;
	}

	inline uint32_t& UpdateId()
	{
		return ScheduleUpdateInfo().updateId;
	}

	inline bool& IsUpdateIdRemoved()
	{
		return ScheduleUpdateInfo().isUpdateIdRemoved;
	}

	/*inline uint32_t& PostUpdateId()
	{
		return ScheduleUpdateInfo().postUpdateId;
	}

	inline bool& IsPostUpdateIdRemoved()
	{
		return ScheduleUpdateInfo().isPostUpdateIdRemoved;
	}*/

	bool HasCollisionContactPairsBegin();
	bool HasCollisionContactPairsEnd();
	bool HasCollisionContactPairs();
	//bool HasCollisionModified();
	bool HasCollisionAnyChanged();

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

protected:
	void CloneFrom(Serializer* serializer, Serializable* another) override;
	void SerializeToBinary(Serializer* serializer, ByteStream& stream) const override;
	void DeserializeFromBinary(Serializer* serializer, const ByteStream& stream) override;
	void SerializeToJson(Serializer* serializer, json& j) const override;
	void DeserializeFromJson(Serializer* serializer, const json& j) override;

};

NAMESPACE_END