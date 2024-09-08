#include "AnimPlayerLayer.h"

#include "MainSystem/Animation/Components/AnimationComponent.h"
#include "MainSystem/Animation/AnimationSystem.h"
#include "MainSystem/MainSystemTaskPacking.h"
#include "MainSystem/Rendering/RenderingSystem.h"
#include "MainSystem/Scripting/ScriptingSystem.h"

#include "Scene/Scene.h"
#include "Scene/GameObject.h"

#include "Resources/AnimModel.h"

NAMESPACE_BEGIN

void AnimPlayerLayer::Run(float dt)
{
	size_t iter = 0;
	if (GetComponent()->GetGameObject())
	{
		auto scene = GetComponent()->GetGameObject()->GetScene();
		if (scene)
		{
			iter = scene->GetIterationCount();
		}
	}

	m_timeLock.lock();

	m_t += dt * m_ticksPerSecond;

	// early dispatch event
	auto prevT = m_t;
	auto nextT = prevT + dt * m_ticksPerSecond;
	m_lock.lock();
	for (auto& event : m_events)
	{
		if (prevT < event->m_t && nextT > event->m_t)
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
	}
	m_lock.unlock();

	if (m_t > m_tickDuration)
	{
		uint32_t num = (uint32_t)std::floor(m_t / m_tickDuration);
		m_t -= num * m_tickDuration;

		std::memcpy(m_keyFramesIndex.data(), m_startKeyFrameIndex.data(),
			m_keyFramesIndex.size() * sizeof(KeyFramesIndex));
	}

	m_timeSign = bool(iter % 2);
	m_timeLock.unlock();

	auto t = m_t + m_startTick;

	auto& nodes = m_model->m_nodes;
	auto& nodeToChannelId = m_animation->GetNodeToChannelId();
	auto& channels = m_animation->GetChannels();

	auto rootBoneNodeId = m_model->m_rootBoneNodeId;

	//// root transform
	//{
	//	auto& node = nodes[0];
	//	auto& localTransform = m_localTransforms[0];
	//	localTransform = node.localTransform;//GetGameObject()->ReadGlobalTransformMat();

	//	auto& channelId = nodeToChannelId[0];
	//	if (channelId != INVALID_ID)
	//	{
	//		auto& channel = channels[channelId];
	//		auto& index = m_keyFramesIndex[channelId];

	//		if (rootBoneNodeId != 0)
	//		{
	//			localTransform.Scale()			= channel.FindScale(&index.s, index.s, t);
	//			localTransform.Rotation()		= channel.FindRotation(&index.r, index.r, t);
	//			localTransform.Translation()	= channel.FindTranslation(&index.t, index.t, t);
	//		}
	//		else
	//		{
	//			if (!m_disableRootMotionScaling)
	//			{
	//				localTransform.Scale() = channel.FindScale(&index.s, index.s, t);
	//			}
	//			if (!m_disableRootMotionRotation)
	//			{
	//				localTransform.Rotation() = channel.FindRotation(&index.r, index.r, t);
	//			}
	//			if (!m_disableRootMotionTranslation)
	//			{
	//				localTransform.Translation() = channel.FindTranslation(&index.t, index.t, t);
	//			}

	//			if (m_disableRootMotionScaling && m_disableRootMotionRotation && m_disableRootMotionTranslation)
	//			{
	//				localTransform = {};
	//			}
	//		}
	//	}

	//	assert(node.parentId == INVALID_ID);
	//}

	{
		auto num = nodes.size();
		for (size_t i = 0; i < num; i++)
		{
			auto& node = nodes[i];
			auto& channelId = nodeToChannelId[i];
			auto& localTransform = m_localTransforms[i];

			localTransform = node.localTransform;
			if (channelId != INVALID_ID)
			{
				auto& channel = channels[channelId];
				auto& index = m_keyFramesIndex[channelId];

				if (rootBoneNodeId != i)
				{
					localTransform.Scale()			= channel.FindScale(&index.s, index.s, t);
					localTransform.Rotation()		= channel.FindRotation(&index.r, index.r, t);
					localTransform.Translation()	= channel.FindTranslation(&index.t, index.t, t);
				}
				else
				{
					if (!m_disableRootMotionScaling)
					{
						localTransform.Scale() = channel.FindScale(&index.s, index.s, t);
					}
					if (!m_disableRootMotionRotation)
					{
						localTransform.Rotation() = channel.FindRotation(&index.r, index.r, t);
					}
					if (!m_disableRootMotionTranslation)
					{
						localTransform.Translation() = channel.FindTranslation(&index.t, index.t, t);
					}

					if (m_disableRootMotionScaling && m_disableRootMotionRotation && m_disableRootMotionTranslation)
					{
						localTransform = {};
					}
				}
			}
		}
	}
}

