#pragma once

#include "Animator.h"
#include "PBRMaterial.h"

#include <AnimModel3D.h>
#include <IObject.h>


class PBRMultiMeshAnimObject : public IRenderableObject
{
public:
	
	std::vector<PBRMaterialHandle> m_meshsMaterial;

	TBNAnimator m_animator;

public:
	inline PBRMultiMeshAnimObject(const std::wstring& modelPath, std::vector<PBRMaterialPath>& paths,
		const Mat4x4& preTransform = Mat4x4())
	{
		void* args[] = { (void*)&preTransform };
		auto* temp = Resource::Get<TBNAnimModel>(modelPath, 1, args);
		m_animator = TBNAnimator(temp);
		Resource::Release(&temp);

		m_meshsMaterial.resize(paths.size());
		size_t i = 0;
		for (auto& m : m_meshsMaterial)
		{
			m.From(paths[i]);
			i++;
			if (i == m_animator.m_model->m_renderBuf.size()) break;
		}
	};

	inline ~PBRMultiMeshAnimObject()
	{
		m_animator.Free();
		for (auto& m : m_meshsMaterial)
		{
			m.material.Free();
		}
	};

public:
	inline auto& Animator() { return m_animator; };
	inline auto* Model() { return m_animator.m_model; }

public:
	inline void Update(Engine* engine) override
	{
		//bool endCurAnim = false;
		//offset with m_transform
		m_animator.Play(engine);
		/*if (endCurAnim)
		{
			if (m_animator.CurrentAnimation()->id == 2)
			{
				m_transform *= GetTranslationMatrix(0, 0, -13.8f);
			}
			m_animator.SetAnimation(11);
			m_animator.Reset(true);
			
		}*/
	};

	inline void Render(IRenderer* renderer) override
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
	};

};