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

	auto count = m_localTransforms.size();
	for (size_t i = 0; i < count; i++)
	{
		auto& node = nodes[i];
		auto& localTransform = m_localTransforms[i];
		for (auto& input : m_inputs)
		{
			if (input.outputLayer && input.mask[i])
			{
				localTransform = input.outputLayer->NodeLocalTransforms()[i];
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
	assert(mask.size() == m_localTransforms.size());

	MAIN_SYSTEM_TASK_IMPL_COMMON_2(GetComponent(),
		AnimationSystem, AsyncTaskRunner, layer, mask,
		{
			self->AddInputImpl(layer, mask);
		}
	);
}

void AnimJointLayer::SetMask(ID index, const std::vector<bool>& mask)
{
	assert(mask.size() == m_localTransforms.size());

	MAIN_SYSTEM_TASK_IMPL_COMMON_2(GetComponent(),
		AnimationSystem, AsyncTaskRunner, index, mask,
		{
			self->SetMaskImpl(index, mask);
		}
	);
}

NAMESPACE_END