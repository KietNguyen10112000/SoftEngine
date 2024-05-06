#include "AnimModel.h"

#include "TaskSystem/TaskSystem.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "Texture2D.h"
#include "Scene/GameObject.h"

#include "Utils/LoadAnimModel.h"

NAMESPACE_BEGIN

namespace ResourceUtils
{
	extern void LoadAllAnimMeshsForAnimModel(AnimModel* model, const aiScene* scene);
	extern void LoadAllMeshsForModel3DBasic(Model3DBasic* model3D, const aiScene* scene, bool ignoreBones);
	extern void LoadAnimMeshVertices(AnimModel::AnimMeshVertices* animMeshVertices, AnimModel* model, aiMesh* mesh);
	extern void LoadMaterialsForAnimModel(const String& basePath, const String& defaultDiffusePath, std::vector<Resource<Texture2D>>& diffuseTextures, const aiScene* scene);

	inline void LoadAnimModelBoundNodeIds(std::vector<String>& diffuseTextures, AnimModel* model, const aiScene* scene)
	{
		constexpr static void (*ProcessNode)(std::vector<String>&, AnimModel*, const aiScene*, aiNode*, ID&, ID&, ID&) =
		[](std::vector<String>& diffuseTextures, AnimModel* model, const aiScene* scene, aiNode* node, ID& nodeId, ID& staticMeshId, ID& animMeshId)-> void
		{
			for (size_t i = 0; i < node->mNumMeshes; i++)
			{
				auto aiMesh = scene->mMeshes[node->mMeshes[i]];

				if (!aiMesh->HasBones())
				{
					model->m_boundNodeIds.push_back(nodeId);

					if (aiMesh->mMaterialIndex >= 0)
						model->m_meshes[staticMeshId].m_defaultDiffusePath = diffuseTextures[aiMesh->mMaterialIndex];

					staticMeshId++;
					continue;
				}

				if (aiMesh->mMaterialIndex >= 0)
					model->m_animMeshes[animMeshId].m_defaultDiffusePath = diffuseTextures[aiMesh->mMaterialIndex];

				animMeshId++;
			}
			
			nodeId++;
			for (size_t i = 0; i < node->mNumChildren; i++)
			{
				ProcessNode(diffuseTextures, model, scene, node->mChildren[i], nodeId, staticMeshId, animMeshId);
			}
		};

		size_t nodeId = 0;
		size_t staticMeshId = 0;
		size_t animMeshId = 0;
		ProcessNode(diffuseTextures, model, scene, scene->mRootNode, nodeId, staticMeshId, animMeshId);

		model->m_boundNodeIds.resize(model->m_meshes.size() + model->m_animMeshes.size(), INVALID_ID);
	}
}

const char* AnimModel::CACHE_EXTENSION = ".AnimModel";

AnimModel::AnimModel(String path) : Model3DBasic(path)
{
	auto fs = FileSystem::Get();

	std::vector<String> diffuseTextures;

	std::string_view pathview(path.c_str());
	String basePath = path.SubString(0, pathview.find_last_of('/') + 1);

	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(fs->GetResourcesPath(path).c_str(),
		aiProcess_Triangulate | aiProcess_CalcTangentSpace | aiProcess_GenSmoothNormals | aiProcess_ConvertToLeftHanded);

	ResourceUtils::LoadMaterialsForAnimModel(basePath, diffuseTextures, scene);

	ResourceUtils::LoadAllMeshsForModel3DBasic(this, scene, false);

	ResourceUtils::LoadAllAnimMeshsForAnimModel(this, scene);

	ResourceUtils::AnimModelLoadingCtx ctx(this, scene, false);

	ResourceUtils::FlattenAnimModelHierarchy(&ctx, this, scene);

	ResourceUtils::LoadAnimModelBoundNodeIds(diffuseTextures, this, scene);

	auto& srcNodes = ctx.nodes;
	auto& destNodes = m_nodes;
	if (destNodes.empty())
	{
		destNodes.resize(srcNodes.size());
		for (size_t i = 0; i < srcNodes.size(); i++)
		{
			auto& srcNode = srcNodes[i];
			auto& destNode = destNodes[i];

			destNode.boneId = srcNode.boneId;
			destNode.parentId = srcNode.parentId;
			destNode.localTransform = srcNode.localTransform;
		}
	}

	{
		struct LoadMotionParam
		{
			AnimMotion* motion;
			ID animationId;
			AnimModel* model;
			AnimModel::AnimMeshVertices* vertices;
		};

		std::vector<Resource<AnimMotion>> motions;
		ResourceUtils::LoadAnimMotion(path, (void*)scene, motions);

		std::vector<Task> tasks;
		std::vector<LoadMotionParam> params;

		tasks.resize(motions.size());
		params.resize(motions.size());

		size_t i = 0;
		for (auto& motion : motions)
		{
			auto& param = params[i];
			auto& task = tasks[i];

			param.animationId = PlaceHolderAnimation(motion);
			param.motion = motion;
			param.model = this;
			param.vertices = ctx.animMeshesVertices.data();

			task.Params() = &param;
			task.Entry() = [](void* p)
				{
					TASK_SYSTEM_UNPACK_PARAM_REF_4(LoadMotionParam, p, animationId, motion, model, vertices);
					model->LoadAnimation(animationId, motion, vertices);
				};

			i++;
		}

		TaskSystem::SubmitAndWait(tasks.data(), tasks.size(), Task::CRITICAL);
	}
	
}

