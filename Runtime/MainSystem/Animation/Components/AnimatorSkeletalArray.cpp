#include "AnimatorSkeletalArray.h"

#include "Scene/GameObject.h"

#include "MainSystem/Animation/AnimationSystem.h"
#include "MainSystem/Animation/AnimLayer/AnimLayer.h"

#include "MainSystem/Physics/Components/CharacterController.h"
#include "MainSystem/Physics/Components/RigidBodyDynamic.h"

#include "MainSystem/Rendering/Components/AnimModelStaticMeshRenderer.h"
#include "MainSystem/MainSystemTaskPacking.h"

#include "Graphics/DebugGraphics.h"

#include "PhysX/Utils.h"
#include "PhysX/PhysX.h"

#include "imgui/imgui.h"

NAMESPACE_BEGIN

class AnimCCTBufferLayer : public AnimLayer
{
public:
	SERIALIZABLE_CLASS(AnimCCTBufferLayer);

	virtual void Run(float dt) override
	{

	}

	// Inherited via AnimLayer
	void SerializeToBinary(Serializer* serializer, ByteStream& stream) const override
	{
	}

	void DeserializeFromBinary(Serializer* serializer, const ByteStream& stream) override
	{
	}

	Handle<ClassMetadata> GetMetadata(size_t sign) override
	{
		return Handle<ClassMetadata>();
	}

	void OnPropertyChanged(const UnknownAddress& var, const Variant& newValue) override
	{
	}

};

AnimatorSkeletalArray::AnimatorSkeletalArray() : AnimationComponent(ANIMATION_TYPE_SKELETAL_ARRAY)
{
	m_deferBufferLayer = std::make_shared<AnimCCTBufferLayer>();
}

AnimatorSkeletalArray::~AnimatorSkeletalArray()
{
	/*for (auto& layer : m_animLayers)
	{
		delete layer;
	}*/
	m_animLayers.clear();
}

