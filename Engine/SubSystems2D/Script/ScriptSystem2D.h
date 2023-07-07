#pragma once

#include "../SubSystem2D.h"

#include "Components2D/Script/PhysicsInterfaceStructs.h"

NAMESPACE_BEGIN

class Scene2D;
class Script2D;
class GameObject2D;

class API ScriptSystem2D : public SubSystem2D
{
private:
	friend class Script2D;

	std::Vector<Script2D*> m_onGUI;

	std::Vector<Script2D*> m_onCollide;
	std::Vector<Script2D*> m_onCollisionEnter;
	std::Vector<Script2D*> m_onCollisionExit;
	std::Vector<GameObject2D*> m_onUpdate;

	Scene2DQuerySession* m_querySession;

	// single thread, single rayQueryInfo
	Ray2DQueryInfo m_rayQueryInfo;

	ColliderQueryInfo m_colliderQueryInfo;

public:
	ScriptSystem2D(Scene2D* scene);
	~ScriptSystem2D();

public:
	virtual void PrevIteration(float dt) override;
	virtual void Iteration(float dt) override;
	virtual void PostIteration(float dt) override;
	virtual void AddSubSystemComponent(SubSystemComponent2D* comp) override;
	virtual void RemoveSubSystemComponent(SubSystemComponent2D* comp) override;

public:
	template <typename Func>
	inline void ForEachOnGUIScripts(Func func)
	{
		for (auto script : m_onGUI)
		{
			func(script);
		}
	}

	template <typename Func>
	inline void ForEachOnCollideScripts(Func func)
	{
		for (auto script : m_onCollide)
		{
			func(script);
		}
	}

public:
	// don't delete result, just use it
	Ray2DQueryInfo* RayQuery(const Vec2& begin, const Vec2& end, size_t sortLevel);
	ColliderQueryInfo* ColliderQuery(Collider2D* collider, const Transform2D& transform);

};

NAMESPACE_END