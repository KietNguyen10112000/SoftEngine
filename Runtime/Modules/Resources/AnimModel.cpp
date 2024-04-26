#include "AnimModel.h"

#include "TaskSystem/TaskSystem.h"

NAMESPACE_BEGIN

const char* AnimModel::CACHE_EXTENSION = ".AnimModel";

AnimModel::AnimModel(String path, bool placeholder) : Model3DBasic(path, true)
{
}

void AnimModel::CreateCache(Animation* animation, ByteStream& stream, const String& streamPath)
{
	{
		// write aabb cache

		stream.Put(animation->m_animMeshLocalAABoxKeyFrames.size());

		for (auto& aabbKeyFrames : animation->m_animMeshLocalAABoxKeyFrames)
		{
			stream.Put(aabbKeyFrames.aaBox.size());

			for (auto& aabbKeyFrame : aabbKeyFrames.aaBox)
			{
				stream.Put(aabbKeyFrame);
			}

			stream.Put(aabbKeyFrames.boundAABox);
		}
	}

	FileSystem::Get()->WriteStream(streamPath.c_str(), &stream);
}

void AnimModel::ReadCache(Animation* animation, ByteStream& stream)
{
	{
		// read aabb cache

		size_t temp;
		stream.Pick(temp);
		animation->m_animMeshLocalAABoxKeyFrames.resize(temp);

		for (auto& aabbKeyFrames : animation->m_animMeshLocalAABoxKeyFrames)
		{
			stream.Pick(temp);
			aabbKeyFrames.aaBox.resize(temp);

			for (auto& aabbKeyFrame : aabbKeyFrames.aaBox)
			{
				stream.Pick(aabbKeyFrame);
			}

			stream.Pick(aabbKeyFrames.boundAABox);
		}
	}
}

