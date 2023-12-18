#include "AnimationSystem.h"

#include "Resources/AnimModel.h"

#include "MainSystem/Animation/Components/AnimSkeletalGameObject.h"
#include "MainSystem/Animation/Components/AnimatorSkeletalArray.h"

#include "Scene/GameObject.h"

NAMESPACE_BEGIN

AnimationSystem::AnimationSystem(Scene* scene) : MainSystem(scene)
{
}

AnimationSystem::~AnimationSystem()
{
}

void AnimationSystem::AddAnimMeshRenderingBuffer(void* p, AnimatorSkeletalGameObject* animator)
{
	auto animMeshRenderingBuffer = (AnimModel::AnimMeshRenderingBuffer*)p;
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

void AnimationSystem::RemoveMeshRenderingBuffer(void* p, AnimatorSkeletalGameObject* animator)
{
	auto animMeshRenderingBuffer = (AnimModel::AnimMeshRenderingBuffer*)p;
	auto& id = animMeshRenderingBuffer->id;

	assert(id != INVALID_ID);
	assert(p == m_animMeshRenderingBufferCount[id].p);
	assert(animator == m_animMeshRenderingBufferCount[id].animator);

	auto& count = m_animMeshRenderingBufferCount[id].count;

	if (count == 1)
	{
		auto pback = (AnimModel::AnimMeshRenderingBuffer*)p;
		pback->id = id;

		STD_VECTOR_ROLL_TO_FILL_BLANK_2(m_animMeshRenderingBufferCount, id);

		id = INVALID_ID;
		return;
	}

	count--;
}

void AnimationSystem::CalculateAABBForMeshRenderingBuffer(AnimMeshRenderingBufferCounter* counter)
{
	auto animMeshRenderingBuffer = (AnimModel::AnimMeshRenderingBuffer*)(counter->p);
	auto& buffer = animMeshRenderingBuffer->buffer;
	auto animator = counter->animator;

	auto& aabbIndex = animator->m_aabbKeyFrameIndex;
	if (animator->m_numUpdateAABB != animator->m_numAnimIterationCount)
	{
		for (auto& index : aabbIndex)
		{
			index = 0;
		}

		animator->m_numUpdateAABB = animator->m_numAnimIterationCount;
	}

	bool update = false;

	m_scene->BeginWrite<false>(buffer);

	auto read = buffer.Read();
	auto write = buffer.Write();

	auto& animation = animator->m_model3D->m_animations[animator->m_animationId];
	auto num = write->meshesAABB.size();
	for (uint32_t i = 0; i < num; i++)
	{
		auto& index = aabbIndex[i];
		write->meshesAABB[i] = animation.animMeshLocalAABoxKeyFrames[i].Find(&index, index, animator->m_t);
		if (std::memcmp(&write->meshesAABB[i], &read->meshesAABB[i], sizeof(AABox)))
		{
			update = true;
		}
	}

	if (update)
	{
		m_scene->EndWrite<true>(buffer);

		for (auto& obj : animator->m_animMeshRendererObjs)
		{
			if (obj->m_scene)
				m_scene->OnObjectTransformChanged(obj);
		}
	}
	else
	{
		m_scene->EndWrite<false>(buffer);
	}
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
	{
		auto animComp = (AnimatorSkeletalArray*)animationComp;
		animComp->m_animationSystemId = m_animSkeletalArrays.size();
		m_animSkeletalArrays.push_back(animComp);
		break;
	}
	case soft::ANIMATION_TYPE_SKELETAL_SELF_HIERARCHY:
		break;
	default:
		break;
	}
}

void AnimationSystem::RemoveComponent(MainComponent* comp)
{
	auto animationComp = (AnimationComponent*)comp;

	auto type = animationComp->GetAnimationType();
	switch (type)
	{
	case soft::ANIMATION_TYPE_SKELETAL_GAME_OBJECT:
	{
		auto animComp = (AnimSkeletalGameObject*)animationComp;
		STD_VECTOR_ROLL_TO_FILL_BLANK(m_animSkeletalGameObjects, animComp, m_animationSystemId);
		break;
	}
	case soft::ANIMATION_TYPE_SKELETAL_ARRAY:
	{
		auto animComp = (AnimatorSkeletalArray*)animationComp;
		STD_VECTOR_ROLL_TO_FILL_BLANK(m_animSkeletalArrays, animComp, m_animationSystemId);
		break;
	}
	case soft::ANIMATION_TYPE_SKELETAL_SELF_HIERARCHY:
		break;
	default:
		break;
	}
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
	GetPrevAsyncTaskRunnerMT()->ProcessAllTasksMT(this);
	GetPrevAsyncTaskRunnerST()->ProcessAllTasks(this);

	TaskUtils::ForEachStdVector(m_animMeshRenderingBufferCount, 
		[this, dt](AnimMeshRenderingBufferCounter& data, ID) 
		{
			auto animator = data.animator;
			animator->Update(dt);
			CalculateAABBForMeshRenderingBuffer(&data);
		},
		TaskSystem::GetWorkerCount()
	);

	TaskUtils::ForEachStdVector(m_animSkeletalGameObjects,
		[dt](AnimSkeletalGameObject* comp, ID)
		{
			comp->Update(dt);
		},
		TaskSystem::GetWorkerCount()
	);

	auto scene = m_scene;
	TaskUtils::ForEachStdVector(m_animSkeletalArrays,
		[dt, scene](AnimatorSkeletalArray* comp, ID)
		{
			comp->Update(scene, dt);
		},
		m_animSkeletalArrays.size() < 9000 ? 4 : TaskSystem::GetWorkerCount()
	);

	/*for (auto& data : m_animMeshRenderingBufferCount)
	{
		auto animator = data.animator;
		animator->Update(dt);
		CalculateAABBForMeshRenderingBuffer(&data);
	}

	for (auto& comp : m_animSkeletalGameObjects)
	{
		comp->Update(dt);
	}*/
}

void AnimationSystem::PostIteration()
{
}

NAMESPACE_END