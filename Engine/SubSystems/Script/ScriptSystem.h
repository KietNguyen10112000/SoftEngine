#pragma once

#include "SubSystems/SubSystem.h"

NAMESPACE_BEGIN

class Scene;
class Script;

class ScriptSystem : public SubSystem
{
protected:
	struct ScriptTaskParam
	{
		SubSystemMergingUnit* mergingUnit;
		Script** begin;
		Script** end;
		float dt;
		float pad;
	};

	std::Vector<Script*> m_scripts;

	ScriptTaskParam m_tasksParam[ThreadLimit::MAX_THREADS];
	Task m_tasks[ThreadLimit::MAX_THREADS];

public:
	ScriptSystem(Scene* scene);

public:
	virtual void PrevIteration(float dt) override;

	virtual void Iteration(float dt) override;

	virtual void PostIteration(float dt) override;

public:
	void AddScript(Script* script);
	void RemoveScript(Script* script);

};

NAMESPACE_END