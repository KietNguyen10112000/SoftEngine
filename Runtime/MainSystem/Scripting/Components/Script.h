#pragma once

#include "MainSystem/MainSystemInfo.h"
#include "Common/Base/MainComponent.h"

#include "../ScriptMeta.h"

#include "Scene/Scene.h"
#include "Scene/GameObject.h"

#include <bitset>

NAMESPACE_BEGIN

#define SCRIPT_DEFAULT_METHOD(clazz)															\
COMPONENT_CLASS(clazz);																			\
TRACEABLE_FRIEND();																				\
template <typename ChildClass>																	\
friend static void Script::_InitializeScriptMetaData(ScriptMetaData* metaData);					\
private:																						\
	virtual ScriptMetaData* GetScriptMetaData() override										\
	{																							\
		static ScriptMetaData* metaData = nullptr;												\
		if (metaData == nullptr)																\
		{																						\
			metaData = new ScriptMetaData();metaData->className = GetClassName();				\
			_InitializeScriptMetaData<clazz>(metaData);											\
			ScriptMeta::Get()->RegisterScriptMetaData(metaData);								\
		}																						\
		return 	metaData;																		\
																								\
	};																							\
public:

class Script : public MainComponent
{
private:
	friend class ScriptScheduler;
	friend class GameObject;
	MAIN_SYSTEM_FRIEND_CLASSES();

	constexpr static ID COMPONENT_ID = MainSystemInfo::SCRIPTING_ID;

	ID m_onGUIId		= INVALID_ID;
	ID m_onUpdateId		= INVALID_ID;
	ID m_onCollideId	= INVALID_ID;

	Scene* m_scene;

	// Inherited via MainComponent
	virtual void Serialize(ByteStream& stream) override;

	virtual void Deserialize(ByteStreamRead& stream) override;

	virtual void CleanUp() override;

	virtual Handle<ClassMetadata> GetMetadata(size_t sign) override;

	virtual void OnPropertyChanged(const UnknownAddress& var) override;

	virtual const char* GetClassName() override;

	virtual void OnComponentAdded() override;

	virtual void OnComponentRemoved() override;

	virtual void OnTransformChanged() override;

	virtual AABox GetGlobalAABB() override;

protected:
#define SET_VTB_OVERRIDDEN_IDX(_where, funcName, idxName)						\
{																				\
	auto f = &Script::funcName;													\
	auto f1 = &ChildClass::funcName;											\
	if constexpr (!std::is_same<decltype(f), decltype(f1)>::value)				\
	{																			\
		_where.set(idxName);													\
	}																			\
}

	template <typename ChildClass>
	inline static void _InitializeScriptMetaData(ScriptMetaData* metaData)
	{
		auto& vtbIds = metaData->overriddenVtbIdx;
		SET_VTB_OVERRIDDEN_IDX(vtbIds, OnGUI, ScriptMeta::Get()->onGUIVtbIdx);
		//SET_VTB_OVERRIDDEN_IDX(OnCollide, ScriptMeta::Get().onCollideVtbIdx);
		//SET_VTB_OVERRIDDEN_IDX(OnCollisionEnter, ScriptMeta::Get().onCollisionEnterVtbIdx);
		//SET_VTB_OVERRIDDEN_IDX(OnCollisionExit, ScriptMeta::Get().onCollisionExitVtbIdx);
		SET_VTB_OVERRIDDEN_IDX(vtbIds, OnUpdate, ScriptMeta::Get()->onUpdateVtbIdx);
	}

#undef SET_VTB_OVERRIDDEN_IDX

	virtual ScriptMetaData* GetScriptMetaData() = 0;

protected:
	virtual void OnStart();
	virtual void OnUpdate(float dt);
	//virtual void OnCollide(class GameObject* another, const Collision2DPair& pair);
	//virtual void OnCollisionEnter(class GameObject* another, const Collision2DPair& pair);
	//virtual void OnCollisionExit(class GameObject* another, const Collision2DPair& pair);
	virtual void OnGUI();

	inline auto GetScene()
	{
		return m_scene;
	}

	inline auto Input()
	{
		return m_scene->GetInput();
	}

	inline const auto& GetLocalTransform()
	{
		return GetGameObject()->ReadLocalTransform();
	}

	inline auto SetLocalTransform(const Transform& transform)
	{
		GetGameObject()->SetLocalTransform(transform);
	}

};

NAMESPACE_END