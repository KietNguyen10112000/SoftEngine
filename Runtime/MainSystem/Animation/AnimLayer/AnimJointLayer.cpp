#include "AnimJointLayer.h"

#include "Scene/GameObject.h"

#include "MainSystem/MainSystemTaskPacking.h"
#include "MainSystem/Animation/AnimationSystem.h"

NAMESPACE_BEGIN

void AnimJointLayer::SerializeToBinary(Serializer* serializer, ByteStream& stream) const
{
}

void AnimJointLayer::DeserializeFromBinary(Serializer* serializer, const ByteStream& stream)
{
}

void AnimJointLayer::SerializeToJson(Serializer* serializer, json& j) const
{
	AnimLayer::SerializeToJson(serializer, j);
	{
		auto arr = json::array();
		for (auto& input : m_inputs)
		{
			json jInput;
			jInput["Layer"] = serializer->Serialize(input.layer);
			jInput["Mask"] = input.mask;
			arr.push_back(jInput);
		}
		j["Inputs"] = arr;
	}
}

void AnimJointLayer::DeserializeFromJson(Serializer* serializer, const json& j)
{
	AnimLayer::DeserializeFromJson(serializer, j);
	{
		auto& arr = j["Inputs"];
		for (size_t i = 0; i < arr.size(); i++)
		{
			auto& jInput = arr[i];

			InputLayer& input = m_inputs.emplace_back();
			serializer->Deserialize(jInput["Layer"], input.layer);
			input.mask = jInput["Weight"].get<std::vector<bool>>();
		}
	}
}

Handle<ClassMetadata> AnimJointLayer::GetMetadata(size_t sign)
{
	return Handle<ClassMetadata>();
}

void AnimJointLayer::OnPropertyChanged(const UnknownAddress& var, const Variant& newValue)
{
}

void AnimJointLayer::Run(float dt)
{
	auto& nodes = m_model->m_nodes;
	for (auto& input : m_inputs)
	{
		input.outputLayer = input.layer ? input.layer->GetOutput() : nullptr;
	}

	auto count = m_globalTransforms.size();
	for (size_t i = 0; i < count; i++)
	{
		auto& node = nodes[i];
		auto& mat = m_globalTransforms[i];
		mat = Mat4::Zero();
		for (auto& input : m_inputs)
		//auto& input = m_inputs[0];
		{
			if (input.outputLayer && input.mask[i])
			{
				auto& nodeGlobalTransform = input.outputLayer->NodeGlobalTransforms()[i];
				if (node.parentId != INVALID_ID)
				{
					auto& parentGlobalTransform = m_globalTransforms[node.parentId];
					auto& oriParentGlobalTransform = input.outputLayer->NodeGlobalTransforms()[node.parentId];
					if (parentGlobalTransform != oriParentGlobalTransform)
					{
						mat += ((nodeGlobalTransform * oriParentGlobalTransform.GetInverse()) * parentGlobalTransform);
						continue;
					}
				}

				mat += (input.outputLayer->NodeGlobalTransforms()[i]);
			}
		}
	}

	count = m_meshesAABB.size();
	for (size_t i = 0; i < count; i++)
	{
		auto& aabb = m_meshesAABB[i];
		aabb.m_center = { 0,0,0 };
		aabb.m_halfDimensions = { 0,0,0 };
		for (auto& input : m_inputs)
		{
			if (input.layer)
			{
				auto& temp = input.outputLayer->MeshesAABB()[i];
				aabb.m_center += (temp.m_center * float(input.mask[i]));
				aabb.m_halfDimensions += (temp.m_halfDimensions * float(input.mask[i]));
			}
		}
	}
}

void AnimJointLayer::AddInputImpl(AnimLayer* layer, const std::vector<bool>& mask)
{
	auto& input = m_inputs.emplace_back();
	input.layer = layer;
	input.mask = mask;
}

void AnimJointLayer::SetMaskImpl(ID index, const std::vector<bool>& mask)
{
	auto& input = m_inputs[index];
	input.mask = mask;
}

void AnimJointLayer::AddInput(AnimLayer* layer, const std::vector<bool>& mask)
{
	assert(mask.size() == m_globalTransforms.size());

	MAIN_SYSTEM_TASK_IMPL_COMMON_2(GetComponent(),
		AnimationSystem, AsyncTaskRunner, layer, mask,
		{
			self->AddInputImpl(layer, mask);
		}
	);
}

void AnimJointLayer::SetMask(ID index, const std::vector<bool>& mask)
{
	assert(mask.size() == m_globalTransforms.size());

	MAIN_SYSTEM_TASK_IMPL_COMMON_2(GetComponent(),
		AnimationSystem, AsyncTaskRunner, index, mask,
		{
			self->SetMaskImpl(index, mask);
		}
	);
}

NAMESPACE_END