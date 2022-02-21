#include "StaticObjectInstancing.h"

#include <Resource.h>
#include <RenderPipeline.h>
#include <Model3D.h>

NormalMappingObjectInstancing::NormalMappingObjectInstancing(
	const std::wstring& modelPath,
	const std::wstring& diffuseMapPath,
	const std::wstring& normalMapPath,
	const Mat4x4& preTransform)
{
	void* args[] = { (void*)&preTransform };
	m_model = Resource::Get<TBNModel>(modelPath, 1, args);

	size_t flag = Texture2D::FLAG::DISCARD_SRGB;
	args[0] = &flag;

	m_diffuseMap = Resource::Get<Texture2D>(diffuseMapPath, 1, args);
	m_normalMap = Resource::Get<Texture2D>(normalMapPath, 1, args);

	m_resourceHandles[0] = m_diffuseMap->GetNativeHandle();
	m_resourceHandles[1] = m_normalMap->GetNativeHandle();

	m_rpl = RenderPipelineManager::Get(
		INSTANCING_OBJ_INPUT_LAYOUT,
		L"Model3D/Instancing/VS_TBN_Model",
		L"Model3D/PSNormalMapping"
	);

}

NormalMappingObjectInstancing::NormalMappingObjectInstancing(
	const std::wstring& modelPath, 
	const std::wstring& diffuseMapPath, 
	const std::wstring& normalMapPath, 
	Mat4x4* perInstanceTransforms, size_t instanceCount,
	const Mat4x4& preTransform)
{
	void* args[] = { (void*)&preTransform };
	m_model = Resource::Get<TBNModel>(modelPath, 1, args);

	size_t flag = Texture2D::FLAG::DISCARD_SRGB;
	args[0] = &flag;

	m_diffuseMap = Resource::Get<Texture2D>(diffuseMapPath, 1, args);
	m_normalMap = Resource::Get<Texture2D>(normalMapPath, 1, args);

	m_resourceHandles[0] = m_diffuseMap->GetNativeHandle();
	m_resourceHandles[1] = m_normalMap->GetNativeHandle();

	m_rpl = RenderPipelineManager::Get(
		INSTANCING_OBJ_INPUT_LAYOUT,
		L"Model3D/Instancing/VS_TBN_Model",
		L"Model3D/PSNormalMapping"
	);

	m_instanceBuffer = new VertexBuffer(&perInstanceTransforms[0], instanceCount, sizeof(Mat4x4));
	m_instanceCount = instanceCount;
	
}

NormalMappingObjectInstancing::~NormalMappingObjectInstancing()
{
	Resource::Release(&m_model);
	Resource::Release(&m_diffuseMap);
	Resource::Release(&m_normalMap);

	delete m_instanceBuffer;
}

void NormalMappingObjectInstancing::Render(IRenderer* renderer)
{
	FlushTransform();
	m_rpl->PSSetResources(m_resourceHandles, 2, 0);
	
	size_t count = 0;
	auto meshs = m_model->GetMeshs(count);

	for (size_t i = 0; i < count; i++)
	{
		auto& mesh = meshs[i];
		m_rpl->VSSetVar(mesh.svar, MESH_LOCAL_TRANSFORM_LOCATION);
		m_rpl->PSSetVar(mesh.materialSvar, MESH_PS_MATERIAL_LOCATION);
		renderer->RenderInstance(m_rpl, mesh.vb, m_instanceBuffer, m_instanceCount, mesh.ib);
	}
}

void NormalMappingObjectInstancing::Update(Engine* engine)
{
}
