#pragma once

#include "SubSystems/SubSystem.h"

NAMESPACE_BEGIN

class Scene;
class Script;

class ScriptSystem : public SubSystem
{
private:
	std::Vector<Script*> m_onGUI;

public:
	ScriptSystem(Scene* scene);

private:
	static void ForEachScript(ID, SubSystem*, GameObject*, void*);

public:
	virtual void PrevIteration(float dt) override;

	virtual void Iteration(float dt) override;

	virtual void PostIteration(float dt) override;

	virtual bool FilterAddSubSystemComponent(SubSystemComponent* comp) override;
	virtual bool FilterRemoveSubSystemComponent(SubSystemComponent* comp) override;

public:
	template <typename Func>
	inline void ForEachOnGUIScripts(Func func)
	{
		for (auto script : m_onGUI)
		{
			func(script);
		}
	}

};

NAMESPACE_END