void AnimatorSkeletalArray::InitAnimLayer(AnimLayer* animLayer)
{
	animLayer->m_model = m_model3D;
	animLayer->m_ownerComp = this;
	animLayer->m_globalTransforms.resize(m_model3D->m_nodes.size());
	//animLayer->m_localTransforms.resize(m_model3D->m_nodes.size());
	animLayer->m_meshesAABB.resize(m_model3D->m_animMeshes.size());

	animLayer->Initialize();
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

void AnimatorSkeletalArray::Update(Scene* scene, float dt)
{
	if (!m_isRunning || m_rigidBodyProxyControlMode == RIGID_BODY_PROXY_CONTROL_MODE::RIGID_BODY_TO_ANIMATOR)
	{
		return;
	}

	/*if (m_isEnableDeferPublicResults && m_deferBufferLayer->m_globalTransforms.empty())
	{
		ResetDeferBufferLayer();
	}*/

	AnimLayer* last = nullptr;
	for (auto& layer : m_animLayers)
	{
		layer->PrevRun(dt);
	}
	for (auto& layer : m_animLayers)
	{
		if (layer && layer->IsEnable())
		{
			layer->Run(dt);
		}

		if (layer)
		{
			last = layer->GetOutput();
		}
	}

	if (last)
	{
		//last = last->GetOutput();

		if (m_rigidBodyProxyControlMode == RIGID_BODY_PROXY_CONTROL_MODE::ANIMATOR_TO_RIGID_BODY)
		{
			PublicResultToRigidBodies(scene, last);
		}

		if (m_cct)
		{
			ForwardCTTUpdateDataToRenderer(scene, last);
		}
		else
		{
			UpdateDataToRenderer(scene, last);
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
	/*auto debugGraphics = Graphics::Get()->GetDebugGraphics();
	if (!debugGraphics) return;

	auto mat = m_lastOutput->m_globalTransforms[m_model3D->m_rootBoneNodeId];
	mat *= GetGameObject()->GetCommittedGlobalTransform();

	debugGraphics->DrawDirection(mat.Position(), mat.Forward().Normal(), { 0,0,1,1 }, { 0,0,1,1 });
	debugGraphics->DrawDirection(mat.Position(), mat.Right().Normal(), { 1,0,0,1 }, { 1,0,0,1 });
	debugGraphics->DrawDirection(mat.Position(), mat.Up().Normal(), { 0,1,0,1 }, { 0,1,0,1 });*/

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

void AnimatorSkeletalArray::PublicResultToRigidBodies(Scene* _scene, AnimLayer* last)
{
	if (m_pivotRigidBody == nullptr)
	{
		for (auto& b : m_rigidBodyProxy)
		{
			if (b)
			{
				m_pivotRigidBody = b;
				break;
			}
		}
	}

	if (m_pivotRigidBody)
	{
		assert(m_pivotRigidBody->HasComponent<RigidBodyDynamic>());
		m_pivotRigidBody->GetComponentRaw<RigidBodyDynamic>()->RunAnimatorMotionMatchingCallback(
			[](AnimatorSkeletalArray* self, ID param)
			{
				auto last = (AnimLayer*)param;
				auto& globalTransform = self->GetGameObject()->GetCommittedGlobalTransform();
				auto& globals = last->NodeGlobalTransforms();

				auto& proxies = self->m_rigidBodyProxy;

				auto count = globals.size();
				for (size_t i = 0; i < count; i++)
				{
					auto& global = globals[i];
					auto& proxy = proxies[i];
					if (proxy)
					{
						auto m = self->m_rigidBodyAnimToPhysOffsets[i] * global * globalTransform;
						assert(proxy->HasComponent<RigidBodyDynamic>());

						auto comp = proxy->GetComponentRaw<RigidBodyDynamic>();
						auto pxBody = comp->m_pxActor->is<physx::PxRigidDynamic>();
						pxBody->setKinematicTarget(PhysXUtils::ToPxTransform(Transform::FromTransformMatrix(m)));
					}
				}
			},
			this, ID(last)
		);
	}
}

void AnimatorSkeletalArray::FetchResultFromRigidBodies()
{
	auto& nodes = m_model3D->m_nodes;
	auto& proxies = m_rigidBodyProxy;
	auto& globals = m_deferBufferLayer->NodeGlobalTransforms();
	auto count = globals.size();

	/*if (globals.size() != nodes.size())
	{
		globals.resize(nodes.size());
	}*/

	auto scale = Mat4::Scaling(GetGameObject()->GetLocalTransform().GetScale());

	for (size_t i = 0; i < count; i++)
	{
		auto& global = globals[i];
		auto& proxy = proxies[i];
		if (proxy)
		{
			assert(proxy->HasComponent<RigidBodyDynamic>());
			auto comp = proxy->GetComponentRaw<RigidBodyDynamic>();
			auto pxBody = comp->m_pxActor->is<physx::PxRigidDynamic>();
			auto mat = PhysXUtils::ToTransform(pxBody->getGlobalPose()).ToTransformMatrix();
			mat = m_model3D->m_boneOffsetInvMatrixs[nodes[i].boneId] * m_rigidBodyPhysToAnimOffsets[i] * mat;
			global = mat;
		}
	}

	UpdateDataToRenderer(GetGameObject()->GetScene(), m_deferBufferLayer->NodeGlobalTransforms(), m_deferBufferLayer->MeshesAABB());
}

void AnimatorSkeletalArray::SetEnableDeferPublicResult(bool enable)
{
	enable = (m_cct != 0)
		|| (m_rigidBodyProxyControlMode == RIGID_BODY_PROXY_CONTROL_MODE::ANIMATOR_TO_RIGID_BODY);

	if (m_isEnableDeferPublicResults != enable)
	{
		//m_deferBufferLayer->m_globalTransforms.clear();
		m_isEnableDeferPublicResults = enable;
	}
}

void AnimatorSkeletalArray::ResetDeferBufferLayer()
{
	assert(m_lastOutput != nullptr);

	m_deferBufferLayer->m_globalTransforms.resize(m_lastOutput->NodeGlobalTransforms().size());
	m_deferBufferLayer->m_meshesAABB.resize(m_lastOutput->MeshesAABB().size());

	std::memcpy(m_deferBufferLayer->m_globalTransforms.data(), m_lastOutput->NodeGlobalTransforms().data(), m_lastOutput->NodeGlobalTransforms().size() * sizeof(Mat4));
	std::memcpy(m_deferBufferLayer->m_meshesAABB.data(), m_lastOutput->MeshesAABB().data(), m_lastOutput->MeshesAABB().size() * sizeof(AABox));
}

void AnimatorSkeletalArray::SetDiscardObjectTransformForRenderingObjects(bool discard)
{
	MAIN_SYSTEM_TASK_COMMON_1(
		AnimationSystem, AsyncTaskRunner, discard,
		{
			self->m_animMeshRenderingBuffer->discardObjectTransform = discard;
			for (auto& o : self->m_meshRendererObjs)
			{
				if (o->HasComponent<AnimModelStaticMeshRenderer>())
				{
					o->GetComponentRaw<AnimModelStaticMeshRenderer>()->m_discardObjectTransform = discard;
				}
			}
		}
	);
}

void AnimatorSkeletalArray::SwitchBackTo_ANIMATOR_TO_RIGID_BODY_From_RIGID_BODY_TO_ANIMATOR()
{
	auto offset = GetGameObject()->GetCommittedGlobalTransform().GetInverse();
	auto& globals = m_deferBufferLayer->m_globalTransforms;
	for (auto& m : globals)
	{
		m *= offset;
	}
}

void AnimatorSkeletalArray::SetRigidBodiesControlModeImpl(RIGID_BODY_PROXY_CONTROL_MODE::MODE mode)
{
	auto prevMode = m_rigidBodyProxyControlMode;

	m_rigidBodyProxyControlMode = mode;
	m_pivotRigidBody = nullptr;

	switch (mode)
	{
	case soft::AnimatorSkeletalArray::RIGID_BODY_PROXY_CONTROL_MODE::DISABLED:
		SetDiscardObjectTransformForRenderingObjects(false);
		SetEnableDeferPublicResult(false);
		//ResetDeferBufferLayer();
		break;
	case soft::AnimatorSkeletalArray::RIGID_BODY_PROXY_CONTROL_MODE::ANIMATOR_TO_RIGID_BODY:
		SetDiscardObjectTransformForRenderingObjects(false);
		SetEnableDeferPublicResult(true);

		if (prevMode == AnimatorSkeletalArray::RIGID_BODY_PROXY_CONTROL_MODE::DISABLED)
		{
			ResetDeferBufferLayer();
		}

		if (prevMode == RIGID_BODY_PROXY_CONTROL_MODE::RIGID_BODY_TO_ANIMATOR)
		{
			SwitchBackTo_ANIMATOR_TO_RIGID_BODY_From_RIGID_BODY_TO_ANIMATOR();
		}

		break;
	case soft::AnimatorSkeletalArray::RIGID_BODY_PROXY_CONTROL_MODE::RIGID_BODY_TO_ANIMATOR:
		SetDiscardObjectTransformForRenderingObjects(true);
		ResetDeferBufferLayer();
		break;
	default:
		break;
	}
}

void AnimatorSkeletalArray::UpdateDataToRenderer(Scene* _scene, const std::vector<Mat4>& globalTransforms, const std::vector<AABox>& meshesAABB)
{
	auto scene = _scene ? _scene : GetCommittedObject()->GetCommittedScene();

	auto& nodes = m_model3D->m_nodes;
	//auto& globalTransforms = last->m_globalTransforms;
	auto animMeshRenderingBuffer = m_animMeshRenderingBuffer.get();
	auto& buffer = animMeshRenderingBuffer->buffer;

	{
		auto& offsets = m_model3D->m_boneOffsetMatrixs;
		auto& boneBuffer = m_animMeshRenderingBuffer->buffer;
		scene->BeginWrite<false>(boneBuffer);
		auto& bones = boneBuffer.Write()->bones;
		//auto& nodes = m_model3D->m_nodes;

		size_t i = 0;
		for (auto& node : nodes)
		{
			if (node.boneId != INVALID_ID)
			{
				bones[node.boneId] = offsets[node.boneId] * globalTransforms[i];
			}
			i++;
		}

		scene->EndWrite(boneBuffer);
	}

	//auto& index = m_aabbKeyFrameIndex;

	bool update = false;

	scene->BeginWrite<false>(buffer);

	auto read = buffer.Read();
	auto write = buffer.Write();

	auto num = write->meshesAABB.size();
	for (uint32_t i = 0; i < num; i++)
	{
		write->meshesAABB[i] = meshesAABB[i];
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
					obj->ForceRefreshTransform();
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

			obj->ForceRefreshTransform();
		}
	}
}

void AnimatorSkeletalArray::UpdateDataToRenderer(Scene* _scene, AnimLayer* last)
{
	m_lastOutput = last;

	if (m_isEnableDeferPublicResults)
	{
		UpdateDataToRenderer(_scene, m_deferBufferLayer->NodeGlobalTransforms(), m_deferBufferLayer->MeshesAABB());
		std::memcpy(m_deferBufferLayer->m_globalTransforms.data(), m_lastOutput->NodeGlobalTransforms().data(), m_lastOutput->NodeGlobalTransforms().size() * sizeof(Mat4));
		std::memcpy(m_deferBufferLayer->m_meshesAABB.data(), m_lastOutput->MeshesAABB().data(), m_lastOutput->MeshesAABB().size() * sizeof(AABox));
	}
	else
	{
		UpdateDataToRenderer(_scene, last->NodeGlobalTransforms(), last->MeshesAABB());
	}
}

void AnimatorSkeletalArray::SetRunning(bool running)
{
	m_isRunning = running;
}

void AnimatorSkeletalArray::SetForwardCCTImpl(CharacterController* cct, const Vec3& lockUpDirection)
{
	m_cct = cct;
	m_cctLockedUpDirection = lockUpDirection;

	if (cct == nullptr)
	{
		SetEnableDeferPublicResult(false);
		return;
	}

	SetEnableDeferPublicResult(true);

	Vec3 scaling; Quaternion rotation; Vec3 translation;
	auto& rootLocalTransform = m_lastOutput->NodeGlobalTransforms()[m_model3D->m_rootBoneNodeId];
	auto mat = rootLocalTransform;
	mat *= GetGameObject()->GetCommittedGlobalTransform();
	mat.Decompose(scaling, rotation, translation);
	m_cctPrevPosition = translation;
	m_cctPrevRotation = rotation;

	m_cctOffset = GetGameObject()->GetCommittedGlobalTransform();
	m_parentOffset = m_cctOffset * m_cct->GetGameObject()->GetCommittedGlobalTransform().GetInverse();

	auto cctStartForward = m_cct->GetGameObject()->GetCommittedGlobalTransform().Forward().Normal();
	/*auto cctRootTransformGlobal = m_cct->GetGameObject()->GetCommittedGlobalTransform();
	cctRootTransformGlobal = cctRootTransformGlobal * GetGameObject()->GetLocalTransform().ToTransformMatrix().GetInverse();
	cctRootTransformGlobal = cctRootTransformGlobal * rootLocalTransform.GetInverse();*/
	auto& p = m_cct->GetGameObject()->GetCommittedGlobalTransform().Position();
	m_rootOffset = Mat4::Translation(GetGameObject()->GetCommittedGlobalTransform().GetInverse().Transform(p));
	m_rootOffset = Mat4::Translation(-rootLocalTransform.GetInverse().Transform(m_rootOffset.Position()));
	m_rootOffset *= Mat4::Scaling(GetGameObject()->GetLocalTransform().GetScale());
	m_rootOffset *= Mat4::Rotation(Quaternion::RotationFromTo(-Vec3::Z_AXIS, Vec3(cctStartForward.x, 0, cctStartForward.z).Normal()));

	//m_offset = GetGameObject()->GetLocalTransform();

}

void AnimatorSkeletalArray::SetForwardCCT(CharacterController* cct, const Vec3& lockUpDirection)
{
	MAIN_SYSTEM_TASK_COMMON_2(
		AnimationSystem, AsyncTaskRunner, cct, lockUpDirection,
		{
			self->SetForwardCCTImpl(cct, lockUpDirection);
		}
	);
}

void AnimatorSkeletalArray::CalculateAnimToPhysOffsets()
{
	auto& boundObjects = m_rigidBodyProxy;
	auto& model = m_model3D;
	auto& nodes = model->m_nodes;
	auto& boneOffsets = model->m_boneOffsetMatrixs;

	auto& offset0 = m_rigidBodyAnimToPhysOffsets;
	if (offset0.size() != nodes.size())
	{
		offset0.resize(nodes.size());
	}

	auto& offset1 = m_rigidBodyPhysToAnimOffsets;
	if (offset1.size() != nodes.size())
	{
		offset1.resize(nodes.size());
	}

	for (size_t i = 0; i < nodes.size(); i++)
	{
		if (nodes[i].boneId != INVALID_ID && boundObjects[i])
		{
			offset0[i] = (boundObjects[i]->GetCommittedGlobalTransform() * GetGameObject()->GetCommittedGlobalTransform().GetInverse())
				* boneOffsets[nodes[i].boneId];
			offset1[i] = GetGameObject()->GetCommittedGlobalTransform() * boundObjects[i]->GetCommittedGlobalTransform().GetInverse();
		}
	}
}

void AnimatorSkeletalArray::SetRigidBodiesControlMode(RIGID_BODY_PROXY_CONTROL_MODE::MODE mode)
{
	MAIN_SYSTEM_TASK_COMMON_1(
		AnimationSystem, AsyncTaskRunner, mode,
		{
			self->SetRigidBodiesControlModeImpl(mode);
		}
	);
}

void AnimatorSkeletalArray::ForwardCTTUpdateDataToRenderer(Scene* _scene, AnimLayer* last)
{
#ifdef _DEBUG
	if (m_deferBufferLayer->m_globalTransforms.empty())
	{
		assert(0);
	}
#endif // _DEBUG

	//m_lastOutput = last;

	//UpdateDataToRenderer(_scene, m_cctGlobalTransformBuffer, m_cctAABoxBuffer);
	m_lastOutput = m_deferBufferLayer.get();
	UpdateDataToRenderer(_scene, m_deferBufferLayer->NodeGlobalTransforms(), m_deferBufferLayer->MeshesAABB());

	Vec3 scaling; Quaternion rotation; Vec3 translation;
	auto& rootLocalTransform = last->NodeGlobalTransforms()[m_model3D->m_rootBoneNodeId];
	auto mat = rootLocalTransform;
	mat *= m_cctOffset;
	mat.Decompose(scaling, rotation, translation);

	/*auto debugGraphics = Graphics::Get()->GetDebugGraphics();
	debugGraphics->DrawDirection(mat.Position(), mat.Forward().Normal(), { 0,0,1,1 }, { 0,0,1,1 });
	debugGraphics->DrawDirection(mat.Position(), mat.Right().Normal(), { 1,0,0,1 }, { 1,0,0,1 });
	debugGraphics->DrawDirection(mat.Position(), mat.Up().Normal(), { 0,1,0,1 }, { 0,1,0,1 });*/

	auto dMove = translation - m_cctPrevPosition;
	m_cct->Move(dMove);
	m_cctPrevPosition = translation;

	Quaternion dRot = Mat4::Rotation(m_cctPrevRotation).GetInverse() * Mat4::Rotation(rotation);
	m_cctPrevRotation = rotation;

	auto cctRotation = m_cct->CCTGetRotation() * dRot;
	if (m_cctLockedUpDirection != Vec3::ZERO)
	{
		auto cctRotMat = Mat4::Rotation(cctRotation);
		auto up = cctRotMat.Up().Normal();
		if (up != m_cctLockedUpDirection)
		{
			cctRotMat *= Mat4::Rotation(Quaternion::RotationFromTo(up, m_cctLockedUpDirection));
			cctRotation = cctRotMat;
		}
	}

	m_cct->CCTSetRotation(cctRotation);

	/*mat = rootLocalTransform * GetGameObject()->GetLocalTransform().ToTransformMatrix() * m_cct->GetGameObject()->GetCommittedGlobalTransform();
	mat.Decompose(scaling, rotation, translation);
	m_cct->CCTSetRotation(rotation);*/

	{
		CopyDataToForwardCTTUpdateDataToRenderer(last);

		/*assert(m_rigidBodyProxyControlMode != RIGID_BODY_PROXY_CONTROL_MODE::RIGID_BODY_TO_ANIMATOR);
		if (m_rigidBodyProxyControlMode == RIGID_BODY_PROXY_CONTROL_MODE::ANIMATOR_TO_RIGID_BODY)
		{
			PublicResultToRigidBodies(_scene, m_deferBufferLayer.get());
		}*/
	}
}

void AnimatorSkeletalArray::CopyDataToForwardCTTUpdateDataToRenderer(AnimLayer* last)
{
	std::memcpy(m_deferBufferLayer->m_globalTransforms.data(), last->NodeGlobalTransforms().data(), sizeof(Mat4) * last->NodeGlobalTransforms().size());
	std::memcpy(m_deferBufferLayer->m_meshesAABB.data(), last->MeshesAABB().data(), sizeof(AABox) * last->MeshesAABB().size());

	{
		// because we forward root transform to cct, so need to discard root transfrom from node to prevent transformed twices
		auto mat = last->NodeGlobalTransforms()[m_model3D->m_rootBoneNodeId].GetInverse();// *m_rootOffset;

		auto rootLocalTransform = last->NodeGlobalTransforms()[m_model3D->m_rootBoneNodeId];
		auto m = (m_parentOffset * Mat4::Rotation(m_cct->CCTGetRotation()));
		rootLocalTransform.Position() = Vec3::ZERO;
		//rootLocalTransform = {};
		rootLocalTransform *= m_rootOffset;
		//rootLocalTransform *= Mat4::Scaling(GetGameObject()->GetLocalTransform().GetScale());
		//auto f = Mat4::Rotation(m_cctStartRotation).Transform(Vec3::Z_AXIS).Normal();
		//rootLocalTransform *= Mat4::Rotation(Quaternion::RotationFromTo(-Vec3::Z_AXIS, Vec3(m_cctStartForward.x, 0, m_cctStartForward.z).Normal()));
		rootLocalTransform *= m.GetInverse();

		auto temp = mat * rootLocalTransform;
		for (auto& transform : m_deferBufferLayer->m_globalTransforms)
		{
			transform *= temp;
		}

		for (auto& aabb : m_deferBufferLayer->m_meshesAABB)
		{
			aabb.Transform(temp);
		}
	}
}

void AnimatorSkeletalArray::CloneFrom(Serializer* serializer, Serializable* another)
{
	auto src = (AnimatorSkeletalArray*)another;
	for (auto& layer : src->m_animLayers)
	{
		m_animLayers.Push(serializer->Clone(layer));
	}

	for (auto& obj : src->m_meshRendererObjs)
	{
		m_meshRendererObjs.Push(serializer->Clone(obj));
	}

	m_animMeshRenderingBuffer = serializer->Clone(src->m_animMeshRenderingBuffer);

	m_model3D = src->m_model3D;
	for (auto& body : src->m_rigidBodyProxy)
	{
		m_rigidBodyProxy.Push(serializer->Clone(body));
	}
}

void AnimatorSkeletalArray::SerializeToBinary(Serializer* serializer, ByteStream& stream) const
{
}

void AnimatorSkeletalArray::DeserializeFromBinary(Serializer* serializer, const ByteStream& stream)
{
}

void AnimatorSkeletalArray::SerializeToJson(Serializer* serializer, json& j) const
{
	j["GameObject"] = serializer->Serialize(((AnimatorSkeletalArray*)this)->GetGameObject());
	j["Model"] = serializer->Serialize(m_model3D);
	
	{
		auto arr = json::array();
		for (auto& layer : m_animLayers)
		{
			arr.push_back(serializer->Serialize(layer));
		}
		j["AnimLayers"] = arr;
	}

	{
		auto arr = json::array();
		for (auto& obj : m_meshRendererObjs)
		{
			arr.push_back(serializer->Serialize(obj));
		}
		j["MeshRendererObjs"] = arr;
	}

	{
		j["AnimMeshRenderingBuffer"] = serializer->Serialize(m_animMeshRenderingBuffer);
	}

	{
		size_t countNotNull = 0;
		auto arr = json::array();
		for (auto& body : m_rigidBodyProxy)
		{
			if (body)
			{
				countNotNull++;
			}
			arr.push_back(serializer->Serialize(body));
		}

		auto offsets = json::array();
		for (auto& o : m_rigidBodyAnimToPhysOffsets)
		{
			offsets.push_back(o);
		}

		auto offsets2 = json::array();
		for (auto& o : m_rigidBodyPhysToAnimOffsets)
		{
			offsets2.push_back(o);
		}

		assert(offsets.size() == arr.size());

		if (countNotNull != 0)
		{
			j["RigidBodyProxy"] = arr; 
			j["RigidBodyAnimToPhysOffsets"] = offsets;
			j["RigidBodyPhysToAnimOffsets"] = offsets2;
		}
	}
}

void AnimatorSkeletalArray::DeserializeFromJson(Serializer* serializer, const json& j)
{
	serializer->Deserialize(j["GameObject"], m_object);

	serializer->Deserialize(j["Model"], m_model3D);

	{
		auto& arr = j["AnimLayers"];
		auto count = arr.size();
		for (size_t i = 0; i < count; i++)
		{
			m_animLayers.Push(nullptr);
			serializer->Deserialize(arr[i], m_animLayers.back());
		}
	}

	{
		auto& arr = j["MeshRendererObjs"];
		auto count = arr.size();
		for (size_t i = 0; i < count; i++)
		{
			m_meshRendererObjs.Push(nullptr);
			serializer->Deserialize(arr[i], m_meshRendererObjs.back());
		}
	}

	{
		serializer->Deserialize(j["AnimMeshRenderingBuffer"], m_animMeshRenderingBuffer);
	}

	if (j.contains("RigidBodyProxy"))
	{
		auto& arr = j["RigidBodyProxy"];
		Handle<GameObject> temp;
		for (size_t i = 0; i < arr.size(); i++)
		{
			serializer->Deserialize(arr[i], temp);
			m_rigidBodyProxy.Push(temp);
		}

		m_rigidBodyAnimToPhysOffsets = j["RigidBodyAnimToPhysOffsets"];

		if (j.contains("RigidBodyPhysToAnimOffsets"))
			m_rigidBodyPhysToAnimOffsets = j["RigidBodyPhysToAnimOffsets"];
	}
}

void AnimatorSkeletalArray::OnPropertyChanged(const UnknownAddress& var, const Variant& newValue)
{
	/*if (var.Is(1))
	{
		Play(-1, newValue.As<ID>(), 0, -1, -1, 0);
	}

	if (var.Is(3))
	{
		SetDuration(newValue.As<float>());
	}*/
}

NAMESPACE_END