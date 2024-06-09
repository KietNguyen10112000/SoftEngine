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
		input.outputLayer = input.layer->GetOutput();
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