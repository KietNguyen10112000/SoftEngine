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
	PBRMultiMeshAnimObject(const std::wstring& modelPath, std::vector<PBRMaterialPath>& paths,
		const Mat4x4& preTransform = Mat4x4());

	PBRMultiMeshAnimObject(const std::wstring& modelPath,
		const Mat4x4& preTransform = Mat4x4());

	~PBRMultiMeshAnimObject();

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

	void Render(IRenderer* renderer) override;

public:
	//return current animation aabb
	virtual AABB GetLocalAABB() override;

};