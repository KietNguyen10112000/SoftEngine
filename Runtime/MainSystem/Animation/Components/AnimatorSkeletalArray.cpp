#include "AnimatorSkeletalArray.h"

#include "Scene/GameObject.h"

#include "MainSystem/Animation/AnimationSystem.h"
#include "MainSystem/Rendering/Components/AnimModelStaticMeshRenderer.h"

NAMESPACE_BEGIN

AnimatorSkeletalArray::AnimatorSkeletalArray() : Animator(ANIMATION_TYPE_SKELETAL_ARRAY)
{
	m_currentAnimTrack = (decltype(m_currentAnimTrack))m_currentAnimTrackBuffer.Read();
	m_blendingAnimTrack = nullptr;
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
	auto& animationId = m_currentAnimTrack->animationId;
	auto& tickDuration = m_currentAnimTrack->tickDuration;
	auto& ticksPerSecond = m_currentAnimTrack->ticksPerSecond;

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
		ticksPerSecond = tickDuration / sec;
		return;
	}

	auto system = GetGameObject()->GetScene()->GetAnimationSystem();
	auto taskRunner = system->AsyncTaskRunner();

	auto task = taskRunner->CreateTask(
		[](AnimationSystem* system, void* p)
		{
			TASK_SYSTEM_UNPACK_PARAM_REF_2(Param, p, animator, sec);
			animator->m_currentAnimTrack->ticksPerSecond = sec;
		}
	);

	auto param = taskRunner->CreateParam<Param>(&task);
	param->animator = this;
	param->sec = tickDuration / sec;

	taskRunner->RunAsync(this, &task);
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

void AnimatorSkeletalArray::Play(float startTransitTime, ID animationId, float startTime, float beginTime, float endTime, float blendTime)
{
	struct Param
	{
		AnimatorSkeletalArray* animator;
		float blendTime;
		float startTime;

		float startTransitTime = 0;
		float padd;
	};

	startTime = std::max(0.0f, std::clamp(startTime, beginTime, endTime));

	blendTime = std::max(0.0f, blendTime);

	if (animationId >= m_model3D->m_animations.size())
	{
		return;
	}

	if (!GetGameObject()->IsInAnyScene())
	{
		m_model3D->InitializeAnimationTrack(animationId, m_currentAnimTrack, beginTime, endTime);
		SetAnimationImpl(startTransitTime, blendTime, startTime);
		return;
	}

	auto scene = GetGameObject()->GetScene();

	auto& buffer = blendTime > 0 ? m_blendingAnimTrackBuffer : m_currentAnimTrackBuffer;
	scene->BeginWrite<false>(buffer);

	auto track = buffer.Write();
	m_model3D->InitializeAnimationTrack(animationId, track, beginTime, endTime);

	scene->EndWrite(buffer);

	auto system = scene->GetAnimationSystem();
	auto taskRunner = system->AsyncTaskRunner();

	auto task = taskRunner->CreateTask(
		[](AnimationSystem* system, void* p)
		{
			TASK_SYSTEM_UNPACK_PARAM_REF_4(Param, p, animator, blendTime, startTime, startTransitTime);
			animator->SetAnimationImpl(startTransitTime, blendTime, startTime);
		}
	);

	auto param = taskRunner->CreateParam<Param>(&task);
	param->animator = this;
	param->blendTime = blendTime;
	param->startTime = startTime;
	param->startTransitTime = startTransitTime;

	taskRunner->RunAsync(this, &task);
}

void AnimatorSkeletalArray::SetPause(bool pause)
{
	m_paused = pause;
}

