#include "AnimTransitLayer.h"

#include "MainSystem/Animation/Components/AnimationComponent.h"
#include "MainSystem/Animation/Components/AnimatorSkeletalArray.h"
#include "MainSystem/Animation/AnimationSystem.h"
#include "MainSystem/MainSystemTaskPacking.h"
#include "MainSystem/Rendering/RenderingSystem.h"
#include "MainSystem/Scripting/ScriptingSystem.h"

#include "Scene/Scene.h"
#include "Scene/GameObject.h"

#include "AnimPlayerLayer.h"

NAMESPACE_BEGIN

void AnimTransitLayer::OnEndTransit()
{
	m_lock.lock();
	for (auto& event : m_events)
	{
		switch (event->m_callerCompId)
		{
		case MainSystemInfo::RENDERING_ID: {
			auto system = GetCommittedObject()->GetScene()->GetRenderingSystem();
			system->MAsyncTaskRunnerST()->RunAsync(event->m_callback);
			break;
		}
		case MainSystemInfo::SCRIPTING_ID: {
			auto system = GetCommittedObject()->GetScene()->GetScriptingSystem();
			system->MAsyncTaskRunnerST()->RunAsync(event->m_callback);
			break;
		}
		default:
			break;
		}
	}
	m_lock.unlock();
}

void AnimTransitLayer::Initialize()
{
	m_lastLocalTransforms.resize(NodeLocalTransforms().size());
}

void AnimTransitLayer::PrevRun(float dt)
{
	if (m_transitTime < dt && m_transitTime > -1.0f)
	{
		m_transitTime = -1.0f;

		OnEndTransit();

		if (m_curFadeState.direction == TransitDirection::BACKWARD)
		{
			auto l0 = dynamic_cast<AnimPlayerLayer*>(m_input->GetOutput());
			if (l0)
			{
				l0->m_needResetKeyFrameIndex = true;
				l0->SetAnimationImpl(m_curFadeState.animation, m_curFadeState.startTime, m_curFadeState.endTime);
			}
		}
		m_curFadeState = {};

		if (!m_queue.empty())
		{
			auto& currentFateState = m_queue.front();
			m_transitTime = currentFateState.fadeTime;
			m_transitTotalTime = currentFateState.fadeTime;
			m_curFadeState = currentFateState;

			auto l0 = dynamic_cast<AnimPlayerLayer*>(m_input->GetOutput());
			if (l0)
			{
				l0->m_needResetKeyFrameIndex = true;
				l0->SetAnimationImpl(currentFateState.animation, currentFateState.startTime, currentFateState.endTime);
			}

			auto lastLayer = GetComponentAs<AnimatorSkeletalArray>()->GetLastAnimLayerOutput();
			if (lastLayer)
			{
				if (currentFateState.direction == TransitDirection::FORWARD)
				{
					std::memcpy(m_lastLocalTransforms.data(), lastLayer->NodeLocalTransforms().data(), m_lastLocalTransforms.size() * sizeof(Mat4));
				}
				else if (currentFateState.direction == TransitDirection::BACKWARD)
				{
					AnimPlayerLayer::MakeClipCut(m_lastLocalTransforms, m_model,
						currentFateState.animation.get(), currentFateState.startTime * currentFateState.animation->GetTicksPerSecond());
				}
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

	auto input = m_input->GetOutput();

	if (m_transitTime < dt)
	{
		/*if (m_transitTime > -1.0f && !m_input->IsEnable())
		{
			m_input->SetEnable(true);
		}

		m_transitTime = -1.0f;*/
		return;
	}

	//m_input->SetEnable(false);

	m_transitTime -= dt;

	auto sBlend = 1.0f - std::min(1.0f, m_transitTime / m_transitTotalTime);
	if (m_curFadeState.direction == TransitDirection::BACKWARD)
	{
		sBlend = 1 - sBlend;
	}

	auto num = m_localTransforms.size();

	auto& transforms0 = input->NodeLocalTransforms();
	auto& transforms1 = m_localTransforms;
	//auto& ltransforms0 = curLayer->NodeLocalTransforms();
	//auto& ltransforms1 = prevLayer->NodeLocalTransforms();
	for (size_t i = 0; i < num; i++)
	{
		auto& v0 = transforms0[i];
		auto& v1 = transforms1[i];

		m_localTransforms[i] = ActionInterpolation<Transform>::InterpolationFnStruct<Transform>::Fn(v1, v0, sBlend);
	}
}

AnimLayer* AnimTransitLayer::GetOutput()
{
	if (!IsEnabledImpl())
	{
		return m_input->GetOutput();
	}

	if (m_transitTime < 0)
	{
		return m_input->GetOutput();
	}

	return this;
}

void AnimTransitLayer::SetInput(AnimLayer* l)
{
	m_input = l;
}

void AnimTransitLayer::FadeTo(TransitDirection::DIRECTION direction, float fadeTime, const SharedPtr<Animation>& animation, float startTime, float endTime)
{
	if (direction == TransitDirection::FORWARD)
	{
		auto l0 = dynamic_cast<AnimPlayerLayer*>(m_input->GetOutput());
		if (l0)
		{
			l0->SetAnimation(animation, startTime, endTime);
		}
	}

	auto startTick = std::max(0.0f, startTime) * animation->GetTicksPerSecond();
	m_lastFadeState.animation = animation;
	m_lastFadeState.direction = direction;
	m_lastFadeState.startTime = startTime;
	m_lastFadeState.endTime = endTime;
	m_lastFadeState.fadeTime = fadeTime;

	MAIN_SYSTEM_TASK_IMPL_4(GetComponent(),
		AnimationSystem, AsyncTaskRunner, fadeTime, direction, animation, startTick,
		{
			self->m_transitTime = fadeTime;
			self->m_transitTotalTime = fadeTime;

			auto lastLayer = self->GetComponentAs<AnimatorSkeletalArray>()->GetLastAnimLayerOutput();
			if (lastLayer)
			{
				if (direction == TransitDirection::FORWARD)
				{
					std::memcpy(self->m_lastLocalTransforms.data(), lastLayer->NodeLocalTransforms().data(), self->m_lastLocalTransforms.size() * sizeof(Mat4));
				}
				else if (direction == TransitDirection::BACKWARD)
				{
					AnimPlayerLayer::MakeClipCut(self->m_lastLocalTransforms, self->m_model,
						animation.get(), startTick);
				}
			}

			self->m_curFadeState = self->m_lastFadeState;
		}
	);

}

void AnimTransitLayer::QueuedFadeTo(TransitDirection::DIRECTION direction, float fadeTime, const SharedPtr<Animation>& animation, float startTime, float endTime)
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

	m_lastLocalTransforms.resize(m_lastLocalTransforms.size());

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

void AnimTransitLayer::RemoveListener(EventListener* listener)
{
	if (listener->m_id == uint32_t(INVALID_ID) || listener != m_events[listener->m_id])
	{
		return;
	}

	m_lock.lock();

	m_events.Remove(m_events.begin() + listener->m_id);
	for (auto& e : m_events)
	{
		e->m_id = &e - m_events.data();
	}

	m_lock.unlock();
}

NAMESPACE_END