void AnimPlayerLayer::SetAnimationImpl(const SharedPtr<Animation>& animation, float startTime, float endTime)
{
	m_animation = animation;//m_model->m_animations[animationId];

	auto& channels = m_animation->GetChannels();

	auto startTick = startTime < 0 ? 0 : startTime * m_animation->GetTicksPerSecond();
	auto endTick = endTime < 0 ? m_animation->GetTickDuration() : endTime * m_animation->GetTicksPerSecond();

	auto& startIndex = m_startKeyFrameIndex;

	auto num = (uint32_t)channels.size();
	startIndex.resize(num);
	m_keyFramesIndex.resize(num);

	for (uint32_t i = 0; i < num; i++)
	{
		auto& channel = channels[i];
		auto& index = startIndex[i];

		channel.BinaryFindScale(startTick, &index.s);
		channel.BinaryFindRotation(startTick, &index.r);
		channel.BinaryFindTranslation(startTick, &index.t);
	}

	m_startTick = startTick;
	m_tickDuration = endTick - startTick;
	m_ticksPerSecond = m_animation->GetTicksPerSecond();

	if (m_needResetKeyFrameIndex)
	{
		std::memcpy(m_keyFramesIndex.data(), m_startKeyFrameIndex.data(),
			m_keyFramesIndex.size() * sizeof(KeyFramesIndex));

		m_needResetKeyFrameIndex = false;
	}

	m_t = 0;
}

void AnimPlayerLayer::SetAnimation(const SharedPtr<Animation>& animation, float startTime, float endTime)
{
	assert(m_model->FindAnimation(animation->GetMotion()) == animation);

	m_needResetKeyFrameIndex = true;

	//auto animation = m_model->m_animations[animationId];
	if (!GetCommittedObject() || !GetCommittedObject()->IsInAnyScene())
	{
		this->SetAnimationImpl(animation, startTime, endTime);
		return;
	}

	MAIN_SYSTEM_TASK_IMPL_3(GetComponent(),
		AnimationSystem, AsyncTaskRunner, animation, startTime, endTime,
		{
			self->SetAnimationImpl(animation, startTime, endTime);
		}
	);
}

void AnimPlayerLayer::SetTimeImpl(float tick, float startTick, float tickDuration, float tickPerSecond)
{
	if (tick >= 0)
	{
		m_t = tick;
	}

	if (startTick >= 0)
	{
		m_startTick = startTick;

		auto& channels = m_animation->GetChannels();
		auto& startIndex = m_startKeyFrameIndex;

		auto num = (uint32_t)channels.size();
		startIndex.resize(num);
		m_keyFramesIndex.resize(num);
		for (uint32_t i = 0; i < num; i++)
		{
			auto& channel = channels[i];
			auto& index = startIndex[i];

			channel.BinaryFindScale(startTick, &index.s);
			channel.BinaryFindRotation(startTick, &index.r);
			channel.BinaryFindTranslation(startTick, &index.t);
		}
	}

	if (tickDuration >= 0)
	{
		m_tickDuration = tickDuration;
	}

	if (tickPerSecond >= 0)
	{
		m_ticksPerSecond = tickPerSecond;
	}

	if (m_needResetKeyFrameIndex)
	{
		std::memcpy(m_keyFramesIndex.data(), m_startKeyFrameIndex.data(),
			m_keyFramesIndex.size() * sizeof(KeyFramesIndex));

		m_needResetKeyFrameIndex = false;
	}

	if (!IsEnabledImpl())
	{
		Run(0);
	}
}

void AnimPlayerLayer::SetCurrentTimeImpl(float t)
{
	m_needResetKeyFrameIndex = true;

	auto tick = t * m_ticksPerSecond;
	tick = std::clamp(tick, m_startTick, m_startTick + m_tickDuration);

	SetTimeImpl(tick, -1, -1, -1);
}

void AnimPlayerLayer::SetCurrentTime(float t)
{
	MAIN_SYSTEM_TASK_IMPL_COMMON_1(GetComponent(),
		AnimationSystem, AsyncTaskRunner, t,
		{
			self->SetCurrentTimeImpl(t);
		}
	);
}

void AnimPlayerLayer::SetStartTimeImpl(float t)
{
	m_needResetKeyFrameIndex = true;

	auto startTick = t < 0 ? 0 : t * m_animation->GetTicksPerSecond();
	startTick = std::clamp(startTick, 0.0f, m_animation->GetTickDuration());

	float tick = (m_t + m_startTick) - startTick;
	if (tick < 0)
	{
		tick = 0;
	}

	float tickDuration = (m_tickDuration + m_startTick - startTick);
	if (tickDuration < 0)
	{
		tickDuration = 0;
	}

	SetTimeImpl(tick, startTick, tickDuration, -1);
}

