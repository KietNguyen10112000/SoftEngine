#pragma once

#include <Resource.h>

#ifdef _DEBUG

#define DEFAULT_DIFFUSE_MAP_PATH		L"D:/KEngine/ResourceFile/temp_img/PBR/DefaultPBRMaterial/diffuse.png"
#define DEFAULT_NORMAL_MAP_PATH			L"D:/KEngine/ResourceFile/temp_img/PBR/DefaultPBRMaterial/normal.png"
#define DEFAULT_METALLIC_MAP_PATH		L"D:/KEngine/ResourceFile/temp_img/PBR/DefaultPBRMaterial/mettalic.png"
#define DEFAULT_ROUGHNESS_MAP_PATH		L"D:/KEngine/ResourceFile/temp_img/PBR/DefaultPBRMaterial/roughness.png"
#define DEFAULT_AO_MAP_PATH				L"D:/KEngine/ResourceFile/temp_img/PBR/DefaultPBRMaterial/ao.png"

#else

#define DEFAULT_DIFFUSE_MAP_PATH		L"DefaultPBRMaterial/diffuse.png"
#define DEFAULT_NORMAL_MAP_PATH			L"DefaultPBRMaterial/normal.png"
#define DEFAULT_METALLIC_MAP_PATH		L"DefaultPBRMaterial/mettalic.png"
#define DEFAULT_ROUGHNESS_MAP_PATH		L"DefaultPBRMaterial/roughness.png"
#define DEFAULT_AO_MAP_PATH				L"DefaultPBRMaterial/ao.png"

#endif // _DEBUG

//diffuse, normal, metallic, roughness, ao
struct PBRMaterialPath
{
	std::wstring diffuseMap;
	std::wstring normalMap;
	std::wstring metallicMap;
	std::wstring roughnessMap;
	std::wstring aoMap;

	inline void FillEmpty()
	{
		if (diffuseMap.empty()) diffuseMap = DEFAULT_DIFFUSE_MAP_PATH;
		if (normalMap.empty()) normalMap = DEFAULT_NORMAL_MAP_PATH;
		if (metallicMap.empty()) metallicMap = DEFAULT_METALLIC_MAP_PATH;
		if (roughnessMap.empty()) roughnessMap = DEFAULT_ROUGHNESS_MAP_PATH;
		if (aoMap.empty()) aoMap = DEFAULT_AO_MAP_PATH;
	}
};

struct PBRMaterial
{
	Texture2D* diffuseMap = nullptr;
	Texture2D* normalMap = nullptr;
	Texture2D* metallicMap = nullptr;
	Texture2D* roughnessMap = nullptr;
	Texture2D* aoMap = nullptr;

	inline void From(PBRMaterialPath& path)
	{
		path.FillEmpty();
		void* args[] = { 0 };
		size_t flag = Texture2D::FLAG::DISCARD_SRGB;
		args[0] = &flag;
		diffuseMap = Resource::Get<Texture2D>(path.diffuseMap, 1, args);
		normalMap = Resource::Get<Texture2D>(path.normalMap, 1, args);
		metallicMap = Resource::Get<Texture2D>(path.metallicMap, 1, args);
		roughnessMap = Resource::Get<Texture2D>(path.roughnessMap, 1, args);
		aoMap = Resource::Get<Texture2D>(path.aoMap, 1, args);
	}

	inline void Free()
	{
		Resource::Release(&diffuseMap);
		Resource::Release(&normalMap);
		Resource::Release(&metallicMap);
		Resource::Release(&roughnessMap);
		Resource::Release(&aoMap);
	}
};

struct PBRMaterialHandle
{
	PBRMaterial material;
	NativeResourceHandle* resourceHandles[5] = {};

	inline void From(PBRMaterialPath& path)
	{
		material.From(path);
		resourceHandles[0] = material.diffuseMap->GetNativeHandle();
		resourceHandles[1] = material.normalMap->GetNativeHandle();
		resourceHandles[2] = material.metallicMap->GetNativeHandle();
		resourceHandles[3] = material.roughnessMap->GetNativeHandle();
		resourceHandles[4] = material.aoMap->GetNativeHandle();
	}

	inline void Free()
	{
		material.Free();
	}
};