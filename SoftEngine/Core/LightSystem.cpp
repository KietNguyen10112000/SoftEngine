#include "ILightSystem.h"
#include <vector>
#include <iostream>

#include <Math/Collision.h>

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
	if (alloc.pos.x == -1 || alloc.pos.y == -1) return;

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

	//TODO: delete extra data
}

//Light& LightSystem::GetLight(LightID id)
//{
//	return m_lights[id];
//}

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

	Light& target = m_lights[m_activeLights.back().index];

	target.activeIndex = light.activeIndex;

	m_activeLights[light.activeIndex] = target;
	
	m_activeLights.pop_back();

	light.activeIndex = UINT32_MAX;

	m_updateActiveLights.insert(target.activeIndex);
}

void LightSystem::UpdatePointLight(ExtraDataPointLight* p, Light& light, Mat4x4* vpMat, float near, float far)
{
	float asp = 1;

	Mat4x4 view;
	Mat4x4 proj;
	proj.SetPerspectiveFovLH(PI / 2 + _FIX_BORDER_POINT_LIGHT_OFFSET, near, far, asp);

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

void LightSystem::UpdateDirLight(ExtraDataDirLight* p, Light& light, Mat4x4* vpMat, float fov, float near, float far, float w, float h)
{
	p->farPlane = far;
	p->nearPlane = near;
	p->fov = fov == -1 ? light.spotAngle : fov;
	p->w = w;
	p->h = h;

	Mat4x4 view;
	Mat4x4 proj;
	view.SetLookAtLH(light.pos, light.pos + Vec3(light.dir.x, light.dir.y, light.dir.z), { 0,1,0 });
	
	if (light.type == LIGHT_TYPE::DIRECTIONAL_LIGHT)
	{
		proj.SetOrthographicLH(w, h, near, far);
	}
	else
	{
		proj.SetPerspectiveFovLH(p->fov, near, far, p->alloc.dim.x / p->alloc.dim.y);
	}

	//auto temp = view * proj;

	*vpMat = view * proj;
}

void LightSystem::DoAddShadow(Light& light, LightShadow& shadow)
{
	if (m_activeLightShadowFreeSpaces.size() != 0)
	{
		auto index = m_activeLightShadowFreeSpaces.back();
		m_activeLightShadow[index] = shadow;
		m_activeLightShadowFreeSpaces.pop_back();

		m_updateActiveShadowLights.insert(index);

		light.activeShadowIndex = index;

		if (light.activeIndex != UINT32_MAX)
			m_activeLights[light.activeIndex].activeShadowIndex = index;
	}
	else
	{
		light.activeShadowIndex = m_activeLightShadow.size();
		m_updateActiveShadowLights.insert(light.activeShadowIndex);
		m_activeLightShadow.push_back(shadow);

		if (light.activeIndex != UINT32_MAX)
			m_activeLights[light.activeIndex].activeShadowIndex = light.activeShadowIndex;
	}

	m_updateActiveLights.insert(light.activeIndex);
}

void LightSystem::AddShadow(LightID id, Vec2 shadowMapDimentions, float fov, float near, float far, float w, float h)
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

			//p->shadowIndex = m_activeLightShadow.size();
			
		}
		else if(light.type == LIGHT_TYPE::DIRECTIONAL_LIGHT || light.type == LIGHT_TYPE::SPOT_LIGHT)
		{
			m_lightExtraData[id] = new ExtraDataDirLight();
			auto p = (ExtraDataDirLight*)m_lightExtraData[id];
			p->alloc = GetFreeSpaceAlloc(shadowMapDimentions, id);

			UpdateDirLight(p, light, shadow.viewProj, fov, near, far, w, h);

			//shadow.activeIndex = light.activeIndex;
			shadow.texelDim.x = 1 / p->alloc.dim.x;
			shadow.texelDim.y = 1 / p->alloc.dim.y;
			shadow.uvOffset[0].x = p->alloc.pos.x / (float)(m_shadowMapSize * MIN_SHADOW_BLOCK_SIZE);
			shadow.uvOffset[0].y = p->alloc.pos.y / (float)(m_shadowMapSize * MIN_SHADOW_BLOCK_SIZE);

			//p->shadowIndex = m_activeLightShadow.size();

		}
		else if (m_lights[id].type == LIGHT_TYPE::CSM_DIRECTIONAL_LIGHT)
		{
			m_lightExtraData[id] = new ExtraDataCSMDirLight();

			auto p = (ExtraDataCSMDirLight*)m_lightExtraData[id];

			for (size_t i = 0; i < ShadowMap_NUM_CASCADE; i++)
			{
				p->alloc[i] = GetFreeSpaceAlloc(shadowMapDimentions, id);
				shadow.uvOffset[i].x = p->alloc[i].pos.x / (float)(m_shadowMapSize * MIN_SHADOW_BLOCK_SIZE);
				shadow.uvOffset[i].y = p->alloc[i].pos.y / (float)(m_shadowMapSize * MIN_SHADOW_BLOCK_SIZE);
			}

			p->alloc[ShadowMap_NUM_CASCADE].pos = { -1,-1 };
			p->alloc[ShadowMap_NUM_CASCADE + 1].pos = { -1,-1 };

			shadow.texelDim.x = 1 / p->alloc->dim.x;
			shadow.texelDim.y = 1 / p->alloc->dim.y;
		}

		DoAddShadow(light, shadow);

	}
}

