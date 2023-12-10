#include "AnimatorSkeletalArray.h"

#include "Scene/GameObject.h"

#include "MainSystem/Animation/AnimationSystem.h"
#include "MainSystem/Rendering/Components/AnimModelStaticMeshRenderer.h"

NAMESPACE_BEGIN

AnimatorSkeletalArray::AnimatorSkeletalArray() : Animator(ANIMATION_TYPE_SKELETAL_ARRAY)
{

}

void AnimatorSkeletalArray::OnComponentAdded()
{
}

void AnimatorSkeletalArray::OnComponentRemoved()
{
}

void AnimatorSkeletalArray::OnTransformChanged()
{
}

AABox AnimatorSkeletalArray::GetGlobalAABB()
{
	return AABox();
}

ID AnimatorSkeletalArray::FindAnimation(const String& name)
{
	return ID();
}

void AnimatorSkeletalArray::GetAnimationsName(std::vector<String>& output) const
{
}

void AnimatorSkeletalArray::SetDuration(float sec)
{
	struct Param
	{
		AnimatorSkeletalArray* animator;
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

void AnimatorSkeletalArray::SetDuration(float sec, ID animationId)
{
}

float AnimatorSkeletalArray::GetDuration() const
{
	return 0.0f;
}

ID AnimatorSkeletalArray::GetCurrentAnimationId() const
{
	return ID();
}

void AnimatorSkeletalArray::Play(ID animationId, float blendTime)
{
	struct Param
	{
		AnimatorSkeletalArray* animator;
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

void AnimatorSkeletalArray::Serialize(Serializer* serializer)
{
}

void AnimatorSkeletalArray::Deserialize(Serializer* serializer)
{
}

void AnimatorSkeletalArray::CleanUp()
{
}

Handle<ClassMetadata> AnimatorSkeletalArray::GetMetadata(size_t sign)
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
			auto animator = ((AnimatorSkeletalArray*)instance);
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
			ret.As<float>() = tickDuration / ((AnimatorSkeletalArray*)instance)->m_ticksPerSecond;
			return ret;
		},
		this
	);
	metadata->AddProperty(accessor);

	return metadata;
}

void AnimatorSkeletalArray::OnPropertyChanged(const UnknownAddress& var, const Variant& newValue)
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

Handle<Serializable> AnimatorSkeletalArray::Clone(Serializer* serializer)
{
	struct CloneParam
	{
		AnimatorSkeletalArray* src;
		AnimatorSkeletalArray* dest;
	};

	auto ret = mheap::New<AnimatorSkeletalArray>();

	auto& addresses = serializer->GetAddressMap();
	/*auto& addresses = serializer->GetAddressMap();

	{
		addresses.insert({ this, ret.Get() });
	}*/

	auto callbackRunner = serializer->GetCallbackRunner();
	auto task = callbackRunner->CreateTask([](Serializer* serializer, void* p)
		{
			TASK_SYSTEM_UNPACK_PARAM_2(CloneParam, p, src, dest);

			auto& addresses = serializer->GetAddressMap();

			auto& objs = src->m_meshRendererObjs;
			for (auto& obj : objs)
			{
				auto it = addresses.find(obj.Get());
				assert(it != addresses.end());
				dest->m_meshRendererObjs.Push((GameObject*)(it->second));
			}

			/*{
				auto it = addresses.find(src->m_animMeshRenderingBuffer.get());
				assert(it != addresses.end());
				dest->m_animMeshRenderingBuffer = *(decltype(dest->m_animMeshRenderingBuffer)*)(it->second);
			}*/
		}
	);

	AnimModel::AnimMeshRenderingBufferData buffer;
	buffer.bones.resize(m_model3D->m_boneIds.size());
	buffer.meshesAABB.resize(m_model3D->m_animMeshes.size());
	auto buf = std::make_shared<AnimModel::AnimMeshRenderingBuffer>();
	buf->buffer.Initialize(buffer);

	ret->m_animMeshRenderingBuffer = buf;
	addresses.insert({ m_animMeshRenderingBuffer.get(), &ret->m_animMeshRenderingBuffer });

	auto param = callbackRunner->CreateParam<CloneParam>(&task);
	param->src = this;
	param->dest = ret.Get();

	callbackRunner->RunAsync(&task);

	ret->m_animationId = m_animationId;
	ret->m_model3D = m_model3D;
	ret->m_tickDuration = m_tickDuration;
	ret->m_ticksPerSecond = m_ticksPerSecond;

	ret->m_aabbKeyFrameIndex = 0;
	ret->m_globalTransforms.resize(m_globalTransforms.size());
	ret->m_keyFramesIndex.resize(m_keyFramesIndex.size());

	return ret;
}

void AnimatorSkeletalArray::Update(Scene* scene, float dt)
{
	m_t += dt * m_ticksPerSecond;
	if (m_t > m_tickDuration)
	{
		uint32_t num = (uint32_t)std::floor(m_t / m_tickDuration);
		m_t -= num * m_tickDuration;

		for (auto& index : m_keyFramesIndex)
		{
			index.s = 0;
			index.r = 0;
			index.t = 0;
		}

		m_aabbKeyFrameIndex = 0;
	}

	{
		auto& nodes = m_model3D->m_nodes;
		auto num = nodes.size();
		auto& channels = m_model3D->m_animations[m_animationId].channels;

		auto& boneBuffer = m_animMeshRenderingBuffer->buffer;

		auto& offsets = m_model3D->m_boneOffsetMatrixs;

		scene->BeginWrite<false>(boneBuffer);

		auto& bones = m_animMeshRenderingBuffer->buffer.Write()->bones;

		{
			auto& node = nodes[0];

			m_globalTransforms[0] = GetGameObject()->ReadGlobalTransformMat();

			if (node.boneId != INVALID_ID)
			{
				auto& channel = channels[node.boneId];
				auto& index = m_keyFramesIndex[node.boneId];

				Mat4 scaling;
				channel.FindScaleMatrix(&scaling, &index.s, index.s, m_t);
				Mat4 rotation;
				channel.FindRotationMatrix(&rotation, &index.r, index.r, m_t);
				Mat4 translation;
				channel.FindTranslationMatrix(&translation, &index.t, index.t, m_t);

				m_globalTransforms[0] = scaling * rotation * translation;
			}

			assert(node.parentId == INVALID_ID);

			if (node.boneId != INVALID_ID)
			{
				bones[node.boneId] = offsets[node.boneId] * m_globalTransforms[0];
			}
		}

		for (size_t i = 1; i < num; i++)
		{
			auto& node = nodes[i];

			auto& globalTransform = m_globalTransforms[i];

			globalTransform = node.localTransform;

			if (node.boneId != INVALID_ID)
			{
				auto& channel = channels[node.boneId];
				auto& index = m_keyFramesIndex[node.boneId];

				Mat4 scaling;
				channel.FindScaleMatrix(&scaling, &index.s, index.s, m_t);
				Mat4 rotation;
				channel.FindRotationMatrix(&rotation, &index.r, index.r, m_t);
				Mat4 translation;
				channel.FindTranslationMatrix(&translation, &index.t, index.t, m_t);

				globalTransform = scaling * rotation * translation;
			}

			globalTransform = globalTransform * m_globalTransforms[node.parentId];

			if (node.boneId != INVALID_ID)
			{
				bones[node.boneId] = offsets[node.boneId] * globalTransform;
			}
		}

		scene->EndWrite(boneBuffer);
	}
	

	{
		auto animMeshRenderingBuffer = m_animMeshRenderingBuffer.get();
		auto& buffer = animMeshRenderingBuffer->buffer;

		auto& index = m_aabbKeyFrameIndex;

		bool update = false;

		scene->BeginWrite<false>(buffer);

		auto read = buffer.Read();
		auto write = buffer.Write();

		auto& animation = m_model3D->m_animations[m_animationId];
		auto num = write->meshesAABB.size();
		for (uint32_t i = 0; i < num; i++)
		{
			write->meshesAABB[i] = animation.animMeshLocalAABoxKeyFrames[i].Find(&index, index, m_t);
			if (std::memcmp(&write->meshesAABB[i], &read->meshesAABB[i], sizeof(AABox)))
			{
				update = true;
			}
		}

		auto& boundNodeIds = m_model3D->m_boundNodeIds;

		if (update)
		{
			scene->EndWrite<true>(buffer);

			num = m_meshRendererObjs.size();
			for (size_t i = 0; i < num; i++)
			{
				if (boundNodeIds[i] == INVALID_ID)
				{
					auto& obj = m_meshRendererObjs[i];
					if (obj->GetScene() == scene)
						scene->OnObjectTransformChanged(obj);
				}
			}
		}
		else
		{
			scene->EndWrite<false>(buffer);
		}

		num = m_meshRendererObjs.size();
		for (size_t i = 0; i < num; i++)
		{
			if (boundNodeIds[i] != INVALID_ID)
			{
				auto& obj = m_meshRendererObjs[i];

				auto& boundNodeTransform = m_globalTransforms[boundNodeIds[i]];

				auto& buffer = obj->GetComponentRaw<AnimModelStaticMeshRenderer>()->m_myGlobalTransform;
				
				scene->BeginWrite<false>(buffer);
				
				auto buf = buffer.Write();
				*buf = boundNodeTransform;

				scene->EndWrite(buffer);

				scene->OnObjectTransformChanged(obj);
			}
		}
	}
	
}

NAMESPACE_END