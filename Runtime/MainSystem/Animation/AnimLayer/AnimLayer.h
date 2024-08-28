#pragma once

#include "Common/Base/Serializer.h"
#include "Common/Base/Serializable.h"

#include "MainSystem/Animation/Components/AnimationComponent.h"

#include "../Utils/Animation.h"

#include "Resources/AnimModel.h"

NAMESPACE_BEGIN

class API AnimLayer : public Serializable
{
private:
	friend class AnimatorSkeletalArray;
	friend class GameObject;
	friend class AnimationComponent;
	friend class AnimBlendLayer;

	bool m_isEnabled = true;
	bool m_padd[7];

	AnimationComponent* m_ownerComp = nullptr;

protected:
	AnimModel* m_model = nullptr;

	//std::vector<Mat4> m_localTransforms;
	std::vector<Mat4> m_globalTransforms;
	std::vector<AABox> m_meshesAABB;

	inline AnimLayer() {};

	inline void CloneFrom(Serializer* serializer, Serializable* another) override
	{
		auto src = (AnimLayer*)another;

		m_ownerComp = serializer->Clone(src->m_ownerComp);

		m_model = src->m_model;
		m_globalTransforms = src->m_globalTransforms;
		m_meshesAABB = src->m_meshesAABB;
	}

	inline void SerializeToJson(Serializer* serializer, json& j) const
	{
		j["OwnerComp"] = serializer->Serialize(m_ownerComp);
		j["Model"] = serializer->Serialize(m_model);

		j["GlobalTransformsSize"] = m_globalTransforms.size();
		j["MeshesAABBSize"] = m_meshesAABB.size();

		j["IsEnabled"] = m_isEnabled;
	}

	inline void DeserializeFromJson(Serializer* serializer, const json& j)
	{
		serializer->Deserialize(j["OwnerComp"], m_ownerComp);
		serializer->Deserialize(j["Model"], m_model);

		size_t size = j["GlobalTransformsSize"];
		m_globalTransforms.resize(size);
		//m_localTransforms.resize(size);

		size = j["MeshesAABBSize"];
		m_meshesAABB.resize(size);

		if (j.contains("IsEnabled"))
		{
			m_isEnabled = j["IsEnabled"];
		}
	}

	inline void SetEnabledImpl(bool enabled)
	{
		m_isEnabled = enabled;
	}

	inline bool IsEnabledImpl() const
	{
		return m_isEnabled;
	}

public:
	inline virtual ~AnimLayer() {};

	inline virtual void Initialize() {}; 
	inline virtual void PrevRun(float dt) {};
	virtual void Run(float dt) = 0;

	inline virtual AnimLayer* GetOutput()
	{
		return this;
	}

	inline auto& NodeGlobalTransforms()
	{
		return m_globalTransforms;
	}

	inline auto* GetAnimModel()
	{
		return m_model;
	}

	/*inline auto& NodeLocalTransforms()
	{
		return m_localTransforms;
	}*/

	inline auto& MeshesAABB()
	{
		return m_meshesAABB;
	}

	inline bool IsEnabled() const
	{
		return m_isEnabled;
	}

	void SetEnabled(bool enabled);

	inline auto* GetCommittedObject()
	{
		return m_ownerComp->GetCommittedObject();
	}

	inline auto* GetComponent()
	{
		return m_ownerComp;
	}

	template <typename T> 
	inline auto* GetComponentAs()
	{
		return (T*)m_ownerComp;
	}
};

NAMESPACE_END