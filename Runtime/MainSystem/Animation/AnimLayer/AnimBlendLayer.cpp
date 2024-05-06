#include "AnimBlendLayer.h"

#include "MainSystem/Animation/Components/AnimationComponent.h"
#include "MainSystem/Animation/AnimationSystem.h"
#include "MainSystem/MainSystemTaskPacking.h"

#include "Scene/Scene.h"
#include "Scene/GameObject.h"

#include "AnimPlayerLayer.h"

NAMESPACE_BEGIN

void AnimBlendLayer::Run(float dt)
{
	auto curLayer = m_input[m_currentLayerId];
	auto prevLayer = m_input[(m_currentLayerId + 1) % 2];

	if (!curLayer && prevLayer)
	{
		return;
	}

	if (m_blendTime < dt)
	{
		m_blendTime = -1.0f;
		if (prevLayer->IsEnable())
		{
			prevLayer->SetEnable(false);
		}
		return;
	}

	m_blendTime -= dt;

	auto sBlend = 1.0f - std::min(1.0f, m_blendTime / m_blendTotalTime);

	auto num = m_globalTransforms.size();

	auto& transforms0 = curLayer->NodeGlobalTransforms();
	auto& transforms1 = prevLayer->NodeGlobalTransforms();
	for (size_t i = 0; i < num; i++)
	{
		auto& v0 = transforms0[i];
		auto& v1 = transforms1[i];

		m_globalTransforms[i] = Lerp(v1, v0, sBlend);
	}

	num = m_meshesAABB.size();

	auto& meshAABB0 = curLayer->MeshesAABB();
	auto& meshAABB1 = prevLayer->MeshesAABB();
	for (size_t i = 0; i < num; i++)
	{
		auto& v0 = meshAABB0[i];
		auto& v1 = meshAABB1[i];

		auto& aabb = m_meshesAABB[i];
		aabb.m_center = Lerp(v1.m_center, v0.m_center, sBlend);
		aabb.m_halfDimensions = Lerp(v1.m_halfDimensions, v0.m_halfDimensions, sBlend);
	}
}

AnimLayer* AnimBlendLayer::GetOutput()
{
	if (!IsEnable())
	{
		return m_input[m_currentLayerId];
	}

	if (m_blendTime < 0)
	{
		return m_input[m_currentLayerId];
	}

	return this;
}

void AnimBlendLayer::SetInput(AnimLayer* l1, AnimLayer* l2)
{
	m_input[0] = l1;
	m_input[1] = l2;
}

void AnimBlendLayer::FadeTo(ID animationId, float startTime, float endTime, float fadeTime)
{
	auto curLayer = m_input[m_currentLayerId];
	//auto prevLayer = m_input[(m_currentLayerId + 1) % 2];

	auto l0 = dynamic_cast<AnimPlayerLayer*>(curLayer);
	//auto l1 = dynamic_cast<AnimPlayerLayer*>(prevLayer);

	if (l0)
	{
		l0->SetAnimation(animationId, startTime, endTime);
	}

	MAIN_SYSTEM_TASK_EXT_1(GetComponent(),
		AnimationSystem, AsyncTaskRunner, fadeTime,
		{
			self->m_blendTime = fadeTime;
			self->m_blendTotalTime = fadeTime;
			self->m_currentLayerId = (self->m_currentLayerId + 1) % 2;
		}
	);

}

void AnimBlendLayer::SerializeToBinary(Serializer* serializer, ByteStream& stream) const
{
}

void AnimBlendLayer::DeserializeFromBinary(Serializer* serializer, const ByteStream& stream)
{
}

void AnimBlendLayer::SerializeToJson(Serializer* serializer, json& j) const
{
}

void AnimBlendLayer::DeserializeFromJson(Serializer* serializer, const json& j)
{
}

void AnimBlendLayer::CloneFrom(Serializer* serializer, Serializable* another)
{
	AnimLayer::CloneFrom(serializer, another);

	auto src = (AnimBlendLayer*)another;
	m_input[0] = serializer->Clone(src->m_input[0]);
	m_input[1] = serializer->Clone(src->m_input[1]);
}

Handle<ClassMetadata> AnimBlendLayer::GetMetadata(size_t sign)
{
	return Handle<ClassMetadata>();
}

void AnimBlendLayer::OnPropertyChanged(const UnknownAddress& var, const Variant& newValue)
{
}

NAMESPACE_END