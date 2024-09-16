#pragma once

#include "../MainSystem.h"

#include "Core/Structures/STD/STDContainers.h"
#include "Core/Structures/Managed/Array.h"
#include "Core/Structures/Raw/UnorderedList.h"

#include "TaskSystem/TaskSystem.h"

#include "Common/Base/AsyncTaskRunnerRaw.h"
#include "Runtime/Config.h"

#include "Scene/Scene.h"
#include "Scene/GameObjectDependenciesResolver.h"

#include "PhysicsClasses.h"
#include "Query/PhysicsQueryFilterCallback.h"

namespace physx
{
class PxScene;
class PxControllerManager;
}

NAMESPACE_BEGIN

class PhysicsComponent;
class Joint;
class ActionBase;
class ActionPhysicsQuery;
class ActionPhysicsSweep;

class API PhysicsSystem : public MainSystem
{
private:
	PHYSICS_FRIEND_CLASSES();

	friend class PhysXSimulationCallback;
	friend class CharacterControllerHitCallback;
	friend class PhysXSimulationFilterCallback;

	friend class ActionPhysicsSweep;
	friend class ActionPhysicsOverlap;

	constexpr static size_t NUM_DEFER_BUFFER = Config::NUM_DEFER_BUFFER;
	constexpr static size_t NUM_TRASH_ARRAY = 2;

	class PhysicsSystemDependenciesResolver : public GameObjectDependenciesResolver
	{
	public:
		virtual void Resolve(GameObjectDependenciesRecorder* recorder, GameObject* input) override;

	};

	raw::AsyncTaskRunner<PhysicsSystem> m_asyncTaskRunnerST[NUM_DEFER_BUFFER] = {};
	raw::AsyncTaskRunner<PhysicsSystem> m_asyncTaskRunnerMT[NUM_DEFER_BUFFER] = {};

	raw::AsyncTaskRunnerForMainComponent<PhysicsSystem> m_asyncTaskRunner[NUM_DEFER_BUFFER] = {};

	physx::PxScene* m_pxScene = nullptr;
	physx::PxControllerManager* m_pxControllerManager = nullptr;

	std::vector<PhysicsComponent*> m_prevUpdateList;
	std::vector<PhysicsComponent*> m_updateList;
	//std::vector<PhysicsComponent*> m_postUpdateList;

	std::vector<PhysicsComponent*> m_removePrevUpdateList;
	std::vector<PhysicsComponent*> m_removeUpdateList;
	//std::vector<PhysicsComponent*> m_removePostUpdateList;

	Spinlock m_prevUpdateListLock;
	Spinlock m_updateListLock;
	//Spinlock m_postUpdateListLock;
	bool m_isQueryAvailable = false;
	bool m_padd[1];

	float m_dt;

	Vec3 m_gravity = {};

	std::vector<PhysicsComponent*> m_activeComponentsHasContact;

	size_t m_physxSimulationCallback[8] = {};
	size_t m_physXSimulationFilterCallback[2] = {};

	Array<Handle<void>> m_trashComps[NUM_TRASH_ARRAY];
	size_t m_trashId = 0;

	std::vector<Joint*> m_brokenJoints;

	TaskWaitingHandle m_otherSubsystemsCallbackWaitingHandle = { 0,0 };

	std::atomic<uint32_t> m_numWritingQueries = 0;
	std::atomic<uint32_t> m_numWritingDirectQueries = 0;

	raw::ConcurrentArrayList<SharedPtr<ActionPhysicsQuery>> m_queries;

	struct SerialQueries
	{
		std::vector<SharedPtr<ActionPhysicsQuery>> queries;
		std::function<bool(PhysicsSystem*, const ActionPhysicsQuery*, const ActionPhysicsQuery*)> checker;
	};

	std::atomic<uint32_t> m_numWritingSerialQueries = 0;
	std::atomic<uint32_t> m_serialQueriesDebugBeginEndCall = 0;
	raw::ConcurrentArrayList<SerialQueries*> m_serialQueries;

private:
	TRACEABLE_FRIEND();
	inline void Trace(Tracer* tracer)
	{
		tracer->Trace(m_trashComps);
	}

public:
	PhysicsSystem(Scene* scene);
	~PhysicsSystem();
	virtual void Finalize() override;

private:
	inline auto* GetCurrentAsyncTaskRunnerST()
	{
		return &m_asyncTaskRunnerST[m_scene->GetCurrentDeferBufferIdx()];
	}

