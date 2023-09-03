#pragma once

#include "Core/TypeDef.h"

#include "MainSystemInfo.h"

NAMESPACE_BEGIN

class GameObject;
class MainComponent;

class MainSystem
{
private:
	MAIN_SYSTEM_FRIEND_CLASSES();
	
public:
	virtual ~MainSystem() {};

protected:
	// direct implementation
	virtual void AddComponent(MainComponent* comp) = 0;
	virtual void RemoveComponent(MainComponent* comp) = 0;
	virtual void OnObjectTransformChanged(MainComponent* comp) = 0;

};

NAMESPACE_END