void AnimModel::LoadAABoxAnimMesh(AnimModel::AnimMesh* animMesh, Animation* animation, AnimMeshVertices* animMeshesVertices)
{
	auto meshId = animMesh->m_model3DIdx;
	auto& aaBoxKeyFrames = animation->m_animMeshLocalAABoxKeyFrames[meshId].aaBox;

	auto& channels = animation->GetChannels();
	for (auto& channel : channels)
	{
		AABoxKeyFrame aaBoxKeyFrame;
		for (auto& keyFrame : channel.scaling)
		{
			aaBoxKeyFrame.time = keyFrame.time;
			auto it = std::lower_bound(aaBoxKeyFrames.begin(), aaBoxKeyFrames.end(), aaBoxKeyFrame,
				[](const AABoxKeyFrame& a, const AABoxKeyFrame& b)
				{
					return a.time < b.time;
				}
			);

			if (it != aaBoxKeyFrames.end() && it->time == keyFrame.time)
			{
				continue;
			}

			if (aaBoxKeyFrames.empty())
			{
				aaBoxKeyFrames.push_back(aaBoxKeyFrame);
			}
			else
			{
				aaBoxKeyFrames.insert(it, aaBoxKeyFrame);
			}
		}

		for (auto& keyFrame : channel.rotation)
		{
			aaBoxKeyFrame.time = keyFrame.time;
			auto it = std::lower_bound(aaBoxKeyFrames.begin(), aaBoxKeyFrames.end(), aaBoxKeyFrame,
				[](const AABoxKeyFrame& a, const AABoxKeyFrame& b)
				{
					return a.time < b.time;
				}
			);

			if (it != aaBoxKeyFrames.end() && it->time == keyFrame.time)
			{
				continue;
			}

			if (aaBoxKeyFrames.empty())
			{
				aaBoxKeyFrames.push_back(aaBoxKeyFrame);
			}
			else
			{
				aaBoxKeyFrames.insert(it, aaBoxKeyFrame);
			}
		}

		for (auto& keyFrame : channel.translation)
		{
			aaBoxKeyFrame.time = keyFrame.time;
			auto it = std::lower_bound(aaBoxKeyFrames.begin(), aaBoxKeyFrames.end(), aaBoxKeyFrame,
				[](const AABoxKeyFrame& a, const AABoxKeyFrame& b)
				{
					return a.time < b.time;
				}
			);

			if (it != aaBoxKeyFrames.end() && it->time == keyFrame.time)
			{
				continue;
			}

			if (aaBoxKeyFrames.empty())
			{
				aaBoxKeyFrames.push_back(aaBoxKeyFrame);
			}
			else
			{
				aaBoxKeyFrames.insert(it, aaBoxKeyFrame);
			}
		}
	}

	//// cal AABB of each key frame
	//std::vector<Task> tasks;
	//tasks.resize(aaBoxKeyFrames.size());
	//
	//std::vector<AnimModelLoadingCtx::CalAABBTaskParam2> taskParams;
	//taskParams.resize(aaBoxKeyFrames.size());

	//for (size_t i = 0; i < aaBoxKeyFrames.size(); i++)
	//{
	//	auto& task = tasks[i];
	//	auto& _param = taskParams[i];

	//	task.Params() = &_param;
	//	task.Entry() = [](void* p)
	//	{
	//		TASK_SYSTEM_UNPACK_PARAM_2(AnimModelLoadingCtx::CalAABBTaskParam2, p, output, param1);

	//		LoadAABoxForKeyFrame(output, param);
	//	};
	//	
	//	_param.output = &aaBoxKeyFrames[i];
	//	_param.param1 = param;

	//	//task.Entry()(task.Params());
	//}

	//TaskSystem::SubmitAndWait(tasks.data(), tasks.size(), Task::CRITICAL);

	auto& nodes = m_nodes;
	auto& vertices = animMeshesVertices->vertices;
	auto& indices = animMeshesVertices->indices;

	std::vector<Mat4> globalTransform;
	globalTransform.resize(nodes.size());

	std::vector<KeyFramesIndex> keyFramesIndex;
	keyFramesIndex.resize(nodes.size());

	std::vector<Mat4> bones;
	bones.resize(m_boneOffsetMatrixs.size());

	for (auto& aaBoxKeyFrame : aaBoxKeyFrames)
	{
		//::memset(keyFramesIndex.data(), 0, keyFramesIndex.size() * sizeof(KeyFramesIndex));

		for (size_t i = 0; i < nodes.size(); i++)
		{
			auto& node = nodes[i];
			globalTransform[i] = node.localTransform;

			auto channelId = animation->m_nodeToChannelId[i];

			if (channelId != INVALID_ID)
			{
				auto& channel = channels[channelId];
				auto& index = keyFramesIndex[channelId];
				//auto& tempId = tempIndex[node.boneId];

				Mat4 scaling;
				channel.FindScaleMatrix(&scaling, &index.s, index.s, aaBoxKeyFrame.time);
				Mat4 rotation;
				channel.FindRotationMatrix(&rotation, &index.r, index.r, aaBoxKeyFrame.time);
				Mat4 translation;
				channel.FindTranslationMatrix(&translation, &index.t, index.t, aaBoxKeyFrame.time);

				globalTransform[i] = scaling * rotation * translation;
			}

			globalTransform[i] = globalTransform[i] *
				(node.parentId == INVALID_ID ? Mat4::Identity() : globalTransform[node.parentId]);

			/*if (node.parentId != INVALID_ID)
			{
				assert(node.id > node.parentId);
			}*/

			if (channelId != INVALID_ID)
			{
				bones[node.boneId] = m_boneOffsetMatrixs[node.boneId] * globalTransform[i];
			}
		}

		float
			maxX = -FLT_MAX, minX = FLT_MAX,
			maxY = -FLT_MAX, minY = FLT_MAX,
			maxZ = -FLT_MAX, minZ = FLT_MAX;

		for (auto& index : indices)
		{
			auto& vertex = vertices[index];

			Mat4 boneTransform;
			::memset(&boneTransform, 0, sizeof(Mat4));
			for (uint32_t i = 0; i < 16; i++)
			{
				boneTransform += bones[vertex.boneID[i]] * vertex.weight[i];
			}

			auto position = Vec4(vertex.position, 1.0f) * boneTransform;
			position /= position.w;

			maxX = std::max(maxX, position.x);
			minX = std::min(minX, position.x);

			maxY = std::max(maxY, position.y);
			minY = std::min(minY, position.y);

			maxZ = std::max(maxZ, position.z);
			minZ = std::min(minZ, position.z);
		}

		auto dimensions = Vec3(maxX - minX, maxY - minY, maxZ - minZ);
		auto center = Vec3(minX, minY, minZ) + dimensions / 2.0f;
		aaBoxKeyFrame.value = AABox(center, dimensions);
	}

	auto boundAABox = aaBoxKeyFrames[0].value;
	for (auto& keyFrame : aaBoxKeyFrames)
	{
		boundAABox.Joint(keyFrame.value);
	}

	animation->m_animMeshLocalAABoxKeyFrames[meshId].boundAABox = boundAABox;
}