	inline auto* GetCurrentAsyncTaskRunnerMT()
	{
		return &m_asyncTaskRunnerMT[m_scene->GetCurrentDeferBufferIdx()];
	}

	inline auto* GetCurrentAsyncTaskRunner()
	{
		return &m_asyncTaskRunner[m_scene->GetCurrentDeferBufferIdx()];
	}

	inline auto* GetPrevAsyncTaskRunnerST()
	{
		return &m_asyncTaskRunnerST[m_scene->GetPrevDeferBufferIdx()];
	}

	inline auto* GetPrevAsyncTaskRunnerMT()
	{
		return &m_asyncTaskRunnerMT[m_scene->GetPrevDeferBufferIdx()];
	}

	inline auto* GetPrevAsyncTaskRunner()
	{
		return &m_asyncTaskRunner[m_scene->GetPrevDeferBufferIdx()];
	}

	inline auto& GetCurrentTrash()
	{
		return m_trashComps[m_trashId % NUM_TRASH_ARRAY];
	}

	inline auto& GetPrevTrash()
	{
		return m_trashComps[(m_trashId + NUM_TRASH_ARRAY - 1) % NUM_TRASH_ARRAY];
	}

	void SchedulePrevUpdateImpl(PhysicsComponent* comp);
	void UnschedulePrevUpdateImpl(PhysicsComponent* comp);

	void ScheduleUpdateImpl(PhysicsComponent* comp);
	void UnscheduleUpdateImpl(PhysicsComponent* comp);

	//void SchedulePostUpdateImpl(PhysicsComponent* comp);
	//void UnschedulePostUpdateImpl(PhysicsComponent* comp);

	void RebuildUpdateList();
	void ProcessPrevUpdateList();
	void ProcessUpdateList();
	//void ProcessPostUpdateList();

	void ProcessCollisionList();

public:
	// Inherited via MainSystem
	virtual void FlushAsyncTasks() override;

	virtual void BeginModification() override;

	virtual void AddComponent(MainComponent* comp) override;

	virtual void RemoveComponent(MainComponent* comp) override;

	virtual void OnObjectTransformChanged(MainComponent* comp) override;

	virtual void EndModification() override;

	virtual void PrevIteration() override;

	virtual void Iteration(float dt) override;

	virtual void PostIteration() override;

	inline auto* AsyncTaskRunnerST()
	{
		return GetCurrentAsyncTaskRunnerST();
	}

	inline auto* AsyncTaskRunnerMT()
	{
		return GetCurrentAsyncTaskRunnerMT();
	}

	inline auto* AsyncTaskRunner()
	{
		return GetCurrentAsyncTaskRunner();
	}

public:
	inline void SchedulePrevUpdate(PhysicsComponent* comp)
	{
		m_prevUpdateListLock.lock();
		SchedulePrevUpdateImpl(comp);
		m_prevUpdateListLock.unlock();
	}

	void UnschedulePrevUpdate(PhysicsComponent* comp);

	inline void ScheduleUpdate(PhysicsComponent* comp)
	{
		m_updateListLock.lock();
		ScheduleUpdateImpl(comp);
		m_updateListLock.unlock();
	}

	void UnscheduleUpdate(PhysicsComponent* comp);

	/*inline void SchedulePostUpdate(PhysicsComponent* comp)
	{
		m_postUpdateListLock.lock();
		SchedulePostUpdateImpl(comp);
		m_postUpdateListLock.unlock();
	}

	void UnschedulePostUpdate(PhysicsComponent* comp);*/

	inline auto& GetGravity() const
	{
		return m_gravity;
	}

	inline static SharedPtr<GameObjectDependenciesResolver> GetDependenciesResolver()
	{
		return std::make_shared<PhysicsSystemDependenciesResolver>();
	}

private:
	bool SweepImpl(PhysicsSweepResult& output, const PhysicsShape* shape, const Transform& startTransform, const Vec3& distance, PhysicsQueryFilterCallback* filter);
	bool OverlapImpl(PhysicsOverlapResult& output, const PhysicsShape* shape, const Transform& startTransform, PhysicsQueryFilterCallback* filter);

	template <typename T> 
	inline void RecordOrExecuteQuery(T& queryAction)
	{
		++m_numWritingQueries;

		if (m_isQueryAvailable)
		{
			--m_numWritingQueries;
			queryAction->ExecuteQuery();
			return;
		}

		m_queries.Add(queryAction);

		--m_numWritingQueries;
	}

	void FlushAllQueries();

