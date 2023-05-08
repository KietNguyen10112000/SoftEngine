#pragma once

#include "SubSystems2D/SubSystem2D.h"

//#include "Objects/Physics/Collision/Manifold.h"
#include "Objects2D/Physics/Collision/Collision2DPair.h"

NAMESPACE_BEGIN

class Scene2D;

class PhysicsSystem2D : public SubSystem2D
{
private:
	size_t m_iterationCount = 0;

	Scene2DQuerySession* m_querySession;

	// game objects that have dynamic physics component
	std::Vector<Physics2D*> m_boardPhaseEntries;
	std::Vector<Physics2D*> m_boardPhaseStack;

	std::Vector<Collision2DPair*> m_narrowPhaseEntries;

	std::Vector<Collision2DPair*> m_freePairs;
	std::Vector<Collision2DPair*> m_allocatedPairs;

public:
	PhysicsSystem2D(Scene2D* scene);
	~PhysicsSystem2D();

protected:
	inline void FreeCollisionPairs(std::Vector<Collision2DPair*>& pairs)
	{
		for (auto& p : pairs)
		{
			if ((--(p->refCount)) == 0)
			{
				m_freePairs.push_back(p);
			}
		}
		pairs.clear();
	}

	inline auto AllocateCollisionPair(Physics2D* A, Physics2D* B, size_t refCount)
	{
		Collision2DPair* ret;
		if (!m_freePairs.empty())
		{
			ret = m_freePairs.back();
			m_freePairs.pop_back();
			goto Return;
		}

		ret = rheap::New<Collision2DPair>();
		ret->id = m_allocatedPairs.size();
		m_allocatedPairs.push_back(ret);
		
	Return:
		ret->A = A;
		ret->B = B;
		ret->refCount = refCount;
		ret->result.penetration = 0;

		m_narrowPhaseEntries.push_back(ret);

		return ret;
	}

	/*inline void DeallocateCollisionPair(Collision2DPair* pair)
	{
		m_freePairs.push_back(pair);
	}*/

	void BoardPhase();
	void SolveAllCollisionPairs();
	void NarrowPhase();

public:
	virtual void PrevIteration(float dt) override;
	virtual void Iteration(float dt) override;
	virtual void PostIteration(float dt) override;
	virtual void AddSubSystemComponent(SubSystemComponent2D* comp) override;
	virtual void RemoveSubSystemComponent(SubSystemComponent2D* comp) override;

};

NAMESPACE_END