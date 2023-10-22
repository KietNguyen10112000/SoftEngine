#pragma once

#include "Core/TypeDef.h"

#include "Core/Structures/STD/STDContainers.h"

NAMESPACE_BEGIN

class Script;

class ScriptScheduler
{
public:
	std::vector<Script*> m_onGUIs;
	std::vector<Script*> m_onUpdates;

public:
	ScriptScheduler();

	void CallOnUpdate(float dt);
	void CallOnGUI();

};

NAMESPACE_END