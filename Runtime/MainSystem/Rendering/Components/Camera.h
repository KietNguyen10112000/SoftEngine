#pragma once

#include "RenderingComponent.h"

#include "MainSystem/Rendering/RenderingPipeline/RenderingPipeline.h"

NAMESPACE_BEGIN

class BaseCamera : public RenderingComponent
{
private:
	friend class RenderingSystem;

	ID m_activeID = INVALID_ID;
	uint32_t m_isDisplaying = false;
	uint32_t m_priority = 0;

	Mat4 m_proj;
	//Mat4 m_view;

protected:
	SharedPtr<GraphicsRenderTarget> m_renderTarget = nullptr;
	RenderingPipeline* m_pipeline = nullptr;

public:
	inline BaseCamera(RENDER_TYPE type) : RenderingComponent(type) {};

	inline virtual ~BaseCamera()
	{
		if (m_pipeline)
		{
			delete m_pipeline;
		}
	}

	inline auto& Projection()
	{
		return m_proj;
	}

	inline auto GetView() const
	{
		return GlobalTransform().GetInverse();
	}
	/*inline auto& View()
	{
		return m_view;
	}*/

};

class Camera : public BaseCamera
{
private:
	friend class RenderingSystem;

public:
	COMPONENT_CLASS(Camera);

	Camera();
	//virtual ~Camera();

	// Inherited via RenderingComponent
	virtual void Serialize(Serializer* serializer) override;

	virtual void Deserialize(Serializer* serializer) override;

	virtual void CleanUp() override;

	virtual Handle<ClassMetadata> GetMetadata(size_t sign) override;

	virtual void OnPropertyChanged(const UnknownAddress& var, const Variant& newValue) override;

	virtual void OnComponentAdded() override;

	virtual void OnComponentRemoved() override;

	virtual AABox GetGlobalAABB() override;

};

NAMESPACE_END