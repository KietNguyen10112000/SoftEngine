#include "AnimTransitLayer.h"

#include "MainSystem/Animation/Components/AnimationComponent.h"
#include "MainSystem/Animation/Components/AnimatorSkeletalArray.h"
#include "MainSystem/Animation/AnimationSystem.h"
#include "MainSystem/MainSystemTaskPacking.h"

#include "Scene/Scene.h"
#include "Scene/GameObject.h"

#include "AnimPlayerLayer.h"

NAMESPACE_BEGIN

void AnimTransitLayer::Initialize()
{
	m_lastGlobalTransforms.resize(NodeGlobalTransforms().size());
	m_lastMeshesAABB.resize(MeshesAABB().size());
}

void AnimTransitLayer::PrevRun(float dt)
{
	if (m_transitTime < dt)
	{
		if (!m_queue.empty())
		{
			auto& currentFateState = m_queue.front();
			m_transitTime = currentFateState.fadeTime;
			m_transitTotalTime = currentFateState.fadeTime;

			auto l0 = dynamic_cast<AnimPlayerLayer*>(m_input);
			if (l0)
			{
				l0->SetAnimationImpl(currentFateState.animation, currentFateState.startTime, currentFateState.endTime);
			}

			auto lastLayer = GetComponentAs<AnimatorSkeletalArray>()->GetLastAnimLayerOutput();
			if (lastLayer)
			{
				std::memcpy(m_lastGlobalTransforms.data(), lastLayer->NodeGlobalTransforms().data(), m_lastGlobalTransforms.size() * sizeof(Mat4));
				std::memcpy(m_lastMeshesAABB.data(), lastLayer->MeshesAABB().data(), m_lastMeshesAABB.size() * sizeof(AABox));
			}

			m_queue.pop();
		}
	}
}

void AnimTransitLayer::Run(float dt)
{
	if (!m_input)
	{
		return;
	}

	if (m_transitTime < dt)
	{
		if (m_transitTime > -1.0f && !m_input->IsEnable())
		{
			m_input->SetEnable(true);
		}

		m_transitTime = -1.0f;
		return;
	}

	m_input->SetEnable(false);

	m_transitTime -= dt;

	auto sBlend = 1.0f - std::min(1.0f, m_transitTime / m_transitTotalTime);

	auto num = m_globalTransforms.size();

	auto& transforms0 = m_input->NodeGlobalTransforms();
	auto& transforms1 = m_lastGlobalTransforms;
	//auto& ltransforms0 = curLayer->NodeLocalTransforms();
	//auto& ltransforms1 = prevLayer->NodeLocalTransforms();
	for (size_t i = 0; i < num; i++)
	{
		auto& v0 = transforms0[i];
		auto& v1 = transforms1[i];

		m_globalTransforms[i] = Lerp(v1, v0, sBlend);
		//m_localTransforms[i] = Lerp(ltransforms1[i], ltransforms0[i], sBlend);
	}

	num = m_meshesAABB.size();

	auto& meshAABB0 = m_input->MeshesAABB();
	auto& meshAABB1 = m_lastMeshesAABB;
	for (size_t i = 0; i < num; i++)
	{
		auto& v0 = meshAABB0[i];
		auto& v1 = meshAABB1[i];

		auto& aabb = m_meshesAABB[i];
		aabb.m_center = Lerp(v1.m_center, v0.m_center, sBlend);
		aabb.m_halfDimensions = Lerp(v1.m_halfDimensions, v0.m_halfDimensions, sBlend);
	}
}

AnimLayer* AnimTransitLayer::GetOutput()
{
	if (!IsEnable())
	{
		return m_input;
	}

	if (m_transitTime < 0)
	{
		return m_input;
	}

	return this;
}

void AnimTransitLayer::SetInput(AnimLayer* l)
{
	m_input = l;
}

void AnimTransitLayer::FadeTo(float fadeTime, Animation* animation, float startTime, float endTime)
{
	auto l0 = dynamic_cast<AnimPlayerLayer*>(m_input);

	if (l0)
	{
		l0->SetAnimation(animation, startTime, endTime);
	}

	MAIN_SYSTEM_TASK_IMPL_1(GetComponent(),
		AnimationSystem, AsyncTaskRunner, fadeTime,
		{
			self->m_transitTime = fadeTime;
			self->m_transitTotalTime = fadeTime;

			auto lastLayer = self->GetComponentAs<AnimatorSkeletalArray>()->GetLastAnimLayerOutput();
			if (lastLayer)
			{
				std::memcpy(self->m_lastGlobalTransforms.data(), lastLayer->NodeGlobalTransforms().data(), self->m_lastGlobalTransforms.size() * sizeof(Mat4));
				std::memcpy(self->m_lastMeshesAABB.data(), lastLayer->MeshesAABB().data(), self->m_lastMeshesAABB.size() * sizeof(AABox));
			}
		}
	);

}

void AnimTransitLayer::QueuedFadeTo(float fadeTime, Animation* animation, float startTime, float endTime)
{
}

bool AnimTransitLayer::IsEndFade() const
{
	return m_transitTime < 0;
}

void AnimTransitLayer::SerializeToBinary(Serializer* serializer, ByteStream& stream) const
{
}

void AnimTransitLayer::DeserializeFromBinary(Serializer* serializer, const ByteStream& stream)
{
}

void AnimTransitLayer::SerializeToJson(Serializer* serializer, json& j) const
{
	AnimLayer::SerializeToJson(serializer, j);

	j["Input"] = serializer->Serialize(m_input); 
}

void AnimTransitLayer::DeserializeFromJson(Serializer* serializer, const json& j)
{
	AnimLayer::DeserializeFromJson(serializer, j);

	serializer->Deserialize(j["Input"], m_input);

	Run(0);
}

void AnimTransitLayer::CloneFrom(Serializer* serializer, Serializable* another)
{
	AnimLayer::CloneFrom(serializer, another);

	auto src = (AnimTransitLayer*)another;
	m_input = serializer->Clone(src->m_input);
}

Handle<ClassMetadata> AnimTransitLayer::GetMetadata(size_t sign)
{
	return Handle<ClassMetadata>();
}

void AnimTransitLayer::OnPropertyChanged(const UnknownAddress& var, const Variant& newValue)
{
}

NAMESPACE_END