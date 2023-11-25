#include "AnimationSystem.h"

#include "Resources/AnimModel.h"

#include "MainSystem/Animation/Components/AnimSkeletalGameObject.h"

NAMESPACE_BEGIN

AnimationSystem::AnimationSystem(Scene* scene) : MainSystem(scene)
{
}

AnimationSystem::~AnimationSystem()
{
}

void AnimationSystem::AddAnimMeshRenderingBuffer(void* p, Animator* animator)
{
	auto& animMeshRenderingBuffer = *(SharedPtr<AnimModel::AnimMeshRenderingBuffer>*)p;
	auto& id = animMeshRenderingBuffer->id;

	if (id != INVALID_ID)
	{
		assert(p == m_animMeshRenderingBufferCount[id].p);
		assert(animator == m_animMeshRenderingBufferCount[id].animator);

		m_animMeshRenderingBufferCount[id].count++;
		return;
	}

	id = m_animMeshRenderingBufferCount.size();
	m_animMeshRenderingBufferCount.push_back({ 1,p,animator });
}

void AnimationSystem::RemoveMeshRenderingBuffer(void* p, Animator* animator)
{
	auto& animMeshRenderingBuffer = *(SharedPtr<AnimModel::AnimMeshRenderingBuffer>*)p;
	auto& id = animMeshRenderingBuffer->id;

	assert(id != INVALID_ID);
	assert(p == m_animMeshRenderingBufferCount[id].p);
	assert(animator == m_animMeshRenderingBufferCount[id].animator);

	auto& count = m_animMeshRenderingBufferCount[id].count;

	if (count == 1)
	{
		auto& pback = *(SharedPtr<AnimModel::AnimMeshRenderingBuffer>*)p;
		pback->id = id;

		STD_VECTOR_ROLL_TO_FILL_BLANK_2(m_animMeshRenderingBufferCount, id);

		id = INVALID_ID;
		return;
	}

	count--;
}

void AnimationSystem::BeginModification()
{
}

void AnimationSystem::AddComponent(MainComponent* comp)
{
	auto animationComp = (AnimationComponent*)comp;

	auto type = animationComp->GetAnimationType();
	switch (type)
	{
	case soft::ANIMATION_TYPE_SKELETAL_GAME_OBJECT:
	{
		auto animComp = (AnimSkeletalGameObject*)animationComp;
		animComp->m_animationSystemId = m_animSkeletalGameObjects.size();
		m_animSkeletalGameObjects.push_back(animComp);
		break;
	}
	case soft::ANIMATION_TYPE_SKELETAL_ARRAY:
		break;
	case soft::ANIMATION_TYPE_SKELETAL_SELF_HIERARCHY:
		break;
	default:
		break;
	}
}

void AnimationSystem::RemoveComponent(MainComponent* comp)
{
}

void AnimationSystem::OnObjectTransformChanged(MainComponent* comp)
{
}

void AnimationSystem::EndModification()
{
}

void AnimationSystem::PrevIteration()
{
}

void AnimationSystem::Iteration(float dt)
{
}

void AnimationSystem::PostIteration()
{
}

NAMESPACE_END