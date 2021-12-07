#pragma once

#include <IObject.h>

class IStaticObject : public IRenderableObject
{
public:
	inline virtual ~IStaticObject() {};

public:
	inline virtual void Update(Engine* engine) override {};

};

class BasicModel;
class Texture2D;
class TBNModel;
class RawTBNModel;

class BasicObject : public IStaticObject
{
private:
	BasicModel* m_model = nullptr;
	Texture2D* m_diffuse = nullptr;

public:
	BasicObject(const std::wstring& modelPath, const std::wstring& texturePath, const Mat4x4& preTransform = Mat4x4());
	~BasicObject();

public:
	virtual void Render(IRenderer* renderer) override;

	virtual void RenderShadow(IRenderer* renderer) override;
};

class NormalMappingObject : public IStaticObject
{
protected:
	TBNModel* m_model = nullptr;
	Texture2D* m_diffuseMap = nullptr;
	Texture2D* m_normalMap = nullptr;

	NativeResourceHandle* m_resourceHandles[2] = {};

public:
	NormalMappingObject(const std::wstring& modelPath,
		const std::wstring& diffuseMapPath, const std::wstring& normalMapPath, 
		const Mat4x4& preTransform = Mat4x4());

	~NormalMappingObject();

public:
	virtual void Render(IRenderer* renderer) override;

};

class NormalSpecularMappingObject : public IStaticObject
{
protected:
	TBNModel* m_model = nullptr;
	Texture2D* m_diffuseMap = nullptr;
	Texture2D* m_normalMap = nullptr;
	Texture2D* m_specularMap = nullptr;

	NativeResourceHandle* m_resourceHandles[3] = {};

public:
	NormalSpecularMappingObject(
		const std::wstring& modelPath,
		const std::wstring& diffuseMapPath, 
		const std::wstring& normalMapPath,
		const std::wstring& specularMapPath,
		const Mat4x4& preTransform = Mat4x4());

	~NormalSpecularMappingObject();

public:
	virtual void Render(IRenderer* renderer) override;

};

class PBRObject : public IStaticObject
{
protected:
	RawTBNModel* m_model = nullptr;
	Texture2D* m_diffuseMap = nullptr;
	Texture2D* m_normalMap = nullptr;
	Texture2D* m_metallicMap = nullptr;
	Texture2D* m_roughnessMap = nullptr;
	Texture2D* m_aoMap = nullptr;

	NativeResourceHandle* m_resourceHandles[5] = {};

public:
	PBRObject(
		const std::wstring& modelPath,
		const std::wstring& diffuseMapPath,
		const std::wstring& normalMapPath,
		const std::wstring& metallicMapPath,
		const std::wstring& roughnessMapPath,
		const std::wstring& aoMapPath = L"",
		const Mat4x4& preTransform = Mat4x4());

	~PBRObject();

public:
	virtual void Render(IRenderer* renderer) override;

};