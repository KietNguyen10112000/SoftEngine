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
	m_globalTransform = *m_myGlobalTransform.Read();
}

Handle<Serializable> AnimModelStaticMeshRenderer::Clone(Serializer* serializer)
{
	auto ret = mheap::New<AnimModelStaticMeshRenderer>(false);

	ret->m_model3D = m_model3D;
	ret->m_mesh = m_mesh;
	ret->m_texture = m_texture;

	return ret;
}

NAMESPACE_END