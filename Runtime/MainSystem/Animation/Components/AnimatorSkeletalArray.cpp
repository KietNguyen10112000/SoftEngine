#include "AnimatorSkeletalArray.h"

#include "Scene/GameObject.h"

#include "MainSystem/Animation/AnimationSystem.h"
#include "MainSystem/Animation/AnimLayer/AnimLayer.h"

#include "MainSystem/Rendering/Components/AnimModelStaticMeshRenderer.h"
#include "MainSystem/MainSystemTaskPacking.h"

#include "Graphics/DebugGraphics.h"

#include "imgui/imgui.h"

NAMESPACE_BEGIN

AnimatorSkeletalArray::AnimatorSkeletalArray() : Animator(ANIMATION_TYPE_SKELETAL_ARRAY)
{

}

AnimatorSkeletalArray::~AnimatorSkeletalArray()
{
	for (auto& layer : m_animLayers)
	{
		delete layer;
	}
	m_animLayers.clear();
}

void AnimatorSkeletalArray::InitAnimLayer(AnimLayer* animLayer)
{
	animLayer->m_model = m_model3D;
	animLayer->m_ownerComp = this;
	animLayer->m_globalTransforms.resize(m_model3D->m_nodes.size());
	animLayer->m_meshesAABB.resize(m_model3D->m_animMeshes.size());
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
	/*auto& animationId = m_currentAnimTrack->animationId;
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

	float newTicksPerSecond = tickDuration / sec;

	if (!GetGameObject()->IsInAnyScene())
	{
		ticksPerSecond = newTicksPerSecond;
		return;
	}

	MAIN_SYSTEM_TASK_1(
		AnimationSystem, AsyncTaskRunner, newTicksPerSecond, 
		{
			self->m_currentAnimTrack->ticksPerSecond = newTicksPerSecond;
		}
	);*/
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
	
}

void AnimatorSkeletalArray::SetPause(bool pause)
{
	//m_paused = pause;
}

