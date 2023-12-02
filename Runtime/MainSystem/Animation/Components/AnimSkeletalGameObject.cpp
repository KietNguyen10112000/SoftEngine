#include "AnimSkeletalGameObject.h"

#include "Scene/GameObject.h"

#include "MainSystem/Animation/AnimationSystem.h"

NAMESPACE_BEGIN

AnimSkeletalGameObject::AnimSkeletalGameObject() : AnimationComponent(ANIMATION_TYPE_SKELETAL_GAME_OBJECT)
{
}

void AnimSkeletalGameObject::OnComponentAdded()
{
	GetGameObject()->GetScene()->GetAnimationSystem()->AddAnimMeshRenderingBuffer(m_animMeshRenderingBuffer.get(), m_animator.Get());
}

void AnimSkeletalGameObject::OnComponentRemoved()
{
	GetGameObject()->GetScene()->GetAnimationSystem()->AddAnimMeshRenderingBuffer(m_animMeshRenderingBuffer.get(), m_animator.Get());
}

void AnimSkeletalGameObject::OnTransformChanged()
{
}

AABox AnimSkeletalGameObject::GetGlobalAABB()
{
	return AABox();
}

void AnimSkeletalGameObject::Serialize(Serializer* serializer)
{
}

void AnimSkeletalGameObject::Deserialize(Serializer* serializer)
{
}

void AnimSkeletalGameObject::CleanUp()
{
}

Handle<ClassMetadata> AnimSkeletalGameObject::GetMetadata(size_t sign)
{
	return Handle<ClassMetadata>();
}

void AnimSkeletalGameObject::OnPropertyChanged(const UnknownAddress& var, const Variant& newValue)
{
}

Handle<Serializable> AnimSkeletalGameObject::Clone(Serializer* serializer)
{
	struct CloneParam
	{
		AnimSkeletalGameObject* src;
		AnimSkeletalGameObject* dest;
	};

	auto ret = mheap::New<AnimSkeletalGameObject>();

	ret->m_model3D = m_model3D;
	
	auto& addresses = serializer->GetAddressMap();

	auto callbackRunner = serializer->GetCallbackRunner();
	auto task = callbackRunner->CreateTask([](Serializer* serializer, void* p)
		{
			TASK_SYSTEM_UNPACK_PARAM_2(CloneParam, p, src, dest);

			auto& addresses = serializer->GetAddressMap();

			// clone animator
			auto it = addresses.find(src->m_animator.Get());
			assert(it != addresses.end());

			dest->m_animator = (AnimatorSkeletalGameObject*)(it->second);
		}
	);

	auto param = callbackRunner->CreateParam<CloneParam>(&task);
	param->src = this;
	param->dest = ret.Get();

	callbackRunner->RunAsync(&task);
	
	{
		// clone render buffer
		auto it = addresses.find(m_animMeshRenderingBuffer.get());
		if (it != addresses.end())
		{
			ret->m_animMeshRenderingBuffer = *(decltype(m_animMeshRenderingBuffer)*)(it->second);
		}
		else
		{
			AnimModel::AnimMeshRenderingBufferData buffer;
			buffer.bones.resize(m_model3D->m_boneIds.size());
			buffer.meshesAABB.resize(m_model3D->m_animMeshes.size());
			auto buf = std::make_shared<AnimModel::AnimMeshRenderingBuffer>();
			buf->buffer.Initialize(buffer);

			ret->m_animMeshRenderingBuffer = buf;
			addresses.insert({ m_animMeshRenderingBuffer.get(), &ret->m_animMeshRenderingBuffer });
		}
	}
	
	ret->m_boneId = m_boneId;

	return ret;
}

void AnimSkeletalGameObject::Update(float dt)
{
	auto& globalTransform = GetGameObject()->ReadGlobalTransformMat();
	auto scene = GetGameObject()->GetScene();

	auto& buffer = m_animMeshRenderingBuffer->buffer;
	scene->BeginWrite<false>(buffer);

	auto& bones = buffer.Write()->bones;
	bones[m_boneId] = m_model3D->m_boneOffsetMatrixs[m_boneId] * globalTransform;

	scene->EndWrite(buffer);

	if (m_numAnimIterationCount != m_animator->m_numAnimIterationCount)
	{
		// reset KeyFramesIndex
		m_keyFramesIndex.s = 0;
		m_keyFramesIndex.r = 0;
		m_keyFramesIndex.t = 0;
		m_numAnimIterationCount = m_animator->m_numAnimIterationCount;
	}

	auto& channel = m_model3D->m_animations[m_animator->GetCurrentAnimationId()].channels[m_boneId];

	Transform transform;
	transform.Scale() = channel.FindScale(&m_keyFramesIndex.s, m_keyFramesIndex.s, m_animator->m_t);
	transform.Rotation() = channel.FindRotation(&m_keyFramesIndex.r, m_keyFramesIndex.r, m_animator->m_t);
	transform.Position() = channel.FindTranslation(&m_keyFramesIndex.t, m_keyFramesIndex.t, m_animator->m_t);

	GetGameObject()->SetLocalTransform(transform);
}

NAMESPACE_END