void AnimatorSkeletalArray::SetTime(float t)
{
	struct Param
	{
		AnimatorSkeletalArray* animator;
		float sec;
	};

	if (!GetGameObject()->IsInAnyScene())
	{
		SetTimeImpl(t);
		return;
	}

	auto system = GetGameObject()->GetScene()->GetAnimationSystem();
	auto taskRunner = system->AsyncTaskRunner();

	auto task = taskRunner->CreateTask(
		[](AnimationSystem* system, void* p)
		{
			TASK_SYSTEM_UNPACK_PARAM_REF_2(Param, p, animator, sec);
			animator->SetTimeImpl(sec);

			if (animator->m_paused) 
			{
				animator->m_paused = false;
				animator->Update(system->GetScene(), 0.0f);
				animator->m_paused = true;
			}
		}
	);

	auto param = taskRunner->CreateParam<Param>(&task);
	param->animator = this;
	param->sec = t;

	taskRunner->RunAsync(this, &task);
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
		1,
		nullptr,
		[](UnknownAddress& var, Serializable* instance) -> Variant
		{
			auto animator = ((AnimatorSkeletalArray*)instance);
			auto ret = Variant(VARIANT_TYPE::UINT64);
			ret.As<ID>() = animator->m_currentAnimTrack->animationId;
			return ret;
		},
		this
	);
	metadata->AddProperty(accessor);

	accessor = Accessor(
		"Animation name",
		2,
		nullptr,
		[](UnknownAddress& var, Serializable* instance) -> Variant
		{
			auto animator = ((AnimatorSkeletalArray*)instance);
			auto ret = Variant(VARIANT_TYPE::STRING);
			ret.As<String>() = animator->m_model3D->m_animations[animator->m_currentAnimTrack->animationId].name;
			return ret;
		},
		this
	);
	metadata->AddProperty(accessor);

	accessor = Accessor(
		"Duration",
		3,
		nullptr,
		[](UnknownAddress& var, Serializable* instance) -> Variant
		{
			auto& ticksPerSecond = ((AnimatorSkeletalArray*)instance)->m_currentAnimTrack->ticksPerSecond;
			auto& tickDuration = ((AnimatorSkeletalArray*)instance)->m_currentAnimTrack->tickDuration;
			auto ret = Variant(VARIANT_TYPE::FLOAT);
			ret.As<float>() = tickDuration / ticksPerSecond;
			return ret;
		},
		this
	);
	metadata->AddProperty(accessor);

	return metadata;
}

void AnimatorSkeletalArray::OnPropertyChanged(const UnknownAddress& var, const Variant& newValue)
{
	if (var.Is(1))
	{
		Play(-1, newValue.As<ID>(), 0, -1, -1, 0);
	}

	if (var.Is(3))
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

	ret->m_model3D = m_model3D;

	ret->m_aabbKeyFrameIndex.resize(m_aabbKeyFrameIndex.size());
	ret->m_globalTransforms.resize(m_globalTransforms.size());
	ret->m_keyFramesIndex.resize(m_keyFramesIndex.size());

	ret->m_blendAabbKeyFrameIndex.resize(m_aabbKeyFrameIndex.size());
	ret->m_blendKeyFramesIndex.resize(m_keyFramesIndex.size());

	m_model3D->InitializeAnimationTrack(
		m_currentAnimTrack->animationId,
		ret->m_currentAnimTrack,
		m_currentAnimTrack->startTick / m_currentAnimTrack->ticksPerSecond,
		(m_currentAnimTrack->startTick + m_currentAnimTrack->tickDuration) / m_currentAnimTrack->ticksPerSecond
	);

	ret->m_t = 0;
	ret->m_blendTime = 0;
	ret->m_blendingAnimTrack = nullptr;

	if (m_blendingAnimTrack)
	{
		ret->m_blendingAnimTrack = (decltype(m_blendingAnimTrack))(ret->m_blendingAnimTrackBuffer.Read());

		m_model3D->InitializeAnimationTrack(
			m_blendingAnimTrack->animationId,
			ret->m_blendingAnimTrack,
			m_blendingAnimTrack->startTick / m_blendingAnimTrack->ticksPerSecond,
			(m_blendingAnimTrack->startTick + m_blendingAnimTrack->tickDuration) / m_blendingAnimTrack->ticksPerSecond
		);

		std::memcpy(ret->m_blendKeyFramesIndex.data(), m_blendingAnimTrack->startKeyFramesIndex.data(),
			m_keyFramesIndex.size() * sizeof(KeyFramesIndex));

		std::memcpy(ret->m_blendAabbKeyFrameIndex.data(), m_blendingAnimTrack->startAABBKeyFrameIndex.data(),
			m_aabbKeyFrameIndex.size() * sizeof(uint32_t));
	}

	std::memcpy(ret->m_keyFramesIndex.data(), m_currentAnimTrack->startKeyFramesIndex.data(),
		m_keyFramesIndex.size() * sizeof(KeyFramesIndex));

	std::memcpy(ret->m_aabbKeyFrameIndex.data(), m_currentAnimTrack->startAABBKeyFrameIndex.data(),
		m_aabbKeyFrameIndex.size() * sizeof(uint32_t));

	return ret;
}