void LightSystem::AddShadow(LightID id, SHADOW_MAP_QUALITY quality, float fov, float near, float far, float w, float h)
{
	AddShadow(id, shadowMapQuality[quality], fov, near, far, w, h);
}

void LightSystem::AddShadowEx(LightID id, Vec2* shadowMapDimentions, uint32_t numShadow, uint32_t numArgs, void** args)
{
	if (m_lights[id].type == LIGHT_TYPE::CSM_DIRECTIONAL_LIGHT)
	{
		assert(ShadowMap_NUM_CASCADE == numShadow);

		auto& light = m_lights[id];

		LightShadow shadow;

		m_lightExtraData[id] = new ExtraDataCSMDirLight();

		auto p = (ExtraDataCSMDirLight*)m_lightExtraData[id];

		for (size_t i = 0; i < numShadow; i++)
		{
			p->alloc[i] = GetFreeSpaceAlloc(shadowMapDimentions[i], id);
			shadow.uvOffset[i].x = p->alloc[i].pos.x / (float)(m_shadowMapSize * MIN_SHADOW_BLOCK_SIZE);
			shadow.uvOffset[i].y = p->alloc[i].pos.y / (float)(m_shadowMapSize * MIN_SHADOW_BLOCK_SIZE);
		}

		shadow.texelDim.x = 1 / p->alloc->dim.x;
		shadow.texelDim.y = 1 / p->alloc->dim.y;

		DoAddShadow(light, shadow);
	}
	else
	{
		AddShadow(id, shadowMapDimentions[0],
			numArgs != 0 ? *(float*)args[0] : -1.0f,
			numArgs > 0 ? *(float*)args[1] : 0.5f,
			numArgs > 1 ? *(float*)args[2] : 250.0f,
			numArgs > 2 ? *(float*)args[3] : 100.0f,
			numArgs > 3 ? *(float*)args[4] : 100.0f);
	}
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
		//uint16_t removeIndex;
		//LightSystem::LightShadow removeShadow;

		if (light.type == LIGHT_TYPE::CSM_DIRECTIONAL_LIGHT)
		{
			auto p = (ExtraDataCSMDirLight*)m_lightExtraData[id];

			for (size_t i = 0; i < 4; i++)
			{
				FreeSpaceAlloc(p->alloc[i], id);
			}

			delete p;
		}
		else if (light.type == LIGHT_TYPE::POINT_LIGHT)
		{
			auto p = (ExtraDataPointLight*)m_lightExtraData[id];

			//removeIndex = p->shadowIndex;
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

			//removeIndex = p->shadowIndex;
			//removeShadow = m_activeLightShadow[p->shadowIndex];

			FreeSpaceAlloc(p->alloc, id);

			delete p;
		}

		light.activeShadowIndex = UINT32_MAX;

		m_updateActiveLights.insert(id);

		m_lightExtraData[id] = nullptr;

		if (light.activeIndex != UINT32_MAX) m_activeLights[light.activeIndex] = light;
	}
}

