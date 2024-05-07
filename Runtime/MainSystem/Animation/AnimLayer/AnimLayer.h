#pragma once

#include "Common/Base/Serializer.h"
#include "Common/Base/Serializable.h"

#include "MainSystem/Animation/Components/AnimationComponent.h"

#include "../Utils/Animation.h"

NAMESPACE_BEGIN

class AnimLayer : public Serializable
{
private:
	friend class AnimatorSkeletalArray;
	friend class GameObject;
	friend class AnimationComponent;

	bool m_isEnable = true;
	bool m_padd[7];

	AnimationComponent* m_ownerComp = nullptr;

protected:
	AnimModel* m_model = nullptr;

	std::vector<Mat4> m_globalTransforms;
	std::vector<AABox> m_meshesAABB;

	inline AnimLayer() {};

	inline void CloneFrom(Serializer* serializer, Serializable* another) override
	{
		auto src = (AnimLayer*)another;

		m_ownerComp = serializer->Clone(src->m_ownerComp);

		m_model				= src->m_model;
		m_globalTransforms	= src->m_globalTransforms;
		m_meshesAABB		= src->m_meshesAABB;
	}

public:
	inline virtual ~AnimLayer() {};

	virtual void Run(float dt) = 0;

	inline virtual AnimLayer* GetOutput()
	{
		return this;
	}

	inline auto& NodeGlobalTransforms()
	{
		return m_globalTransforms;
	}

	inline auto& MeshesAABB()
	{
		return m_meshesAABB;
	}

	inline bool IsEnable() const
	{
		return m_isEnable;
	}

	inline void SetEnable(bool enable)
	{
		m_isEnable = enable;
	}

	inline auto* GetGameObject()
	{
		return m_ownerComp->GetGameObject();
	}

	inline auto* GetComponent()
	{
		return m_ownerComp;
	}

};

NAMESPACE_END