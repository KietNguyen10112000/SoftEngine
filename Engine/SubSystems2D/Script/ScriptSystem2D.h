#pragma once

#include "../SubSystem2D.h"

NAMESPACE_BEGIN

class Scene2D;
class Script2D;
class GameObject2D;

class ScriptSystem2D : public SubSystem2D
{
private:
	std::Vector<Script2D*> m_onGUI;

	std::Vector<Script2D*> m_onCollide;
	std::Vector<Script2D*> m_onCollisionEnter;
	std::Vector<Script2D*> m_onCollisionExit;

	std::Vector<GameObject2D*> m_onUpdate;

public:
	ScriptSystem2D(Scene2D* scene);

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

};

NAMESPACE_END