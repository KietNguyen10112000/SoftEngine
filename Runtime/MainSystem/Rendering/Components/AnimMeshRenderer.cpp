#include "AnimMeshRenderer.h"

NAMESPACE_BEGIN

AnimMeshRenderer::AnimMeshRenderer() : RenderingComponent(RENDER_TYPE_ANIM_MESH_RENDERER)
{
}

void AnimMeshRenderer::Serialize(Serializer* serializer)
{
}

void AnimMeshRenderer::Deserialize(Serializer* serializer)
{
}

void AnimMeshRenderer::CleanUp()
{
}

Handle<ClassMetadata> AnimMeshRenderer::GetMetadata(size_t sign)
{
	return Handle<ClassMetadata>();
}

void AnimMeshRenderer::OnPropertyChanged(const UnknownAddress& var, const Variant& newValue)
{
}

Handle<Serializable> AnimMeshRenderer::Clone(Serializer* serializer)
{
	struct CloneParam
	{
		AnimMeshRenderer* src;
		AnimMeshRenderer* dest;
	};

	auto ret = mheap::New<AnimMeshRenderer>();

	ret->m_model3D = m_model3D;
	ret->m_mesh = m_mesh;
	ret->m_texture = m_texture;

	auto callbackRunner = serializer->GetCallbackRunner();
	auto task = callbackRunner->CreateTask([](Serializer* serializer, void* p)
		{
			TASK_SYSTEM_UNPACK_PARAM_2(CloneParam, p, src, dest);

			auto& addresses = serializer->GetAddressMap();

			auto it = addresses.find(src->m_animMeshRenderingBuffer.get());
			assert(it != addresses.end());

			dest->m_animMeshRenderingBuffer = *(decltype(dest->m_animMeshRenderingBuffer)*)(it->second);
		}
	);

	auto param = callbackRunner->CreateParam<CloneParam>(&task);
	param->src = this;
	param->dest = ret.Get();

	callbackRunner->RunAsync(&task);

	return ret;
}

void AnimMeshRenderer::OnComponentAdded()
{
}

void AnimMeshRenderer::OnComponentRemoved()
{
}

AABox AnimMeshRenderer::GetGlobalAABB()
{
	return m_animMeshRenderingBuffer->buffer.Read()->meshesAABB[m_mesh->m_model3DIdx].MakeTransform(GlobalTransform());
	//return AABox(Vec3(0,0,0), Vec3(10000,10000,10000));
}

NAMESPACE_END