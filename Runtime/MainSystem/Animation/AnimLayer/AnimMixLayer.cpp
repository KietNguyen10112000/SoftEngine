#include "AnimMixLayer.h"

#include "Scene/GameObject.h"

#include "MainSystem/MainSystemTaskPacking.h"
#include "MainSystem/Animation/AnimationSystem.h"

NAMESPACE_BEGIN

void AnimMixLayer::SerializeToBinary(Serializer* serializer, ByteStream& stream) const
{
}

void AnimMixLayer::DeserializeFromBinary(Serializer* serializer, const ByteStream& stream)
{
}

void AnimMixLayer::SerializeToJson(Serializer* serializer, json& j) const
{
	{
		auto arr = json::array();
		for (auto& input : m_inputs)
		{
			json jInput;
			jInput["Layer"] = serializer->Serialize(input.layer);
			jInput["Weight"] = input.weight;
			arr.push_back(jInput);
		}
		j["Inputs"] = arr;
	}
}

void AnimMixLayer::DeserializeFromJson(Serializer* serializer, const json& j)
{
	{
		auto& arr = j["Inputs"];
		for (size_t i = 0; i < arr.size(); i++)
		{
			auto& jInput = arr[i];

			InputLayer& input = m_inputs.emplace_back();
			serializer->Deserialize(jInput["Layer"], input.layer);
			input.weight = jInput["Weight"].get<std::vector<float>>();
		}
	}
}

Handle<ClassMetadata> AnimMixLayer::GetMetadata(size_t sign)
{
	return Handle<ClassMetadata>();
}

void AnimMixLayer::OnPropertyChanged(const UnknownAddress& var, const Variant& newValue)
{
}

void AnimMixLayer::Run(float dt)
{
	for (auto& input : m_inputs)
	{
		input.outputLayer = input.layer ? input.layer->GetOutput() : nullptr;
	}

	auto count = m_globalTransforms.size();
	for (size_t i = 0; i < count; i++)
	{
		auto& mat = m_globalTransforms[i];
		mat = Mat4::Zero();
		for (auto& input : m_inputs)
		{
			if (input.layer)
			{
				mat += (input.outputLayer->NodeGlobalTransforms()[i] * input.weight[i]);
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
				aabb.m_center += (temp.m_center * input.weight[i]);
				aabb.m_halfDimensions += (temp.m_halfDimensions * input.weight[i]);
			}
		}
	}
}

void AnimMixLayer::AddInputImpl(AnimLayer* layer, const std::vector<float>& weight)
{
	auto& input = m_inputs.emplace_back();
	input.layer = layer;
	input.weight = weight;
}

void AnimMixLayer::SetWeightImpl(ID index, const std::vector<float>& weight)
{
	auto& input = m_inputs[index];
	input.weight = weight;
}

void AnimMixLayer::AddInput(AnimLayer* layer, const std::vector<float>& weight)
{
	assert(weight.size() == m_globalTransforms.size());

	MAIN_SYSTEM_TASK_IMPL_COMMON_2(GetComponent(),
		AnimationSystem, AsyncTaskRunner, layer, weight,
		{
			self->AddInputImpl(layer, weight);
		}
	);
}

void AnimMixLayer::SetWeight(ID index, const std::vector<float>& weight)
{
	assert(weight.size() == m_globalTransforms.size());

	MAIN_SYSTEM_TASK_IMPL_COMMON_2(GetComponent(),
		AnimationSystem, AsyncTaskRunner, index, weight,
		{
			self->SetWeightImpl(index, weight);
		}
	);
}

NAMESPACE_END