void AnimPlayerLayer::SetStartTime(float t)
{
	MAIN_SYSTEM_TASK_IMPL_COMMON_1(GetComponent(),
		AnimationSystem, AsyncTaskRunner, t,
		{
			self->SetStartTimeImpl(t);
		}
	);
}

void AnimPlayerLayer::SetEndTimeImpl(float t)
{
	m_needResetKeyFrameIndex = true;

	auto endTick = t < 0 ? m_animation->GetTickDuration() : t * m_animation->GetTicksPerSecond();
	endTick = std::clamp(endTick, 0.0f, m_animation->GetTickDuration());

	float tickDuration = endTick - m_startTick;
	if (tickDuration < 0)
	{
		tickDuration = 0;
	}

	float tick = -1;
	if (endTick <= m_t)
	{
		tick = endTick;
	}

	float startTick = -1;
	if (endTick <= m_startTick)
	{
		startTick = endTick;
	}

	SetTimeImpl(tick, startTick, tickDuration, -1);
}

void AnimPlayerLayer::SetEndTime(float t)
{
	MAIN_SYSTEM_TASK_IMPL_1(GetComponent(),
		AnimationSystem, AsyncTaskRunner, t,
		{
			self->SetEndTimeImpl(t);
		}
	);
}

void AnimPlayerLayer::SetDurationImpl(float duration)
{
	//m_needResetKeyFrameIndex = true;

	duration = std::max(0.0f, duration);
	float tickPerSecond = m_tickDuration / duration;
	//SetTimeImpl(-1, -1, -1, tickPerSecond);
	m_ticksPerSecond = tickPerSecond;
}

void AnimPlayerLayer::SetDuration(float duration)
{
	MAIN_SYSTEM_TASK_IMPL_COMMON_1(GetComponent(),
		AnimationSystem, AsyncTaskRunner, duration,
		{
			self->SetDurationImpl(duration);
		}
	);
}

void AnimPlayerLayer::SetTimeImpl_(float currentTime, float startTime, float endTime, float duration)
{
	m_needResetKeyFrameIndex = true;

	auto tick = currentTime * m_animation->GetTicksPerSecond();

	auto startTick = startTime < 0 ? 0 : startTime * m_animation->GetTicksPerSecond();
	startTick = std::clamp(startTick, 0.0f, m_animation->GetTickDuration());

	auto endTick = endTime < 0 ? m_animation->GetTickDuration() : endTime * m_animation->GetTicksPerSecond();
	endTick = std::clamp(endTick, startTick, m_animation->GetTickDuration());
	auto tickDuration = endTick - startTick;

	duration = std::max(0.0f, duration);
	float tickPerSecond = m_tickDuration / duration;

	tick = std::clamp(tick, startTick, endTick);

	SetTimeImpl(tick, startTick, tickDuration, tickPerSecond);
}

void AnimPlayerLayer::SetTime(float currentTime, float startTime, float endTime, float duration)
{
	MAIN_SYSTEM_TASK_IMPL_COMMON_4(GetComponent(),
		AnimationSystem, AsyncTaskRunner, currentTime, startTime, endTime, duration,
		{
			self->SetTimeImpl_(currentTime, startTime, endTime, duration);
		}
	);
}

float AnimPlayerLayer::GetTime() const
{
	auto scene = ((AnimPlayerLayer*)this)->GetComponent()->GetGameObject()->GetScene();
	auto iter = scene->GetIterationCount();
	auto dt = scene->Dt();
	auto sign = bool(iter % 2);

	m_timeLock.lock();
	auto t = m_t;
	auto timeSign = m_timeSign;
	m_timeLock.unlock();

	return (t / m_ticksPerSecond) + (timeSign == sign ? 0.0f : dt);
}

void AnimPlayerLayer::RemoveListener(EventListener* listener)
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

void AnimPlayerLayer::SetEnableRootMotion(bool enableScaling, bool enableRotation, bool enableTranslation)
{
	MAIN_SYSTEM_TASK_IMPL_COMMON_3(GetComponent(),
		AnimationSystem, AsyncTaskRunner, enableScaling, enableRotation, enableTranslation,
		{
			self->m_disableRootMotionScaling = !enableScaling;
			self->m_disableRootMotionRotation = !enableRotation;
			self->m_disableRootMotionTranslation = !enableTranslation;
		}
	);
}

void AnimPlayerLayer::CloneFrom(Serializer* serializer, Serializable* another)
{
	AnimLayer::CloneFrom(serializer, another);

	auto src = (AnimPlayerLayer*)another;
	m_animation					= src->m_animation;
	m_keyFramesIndex			= src->m_keyFramesIndex;
	m_startKeyFrameIndex		= src->m_startKeyFrameIndex;
	m_tickDuration				= src->m_tickDuration;
	m_ticksPerSecond			= src->m_ticksPerSecond;
	m_startTick					= src->m_startTick;
	m_t							= src->m_t;

}


