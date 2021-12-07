#include "LightSystem_v2.h"
#include <vector>
#include <iostream>

namespace LightSystem_v2
{

Light::Light(
	uint32_t type,
	float spotAngle, 
	float constantAttenuation, 
	float linearAttenuation, 
	float quadraticAttenuation, 
	float power, 
	Vec3 pos, 
	Vec3 dir, 
	Vec3 color) 
	: type(type), spotAngle(spotAngle), 
	constantAttenuation(constantAttenuation), linearAttenuation(linearAttenuation),
	quadraticAttenuation(quadraticAttenuation), power(power), pos(pos), dir(dir), color(color)
{
}

Light::~Light()
{
}


LightSystem::LightSystem(uint32_t dim)
{
	uint32_t numBlock = dim / MIN_SHADOW_BLOCK_SIZE;

	m_shadowMapSize = numBlock;

//#ifdef _DEBUG
	m_shadowMap.resize(numBlock);
	m_shadowMap.shrink_to_fit();
	for (uint32_t i = 0; i < numBlock; i++)
	{
		m_shadowMap[i].resize(numBlock, UINT32_MAX);
		m_shadowMap[i].shrink_to_fit();
		::memset(&m_shadowMap[i][0], UINT32_MAX, numBlock);
	}
//
//#else
//	m_shadowMap = new uint32_t*[numBlock];
//	for (uint32_t i = 0; i < numBlock; i++)
//	{
//		m_shadowMap[i] = new uint32_t[numBlock];
//		::memset(m_shadowMap[i], UINT32_MAX, numBlock);
//	}
//#endif
}

LightSystem::~LightSystem()
{
}

ShadowAlloc LightSystem::GetFreeSpaceAlloc(Vec2 dim, LightID id)
{
	assert(((int)dim.x % MIN_SHADOW_BLOCK_SIZE) == 0);
	assert(((int)dim.y % MIN_SHADOW_BLOCK_SIZE) == 0);

	uint32_t numBlockX = (uint32_t)dim.x / MIN_SHADOW_BLOCK_SIZE;
	uint32_t numBlockY = (uint32_t)dim.y / MIN_SHADOW_BLOCK_SIZE;

	auto CheckEnoughSpace = [&](uint32_t x, uint32_t y)
	{
		if (x + numBlockX > m_shadowMapSize) return false;
		if (y + numBlockY > m_shadowMapSize) return false;

		for (size_t i = y; i < numBlockY + y; i++)
		{
			for (size_t j = x; j < numBlockX + x; j++)
			{
				if (m_shadowMap[i][j] != UINT32_MAX) return false;
			}
		}

		return true;
	};

	uint32_t index = 0;
	uint32_t indexX, indexY;
	bool flag = false;
	if (m_shadowMapFreeIndex.size() != 0)
	{
		std::vector<Iter> trash;
		auto it = m_shadowMapFreeIndex.begin();
		for(; it != m_shadowMapFreeIndex.end(); it++)
		{
			index = *it;
			uint32_t tindexX = index % m_shadowMapSize;
			uint32_t tindexY = index / m_shadowMapSize;

			if (m_shadowMap[tindexY][tindexX] != UINT32_MAX)
			{
				trash.push_back(it);
				continue;
			}

			if (CheckEnoughSpace(tindexX, tindexY) && !flag)
			{
				flag = true;
				for (size_t i = tindexY; i < numBlockY + tindexY; i++)
				{
					for (size_t j = tindexX; j < numBlockX + tindexX; j++)
					{
						m_shadowMap[i][j] = id;
					}
				}
				trash.push_back(it);
				indexX = tindexX;
				indexY = tindexY;
				//break;
			}
		}

		if (trash.size() != 0)
		{
			for (auto e : trash)
			{
				m_shadowMapFreeIndex.erase(e);
			}	
		}
	}
	
	if (!flag)
	{
		for (size_t i = 0; i < m_shadowMapSize; i++)
		{
			for (size_t j = 0; j < m_shadowMapSize; j++)
			{
				if (CheckEnoughSpace(i, j))
				{
					indexX = i;
					indexY = j;
					flag = true;
					for (size_t k = j; k < numBlockY + j; k++)
					{
						for (size_t l = i; l < numBlockX + i; l++)
						{
							m_shadowMap[k][l] = id;
						}
					}
					break;
				}
			}
			if (flag) break;
		}
	}

	if (!flag)
	{
		std::cout << "Not enough shadow map space.\n";
		return ShadowAlloc();
	}
	
	ShadowAlloc ret;
	ret.pos.x = indexX * MIN_SHADOW_BLOCK_SIZE;
	ret.pos.y = indexY * MIN_SHADOW_BLOCK_SIZE;

	ret.dim = dim;

	//generate free index
	//uint32_t id1[2] = { indexX + 1, indexY };

	uint32_t fid1 = To1DIndex(indexX + numBlockX, indexY, m_shadowMapSize, m_shadowMapSize);
	uint32_t fid2 = To1DIndex(indexX, indexY + numBlockY, m_shadowMapSize, m_shadowMapSize);
	//uint32_t fid3 = To1DIndex(indexX + numBlockX, indexY + numBlockY, m_shadowMapSize, m_shadowMapSize);

	if(indexX + numBlockX != m_shadowMapSize)
		m_shadowMapFreeIndex.insert(fid1);
	if(indexY + numBlockY != m_shadowMapSize)
		m_shadowMapFreeIndex.insert(fid2);
	//m_shadowMapFreeIndex.insert(fid3);

	return ret;
	
}

void LightSystem::FreeSpaceAlloc(const ShadowAlloc& alloc, LightID id)
{
	uint32_t indexX = alloc.pos.x / MIN_SHADOW_BLOCK_SIZE;
	uint32_t indexY = alloc.pos.y / MIN_SHADOW_BLOCK_SIZE;

	uint32_t numBlockX = (uint32_t)alloc.dim.x / MIN_SHADOW_BLOCK_SIZE;
	uint32_t numBlockY = (uint32_t)alloc.dim.y / MIN_SHADOW_BLOCK_SIZE;

	for (size_t i = indexY; i < numBlockY + indexY; i++)
	{
		for (size_t j = indexX; j < numBlockX  + indexX; j++)
		{
			assert(m_shadowMap[i][j] == id);
			m_shadowMap[i][j] = UINT32_MAX;
		}
	}

	uint32_t fid1 = To1DIndex(indexX, indexY, m_shadowMapSize, m_shadowMapSize);
	m_shadowMapFreeIndex.insert(fid1);
}

LightID LightSystem::NewLight(
	uint32_t type,
	float spotAngle, 
	float constantAttenuation, 
	float linearAttenuation, 
	float quadraticAttenuation, 
	float power, 
	Vec3 pos, 
	Vec3 dir, 
	Vec3 color)
{
	if (m_lightFreeSpaces.size() != 0)
	{
		m_lightAllocCounter = *m_lightFreeSpaces.begin();
		m_lightFreeSpaces.erase(m_lightFreeSpaces.begin());
	}

	auto ret = m_lightAllocCounter;

	if (m_lights.size() == ret)
	{
		m_lights.push_back(Light(type, spotAngle, constantAttenuation,
			linearAttenuation, quadraticAttenuation, power, pos, dir, color));
		m_lights.back().index = m_lights.size() - 1;
		m_lightExtraData.push_back(nullptr);
	}
	else
	{
		m_lights[ret] = Light(type, spotAngle, constantAttenuation,
			linearAttenuation, quadraticAttenuation, power, pos, dir, color);
		m_lights[ret].index = ret;
		m_lightExtraData[ret] = nullptr;
	}

	m_lightAllocCounter = m_lights.size();

	return ret;
}

void LightSystem::DeleteLight(LightID id)
{
	m_lightFreeSpaces.insert(id);
	m_lights[id].type = UINT32_MAX;
}

Light& LightSystem::GetLight(LightID id)
{
	return m_lights[id];
}

void LightSystem::UpdateLight(LightID id)
{
	if (m_lights[id].activeIndex == UINT32_MAX) return;

	m_activeLights[m_lights[id].activeIndex] = m_lights[id];
	m_updateActiveLights.insert(m_lights[id].activeIndex);
}

void LightSystem::AddLight(LightID id)
{
	Light& light = GetLight(id);
	
	if (light.activeIndex != UINT32_MAX) return;
	
	if (m_activeLights.size() == MAX_LIGHT) return;
	
	light.activeIndex = m_activeLights.size();
	
	m_updateActiveLights.insert(light.activeIndex);

	m_activeLights.push_back(light);
	
}

void LightSystem::RemoveLight(LightID id)
{
	Light& light = GetLight(id);

	if (light.activeIndex == UINT32_MAX) return;

	Light& target = m_lights[m_activeLights.back().activeIndex];

	target.activeIndex = light.activeIndex;

	m_activeLights[light.activeIndex] = target;
	
	m_activeLights.pop_back();

	m_updateActiveLights.insert(light.activeIndex);
}

void LightSystem::UpdatePointLight(ExtraDataPointLight* p, Light& light, Mat4x4* vpMat, float near, float far)
{
	float asp = 1;

	Mat4x4 view;
	Mat4x4 proj;
	proj.SetPerspectiveFovLH(PI / 2, near, far, asp);

	auto pos = light.pos;
	view.SetLookAtLH(pos, pos + Vec3(1, 0, 0), { 0, 1, 0 });
	vpMat[0] = view * proj;

	view.SetLookAtLH(pos, pos + Vec3(-1, 0, 0), { 0, 1, 0 });
	vpMat[1] = view * proj;

	view.SetLookAtLH(pos, pos + Vec3(0, 1, 0), { 0, 0, -1 });
	vpMat[2] = view * proj;

	view.SetLookAtLH(pos, pos + Vec3(0, -1, 0), { 0, 0, 1 });
	vpMat[3] = view * proj;

	view.SetLookAtLH(pos, pos + Vec3(0, 0, 1), { 0, 1, 0 });
	vpMat[4] = view * proj;

	view.SetLookAtLH(pos, pos + Vec3(0, 0, -1), { 0, 1, 0 });
	vpMat[5] = view * proj;

	p->farPlane = far;
	p->nearPlane = near;
}

void LightSystem::UpdateDirLight(ExtraDataDirLight* p, Light& light, Mat4x4* vpMat, float near, float far, float w, float h)
{
	Mat4x4 view;
	Mat4x4 proj;
	view.SetLookAtLH(light.pos, light.pos + Vec3(light.dir.x, light.dir.y, light.dir.z), { 0,1,0 });
	
	if (light.type == LIGHT_TYPE::DIRECTIONAL_LIGHT)
	{
		proj.SetOrthographicLH(w, h, near, far);
	}
	else
	{
		proj.SetPerspectiveFovLH(PI / 3, 1, 1000, p->alloc.dim.x / p->alloc.dim.y);
	}

	*vpMat = view * proj;
}

void LightSystem::AddShadow(LightID id, Vec2 shadowMapDimentions, float near, float far, float w, float h)
{
	if (m_activeLightShadow.size() - m_activeLightShadowFreeSpaces.size() == MAX_SHADOW_LIGHT) return;

	if (m_lightExtraData[id] == nullptr)
	{
		shadowMapDimentions.y = shadowMapDimentions.x;

		auto& light = m_lights[id];

		LightShadow shadow;

		if (light.type == LIGHT_TYPE::POINT_LIGHT)
		{
			m_lightExtraData[id] = new ExtraDataPointLight();

			auto p = (ExtraDataPointLight*)m_lightExtraData[id];

			//shadow.activeIndex = light.activeIndex;

			for (size_t i = 0; i < 6; i++)
			{
				p->alloc[i] = GetFreeSpaceAlloc(shadowMapDimentions, id);
				shadow.uvOffset[i].x = p->alloc[i].pos.x / (float)(m_shadowMapSize * MIN_SHADOW_BLOCK_SIZE);
				shadow.uvOffset[i].y = p->alloc[i].pos.y / (float)(m_shadowMapSize * MIN_SHADOW_BLOCK_SIZE);
			}

			shadow.texelDim.x = 1 / p->alloc->dim.x;
			shadow.texelDim.y = 1 / p->alloc->dim.y;

			UpdatePointLight(p, light, shadow.viewProj, near, far);

			p->shadowIndex = m_activeLightShadow.size();
			
		}
		else
		{
			m_lightExtraData[id] = new ExtraDataDirLight();
			auto p = (ExtraDataDirLight*)m_lightExtraData[id];
			p->alloc = GetFreeSpaceAlloc(shadowMapDimentions, id);

			UpdateDirLight(p, light, shadow.viewProj, near, far, w, h);

			p->farPlane = far;
			p->nearPlane = near;


			//shadow.activeIndex = light.activeIndex;
			shadow.texelDim.x = 1 / p->alloc.dim.x;
			shadow.texelDim.y = 1 / p->alloc.dim.y;
			shadow.uvOffset[0].x = p->alloc.pos.x / (float)m_shadowMapSize;
			shadow.uvOffset[0].y = p->alloc.pos.y / (float)m_shadowMapSize;

			p->shadowIndex = m_activeLightShadow.size();

		}

		if (m_activeLightShadowFreeSpaces.size() != 0)
		{
			auto index = m_activeLightShadowFreeSpaces.back();
			m_activeLightShadow[index] = shadow;
			m_activeLightShadowFreeSpaces.pop_back();

			m_updateActiveShadowLights.insert(index);

			light.activeShadowIndex = index;
		}
		else
		{
			light.activeShadowIndex = m_activeLightShadow.size();
			m_updateActiveShadowLights.insert(light.activeShadowIndex);
			m_activeLightShadow.push_back(shadow);
		}

		m_updateActiveLights.insert(light.activeIndex);

	}
}

void LightSystem::AddShadow(LightID id, SHADOW_MAP_QUALITY quality, float near, float far, float w, float h)
{
	AddShadow(id, shadowMapQuality[quality], near, far, w, h);
}

void LightSystem::ReallocShadowMap(LightID id, Vec2 newShadowMapDims)
{
	if (m_lightExtraData[id] != nullptr)
	{
		auto& light = m_lights[id];

		if (light.type == LIGHT_TYPE::POINT_LIGHT)
		{
			auto p = (ExtraDataPointLight*)m_lightExtraData[id];

			if (p->alloc->dim.x == newShadowMapDims.x && p->alloc->dim.y == newShadowMapDims.y) return;

			for (size_t i = 0; i < 6; i++)
			{
				FreeSpaceAlloc(p->alloc[i], id);
			}

			for (size_t i = 0; i < 6; i++)
			{
				p->alloc[i] = GetFreeSpaceAlloc(newShadowMapDims, id);
			}

		}
	}
}

void LightSystem::RemoveShadow(LightID id)
{
	if (m_lightExtraData[id] != nullptr)
	{
		Light& light = GetLight(id);
		uint16_t removeIndex;
		//LightSystem::LightShadow removeShadow;

		if (light.type == LIGHT_TYPE::POINT_LIGHT)
		{
			auto p = (ExtraDataPointLight*)m_lightExtraData[id];

			removeIndex = p->shadowIndex;
			//removeShadow = m_activeLightShadow[p->shadowIndex];

			for (size_t i = 0; i < 6; i++)
			{
				FreeSpaceAlloc(p->alloc[i], id);
			}

			delete p;
		}
		else
		{
			auto p = (ExtraDataDirLight*)m_lightExtraData[id];

			removeIndex = p->shadowIndex;
			//removeShadow = m_activeLightShadow[p->shadowIndex];

			FreeSpaceAlloc(p->alloc, id);

			delete p;
		}

		light.activeShadowIndex = UINT32_MAX;

		m_updateActiveLights.insert(light.activeIndex);

		m_lightExtraData[id] = nullptr;
	}
}

Mat4x4* LightSystem::GetShadow(LightID id)
{
	if (m_lightExtraData[id] == nullptr) return nullptr;

	Light& light = GetLight(id);

	return m_activeLightShadow[light.activeShadowIndex].viewProj;
}

void LightSystem::UpdateShadow(LightID id, float near, float far, float w, float h)
{
	if (m_lightExtraData[id] != nullptr)
	{
		Light& light = GetLight(id);
		if (light.type == LIGHT_TYPE::POINT_LIGHT)
		{
			auto p = (ExtraDataPointLight*)m_lightExtraData[id];

			if (near != -1) UpdatePointLight(p, light, m_activeLightShadow[light.activeShadowIndex].viewProj, near, far);

			//m_updateShadowLights.insert(p->alloc.);
			
		}
		else
		{
			auto p = (ExtraDataDirLight*)m_lightExtraData[id];

			if (near != -1) UpdateDirLight(p, light, m_activeLightShadow[light.activeShadowIndex].viewProj, near, far, w, h);
		}
		m_updateActiveShadowLights.insert(light.activeShadowIndex);
	}
}

void LightSystem::Log()
{
	std::cout << "\n============================================================\
==============================================================\n";
	for (size_t i = 0; i < m_shadowMapSize; i++)
	{
		for (size_t j = 0; j < m_shadowMapSize; j++)
		{
			std::cout << (int)(m_shadowMap[i][j] == UINT32_MAX ? -1 : m_shadowMap[i][j]) << '\t';
		}
		std::cout << '\n';
	}
	std::cout << '\n';
	for (auto& e : m_shadowMapFreeIndex)
	{
		std::cout << e << '\t';
	}
	std::cout << '\n';
}

//ShadowID LightSystem::NewShadow(LightID id, const ShadowAlloc*& shadowAlloc, uint32_t numAlloc)
//{
//	Light& light = Get(id);
//
//	if (light.index != UINT32_MAX) return ShadowAllocID_Invalid;
//
//	if (m_shadowLightFreeSpaces.size() != 0)
//	{
//		m_shadowLightAllocCounter = *m_shadowLightFreeSpaces.begin();
//		m_shadowLightFreeSpaces.erase(m_shadowLightFreeSpaces.begin());
//	}
//
//	light.index = m_shadowLightAllocCounter;
//
//	ShadowLightAlloc lAlloc;
//	lAlloc.index = id;
//	lAlloc.shadowAllocCount = numAlloc;
//
//	if (light.type == LIGHT_TYPE::POINT_LIGHT)
//	{
//		lAlloc.shadowAllocCount = 6;
//		const ShadowAlloc& cur = shadowAlloc[0];
//		for (uint32_t i = 0; i < 6; i++)
//		{
//			lAlloc.mem[i] = GetFreeSpaceAlloc(cur.dim);
//		}
//	}
//	else
//	{
//		for (uint32_t i = 0; i < numAlloc; i++)
//		{
//			const ShadowAlloc& cur = shadowAlloc[i];
//			lAlloc.mem[i] = GetFreeSpaceAlloc(cur.dim);
//		}
//	}
//
//	if (m_shadowLights.size() == light.index)
//		m_shadowLights.push_back(lAlloc);
//	else
//		m_shadowLights[light.index] = lAlloc;
//
//	m_shadowLightAllocCounter = m_shadowLights.size();
//
//	return light.index;
//}

//void LightSystem::DeleteShadow(LightID id)
//{
//	Light& light = Get(id);
//	m_shadowLightFreeSpaces.insert(light.index);
//	light.index = UINT32_MAX;
//}
//


//
//void LightSystem::RemoveLight(LightID id)
//{
//	Light& light = Get(id);
//
//	if (light.activeIndex == UINT16_MAX) return;
//
//	m_activeLightFreeSpaces.insert(light.activeIndex);
//
//	m_activeLights[light.activeIndex].type = UINT16_MAX;
//
//	light.activeIndex = UINT16_MAX;
//}



}