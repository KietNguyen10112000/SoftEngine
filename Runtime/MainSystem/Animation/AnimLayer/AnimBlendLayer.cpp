#include "AnimBlendLayer.h"

#include "MainSystem/Animation/Components/AnimationComponent.h"
#include "MainSystem/Animation/AnimationSystem.h"
#include "MainSystem/MainSystemTaskPacking.h"

#include "Scene/Scene.h"
#include "Scene/GameObject.h"

#include "AnimPlayerLayer.h"

NAMESPACE_BEGIN

void AnimBlendLayer::Run(float dt)
{
	auto l0 = m_input[0];
	auto l1 = m_input[1];

	if (!l0 || !l1)
	{
		return;
	}

	m_t = std::clamp(m_t + dt, m_rangeMin, m_rangeMax);

	auto prevSBlend = m_blendFactor;
	auto& sBlend = m_blendFactor;
	sBlend = std::clamp(m_controlFunction->Test(m_t), 0.0f, 1.0f);
	if (prevSBlend == m_blendFactor)
	{
		return;
	}

	if (m_t == m_rangeMax)
	{
		SetEnabledImpl(false);
		m_input[1 - (int)std::round(m_blendFactor)]->SetEnabledImpl(false);
	}

	auto num = m_globalTransforms.size();

	auto& transforms0 = l0->NodeGlobalTransforms();
	auto& transforms1 = l1->NodeGlobalTransforms();
	//auto& ltransforms0 = curLayer->NodeLocalTransforms();
	//auto& ltransforms1 = prevLayer->NodeLocalTransforms();
	for (size_t i = 0; i < num; i++)
	{
		auto& v0 = transforms0[i];
		auto& v1 = transforms1[i];

		m_globalTransforms[i] = Lerp(v0, v1, sBlend);
		//m_localTransforms[i] = Lerp(ltransforms1[i], ltransforms0[i], sBlend);
	}

	num = m_meshesAABB.size();

	auto& meshAABB0 = l0->MeshesAABB();
	auto& meshAABB1 = l1->MeshesAABB();
	for (size_t i = 0; i < num; i++)
	{
		auto& v0 = meshAABB0[i];
		auto& v1 = meshAABB1[i];

		auto& aabb = m_meshesAABB[i];
		aabb.m_center = Lerp(v0.m_center, v1.m_center, sBlend);
		aabb.m_halfDimensions = Lerp(v0.m_halfDimensions, v1.m_halfDimensions, sBlend);
	}
}

AnimLayer* AnimBlendLayer::GetOutput()
{
	if (!IsEnabledImpl() || m_blendFactor == 0.0f || m_blendFactor == 1.0f)
	{
		return GetMainLayer();
	}

	return this;
}

void AnimBlendLayer::SetInput(AnimLayer* l1, AnimLayer* l2)
{
	m_input[0] = l1;
	m_input[1] = l2;
}

void AnimBlendLayer::StartBlending(const SharedPtr<Function1D>& func1D, float rangeMin, float rangeMax)
{
	MAIN_SYSTEM_TASK_IMPL_COMMON_3(GetComponent(),
		AnimationSystem, AsyncTaskRunner, func1D, rangeMin, rangeMax,
		{
			if (func1D)
			{
				self->m_controlFunction = func1D;
			}

			if (self->m_controlFunction == nullptr)
			{
				// f(x) = x;
				self->m_controlFunction = std::make_shared<FunctionLinear1D>(1.0f, 0.0f);
			}

			self->m_rangeMin = rangeMin;
			self->m_rangeMax = rangeMax;

			self->m_t = 0;

			self->m_input[0]->SetEnabledImpl(true);
			self->m_input[1]->SetEnabledImpl(true);
		}
	);
}

void AnimBlendLayer::Restart()
{
	MAIN_SYSTEM_TASK_IMPL_COMMON_0(GetComponent(),
		AnimationSystem, AsyncTaskRunner,
		{
			self->m_t = 0;
			self->m_input[0]->SetEnabledImpl(true);
			self->m_input[1]->SetEnabledImpl(true);
		}
	);
}

void AnimBlendLayer::SetTime(float t)
{
	MAIN_SYSTEM_TASK_IMPL_COMMON_1(GetComponent(),
		AnimationSystem, AsyncTaskRunner, t,
		{
			self->m_t = t;
		}
	);
}

void AnimBlendLayer::SerializeToBinary(Serializer* serializer, ByteStream& stream) const
{
}

void AnimBlendLayer::DeserializeFromBinary(Serializer* serializer, const ByteStream& stream)
{
}

void AnimBlendLayer::SerializeToJson(Serializer* serializer, json& j) const
{
	AnimLayer::SerializeToJson(serializer, j);

	j["Input0"] = serializer->Serialize(m_input[0]); 
	j["Input1"] = serializer->Serialize(m_input[1]);

	j["RangeMin"] = m_rangeMin;
	j["RangeMax"] = m_rangeMax;
	j["Time"] = m_t;

	j["ControlFunction"] = serializer->Serialize(m_controlFunction);
}

void AnimBlendLayer::DeserializeFromJson(Serializer* serializer, const json& j)
{
	AnimLayer::DeserializeFromJson(serializer, j);

	serializer->Deserialize(j["Input0"], m_input[0]);
	serializer->Deserialize(j["Input1"], m_input[1]);

	m_rangeMin = j["RangeMin"];
	m_rangeMax = j["RangeMax"]; 
	m_t = j["Time"];

	serializer->Deserialize(j["ControlFunction"], m_controlFunction);

	m_blendFactor = m_controlFunction->Test(m_t);

	Run(0);
}

void AnimBlendLayer::CloneFrom(Serializer* serializer, Serializable* another)
{
	AnimLayer::CloneFrom(serializer, another);

	auto src = (AnimBlendLayer*)another;
	m_input[0] = serializer->Clone(src->m_input[0]);
	m_input[1] = serializer->Clone(src->m_input[1]);
}

Handle<ClassMetadata> AnimBlendLayer::GetMetadata(size_t sign)
{
	return Handle<ClassMetadata>();
}

void AnimBlendLayer::OnPropertyChanged(const UnknownAddress& var, const Variant& newValue)
{
}

NAMESPACE_END