void AnimPlayerLayer::SerializeToBinary(Serializer* serializer, ByteStream& stream) const
{
}

void AnimPlayerLayer::DeserializeFromBinary(Serializer* serializer, const ByteStream& stream)
{
}

void AnimPlayerLayer::SerializeToJson(Serializer* serializer, json& j) const
{
	AnimLayer::SerializeToJson(serializer, j);

	j["AnimMotion"] = serializer->Serialize(m_animation->GetMotion());

	/*{
		auto arr = json::array();
		for (auto& v : m_startKeyFrameIndex)
		{
			json j1;
			j1["r"] = v.r;
			j1["s"] = v.s;
			j1["t"] = v.t;
			arr.push_back(j1);
		}
		j["StartKeyFrameIndex"] = arr;
	}*/

	j["TickDuration"]		= m_tickDuration;
	j["TicksPerSecond"]		= m_ticksPerSecond;
	j["StartTick"]			= m_startTick;
	j["Time"]				= m_t;
}

void AnimPlayerLayer::DeserializeFromJson(Serializer* serializer, const json& j)
{
	AnimLayer::DeserializeFromJson(serializer, j);

	Resource<AnimMotion> motion;
	serializer->Deserialize(j["AnimMotion"], motion);

	auto animation = m_model->FindAnimation(motion);
	if (!animation)
	{
		animation = m_model->AddAnimation(motion);
	}
	m_animation = animation;

	/*{
		auto& arr = j["StartKeyFrameIndex"];
		auto count = arr.size();
		for (size_t i = 0; i < count; i++)
		{
			auto& j1 = arr[i];
			KeyFramesIndex index;
			index.r = j1["r"];
			index.s = j1["s"];
			index.t = j1["t"];
			m_startKeyFrameIndex.push_back(index);
		}
	}*/

	m_tickDuration		= j["TickDuration"];
	m_ticksPerSecond	= j["TicksPerSecond"];
	m_startTick			= j["StartTick"];
	m_t					= j["Time"];

	m_keyFramesIndex.resize(m_startKeyFrameIndex.size());

	auto endTick = m_startTick + m_tickDuration;
	SetAnimationImpl(
		m_animation,
		m_startTick / m_animation->GetTicksPerSecond(),
		endTick / m_animation->GetTicksPerSecond()
	);

	Run(0);
}

Handle<ClassMetadata> AnimPlayerLayer::GetMetadata(size_t sign)
{
	return Handle<ClassMetadata>();
}

void AnimPlayerLayer::OnPropertyChanged(const UnknownAddress& var, const Variant& newValue)
{
}

void AnimPlayerLayer::MakeClipCut(std::vector<Transform>& localTransforms, AnimModel* model, Animation* animation, float tick)
{
	auto& t = tick;

	auto& nodes = model->m_nodes;
	auto& nodeToChannelId = animation->GetNodeToChannelId();
	auto& channels = animation->GetChannels();

	//{
	//	auto& node = nodes[0];

	//	localTransforms[0] = node.localTransform;//GetGameObject()->ReadGlobalTransformMat();
	//	//localTransforms[0] = node.localTransform;

	//	auto& channelId = nodeToChannelId[0];
	//	if (channelId != INVALID_ID)
	//	{
	//		auto& channel = channels[channelId];

	//		Mat4 scaling;
	//		scaling.SetScale(channel.BinaryFindScale(t, nullptr));
	//		Mat4 rotation;
	//		rotation.SetRotation(channel.BinaryFindRotation(t, nullptr));
	//		Mat4 translation;
	//		translation.SetTranslation(channel.BinaryFindTranslation(t, nullptr));

	//		localTransforms[0] = scaling * rotation * translation;
	//		//localTransforms[0] = globalTransforms[0];
	//	}

	//	assert(node.parentId == INVALID_ID);
	//}

	{
		auto num = nodes.size();
		for (size_t i = 1; i < num; i++)
		{
			auto& node = nodes[i];
			auto& channelId = nodeToChannelId[i];
			auto& localTransform = localTransforms[i];

			localTransform = node.localTransform;

			if (channelId != INVALID_ID)
			{
				auto& channel = channels[channelId];
				localTransform.Scale()			= channel.BinaryFindScale(t, nullptr);
				localTransform.Rotation()		= channel.BinaryFindRotation(t, nullptr);
				localTransform.Translation()	= channel.BinaryFindTranslation(t, nullptr);
			}
		}
	}
}

NAMESPACE_END