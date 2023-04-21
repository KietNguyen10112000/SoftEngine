#pragma once

#include "Components/SubSystemComponent.h"
#include "Components/SubSystemComponentId.h"

#include "Objects/Scene/Scene.h"

#include "Objects/GameObject.h"

NAMESPACE_BEGIN

class API Script : public SubSystemComponent
{
public:
	constexpr static size_t COMPONENT_ID = SubSystemComponentId::SCRIPT_SUBSYSTEM_COMPONENT_ID;


	friend class ScriptSystem;
	friend class GameObject;

private:
	ID m_scriptId	= INVALID_ID;
	ID m_onGUIId	= INVALID_ID;

protected:
	Scene* m_scene = nullptr;

	GameObject::_Transform* m_transform;

private:
	virtual void OnComponentAddedToScene() final override;

	// Inherited via SubSystemComponent
	virtual void OnComponentAdded() override;

	virtual void OnComponentRemoved() override;

	virtual void OnComponentRemovedFromScene() override;

	virtual void SetAsMain() override;

	virtual void SetAsExtra() override;

	virtual void ResolveBranch() override;

	virtual bool IsNewBranch() override;

	inline void BeginUpdate()
	{
		auto& transform = m_object->m_transform;
		m_transform = transform.GetWriteHead();
		auto read = transform.GetReadHead();
		*m_transform = *read;
	}

	inline void EndUpdate()
	{
		auto& transform = m_object->m_transform;
		transform.UpdateReadWriteHead(m_scene->GetIterationCount());

		auto read = transform.GetReadHead();

		if (::memcmp(m_transform, read, sizeof(*m_transform)) != 0)
		{
			m_object->ScheduleRefresh();
		}
	}

	inline void Update(float dt)
	{
		BeginUpdate();
		OnUpdate(dt);
		EndUpdate();
	}

public:
	virtual void OnStart() {};
	virtual void OnUpdate(float dt) {};
	virtual void OnGUI() {};

public:
	inline auto Input()
	{
		return m_scene->GetInput();
	}

	inline Transform& Transform()
	{
		return m_transform->transform;
	}

	inline Mat4& TransformMat4()
	{
		return m_transform->mat;
	}

};

NAMESPACE_END