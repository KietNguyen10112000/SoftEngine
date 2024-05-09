#include "AnimPlayerLayer.h"

#include "MainSystem/Animation/Components/AnimationComponent.h"
#include "MainSystem/Animation/AnimationSystem.h"
#include "MainSystem/MainSystemTaskPacking.h"

#include "Scene/Scene.h"
#include "Scene/GameObject.h"

#include "Resources/AnimModel.h"

NAMESPACE_BEGIN

void AnimPlayerLayer::Run(float dt)
{
	m_t += dt * m_ticksPerSecond;

	if (m_t > m_tickDuration)
	{
		uint32_t num = (uint32_t)std::floor(m_t / m_tickDuration);
		m_t -= num * m_tickDuration;

		std::memcpy(m_keyFramesIndex.data(), m_startKeyFrameIndex.data(),
			m_keyFramesIndex.size() * sizeof(KeyFramesIndex));

		std::memcpy(m_aabbKeyFrameIndex.data(), m_startAABBKeyFrameIndex.data(),
			m_aabbKeyFrameIndex.size() * sizeof(uint32_t));
	}

	auto t = m_t + m_startTick;

	auto& nodes = m_model->m_nodes;
	auto& globalTransforms = m_globalTransforms;
	auto& nodeToChannelId = m_animation->GetNodeToChannelId();
	auto& channels = m_animation->GetChannels();

	// root transform
	{
		auto& node = nodes[0];

		globalTransforms[0] = GetGameObject()->ReadGlobalTransformMat();

		auto& channelId = nodeToChannelId[0];
		if (channelId != INVALID_ID)
		{
			auto& channel = channels[channelId];
			auto& index = m_keyFramesIndex[channelId];

			Mat4 scaling;
			channel.FindScaleMatrix(&scaling, &index.s, index.s, t);
			Mat4 rotation;
			channel.FindRotationMatrix(&rotation, &index.r, index.r, t);
			Mat4 translation;
			channel.FindTranslationMatrix(&translation, &index.t, index.t, t);

			globalTransforms[0] = scaling * rotation * translation;
		}

		assert(node.parentId == INVALID_ID);
	}

	{
		auto num = nodes.size();
		for (size_t i = 1; i < num; i++)
		{
			auto& node = nodes[i];
			auto& channelId = nodeToChannelId[i];
			auto& globalTransform = globalTransforms[i];

			globalTransform = node.localTransform;

			if (channelId != INVALID_ID)
			{
				auto& channel = channels[channelId];
				auto& index = m_keyFramesIndex[channelId];

				Mat4 scaling;
				channel.FindScaleMatrix(&scaling, &index.s, index.s, t);
				Mat4 rotation;
				channel.FindRotationMatrix(&rotation, &index.r, index.r, t);
				Mat4 translation;
				channel.FindTranslationMatrix(&translation, &index.t, index.t, t);

				globalTransform = scaling * rotation * translation;
			}

			globalTransform = globalTransform * globalTransforms[node.parentId];
		}
	}

	{
		auto& animMeshLocalAABoxKeyFrames = m_animation->GetMeshLocalAABBKeyFrames();
		auto num = animMeshLocalAABoxKeyFrames.size();
		for (uint32_t i = 0; i < num; i++)
		{
			auto& index = m_aabbKeyFrameIndex[i];
			m_meshesAABB[i] = animMeshLocalAABoxKeyFrames[i].Find(&index, index, t);
		}
	}
}

void AnimPlayerLayer::SetAnimationImpl(Animation* animation, float startTime, float endTime)
{
	m_animation = animation;//m_model->m_animations[animationId];

	auto& channels = m_animation->GetChannels();
	auto& animMeshLocalAABoxKeyFrames = m_animation->GetMeshLocalAABBKeyFrames();

	auto startTick = startTime < 0 ? 0 : startTime * m_animation->GetTicksPerSecond();
	auto endTick = endTime < 0 ? m_animation->GetTickDuration() : endTime * m_animation->GetTicksPerSecond();

	auto& startIndex = m_startKeyFrameIndex;
	auto& startAABBIndex = m_startAABBKeyFrameIndex;

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

	num = (uint32_t)animMeshLocalAABoxKeyFrames.size();
	startAABBIndex.resize(num);
	m_aabbKeyFrameIndex.resize(num);

	for (uint32_t i = 0; i < num; i++)
	{
		auto& channel = animMeshLocalAABoxKeyFrames[i];
		auto& index = startAABBIndex[i];

		channel.BinaryFind(startTick, &index);
	}

	m_startTick = startTick;
	m_tickDuration = endTick - startTick;
	m_ticksPerSecond = m_animation->GetTicksPerSecond();
}

void AnimPlayerLayer::SetAnimation(ID animationId, float startTime, float endTime)
{
	auto animation = m_model->m_animations[animationId];
	if (!GetGameObject() || !GetGameObject()->IsInAnyScene())
	{
		this->SetAnimationImpl(animation, startTime, endTime);
		return;
	}

	MAIN_SYSTEM_TASK_EXT_3(GetComponent(),
		AnimationSystem, AsyncTaskRunner, animation, startTime, endTime,
		{
			self->SetAnimationImpl(animation, startTime, endTime);
		}
	);
}

void AnimPlayerLayer::CloneFrom(Serializer* serializer, Serializable* another)
{
	AnimLayer::CloneFrom(serializer, another);

	auto src = (AnimPlayerLayer*)another;
	m_animation					= src->m_animation;
	m_keyFramesIndex			= src->m_keyFramesIndex;
	m_aabbKeyFrameIndex			= src->m_aabbKeyFrameIndex;
	m_startKeyFrameIndex		= src->m_startKeyFrameIndex;
	m_startAABBKeyFrameIndex	= src->m_startAABBKeyFrameIndex;
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
	
	{
		auto arr = json::array();
		for (auto& v : m_startAABBKeyFrameIndex)
		{
			arr.push_back(v);
		}
		j["StartAABBKeyFrameIndex"] = arr;
	}

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
		animation = m_model->m_animations[m_model->AddAnimation(motion)];
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

	{
		auto& arr = j["StartAABBKeyFrameIndex"];
		auto count = arr.size();
		for (size_t i = 0; i < count; i++)
		{
			auto& j1 = arr[i];
			uint32_t index = j1;
			m_startAABBKeyFrameIndex.push_back(index);
		}
	}

	m_tickDuration		= j["TickDuration"];
	m_ticksPerSecond	= j["TicksPerSecond"];
	m_startTick			= j["StartTick"];
	m_t					= j["Time"];

	m_keyFramesIndex.resize(m_startKeyFrameIndex.size());
	m_aabbKeyFrameIndex.resize(m_startAABBKeyFrameIndex.size());

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

NAMESPACE_END