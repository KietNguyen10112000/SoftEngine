#include "RotateScript.h"

void RotateScript::OnStart()
{
}

void RotateScript::OnUpdate(float dt)
{
	auto transform = GetLocalTransform();

	auto rot = transform.Rotation().ToMat4();
	rot = rot
		* Mat4::Rotation(Vec3::X_AXIS, m_rotationSpeed.x * dt)
		* Mat4::Rotation(Vec3::Y_AXIS, m_rotationSpeed.y * dt)
		* Mat4::Rotation(Vec3::Z_AXIS, m_rotationSpeed.z * dt);

	transform.Rotation() = rot;

	SetLocalTransform(transform);
}

Handle<ClassMetadata> RotateScript::GetMetadata(size_t sign)
{
	auto metadata = mheap::New<ClassMetadata>("RotateScript", this);

	metadata->AddProperty(Accessor::For("Rotation speed", m_rotationSpeed, this));

	return metadata;
}
