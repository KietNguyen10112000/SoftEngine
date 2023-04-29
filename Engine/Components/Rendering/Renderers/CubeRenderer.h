#pragma once

#include "Renderer.h"

#include "Math/Math.h"

#include "Objects/GameObject.h"

#include "Graphics/Graphics.h"
#include "Graphics/DebugGraphics.h"

NAMESPACE_BEGIN

class CubeRenderer : public Renderer
{
public:
	Vec3 m_dimensions;
	Vec3 m_color;
	
public:
	inline CubeRenderer(const Vec3& dimensions, const Vec3& color) 
		: m_dimensions(dimensions), m_color(color) {};

public:
	virtual void OnComponentAddedToScene() override{}

	virtual void OnComponentAdded() override {}

	virtual void OnComponentRemoved() override {}

	virtual void OnComponentRemovedFromScene() override {}

public:
	virtual void Render(RenderingSystem* rdr) override
	{
		auto mat = m_object->GetTransformMat4();
		Graphics::Get()
			->GetDebugGraphics()
			->DrawCube(
				Mat4::Scaling(m_dimensions / 2.0f) * mat,
				Vec4(m_color, 1.0f)
			);
	}

	virtual AABox GetLocalAABB() override
	{
		return {
			{ 0,0,0 },
			m_dimensions
		};
	}

};

NAMESPACE_END