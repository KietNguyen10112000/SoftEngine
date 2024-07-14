#pragma once

#include "Camera.h"

NAMESPACE_BEGIN

class API CameraTPP : public Camera
{
protected:
	Handle<GameObject> m_target;
	Mat4 m_view;

	bool m_enableTPP = true;

	void Trace(Tracer* tracer)
	{
		tracer->Trace(m_target);
	}

public:
	COMPONENT_CLASS(CameraTPP);
	CameraTPP();

	virtual void OnCameraRenderBegin() override;
	//virtual Mat4 GetView() override;

	// Inherited via Serializable
	void CloneFrom(Serializer* serializer, Serializable* another) override;
	void SerializeToBinary(Serializer* serializer, ByteStream& stream) const override;
	void DeserializeFromBinary(Serializer* serializer, const ByteStream& stream) override;
	void SerializeToJson(Serializer* serializer, json& j) const override;
	void DeserializeFromJson(Serializer* serializer, const json& j) override;

	void SetTarget(const Handle<GameObject>& object);

	void SetViewPoint(const Vec3& point, const Vec3& up = Vec3::UP);

	void SetTPPEnabled(bool enable);

	inline auto IsTPPEnabled() const
	{
		return m_enableTPP;
	}

	inline virtual Mat4 GetCameraGlobalTransform() const override
	{
		if (m_enableTPP)
		{
			return (Mat4::Translation(-m_target->GetCommittedGlobalTransform().Position()) * m_view).GetInverse();
		}

		return GlobalTransform();
	}

};

NAMESPACE_END