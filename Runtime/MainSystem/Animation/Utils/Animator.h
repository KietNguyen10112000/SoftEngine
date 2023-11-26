#pragma once

#include "Core/TypeDef.h"

#include "Resources/AnimModel.h"

NAMESPACE_BEGIN

class Animator
{
public:
	ID m_animationId = 0;

	Resource<AnimModel>	m_model3D;

	SharedPtr<AnimModel::AnimMeshRenderingBuffer> m_animMeshRenderingBuffer;

	Array<Handle<GameObject>> m_animMeshRendererObjs;

	// duration in sec
	float m_duration;
	float m_durationRatio;

	float m_t = 0;

	size_t m_numAnimIterationCount = 0;

private:
	TRACEABLE_FRIEND();
	inline void Trace(Tracer* tracer)
	{
		tracer->Trace(m_animMeshRendererObjs);
	}

public:
	inline ID GetCurrentAnimationId() const
	{
		return m_animationId;
	}

	inline void Update(float dt)
	{
		m_t += dt * m_durationRatio;
		if (m_t > m_duration)
		{
			uint32_t num = (uint32_t)std::floor(m_t / m_duration);
			m_t -= num * m_duration;

			m_numAnimIterationCount += num;
		}
	}

};

NAMESPACE_END