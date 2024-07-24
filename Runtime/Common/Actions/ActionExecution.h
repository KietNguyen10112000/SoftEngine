#pragma once

#include "ActionBase.h"

NAMESPACE_BEGIN

// run actions in parallel
class API ActionExecution : public ActionCompound
{
public:
	// Inherited via ActionBase
	RETURN_CODE Update(float dt) override;

public:
	void RunAction(const SharedPtr<ActionBase>& action);
	void RunActions(const std::vector<SharedPtr<ActionBase>>& actions);

	void StopAction(ActionBase* action);
	void StopAllActions();

	inline void StopAction(const SharedPtr<ActionBase>& action)
	{
		StopAction(action.get());
	}

public:
	static SharedPtr<ActionExecution> New(const std::vector<SharedPtr<ActionBase>>& actions);

};

NAMESPACE_END