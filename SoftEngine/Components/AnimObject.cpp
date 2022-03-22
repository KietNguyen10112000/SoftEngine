#include "AnimObject.h"

PBRMultiMeshAnimObject::PBRMultiMeshAnimObject(const std::wstring& modelPath, std::vector<PBRMaterialPath>& paths, const Mat4x4& preTransform)
{
	void* args[] = { (void*)&preTransform };
	auto* temp = Resource::Get<TBNAnimModel>(modelPath, 1, args);
	m_animator = TBNAnimator(temp);
	Resource::Release(&temp);

	if (!m_animator.IsAABBCalculated())
	{
		m_animator.CalculateAABBPerAnimation();
		m_animator.m_model->FreeMeshsBuffer();
	}

	m_meshsMaterial.resize(paths.size());
	size_t i = 0;
	for (auto& m : m_meshsMaterial)
	{
		m.From(paths[i]);
		i++;
		if (i == m_animator.m_model->m_renderBuf.size()) break;
	}
}

PBRMultiMeshAnimObject::PBRMultiMeshAnimObject(const std::wstring& modelPath, const Mat4x4& preTransform)
{
	std::wstring rootPath = PopPath(modelPath);
	std::vector<std::wstring> materials;
	void* args[] = { (void*)&preTransform, &materials };
	auto* temp = Resource::Get<TBNAnimModel>(modelPath, 1, args);

	m_animator = TBNAnimator(temp);
	Resource::Release(&temp);

	if (!m_animator.IsAABBCalculated())
	{
		m_animator.CalculateAABBPerAnimation();
		m_animator.m_model->FreeMeshsBuffer();
	}

	m_meshsMaterial.resize(materials.size() / 5);

	PBRMaterialPath temp1;

	size_t i = 0;
	for (auto& m : m_meshsMaterial)
	{
		auto id = i * 5;

		temp1 = {
			materials[id	] == L"" ? L"" : CombinePath(rootPath, materials[id    ]),
			materials[id + 1] == L"" ? L"" : CombinePath(rootPath, materials[id + 1]),
			materials[id + 2] == L"" ? L"" : CombinePath(rootPath, materials[id + 2]),
			materials[id + 3] == L"" ? L"" : CombinePath(rootPath, materials[id + 3]),
			materials[id + 4] == L"" ? L"" : CombinePath(rootPath, materials[id + 4])
		};

		m.From(temp1);

		i++;
		if (i == m_animator.m_model->m_renderBuf.size()) break;
	}
}

PBRMultiMeshAnimObject::~PBRMultiMeshAnimObject()
{
	m_animator.Free();
	for (auto& m : m_meshsMaterial)
	{
		m.material.Free();
	}
}

void PBRMultiMeshAnimObject::Render(IRenderer* renderer)
{
	FlushTransform();

	RenderPipeline::VSSetVar(TBNAnimModel::localSV, 2);
	RenderPipeline::VSSetVar(TBNAnimModel::boneSV, 3);

	auto& bones = m_animator.m_bones;
	TBNAnimModel::boneSV->Update(&bones[0], sizeof(Mat4x4) * bones.size());

	size_t i = 0;
	auto& renderBuf = m_animator.m_model->m_renderBuf;
	auto& meshLocalTransform = m_animator.m_meshsLocalTransform;
	auto& rpls = m_animator.m_model->m_rpls;

	auto loop = m_meshsMaterial.size();

	for (auto& buf : renderBuf)
	{
		RenderPipeline::PSSetResources(m_meshsMaterial[i % loop].resourceHandles, 5, 0);
		if (meshLocalTransform[i]) TBNAnimModel::localSV->Update(meshLocalTransform[i], sizeof(Mat4x4));
		renderer->Render(rpls[buf.rplIndex], buf.vb, buf.ib);
		i++;
	}
}

AABB PBRMultiMeshAnimObject::GetLocalAABB()
{
	AABB ret = {};
	Vec3 points[16];
	//for (auto& animation : m_animator.m_model->m_animations)
	//{
	for (auto& AABBs : m_animator.CurrentAnimation()->meshAABBs)
	{
		for (auto& aabb : AABBs.aabb)
		{
			ret.Joint(aabb.value, points);
		}
	}
	//}
	return ret;
}
