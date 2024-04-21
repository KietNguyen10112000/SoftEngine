#pragma once

#include "../Utils/Animation.h"

NAMESPACE_BEGIN

class AnimLayer
{
private:
	friend class AnimatorSkeletalArray;
	friend class GameObject;

	bool m_isEnable = true;
	bool m_padd[7];

	GameObject* m_owner = nullptr;

protected:
	AnimModel* m_model = nullptr;

	std::vector<Mat4> m_globalTransforms;
	std::vector<AABox> m_meshesAABB;

	inline AnimLayer() {};

public:
	inline virtual ~AnimLayer() {};

	virtual void Run(float dt) = 0;

	virtual AnimLayer* MakeInstance() = 0;

	inline virtual void CloneFrom(AnimLayer* anotherLayer)
	{
		m_globalTransforms = anotherLayer->m_globalTransforms;
		m_meshesAABB = anotherLayer->m_meshesAABB;
	}

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
		return m_owner;
	}

};

NAMESPACE_END