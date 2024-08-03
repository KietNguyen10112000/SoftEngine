#include "RenderingComponent.h"

#include "Scene/GameObject.h"

NAMESPACE_BEGIN

void RenderingComponent::OnTransformChanged()
{
	//m_globalTransform = GetGameObject()->ReadGlobalTransformMat();
	//m_globalTransform = GetGameObject()->ReadGlobalTransform().ToTransformMatrix();
}

void RenderingComponent::CloneFrom(Serializer* serializer, Serializable* another)
{
	auto src = (RenderingComponent*)another;
	m_localAlpha = src->m_localAlpha;
	m_globalAlpha = src->m_globalAlpha;
	m_cascadeAlpha = src->m_cascadeAlpha;
}

void RenderingComponent::SerializeToBinary(Serializer* serializer, ByteStream& stream) const
{
}

void RenderingComponent::DeserializeFromBinary(Serializer* serializer, const ByteStream& stream)
{
}

void RenderingComponent::SerializeToJson(Serializer* serializer, json& j) const
{
	j["LocalAlpha"] = m_localAlpha;
	j["GlobalAlpha"] = m_globalAlpha;
	j["CascadeAlpha"] = m_cascadeAlpha;
}

void RenderingComponent::DeserializeFromJson(Serializer* serializer, const json& j)
{
	if (j.contains("LocalAlpha"))
	{
		m_localAlpha = j["LocalAlpha"];
		m_globalAlpha = j["GlobalAlpha"];
		m_cascadeAlpha = j["CascadeAlpha"];
	}
}

void RenderingComponent::SetOpacity(float alpha)
{
	constexpr static void (*Impl)(RenderingComponent*, RenderingComponent*) = [](RenderingComponent* comp, RenderingComponent* last) -> void
	{
		if (comp)
		{
			comp->m_globalAlpha = comp->m_localAlpha * ((comp->m_cascadeAlpha && last) ? last->m_globalAlpha : 1.0f);
			last = comp;
		}
		auto obj = comp->GetGameObject();
		for (auto& child : obj->Children())
		{
			Impl(child->GetComponentRaw<RenderingComponent>(), last);
		}
	};

	m_localAlpha = alpha;
	Impl(this, nullptr);
}

void RenderingComponent::SetCascadeOpacityEnabled(float enable)
{
	m_cascadeAlpha = enable;
}

float RenderingComponent::GetOpacity() const
{
	return m_localAlpha;
}

float RenderingComponent::GetGlobalOpacity() const
{
	return m_globalAlpha;
}

NAMESPACE_END