AnimModel::~AnimModel()
{
	for (auto& anim : m_animations)
	{
		delete anim;
	}
	m_animations.clear();
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
	keyFramesIndex.resize(channels.size());

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

			if (node.boneId != INVALID_ID)
			{
				bones[node.boneId] = m_boneOffsetMatrixs[node.boneId] * globalTransform[i];
				//assert(node.boneId == channelId);
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
	auto animation = new Animation();

	animation->m_motion = motion;

	auto& nodes = m_nodes;
	auto numNode = nodes.size();
	animation->m_nodeToChannelId.resize(numNode, INVALID_ID);

	auto& channels = animation->GetChannels();
	auto& channelsName = animation->m_motion->m_nodeNameEffectedByChannel;
	auto numChannel = channels.size();

	// map channel to bone
	for (size_t i = 0; i < numChannel; i++)
	{
		auto& nodeName = channelsName[i];
		auto it = m_nodeIds.find(nodeName);

		//assert(it != m_nodeIds.end());

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
	auto myPath = GetPath();
	auto animation = m_animations[animationId];

	// load AABB key frames for this animation

	auto motionPath = motion->GetModelFilePath();
	auto motionFileName = FileUtils::GetLastName(motionPath.c_str());
	ByteStream stream;
	auto streamPath = (myPath + "." + motionFileName + "." + motion->m_name.ReplaceAll('|', '-') + AnimModel::CACHE_EXTENSION);
	if (FileSystem::Get()->IsFileChanged(myPath.c_str())
		|| FileSystem::Get()->IsFileChanged(motionPath.c_str())
		|| !FileSystem::Get()->ReadStream(streamPath.c_str(), &stream))
	{
		bool needDelete = false;
		if (vertices == nullptr)
		{
			needDelete = true;

			// TODO: reload vertices from model file
			Assimp::Importer importer;
			importer.SetPropertyInteger(AI_CONFIG_PP_RVC_FLAGS, aiComponent_CAMERAS | aiComponent_LIGHTS | aiComponent_ANIMATIONS);
			const aiScene* scene = importer.ReadFile(myPath.c_str(),
				aiProcess_Triangulate | aiProcess_CalcTangentSpace | aiProcess_GenSmoothNormals
				| aiProcess_ConvertToLeftHanded | aiProcess_RemoveComponent
			);

			uint32_t count = 0;
			for (uint32_t i = 0; i < scene->mNumMeshes; i++)
			{
				auto& mesh = scene->mMeshes[i];
				if (!mesh->HasBones())
				{
					continue;
				}

				count++;
			}

			vertices = new AnimMeshVertices[count]();
			count = 0;
			for (uint32_t i = 0; i < scene->mNumMeshes; i++)
			{
				auto& mesh = scene->mMeshes[i];
				if (!mesh->HasBones())
				{
					continue;
				}

				ResourceUtils::LoadAnimMeshVertices(vertices + count, this, mesh);

				count++;
			}
		}

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

		if (needDelete)
		{
			delete[] vertices;
		}
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

Handle<GameObject> AnimModel::MakeGameObject()
{
	auto model = GetSelfResource();
	auto ret = mheap::New<GameObject>();

	auto animator = ret->NewComponent<AnimatorSkeletalArray>();

	SharedPtr<AnimMeshRenderingBuffer> buffer = std::make_shared<AnimMeshRenderingBuffer>();
	{
		AnimModel::AnimMeshRenderingBufferData bufferData;
		bufferData.bones.resize(m_boneIds.size());
		bufferData.meshesAABB.resize(m_animMeshes.size());
		buffer->buffer.Initialize(bufferData);
	}
	animator->m_animMeshRenderingBuffer = buffer;
	animator->m_model3D = model;

	auto count = m_meshes.size();
	for (size_t i = 0; i < count; i++)
	{
		auto& mesh = m_meshes[i];

		auto obj = mheap::New<GameObject>();
		auto c = obj->NewComponent<AnimModelStaticMeshRenderer>();
		c->m_model3D = model;

		if (mesh.m_defaultDiffusePath.empty())
		{
			c->m_texture = resource::Load<Texture2D>(Texture2D::DEFAULT_FILE);
		}
		else
		{
			c->m_texture = resource::Load<Texture2D>(mesh.m_defaultDiffusePath);
		}

		animator->m_meshRendererObjs.Push(obj);
		ret->AddChild(obj);
	}

	count = m_animMeshes.size();
	for (size_t i = 0; i < count; i++)
	{
		auto& mesh = m_animMeshes[i];

		auto obj = mheap::New<GameObject>();
		auto c = obj->NewComponent<AnimMeshRenderer>();
		c->m_model3D = model;

		if (mesh.m_defaultDiffusePath.empty())
		{
			c->m_texture = resource::Load<Texture2D>(Texture2D::DEFAULT_FILE);
		}
		else
		{
			c->m_texture = resource::Load<Texture2D>(mesh.m_defaultDiffusePath);
		}

		animator->m_meshRendererObjs.Push(obj);
		ret->AddChild(obj);
	}

	return ret;
}

NAMESPACE_END