#include "Camera.h"

#include "Scene/GameObject.h"
#include "Scene/Scene.h"

#include "MainSystem/Rendering/RenderingSystem.h"

NAMESPACE_BEGIN

Camera::Camera() : RenderingComponent(RENDER_TYPE::CAMERA)
{
}

void Camera::Serialize(ByteStream& stream)
{
}

void Camera::Deserialize(ByteStreamRead& stream)
{
}

void Camera::CleanUp()
{
}

Handle<ClassMetadata> Camera::GetMetadata(size_t sign)
{
	return Handle<ClassMetadata>();
}

void Camera::OnPropertyChanged(const UnknownAddress& var)
{
}

void Camera::OnComponentAdded()
{
	GameObject()->Scene()->GetRenderingSystem()->AddCamera(this);
}

void Camera::OnComponentRemoved()
{
	GameObject()->Scene()->GetRenderingSystem()->RemoveCamera(this);
}

AABox Camera::GetGlobalAABB()
{
	return AABox();
}

NAMESPACE_END