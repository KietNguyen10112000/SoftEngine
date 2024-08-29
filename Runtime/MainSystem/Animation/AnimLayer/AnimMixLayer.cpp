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
	AnimLayer::SerializeToJson(serializer, j);
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
	AnimLayer::DeserializeFromJson(serializer, j);
	{
		auto& arr = j["Inputs"];
		for (size_t i = 0; i < arr.size(); i++)
		{
			auto& jInput = arr[i];

			InputLayer& input = m_inputs.emplace_back();
			serializer->Deserialize(jInput["Layer"], input.layer);
			input.weight = jInput["Weight"];
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
		localTransform.Scale() = { 0,0,0 };
		localTransform.Rotation().x = 0;
		localTransform.Rotation().y = 0;
		localTransform.Rotation().z = 0;
		localTransform.Rotation().w = 0;
		localTransform.Position() = { 0,0,0 };
		for (auto& input : m_inputs)
		{
			if (input.layer)
			{
				auto& transform = input.outputLayer->NodeLocalTransforms()[i];

				localTransform.Scale()		+= (transform.Scale() * input.weight);
				localTransform.Rotation().x	+= (transform.Rotation().x * input.weight);
				localTransform.Rotation().y += (transform.Rotation().y * input.weight);
				localTransform.Rotation().z += (transform.Rotation().z * input.weight);
				localTransform.Rotation().w += (transform.Rotation().w * input.weight);
				localTransform.Position()	+= (transform.Position() * input.weight);
			}
		}
	}
}

void AnimMixLayer::AddInputImpl(AnimLayer* layer, float weight)
{
	auto& input = m_inputs.emplace_back();
	input.layer = layer;
	input.weight = weight;
}

void AnimMixLayer::SetWeightImpl(ID index, float weight)
{
	auto& input = m_inputs[index];
	input.weight = weight;
}

void AnimMixLayer::AddInput(AnimLayer* layer, float weight)
{
	MAIN_SYSTEM_TASK_IMPL_COMMON_2(GetComponent(),
		AnimationSystem, AsyncTaskRunner, layer, weight,
		{
			self->AddInputImpl(layer, weight);
		}
	);
}

void AnimMixLayer::SetWeight(ID index, float weight)
{
	MAIN_SYSTEM_TASK_IMPL_COMMON_2(GetComponent(),
		AnimationSystem, AsyncTaskRunner, index, weight,
		{
			self->SetWeightImpl(index, weight);
		}
	);
}

NAMESPACE_END