void AnimatorSkeletalArray::SetTime(float t)
{
	/*if (!GetGameObject()->IsInAnyScene())
	{
		SetTimeImpl(t);
		return;
	}

	MAIN_SYSTEM_TASK_1(
		AnimationSystem, AsyncTaskRunner, t, 
		{
			self->SetTimeImpl(t);

			if (self->m_paused)
			{
				self->m_paused = false;
				self->Update(system->GetScene(), 0.0f);
				self->m_paused = true;
			}
		}
	);*/
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
	/*auto metadata = mheap::New<ClassMetadata>(GetClassName(), this);

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

	return metadata;*/
	return nullptr;
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

void AnimatorSkeletalArray::CloneFrom(Serializer* serializer, Serializable* another)
{
	struct CloneParam
	{
		AnimatorSkeletalArray* src;
		AnimatorSkeletalArray* dest;
	};

	auto dest = this;
	auto src = (AnimatorSkeletalArray*)another;

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
	buffer.bones.resize(src->m_model3D->m_boneIds.size());
	buffer.meshesAABB.resize(src->m_model3D->m_animMeshes.size());
	auto buf = std::make_shared<AnimModel::AnimMeshRenderingBuffer>();
	buf->buffer.Initialize(buffer);

	dest->m_animMeshRenderingBuffer = buf;
	addresses.insert({ src->m_animMeshRenderingBuffer.get(), &dest->m_animMeshRenderingBuffer });

	auto param = callbackRunner->CreateParam<CloneParam>(&task);
	param->src = src;
	param->dest = dest;

	callbackRunner->RunAsync(&task);

	dest->m_model3D = src->m_model3D;
}

void AnimatorSkeletalArray::Update(Scene* scene, float dt)
{
	AnimLayer* last = nullptr;
	for (auto& layer : m_animLayers)
	{
		if (layer && layer->IsEnable())
		{
			layer->Run(dt);
			last = layer;
		}
	}

	if (last)
	{
		auto& globalTransforms = last->m_globalTransforms;
		auto animMeshRenderingBuffer = m_animMeshRenderingBuffer.get();
		auto& buffer = animMeshRenderingBuffer->buffer;

		//auto& index = m_aabbKeyFrameIndex;

		bool update = false;

		scene->BeginWrite<false>(buffer);

		auto read = buffer.Read();
		auto write = buffer.Write();

		auto num = write->meshesAABB.size();
		for (uint32_t i = 0; i < num; i++)
		{
			write->meshesAABB[i] = last->m_meshesAABB[i];
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

				auto& boundNodeTransform = globalTransforms[boundNodeIds[i]];

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

struct MyData
{
	Vec3 targetPos = { 0.5f,1.5f,0 };

	Mat4 origin;

	struct Joint
	{
		int nodeId;
		int parentNodeId;

		Vec3 scale;
		Vec3 position;
	};

	struct NodeEffectedByJoint
	{
		int nodeId;
		Mat4 localTransform;
	};

	std::vector<Joint>		joints;
	std::vector<Vec3>		jointRotations;
	//std::vector<Vec3>		jointRotationEulers;

	std::vector<NodeEffectedByJoint> nodesEffectedByJoint;

	std::vector<Vec3>		rotations;
	bool runningGradientDescent = false;
};

MyData* g_data = nullptr;

void AnimatorSkeletalArray::OnDrawDebug()
{
	//test inverse kinematics
	
	//auto debugGraphics = Graphics::Get()->GetDebugGraphics();
	//if (!debugGraphics)
	//{
	//	return;
	//}

	//auto& offsetMatrix = m_model3D->m_boneOffsetMatrixs;
	//std::vector<Vec3> bonePos;
	//bonePos.reserve(offsetMatrix.size());

	//auto& rootTrans = GetGameObject()->ReadGlobalTransformMat();

	//for (auto& bone : m_model3D->m_boneOffsetMatrixs)
	//{
	//	bonePos.push_back((bone.GetInverse() * rootTrans).Position());
	//}

	//size_t i = 0;
	//auto& nodes = m_model3D->m_nodes;
	//for (auto& node : nodes)
	//{
	//	if (node.boneId != INVALID_ID && node.parentId != INVALID_ID && nodes[node.parentId].boneId != INVALID_ID)
	//	{
	//		auto cur = (Vec4(bonePos[node.boneId], 1.0f) * m_globalTransforms[i]).xyz() + Vec3(0, 0, 3);
	//		auto parent = (Vec4(bonePos[nodes[node.parentId].boneId], 1.0f) * m_globalTransforms[node.parentId]).xyz() + Vec3(0, 0, 3);

	//		debugGraphics->DrawLineSegment(parent, cur);
	//	}
	//	i++;
	//}

	//{
	//	ImGui::Begin("Debug");

	//	if (ImGui::Button("Pause"))
	//	{
	//		SetPause(true);
	//	}

	//	ImGui::SameLine();

	//	if (ImGui::Button("Resume"))
	//	{
	//		SetPause(false);
	//	}

	//	if (!g_data)
	//	{
	//		g_data = new MyData();
	//	}

	//	auto Forward = [&](std::vector<Vec3>& rotations, void(*callback)(size_t, const Vec3&, const Vec3&, const Quaternion&) = nullptr) -> Vec3
	//	{
	//		auto transform = g_data->origin;
	//		auto& joints = g_data->joints;
	//		for (size_t i = 0; i < joints.size(); i++)
	//		{
	//			auto& joint = joints[i];

	//			auto curTransform = 
	//				Mat4::Scaling(joint.scale)
	//				* Mat4::Rotation(rotations[i])
	//				* Mat4::Translation(joint.position)
	//				* transform;

	//			auto parent = (Vec4(bonePos[nodes[nodes[joint.nodeId].parentId].boneId], 1.0f) * transform).xyz();
	//			auto cur = (Vec4(bonePos[nodes[joint.nodeId].boneId], 1.0f) * curTransform).xyz();

	//			if (callback) callback(i, parent, cur, {});

	//			transform = curTransform;
	//		}

	//		return transform.Position();
	//	};

	//	ImGui::DragFloat3("Target Pos", &g_data->targetPos[0], 0.001f, -INFINITY, INFINITY);

	//	/*auto& angles = g_data->angles;
	//	for (size_t i = 0; i < joints.size(); i++)
	//	{
	//		angles[i] = joints[i].angle;
	//	}

	//	auto endEffector = Forward(angles);
	//	auto d = (endEffector - g_data->targetPos).Length();
	//	if (d > 0.001f)
	//	{
	//		for (size_t i = 0; i < joints.size(); i++)
	//		{
	//			angles[i] += 0.1f;
	//			auto curEffector = Forward(angles);
	//			auto curD = (curEffector - g_data->targetPos).Length();
	//			auto gradient = curD - d;

	//			angles[i] -= 0.1f;
	//			angles[i] -= gradient * 0.1f;
	//		}

	//		for (size_t i = 0; i < joints.size(); i++)
	//		{
	//			joints[i].angle = angles[i];
	//		}
	//	}*/

	//	if (ImGui::Button("MakeJoint"))
	//	{
	//		auto& joints = g_data->joints;
	//		auto& jointRotations = g_data->jointRotations;
	//		joints.clear();
	//		jointRotations.clear();
	//		//g_data->jointRotationEulers.clear();

	//		std::vector<int> boneChildCount;
	//		boneChildCount.resize(offsetMatrix.size());

	//		std::vector<int> isJoint;
	//		isJoint.resize(nodes.size(), 0);

	//		boneChildCount[m_model3D->m_boneIds["mixamorig:LeftShoulder"]] = -1;
	//		boneChildCount[m_model3D->m_boneIds["mixamorig:LeftHand"]] = 1;

	//		auto rotation = Mat4::Identity();

	//		bool first = true;
	//		auto p = Vec3();
	//		i = 0;
	//		for (auto& node : nodes)
	//		{
	//			if (node.boneId == INVALID_ID)
	//			{
	//				goto Continue;
	//			}

	//			if (node.parentId != INVALID_ID && nodes[node.parentId].boneId != INVALID_ID && boneChildCount[nodes[node.parentId].boneId] <= 0)
	//			{
	//				auto& parentTransform = m_globalTransforms[node.parentId];
	//				auto& globalTransform = m_globalTransforms[i];
	//				auto& localTransform = globalTransform * parentTransform.GetInverse();

	//				Transform transform;
	//				localTransform.Decompose(transform.Scale(), transform.Rotation(), transform.Translation());

	//				boneChildCount[nodes[node.parentId].boneId]++;

	//				if (first)
	//				{
	//					g_data->origin = parentTransform;
	//					first = false;
	//				}
	//				
	//				joints.push_back({ (int)i, (int)node.parentId, transform.Scale(), transform.Translation() });
	//				jointRotations.push_back(transform.Rotation().ToEulerAngles());

	//				//g_data->jointRotationEulers.push_back(transform.Rotation().ToEulerAngles());

	//				isJoint[i] = 1;
	//			}
	//			else 
	//			{
	//				boneChildCount[node.boneId]++;
	//			}

	//		Continue:
	//			i++;
	//		}

	//		auto& nodesEffectedByJoint = g_data->nodesEffectedByJoint;
	//		i = 0;
	//		for (auto& node : nodes)
	//		{
	//			/*if (node.boneId == INVALID_ID)
	//			{
	//				goto Continue1;
	//			}*/

	//			if (node.parentId != INVALID_ID && isJoint[i] == 0 && isJoint[node.parentId] == 1)
	//			{
	//				auto& parentTransform = m_globalTransforms[node.parentId];
	//				auto& globalTransform = m_globalTransforms[i];
	//				auto& localTransform = globalTransform * parentTransform.GetInverse();

	//				nodesEffectedByJoint.push_back({ (int)i,localTransform });
	//				isJoint[i] = 1;
	//			}

	//		Continue1:
	//			i++;
	//		}
	//	}

	//	{
	//		// render joints
	//		auto endEffector = Forward(g_data->jointRotations,
	//			[](size_t i, const Vec3& head, const Vec3& tail, const Quaternion&)
	//			{
	//				auto color = Vec4(0, 0, 0, 1);
	//				color[i % 3] = 1;
	//				auto debugGraphics = Graphics::Get()->GetDebugGraphics();
	//				debugGraphics->DrawLineSegment(head, tail, color, 0.007f);
	//			}
	//		);

	//		debugGraphics->DrawAABox(AABox(endEffector, Vec3(0.05f)), { 1,1,0.5f,1 }, true);

	//		i = 0;
	//		for (auto& r : g_data->jointRotations)
	//		{
	//			ImGui::PushID(i++);
	//			if (ImGui::DragFloat3("Angle ", &r[0], 0.001f, -INFINITY, INFINITY))
	//			{
	//				//r.Normalize();
	//				//g_data->jointRotations[i - 1] = Quaternion(r);
	//			}
	//			ImGui::PopID();
	//		}
	//	}

	//	if (!g_data->joints.empty())
	//	{
	//		auto scene = GetGameObject()->GetScene();
	//		scene->BeginWrite<false>(m_animMeshRenderingBuffer->buffer);
	//		auto& bones = m_animMeshRenderingBuffer->buffer.Write()->bones;

	//		auto transform = g_data->origin;
	//		auto& joints = g_data->joints;
	//		auto& jointRotations = g_data->jointRotations;
	//		auto& nodesEffectedByJoint = g_data->nodesEffectedByJoint;

	//		for (size_t i = 0; i < joints.size(); i++)
	//		{
	//			auto& joint = joints[i];

	//			auto curTransform =
	//				Mat4::Scaling(joint.scale)
	//				* Mat4::Rotation(jointRotations[i])
	//				* Mat4::Translation(joint.position)
	//				* transform;

	//			m_globalTransforms[joint.nodeId] = curTransform;

	//			transform = curTransform;
	//		}

	//		for (size_t i = 0; i < nodesEffectedByJoint.size(); i++)
	//		{
	//			auto& node = nodesEffectedByJoint[i];
	//			m_globalTransforms[node.nodeId] = node.localTransform * m_globalTransforms[nodes[node.nodeId].parentId];
	//		}

	//		for (size_t i = 0; i < nodes.size(); i++)
	//		{
	//			auto& node = nodes[i];
	//			auto& globalTransform = m_globalTransforms[i];

	//			if (node.boneId != INVALID_ID)
	//			{
	//				bones[node.boneId] = offsetMatrix[node.boneId] * globalTransform;
	//			}
	//		}

	//		scene->EndWrite(m_animMeshRenderingBuffer->buffer);
	//	}

	//	if (ImGui::Button("Run IK"))
	//	{
	//		g_data->runningGradientDescent = !g_data->runningGradientDescent;
	//	}

	//	if (g_data->runningGradientDescent)
	//	{
	//		auto& jointRotations = g_data->jointRotations;
	//		auto& rotations = g_data->rotations;
	//		rotations.resize(jointRotations.size());

	//		for (size_t i = 0; i < jointRotations.size(); i++)
	//		{
	//			rotations[i] = jointRotations[i];
	//		}

	//		auto endEffector = Forward(rotations);
	//		auto d = (endEffector - g_data->targetPos).Length();
	//		if (d > 0.001f)
	//		{
	//			for (size_t i = 0; i < rotations.size(); i++)
	//			{
	//				auto& rotation = rotations[i];
	//				for (size_t j = 0; j < 3; j++)
	//				{
	//					auto& v = rotation[j];

	//					v += 0.1f;
	//					auto curEffector = Forward(rotations);
	//					auto curD = (curEffector - g_data->targetPos).Length();
	//					auto gradient = (curD - d) / 0.1f;

	//					v -= 0.1f;
	//					v -= gradient * 0.01f;
	//				}

	//				//rotation.Normalize();
	//			}

	//			for (size_t i = 0; i < rotations.size(); i++)
	//			{
	//				jointRotations[i] = rotations[i];
	//			}
	//		}

	//		ImGui::Text("%f", d);
	//	}

	//	debugGraphics->DrawAABox(AABox(g_data->targetPos, Vec3(0.05f)), { 1,1,1,1 }, true);

	//	ImGui::End();
	//}
}

NAMESPACE_END