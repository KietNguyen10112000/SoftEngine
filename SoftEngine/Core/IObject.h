#pragma once

#include <Math/Collision.h>

#include "Math/AABB.h"

#include <RenderPipeline.h>

class ShaderVar;
class Engine;

#define OBJECT_TRANSFROM_SHADER_LOCATION 0
#define CAMERA_MVP_SHADER_LOCATION 1
#define CAMERA_POSITION_SHADER_LOCATION 2
#define INSTANCES_TRANSFORM_SHADER_LOCATION 3

class IObject
{
public:
	inline virtual ~IObject() {};

protected:
	Mat4x4 m_transform;

public:
	virtual void Update(Engine* engine) = 0;

public:
	inline virtual Mat4x4& Transform() { return m_transform; };

	inline virtual Vec3 GetPosition() { return m_transform.GetPosition(); };

	inline virtual void SetPosition(const Vec3& position) { m_transform.SetPosition(position); };
	inline virtual void SetPosition(float x, float y, float z) { m_transform.SetPosition(x, y, z); };

	inline virtual std::wstring ToString() { return L"IObject"; };

};

class IRenderableObject : public IObject
{
protected:
	RenderPipeline* m_rpl = nullptr;

	inline static size_t count = 0;

public:
	inline static ShaderVar* shaderTransform = nullptr;

public:
	inline IRenderableObject()
	{
		count++;
		if (shaderTransform == nullptr)
		{
			shaderTransform = new ShaderVar(&m_transform, sizeof(Mat4x4));
			RenderPipeline::VSSetVar(shaderTransform, OBJECT_TRANSFROM_SHADER_LOCATION);
		}
	};
	inline virtual ~IRenderableObject() 
	{ 
		count--;
		RenderPipelineManager::Release(&m_rpl); 
		if (count == 0) delete shaderTransform;
	};

public:
	inline RenderPipeline* GetRenderPipeline() { return m_rpl; };
	inline void SetRenderPipeline(RenderPipeline*& rpl)
	{
		if (m_rpl) RenderPipelineManager::Release(&m_rpl);
		m_rpl = RenderPipelineManager::Get(rpl);
	};

public:
	virtual void Render(IRenderer* renderer) = 0;

	//option
	inline virtual void RenderShadow(IRenderer* renderer) {};

	inline virtual void FlushTransform() { shaderTransform->Update(&m_transform, sizeof(Mat4x4)); };

	//return AABB in world space
	inline virtual AABB GetAABB() 
	{
		AABB ret = GetLocalAABB();
		ret.Transform(m_transform);
		return ret;
	};
	//return AABB in local space
	inline virtual AABB GetLocalAABB() { return {}; };
};

//m_transform is view matrix
class ICamera : public IObject
{
protected:
	Mat4x4 m_mvp;
	Mat4x4 m_proj;

	//camera frustum
	Frustum m_frustum;

	inline static size_t count = 0;

public:
	inline static ShaderVar* shaderMVP = nullptr;

	inline static struct ShaderCameraBuffer{
		Mat4x4 mvp;
		Mat4x4 invMVP;
		Mat4x4 view;
		Mat4x4 proj;
	} cameraBuf = {};

public:
	inline ICamera()
	{
		count++;
		if (shaderMVP == nullptr)
		{
			shaderMVP = new ShaderVar(&cameraBuf, sizeof(ShaderCameraBuffer));
		}
	};
	inline virtual ~ICamera() 
	{
		count--;
		if (count == 0)
		{
			delete shaderMVP;
		}
	};

public:
	inline auto& MVP() { return m_mvp; };
	inline auto& ProjectionMatrix() { return m_proj; };
	inline auto& ViewMatrix() { return m_transform; };

	inline auto* FrustumCorners() { return &m_frustum.m_corners[0]; };
	inline auto& Frustum() { return m_frustum; };

	inline virtual void Update(Engine* engine) {};
};


class IInstancingObject : public IRenderableObject
{
protected:
	Mat4x4 m_instancesTransform;

	inline static uint32_t count = 0;

public:
	inline static ShaderVar* shaderInstancesTransform = nullptr;

public:
	inline IInstancingObject()
	{
		count++;
		if (!shaderInstancesTransform)
		{
			shaderInstancesTransform = new ShaderVar(&m_instancesTransform, sizeof(Mat4x4));
			RenderPipeline::VSSetVar(shaderInstancesTransform, INSTANCES_TRANSFORM_SHADER_LOCATION);
		}
	};

	inline virtual ~IInstancingObject()
	{
		count--;
		if (count == 0)
		{
			delete shaderInstancesTransform;
		}
	};

	inline virtual void FlushTransform() override
	{
		IRenderableObject::FlushTransform();
		shaderInstancesTransform->Update(&m_instancesTransform, sizeof(Mat4x4));
	};

	inline virtual Mat4x4& InstancesTransform() { return m_instancesTransform; };

};
