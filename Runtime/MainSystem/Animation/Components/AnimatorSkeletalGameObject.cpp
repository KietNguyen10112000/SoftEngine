#include "AnimatorSkeletalGameObject.h"

#include "ANIMATION_TYPE.h"

#include "Scene/GameObject.h"
#include "MainSystem/Animation/AnimationSystem.h"

NAMESPACE_BEGIN

AnimatorSkeletalGameObject::AnimatorSkeletalGameObject()
{
}

void AnimatorSkeletalGameObject::OnComponentAdded()
{
}

void AnimatorSkeletalGameObject::OnComponentRemoved()
{
}

void AnimatorSkeletalGameObject::OnTransformChanged()
{
}

AABox AnimatorSkeletalGameObject::GetGlobalAABB()
{
	return AABox();
}

ID AnimatorSkeletalGameObject::FindAnimation(const String& name)
{
	auto& anims = m_model3D->m_animations;

	size_t count = 0;
	for (auto& anim : anims)
	{
		if (anim.name == name)
		{
			return count;
		}

		count++;
	}

	return INVALID_ID;
}

void AnimatorSkeletalGameObject::GetAnimationsName(std::vector<String>& output) const
{
	auto& anims = m_model3D->m_animations;
	for (auto& anim : anims)
	{
		output.push_back(anim.name);
	}
}

void AnimatorSkeletalGameObject::SetDuration(float sec)
{
	struct Param
	{
		AnimatorSkeletalGameObject* animator;
		float sec;
	};

	if (sec <= 0)
	{
		return;
	}

	if (!GetGameObject()->IsInAnyScene())
	{
		m_ticksPerSecond = m_tickDuration / sec;
		return;
	}

	auto system = GetGameObject()->GetScene()->GetAnimationSystem();
	auto taskRunner = system->AsyncTaskRunnerMT();

	auto task = taskRunner->CreateTask(
		[](AnimationSystem* system, void* p)
		{
			TASK_SYSTEM_UNPACK_PARAM_REF_2(Param, p, animator, sec);
			animator->m_ticksPerSecond = sec;
		}
	);

	auto param = taskRunner->CreateParam<Param>(&task);
	param->animator = this;
	param->sec = m_tickDuration / sec;

	taskRunner->RunAsync(&task);
}

void AnimatorSkeletalGameObject::SetDuration(float sec, ID animationId)
{
}

float AnimatorSkeletalGameObject::GetDuration() const
{
	return m_tickDuration / m_ticksPerSecond;
}

void AnimatorSkeletalGameObject::Play(ID animationId, float blendTime)
{
	struct Param
	{
		AnimatorSkeletalGameObject* animator;
		ID animationId;
		float blendTime;
	};

	if (animationId >= m_model3D->m_animations.size())
	{
		return;
	}

	if (!GetGameObject()->IsInAnyScene())
	{
		SetAnimationImpl(animationId, blendTime);
		return;
	}

	auto system = GetGameObject()->GetScene()->GetAnimationSystem();
	auto taskRunner = system->AsyncTaskRunnerMT();

	auto task = taskRunner->CreateTask(
		[](AnimationSystem* system, void* p)
		{
			TASK_SYSTEM_UNPACK_PARAM_REF_3(Param, p, animator, animationId, blendTime);
			animator->SetAnimationImpl(animationId, blendTime);
		}
	);

	auto param = taskRunner->CreateParam<Param>(&task);
	param->animator = this;
	param->animationId = animationId;
	param->blendTime = blendTime;

	taskRunner->RunAsync(&task);
}

void AnimatorSkeletalGameObject::Serialize(soft::Serializer* serializer)
{
}

void AnimatorSkeletalGameObject::Deserialize(soft::Serializer* serializer)
{
}

void AnimatorSkeletalGameObject::CleanUp()
{
}

soft::Handle<soft::ClassMetadata> AnimatorSkeletalGameObject::GetMetadata(size_t sign)
{
	auto metadata = mheap::New<ClassMetadata>(GetClassName(), this);

	auto accessor = Accessor(
		"Animation ID",
		&m_animationId,
		nullptr,
		[](UnknownAddress& var, Serializable* instance) -> Variant
		{
			auto& id = var.As<ID>();
			auto ret = Variant(VARIANT_TYPE::UINT64);
			ret.As<ID>() = id;
			return ret;
		},
		this
	);
	metadata->AddProperty(accessor);

	accessor = Accessor(
		"Animation name",
		1,
		nullptr,
		[](UnknownAddress& var, Serializable* instance) -> Variant
		{
			auto animator = ((AnimatorSkeletalGameObject*)instance);
			auto ret = Variant(VARIANT_TYPE::STRING);
			ret.As<String>() = animator->m_model3D->m_animations[animator->m_animationId].name;
			return ret;
		},
		this
	);
	metadata->AddProperty(accessor);

	accessor = Accessor(
		"Duration",
		&m_tickDuration,
		nullptr,
		[](UnknownAddress& var, Serializable* instance) -> Variant
		{
			auto& tickDuration = var.As<float>();
			auto ret = Variant(VARIANT_TYPE::FLOAT);
			ret.As<float>() = tickDuration / ((AnimatorSkeletalGameObject*)instance)->m_ticksPerSecond;
			return ret;
		},
		this
	);
	metadata->AddProperty(accessor);

	return metadata;
}

void AnimatorSkeletalGameObject::OnPropertyChanged(const soft::UnknownAddress& var, const soft::Variant& newValue)
{
	if (var.Is(&m_animationId))
	{
		Play(newValue.As<ID>(), 0);
	}

	if (var.Is(&m_tickDuration))
	{
		SetDuration(newValue.As<float>());
	}
}

Handle<Serializable> AnimatorSkeletalGameObject::Clone(Serializer* serializer)
{
	struct CloneParam
	{
		AnimatorSkeletalGameObject* src;
		AnimatorSkeletalGameObject* dest;
	};

	auto ret = mheap::New<AnimatorSkeletalGameObject>();

	auto& addresses = serializer->GetAddressMap();

	{
		addresses.insert({ this, ret.Get() });
	}

	auto callbackRunner = serializer->GetCallbackRunner();
	auto task = callbackRunner->CreateTask([](Serializer* serializer, void* p)
		{
			TASK_SYSTEM_UNPACK_PARAM_2(CloneParam, p, src, dest);

			auto& addresses = serializer->GetAddressMap();

			auto& objs = src->m_animMeshRendererObjs;
			for (auto& obj : objs)
			{
				auto it = addresses.find(obj.Get());
				assert(it != addresses.end());
				dest->m_animMeshRendererObjs.Push((GameObject*)(it->second));
			}

			/*{
				auto it = addresses.find(src->m_animMeshRenderingBuffer.get());
				assert(it != addresses.end());
				dest->m_animMeshRenderingBuffer = *(decltype(dest->m_animMeshRenderingBuffer)*)(it->second);
			}*/
		}
	);

	auto param = callbackRunner->CreateParam<CloneParam>(&task);
	param->src = this;
	param->dest = ret.Get();

	callbackRunner->RunAsync(&task);

	ret->m_animationId = m_animationId;
	ret->m_model3D = m_model3D;
	ret->m_tickDuration = m_tickDuration;
	ret->m_ticksPerSecond = m_ticksPerSecond;

	return ret;
}

NAMESPACE_END