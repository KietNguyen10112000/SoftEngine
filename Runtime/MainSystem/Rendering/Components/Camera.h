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
	//Mat4 m_view;

protected:
	Mat4 m_proj;

	SharedPtr<GraphicsRenderTarget> m_renderTarget = nullptr;
	SharedPtr<GraphicsDepthStencilBuffer> m_depthBuffer = nullptr;
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

protected:
	void Init(int renderWidth, int renderHeight);

public:
	inline virtual void OnCameraRenderBegin() {};
	inline virtual void OnCameraRenderEnd() {};

	inline auto& Projection()
	{
		return m_proj;
	}

	inline auto GetView() const
	{
		return GetCameraGlobalTransform().GetInverse();
	}
	/*inline auto& View()
	{
		return m_view;
	}*/

	inline virtual Mat4 GetCameraGlobalTransform() const
	{
		return GlobalTransform();
	}

	inline bool IsDisplaying() const
	{
		return m_isDisplaying != false;
	}

};

class API Camera : public BaseCamera
{
private:
	friend class RenderingSystem;

public:
	COMPONENT_CLASS(Camera);

	Camera();
	//virtual ~Camera();

protected:
	// Inherited via Serializable
	void CloneFrom(Serializer* serializer, Serializable* another) override;
	void SerializeToBinary(Serializer* serializer, ByteStream& stream) const override;
	void DeserializeFromBinary(Serializer* serializer, const ByteStream& stream) override;
	void SerializeToJson(Serializer* serializer, json& j) const override;
	void DeserializeFromJson(Serializer* serializer, const json& j) override;
	Handle<ClassMetadata> GetMetadata(size_t sign) override;
	void OnPropertyChanged(const UnknownAddress& var, const Variant& newValue) override;

	virtual void OnComponentAdded() override;

	virtual void OnComponentRemoved() override;

	virtual AABox GetGlobalAABB() override;

public:
	void SetProjection(const Mat4& projMat);

};

NAMESPACE_END