Mat4x4* LightSystem::GetShadow(LightID id)
{
	if (m_lightExtraData[id] == nullptr) return nullptr;

	Light& light = GetLight(id);

	return m_activeLightShadow[light.activeShadowIndex].viewProj;
}

void LightSystem::UpdateShadow(LightID id, float fov, float near, float far, float w, float h)
{
	if (m_lightExtraData[id] != nullptr)
	{
		Light& light = GetLight(id);
		if (light.type == LIGHT_TYPE::CSM_DIRECTIONAL_LIGHT)
		{
			auto p = (ExtraDataCSMDirLight*)m_lightExtraData[id];

			p->Update(id, this);

		}
		else if (light.type == LIGHT_TYPE::POINT_LIGHT)
		{
			auto p = (ExtraDataPointLight*)m_lightExtraData[id];

			UpdatePointLight(p, light, m_activeLightShadow[light.activeShadowIndex].viewProj, 
				near == -1 ? p->nearPlane : near, 
				far == -1 ? p->farPlane : far);
			
		}
		else
		{
			auto p = (ExtraDataDirLight*)m_lightExtraData[id];

			UpdateDirLight(p, light, m_activeLightShadow[light.activeShadowIndex].viewProj, 
				fov == -1 ? p->fov : fov,
				near == -1 ? p->nearPlane : near, 
				far == -1 ? p->farPlane : far, 
				w == -1 ? p->w : w,
				h == -1 ? p->h : h);
		}
		m_updateActiveShadowLights.insert(light.activeShadowIndex);
	}
}

