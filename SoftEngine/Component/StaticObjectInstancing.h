#pragma once

#include <IObject.h>

class BasicModel;
class Texture2D;
class TBNModel;

#define INSTANCING_OBJ_INPUT_LAYOUT												\
R"(
	struct 
	{
		Vec3 position; POSITION, PER_VERTEX #
		Vec2 textCoord; TEXTCOORD, PER_VERTEX #
		Vec3 tangent; TANGENT, PER_VERTEX #
		Vec3 bitangent; BITANGENT, PER_VERTEX #
		Vec3 normal; NORMAL, PER_VERTEX #
		Vec4 row1; INSTANCE_TRANSFORM_ROW1, PER_INSTANCE, 1 #
		Vec4 row2; INSTANCE_TRANSFORM_ROW2, PER_INSTANCE, 1 #
		Vec4 row3; INSTANCE_TRANSFORM_ROW3, PER_INSTANCE, 1 #
		Vec4 row4; INSTANCE_TRANSFORM_ROW4, PER_INSTANCE, 1 #
	}
)"


class NormalMappingObjectInstancing :  public IInstancingObject
{
protected:
	TBNModel* m_model = nullptr;
	Texture2D* m_diffuseMap = nullptr;
	Texture2D* m_normalMap = nullptr;

	NativeResourceHandle* m_resourceHandles[2] = {};

	VertexBuffer* m_instanceBuffer = nullptr;
	uint32_t m_instanceCount = 0;

public:
	NormalMappingObjectInstancing(
		const std::wstring& modelPath,
		const std::wstring& diffuseMapPath, 
		const std::wstring& normalMapPath,
		const Mat4x4& preTransform = Mat4x4());

	NormalMappingObjectInstancing(
		const std::wstring& modelPath,
		const std::wstring& diffuseMapPath,
		const std::wstring& normalMapPath,
		Mat4x4* perInstanceTransforms, size_t instanceCount,
		const Mat4x4& preTransform = Mat4x4());

	virtual ~NormalMappingObjectInstancing();

public:
	virtual void Render(IRenderer* renderer) override;

	// Inherited via IRenderableObject
	virtual void Update(Engine* engine) override;
};