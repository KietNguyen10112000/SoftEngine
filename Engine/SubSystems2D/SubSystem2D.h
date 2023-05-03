#pragma once

#include "Core/TypeDef.h"

#include "Core/Memory/Memory.h"
#include "Core/Structures/STD/STDContainers.h"

#include "TaskSystem/TaskSystem.h"

#include "Objects2D/Scene2D/Scene2D.h"
//#include "Objects2D/GameObjectDirectAccessor.h"

NAMESPACE_BEGIN

class Scene2D;
class GameObject2D;
class SubSystemComponent2D;

class SubSystem2D
{
protected:
	Scene2D* m_scene;
	const union
	{
		ID COMPONENT_ID;
		ID SUBSYSTEM_ID;
	};
	
public:
	inline SubSystem2D(Scene2D* scene, ID subSystemID) : m_scene(scene), COMPONENT_ID(subSystemID) {};
	inline virtual ~SubSystem2D() {};

public:
	// call before scene reconstruction, so all scene query methods can not be use
	virtual void PrevIteration(float dt) = 0;

	// call after scene reconstruction
	virtual void Iteration(float dt) = 0;

	// call after Iteration =)))
	virtual void PostIteration(float dt) = 0;

public:
	virtual void AddSubSystemComponent(SubSystemComponent2D* comp) = 0;
	virtual void RemoveSubSystemComponent(SubSystemComponent2D* comp) = 0;

public:
	inline auto GetScene() const
	{
		return m_scene;
	}

};

NAMESPACE_END