void LightSystem::ForceUpdateShadow(LightID id)
{
	if (m_lightExtraData[id] != nullptr)
	{
		Light& light = GetLight(id);
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
			if (m_shadowMap[i][j] == UINT32_MAX)
			{
				std::cout << "_ ";
				continue;
			}
			std::cout << m_shadowMap[i][j] << ' ';
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


void __CSM__SeparateFrustum(const Mat4x4& proj, Vec3* corners, std::vector<std::vector<Vec3>>& frustums, 
	float* thres, float* depthThres, float maxLength = -1)
{
	auto* farPlane = &corners[4];
	auto* nearPlane = &corners[0];

	Vec3 dir[4] = {};

	for (size_t i = 0; i < 4; i++)
	{
		dir[i] = (farPlane[i] - nearPlane[i]).Normalize();
	}

	float totalLength = thres[0] + thres[1] + thres[2] + thres[3];
	float expectLength = ((farPlane[0] + farPlane[2]) / 2.0f - (nearPlane[0] + nearPlane[2]) / 2.0f).Length();
	if (maxLength != -1)
	{
		expectLength = maxLength;
	}

	for (size_t i = 0; i < ShadowMap_NUM_CASCADE; i++)
	{
		thres[i] = (thres[i] / totalLength) * expectLength;
	}

	float count = 0;
	for (size_t i = 0; i < ShadowMap_NUM_CASCADE; i++)
	{
		auto c = count + thres[i];

		std::vector<Vec3> temp = {
			//near plane
			nearPlane[0] + dir[0] * count,
			nearPlane[1] + dir[1] * count,
			nearPlane[2] + dir[2] * count,
			nearPlane[3] + dir[3] * count,

			//far plane
			nearPlane[0] + dir[0] * c,
			nearPlane[1] + dir[1] * c,
			nearPlane[2] + dir[2] * c,
			nearPlane[3] + dir[3] * c
		};

		auto farPlaneCenter = (temp[4] + temp[5] + temp[6] + temp[7]) / 4.0f;
		auto v = Vec4(farPlaneCenter, 1.0f) * proj;
		v = v / v.w;

		depthThres[i] = v.z;

		frustums.push_back(temp);

		count = c;
	}

}

void LightSystem::ExtraDataCSMDirLight::Follow(Mat4x4* view, Mat4x4* projection)
{
	this->view = view;
	this->proj = projection;
	this->prevProj = *projection;

	Frustum::GetFrustumCorners(projCorners, *proj);

	float thres[ShadowMap_NUM_CASCADE] = { 50, 100, 150, 150 };
	Separate(thres, -1);
}

//2nd method
inline void __CSM__GetBoundingSphere(Vec3* corners, Vec3* outCenter, float* outRadius)
{
	BoundingSphere sph;
	BoundingSphere::CreateFromPoints(sph, 8, corners, sizeof(XMFLOAT3));
	*outCenter = *(Vec3*)&sph.Center;
	*outRadius = sph.Radius;
}

void LightSystem::ExtraDataCSMDirLight::Separate(float* newThres, float maxLength)
{
	std::vector<std::vector<Vec3>> frustums;

	float thres[ShadowMap_NUM_CASCADE] = {};
	memcpy(thres, newThres, ShadowMap_NUM_CASCADE * sizeof(float));

	__CSM__SeparateFrustum(*proj, projCorners, frustums, thres, depthThres, maxLength);

	for (size_t i = 0; i < frustums.size(); i++)
	{
		float r = 0;
		Vec3 center;

		__CSM__GetBoundingSphere(&frustums[i][0], &center, &r);

		centers[i] = center;
		radiuses[i] = r;
	}
}

Mat4x4 __CSM__Tie(const Vec3& dir, const Mat4x4& _view, const Vec3& center, float radius, float shadowMapResolution)
{
	auto centerInWolrdSpace = ConvertVector(Vec4(center, 1.0f) * _view);

	const float nearOffset = 1000.f;

	Mat4x4 proj;
	proj.SetOrthographicLH(radius * 2.0f, radius * 2.0f, 0, radius * 2.0f + nearOffset);

	Mat4x4 view;
	auto pos = centerInWolrdSpace - dir.Normal() * (radius + nearOffset);

	view.SetLookAtLH(pos, centerInWolrdSpace, { 0,1,0 });

	auto vp = view * proj;

	const float factor = shadowMapResolution / 2;

	//origin must be on a line on pixel grid
	Vec4 shadowOrigin = Vec4(0.0f, 0.0f, 0.0f, 1.0f);
	shadowOrigin = shadowOrigin * vp;
	shadowOrigin = shadowOrigin * factor;

	Vec4 roundedOrigin = Vec4(std::floor(shadowOrigin.x), std::floor(shadowOrigin.y), 0, 0);
	Vec4 roundOffset = roundedOrigin - shadowOrigin;
	roundOffset = roundOffset / factor;

	proj.SetPosition(roundOffset.x, roundOffset.y, 0);

	return view * proj;
}

void __CSM__TieProjShadow(LightID id, LightSystem* lightSys, LightSystem::ExtraDataCSMDirLight* data)
{
	auto& light = lightSys->GetLight(id);
	auto shadowProj = lightSys->GetShadow(id);
	auto& camProj = *data->proj;
	auto& camView = *data->view;

	for (size_t i = 0; i < ShadowMap_NUM_CASCADE; i++)
	{
		shadowProj[i] = __CSM__Tie(light.dir, camView, data->centers[i], data->radiuses[i], data->alloc[0].dim.x);
	}

	float* p = (float*)&shadowProj[ShadowMap_NUM_CASCADE];
	for (size_t i = 0; i < ShadowMap_NUM_CASCADE; i++)
	{
		p[i] = data->depthThres[i];
	}
}

bool LightSystem::ExtraDataCSMDirLight::Update(LightID id, LightSystem* sys)
{
	if (memcmp(proj, &prevProj, sizeof(Mat4x4)))
	{
		Follow(view, proj);
		return true;
	}

	__CSM__TieProjShadow(id, sys, this);

	return true;
}

//#ifdef DX11_RENDERER
//#include "../DX11/LightSystem_v2.cpp"
//#endif