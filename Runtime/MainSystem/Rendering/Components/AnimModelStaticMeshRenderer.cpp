#include "AnimModelStaticMeshRenderer.h"

#include "Scene/GameObject.h"
#include "Scene/Scene.h"

#include "MainSystem/Rendering/RenderingSystem.h"

NAMESPACE_BEGIN

AnimModelStaticMeshRenderer::AnimModelStaticMeshRenderer(bool loadDefault) : MeshBasicRenderer(loadDefault)
{
}

AnimModelStaticMeshRenderer::AnimModelStaticMeshRenderer(String modelPath, String texture2DPath) : MeshBasicRenderer(modelPath, texture2DPath)
{
}

void AnimModelStaticMeshRenderer::OnTransformChanged()
{
	m_globalTransform = (*m_myGlobalTransform.Read()) * GetGameObject()->ReadGlobalTransformMat();
}

void AnimModelStaticMeshRenderer::CloneFrom(Serializer* serializer, Serializable* another)
{
	auto src = (AnimModelStaticMeshRenderer*)another;

	auto ret = this;

	ret->m_model3D = src->m_model3D;
	ret->m_mesh = src->m_mesh;
	ret->m_texture = src->m_texture;
}

NAMESPACE_END