ID AnimModel::PlaceHolderAnimation(const Resource<AnimMotion>& motion)
{
	auto animationId = m_animations.size();
	auto animation = std::make_unique<Animation>();

	animation->m_motion = motion;

	auto& nodes = m_nodes;
	auto numNode = nodes.size();
	animation->m_nodeToChannelId.resize(numNode);

	auto& channels = animation->GetChannels();
	auto& channelsName = animation->m_motion->m_nodeNameEffectedByChannel;
	auto numChannel = channels.size();

	// map channel to bone
	for (size_t i = 0; i < numChannel; i++)
	{
		auto& nodeName = channelsName[i];
		auto it = m_nodeIds.find(nodeName);

		if (it != m_nodeIds.end())
		{
			animation->m_nodeToChannelId[it->second] = i;
		}
	}

	animation->m_animMeshLocalAABoxKeyFrames.resize(m_animMeshes.size());

	m_animations.push_back(animation);
	return animationId;
}

void AnimModel::LoadAnimation(ID animationId, const AnimMotion* motion, AnimMeshVertices* vertices)
{
	auto animation = m_animations[animationId].get();

	if (vertices == nullptr)
	{
		assert(0);

		// TODO: reload vertices from model file
	}

	// load AABB key frames for this animation

	auto myPath = GetPath();
	auto modelPath = motion->GetModelFilePath();
	ByteStream stream;
	modelPath = "Resources/" + modelPath;
	auto streamPath = (myPath + "." + modelPath + "." + String::From(animationId) + "." + AnimModel::CACHE_EXTENSION);
	if (FileSystem::Get()->IsFileChanged(myPath.c_str())
		|| FileSystem::Get()->IsFileChanged(modelPath.c_str())
		|| !FileSystem::Get()->ReadStream(streamPath.c_str(), &stream))
	{
		struct Param
		{
			Animation* animation;
			AnimMesh* animMesh;
			AnimMeshVertices* vertices;
			AnimModel* model;
		};

		auto numTasks = m_animMeshes.size();
		std::vector<Task> tasks;
		tasks.resize(numTasks);

		std::vector<Param> taskParams;
		taskParams.resize(numTasks);

		size_t count = 0;
		for (auto& animMesh : m_animMeshes)
		{
			{
				//LoadAABoxAnimMesh(&ctx, model3D, &animMesh, &animation, scene);
				//std::cout << count << "\n";
				//if (count++ == 4) break;
				//break;

				auto& task = tasks[count];
				auto& param = taskParams[count];

				task.Params() = &param;
				task.Entry() = [](void* p)
					{
						TASK_SYSTEM_UNPACK_PARAM_REF_4(Param, p, animation, animMesh, vertices, model);
						model->LoadAABoxAnimMesh(animMesh, animation, vertices);
					};

				param.animation = animation;
				param.animMesh = &animMesh;
				param.vertices = vertices + count;
				param.model = this;

				//task.Entry()(task.Params());

				count++;
			}

			//std::cout << count << "\n";
			//if (count++ == 10) break;
		}

		TaskSystem::SubmitAndWait(tasks.data(), tasks.size(), Task::CRITICAL);

		CreateCache(animation, stream, streamPath);
	}
	else
	{
		ReadCache(animation, stream);
	}
}

ID AnimModel::AddAnimation(const Resource<AnimMotion>& motion, AnimMeshVertices* vertices)
{
	auto animationId = PlaceHolderAnimation(motion);
	LoadAnimation(animationId, motion, vertices);
	return animationId;
}

NAMESPACE_END