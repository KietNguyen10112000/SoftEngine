#pragma once

#include "MainSystem/MainSystemInfo.h"
#include "Common/Base/MainComponent.h"

#include "RENDER_TYPE.h"

#include "Math/Math.h"

#include "Scene/GameObject.h"

NAMESPACE_BEGIN

class API RenderingComponent : public MainComponent
{
private:
	friend class GameObject;
	MAIN_SYSTEM_FRIEND_CLASSES();
	constexpr static ID COMPONENT_ID = MainSystemInfo::RENDERING_ID;

protected:
	const RENDER_TYPE m_RENDER_TYPE;

	//Mat4 m_globalTransform;
private:
	float m_localAlpha = 1.0f;
	float m_globalAlpha = 1.0f;
	bool m_cascadeAlpha = true;
	bool m_padd[7];

public:
	RenderingComponent(const RENDER_TYPE type) : m_RENDER_TYPE(type) {};
	virtual ~RenderingComponent() {};

protected:
	virtual void OnTransformChanged() override;

	// Inherited via Serializable
	void CloneFrom(Serializer* serializer, Serializable* another) override;
	void SerializeToBinary(Serializer* serializer, ByteStream& stream) const override;
	void DeserializeFromBinary(Serializer* serializer, const ByteStream& stream) override;
	void SerializeToJson(Serializer* serializer, json& j) const override;
	void DeserializeFromJson(Serializer* serializer, const json& j) override;

public:
	void SetOpacity(float alpha);
	void SetCascadeOpacityEnabled(float enable);
	float GetOpacity() const;
	float GetGlobalOpacity() const;

	inline auto GetRenderType() const
	{
		return m_RENDER_TYPE;
	}

	inline auto& GlobalTransform() const
	{
		//return m_globalTransform;
		return m_committedObject->GetCommittedGlobalTransform();
	}

};

NAMESPACE_END