	void ExecuteSerialQueries(SerialQueries* queries);
	void FlushAllSerialQueries();

public:
	using SweepResultCallback = std::function<void(const ActionPhysicsSweep*, const PhysicsSweepResult&)>;
	///
	/// SweepResultCallback is ensured to be execute in the next iteration when the query result is available
	/// 
	///	SweepResultCallback:
	/// + ActionPhysicsSweep: the return from Sweep() call, use to retrieve some infomation about the query
	/// + PhysicsSweepResult: the query result
	/// 
	/// Usage:
	///		actionExecution->RunAction(
	///			physicsSystem->Sweep(...)
	///		);
	/// 
	SharedPtr<ActionBase> Sweep(
		const SweepResultCallback& callback,
		const PhysicsShape* shape,
		const Transform& startTransform, 
		const Vec3& distance, 
		const SharedPtr<PhysicsQueryFilterCallback>& filter = nullptr
	);


	using OverlapResultCallback = std::function<void(const ActionPhysicsOverlap*, const PhysicsOverlapResult&)>;

	SharedPtr<ActionBase> Overlap(
		const OverlapResultCallback& callback,
		const PhysicsShape* shape,
		const Transform& transform,
		const SharedPtr<PhysicsQueryFilterCallback>& filter = nullptr
	);

///
/// >>>>>>>>>>>>>>>> Serial query section >>>>>>>>>>>>>>>>>
/// 
	using QueryPrevCheckCallback = std::function<bool(PhysicsSystem*, const ActionPhysicsQuery*, const ActionPhysicsQuery*)>;
	///
	/// This ensure all queries in a serial query will be executed by the same thread and in order
	/// 
	/// QueryPrevCheckCallback: use to check if this query should be executed
	///		+ return false to discard the query
	///		+ param 0 - ActionPhysicsQuery: prev query
	///		+ param 1 - ActionPhysicsQuery: current query
	///		* QueryPrevCheckCallback should not drain any data from persistent data like a script object, a component,...
	///			because it will be called from a different thread with the calling BeginSerialQuery() thread, 
	///			so, one should create local state and store it by lamda capturing
	///		* Inside QueryPrevCheckCallback, serial query functions, such as SerialSweep, SerialRayCast, can be called to append more query to the serial query
	/// 
	/// Example case: ray cast along a path if hit anything => break
	/// 
	/// Usage:
	///		auto serialId = BeginSerialQuery(...);
	/// 
	///		physicsSystem->SerialSweep(...);
	///		physicsSystem->SerialRayCast(...);
	/// 
	///		EndSerialQuery(serialId);
	///
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

///
/// <<<<<<<<<<<<<<<< End section <<<<<<<<<<<<<<<<<<<<
///


///
/// >>>>>>>>>>>>>>>> Direct query section >>>>>>>>>>>>>>>>>
/// * Allow client to query directly from PhysicsSystem, to implement a complex query case
/// 	
/// Example case: do ray cast till the total length of all rays are reached the a certain value
///
public:
	class DirectQueryInterface
	{
	private:
		friend class PhysicsSystem;

		PhysicsSystem* m_system = nullptr;

		inline DirectQueryInterface(PhysicsSystem* sys) : m_system(sys) {};
		inline ~DirectQueryInterface() {};

	public:
		bool Sweep(PhysicsSweepResult& output, const PhysicsShape* shape, const Transform& startTransform, const Vec3& distance, PhysicsQueryFilterCallback* filter);

	};
private:
	friend class DirectQueryInterface;

	struct QueryRecord
	{
		std::function<void(DirectQueryInterface*)> callback;
		ReentrantLock* lock = nullptr;
	};

	raw::ConcurrentArrayList<QueryRecord> m_directQueries;

	void ImplDirectQuery(const std::function<void(DirectQueryInterface*)>& callback, ReentrantLock* lock);
	void FlushAllDirectQueries();

public:
	using QueryCallback = std::function<void(DirectQueryInterface*)>;
	///
	/// + QueryCallback is ensured to be executed in the current iteration, a little while after Query() call 
	/// In the callback, client can use DirectQueryInterface to directly query the physics scene
	/// 
	/// + ReentrantLock, if being supplied, will be used before calling the callback
	/// 
	/// Usage:
	///		physicsSystem->Query(
	///			[&](DirectQueryInterface* interface)
	///			{
	///				interface->Sweep(...);
	///				interface->RayCast(...);
	///			}
	///		);
	///
	void Query(const QueryCallback& callback, ReentrantLock* lock = nullptr);

///
/// <<<<<<<<<<<<<<<< End section <<<<<<<<<<<<<<<<<<<<
///

};

NAMESPACE_END