void AnimatorSkeletalArray::UpdateNoBlend(Scene* scene)
{
	auto& animationId = m_currentAnimTrack->animationId;
	auto& tickDuration = m_currentAnimTrack->tickDuration;
	auto& ticksPerSecond = m_currentAnimTrack->ticksPerSecond;

	auto t = m_t + m_currentAnimTrack->startTick;

	{
		auto& nodes = m_model3D->m_nodes;
		auto num = nodes.size();
		auto& channels = m_model3D->m_animations[animationId].channels;

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
				channel.FindScaleMatrix(&scaling, &index.s, index.s, t);
				Mat4 rotation;
				channel.FindRotationMatrix(&rotation, &index.r, index.r, t);
				Mat4 translation;
				channel.FindTranslationMatrix(&translation, &index.t, index.t, t);

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
				channel.FindScaleMatrix(&scaling, &index.s, index.s, t);
				Mat4 rotation;
				channel.FindRotationMatrix(&rotation, &index.r, index.r, t);
				Mat4 translation;
				channel.FindTranslationMatrix(&translation, &index.t, index.t, t);

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

		//auto& index = m_aabbKeyFrameIndex;

		bool update = false;

		scene->BeginWrite<false>(buffer);

		auto read = buffer.Read();
		auto write = buffer.Write();

		auto& animation = m_model3D->m_animations[animationId];
		auto num = write->meshesAABB.size();
		for (uint32_t i = 0; i < num; i++)
		{
			auto& index = m_aabbKeyFrameIndex[i];

			write->meshesAABB[i] = animation.animMeshLocalAABoxKeyFrames[i].Find(&index, index, t);
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

void AnimatorSkeletalArray::UpdateBlend(Scene* scene, float dt)
{
	auto& blendAnimationId = m_blendingAnimTrack->animationId;
	auto& blendTickDuration = m_blendingAnimTrack->tickDuration;
	auto& blendTicksPerSecond = m_blendingAnimTrack->ticksPerSecond;

	//m_tBlend += dt * blendTicksPerSecond;
	if (m_tBlend > blendTickDuration)
	{
		uint32_t num = (uint32_t)std::floor(m_tBlend / blendTickDuration);
		m_tBlend -= num * blendTickDuration;

		std::memcpy(m_blendKeyFramesIndex.data(), m_blendingAnimTrack->startKeyFramesIndex.data(),
			m_keyFramesIndex.size() * sizeof(KeyFramesIndex));

		std::memcpy(m_blendAabbKeyFrameIndex.data(), m_blendingAnimTrack->startAABBKeyFrameIndex.data(),
			m_aabbKeyFrameIndex.size() * sizeof(uint32_t));
	}

	m_blendCurTime += dt;

	auto sBlend = 1.0f - std::min(1.0f, m_blendCurTime / m_blendTime);

	auto& animationId = m_currentAnimTrack->animationId;
	auto& tickDuration = m_currentAnimTrack->tickDuration;
	auto& ticksPerSecond = m_currentAnimTrack->ticksPerSecond;

	auto t = m_t + m_currentAnimTrack->startTick;

	auto tBlend = m_tBlend + m_blendingAnimTrack->startTick;

	{
		auto& nodes = m_model3D->m_nodes;
		auto num = nodes.size();
		auto& channels = m_model3D->m_animations[animationId].channels;

		auto& blendChannels = m_model3D->m_animations[blendAnimationId].channels;

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
				auto& blendChannel = blendChannels[node.boneId];

				auto& index = m_keyFramesIndex[node.boneId];
				auto& blendIndex = m_blendKeyFramesIndex[node.boneId];

				Mat4 scaling;
				Mat4 rotation;
				Mat4 translation;

				channel.FindScaleMatrix(&scaling, &index.s, index.s, t);
				channel.FindRotationMatrix(&rotation, &index.r, index.r, t);
				channel.FindTranslationMatrix(&translation, &index.t, index.t, t);
				Mat4 mat1 = scaling * rotation * translation;

				blendChannel.FindScaleMatrix(&scaling, &blendIndex.s, blendIndex.s, tBlend);
				blendChannel.FindRotationMatrix(&rotation, &blendIndex.r, blendIndex.r, tBlend);
				blendChannel.FindTranslationMatrix(&translation, &blendIndex.t, blendIndex.t, tBlend);
				Mat4 mat2 = scaling * rotation * translation;

				m_globalTransforms[0] = Lerp(mat1, mat2, sBlend);
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
				auto& blendChannel = blendChannels[node.boneId];

				auto& index = m_keyFramesIndex[node.boneId];
				auto& blendIndex = m_blendKeyFramesIndex[node.boneId];

				Mat4 scaling;
				Mat4 rotation;
				Mat4 translation;

				channel.FindScaleMatrix(&scaling, &index.s, index.s, t);
				channel.FindRotationMatrix(&rotation, &index.r, index.r, t);
				channel.FindTranslationMatrix(&translation, &index.t, index.t, t);
				Mat4 mat1 = scaling * rotation * translation;

				blendChannel.FindScaleMatrix(&scaling, &blendIndex.s, blendIndex.s, tBlend);
				blendChannel.FindRotationMatrix(&rotation, &blendIndex.r, blendIndex.r, tBlend);
				blendChannel.FindTranslationMatrix(&translation, &blendIndex.t, blendIndex.t, tBlend);
				Mat4 mat2 = scaling * rotation * translation;

				globalTransform = Lerp(mat1, mat2, sBlend);
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

		//auto& index = m_aabbKeyFrameIndex;

		bool update = false;

		scene->BeginWrite<false>(buffer);

		auto read = buffer.Read();
		auto write = buffer.Write();

		auto& animation = m_model3D->m_animations[animationId];
		auto& blendAnimation = m_model3D->m_animations[blendAnimationId];

		auto num = write->meshesAABB.size();
		for (uint32_t i = 0; i < num; i++)
		{
			auto& index = m_aabbKeyFrameIndex[i];
			auto& blendIndex = m_blendAabbKeyFrameIndex[i];

			auto aaBox1 = animation.animMeshLocalAABoxKeyFrames[i].Find(&index, index, t);
			auto aaBox2 = blendAnimation.animMeshLocalAABoxKeyFrames[i].Find(&blendIndex, blendIndex, tBlend);

			auto& output = write->meshesAABB[i];
			output.m_center = Lerp(aaBox1.m_center, aaBox2.m_center, sBlend);
			output.m_halfDimensions = Lerp(aaBox1.m_halfDimensions, aaBox2.m_halfDimensions, sBlend);

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

	if (sBlend == 1.0f)
	{
		/*m_t = m_blendTime;

		m_currentAnimTrack = m_blendingAnimTrack;

		std::memcpy(m_keyFramesIndex.data(), m_currentAnimTrack->startKeyFramesIndex.data(),
			m_keyFramesIndex.size() * sizeof(KeyFramesIndex));

		std::memcpy(m_aabbKeyFrameIndex.data(), m_currentAnimTrack->startAABBKeyFrameIndex.data(),
			m_aabbKeyFrameIndex.size() * sizeof(uint32_t));*/

		m_blendingAnimTrack = nullptr;
	}
}

void AnimatorSkeletalArray::Update(Scene* scene, float dt)
{
	if (m_paused)
	{
		return;
	}

	auto& animationId = m_currentAnimTrack->animationId;
	auto& tickDuration = m_currentAnimTrack->tickDuration;
	auto& ticksPerSecond = m_currentAnimTrack->ticksPerSecond;

	bool doBlend = m_t <= m_startTransitTick;

	m_t += dt * ticksPerSecond;

	doBlend = doBlend && (m_t > m_startTransitTick);

	if (m_t > tickDuration)
	{
		uint32_t num = (uint32_t)std::floor(m_t / tickDuration);
		m_t -= num * tickDuration;

		std::memcpy(m_keyFramesIndex.data(), m_currentAnimTrack->startKeyFramesIndex.data(),
			m_keyFramesIndex.size() * sizeof(KeyFramesIndex));

		std::memcpy(m_aabbKeyFrameIndex.data(), m_currentAnimTrack->startAABBKeyFrameIndex.data(),
			m_aabbKeyFrameIndex.size() * sizeof(uint32_t));
	}

	doBlend = doBlend || m_startTransitTick == INFINITY || m_startTransitTick < 0;

	if (m_blendingAnimTrack && doBlend)
	{
		if (m_startTransitTick != INFINITY)
		{
			m_currentAnimTrack = (decltype(m_currentAnimTrack))m_blendingAnimTrackBuffer.Read();

			m_t = m_transitStartTime * m_currentAnimTrack->ticksPerSecond - m_currentAnimTrack->startTick;

			std::memcpy(m_keyFramesIndex.data(), m_currentAnimTrack->startKeyFramesIndex.data(),
				m_keyFramesIndex.size() * sizeof(KeyFramesIndex));

			std::memcpy(m_aabbKeyFrameIndex.data(), m_currentAnimTrack->startAABBKeyFrameIndex.data(),
				m_aabbKeyFrameIndex.size() * sizeof(uint32_t));

			m_startTransitTick = INFINITY;
		}

		UpdateBlend(scene, dt);
	}
	else
	{
		UpdateNoBlend(scene);
	}
}

NAMESPACE_END