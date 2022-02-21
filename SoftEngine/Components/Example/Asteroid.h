#pragma once

#include <Components/StaticObjectInstancing.h>
#include <Engine/Random.h>

class AsteroidBelt_v1 : public NormalMappingObjectInstancing
{
private:
	Vec3 m_position;
	float m_deltaAngle = 0;

public:
	inline AsteroidBelt_v1(size_t total = 1000, size_t maxRadius = 100);
	inline ~AsteroidBelt_v1();

public:
	inline virtual void Update(Engine* engine) override;

	inline virtual void SetPosition(const Vec3& position) override { m_position = position; };
	inline virtual void SetPosition(float x, float y, float z) override { m_position = { x, y, z }; };

};

inline AsteroidBelt_v1::AsteroidBelt_v1(size_t total, size_t maxRadius) : 
	NormalMappingObjectInstancing(
		//L"D:/KEngine/ResourceFile/model/meteorite/A3/Rock001.fbx",
		L"D:/KEngine/ResourceFile/model/meteorite/rock/rock.obj",
		L"D:/KEngine/ResourceFile/model/meteorite/A3/RockTexture001_diffuse.png",
		L"D:/KEngine/ResourceFile/model/meteorite/A3/RockTexture001_normal.png"//,
		//GetScaleMatrix(0.1f, 0.1f, 0.1f)
	)
{
	std::vector<Mat4x4> transforms;

	float d = 2 * PI / total;

	float curD = 0;

	Mat4x4 temp;
	//temp.SetTranslation(0, 0, 60);

	for (size_t i = 0; i < total; i++)
	{
		temp.SetTranslation(0, Random::Float(-10, 10), maxRadius + Random::Float(-0.3f * maxRadius, maxRadius * 0.2f));
		temp *= GetRotationYMatrix(curD);
		curD += d;

		float r = RandomFloat(1, 2);
		float rA = RandomFloat(0, 2 * PI);
		Vec3 axis = { (float)RandomInt(0, 255), (float)RandomInt(0, 255), (float)RandomInt(0, 255) };

		transforms.push_back(GetScaleMatrix({ r, r, r }) * GetRotationAxisMatrix(axis.Normalize(), rA) * temp);
	}

	m_instanceBuffer = new VertexBuffer(&transforms[0], transforms.size(), sizeof(Mat4x4));
	m_instanceCount = transforms.size();

	m_transform.SetTranslation(0, 0, 5);

	m_position = { 0, 0, 0 };
}

inline AsteroidBelt_v1::~AsteroidBelt_v1()
{
}

inline void AsteroidBelt_v1::Update(Engine* engine)
{
	auto d = PI / 5 * engine->FDeltaTime();
	m_transform *= GetRotationYMatrix(d);

	m_deltaAngle += d;
	m_instancesTransform.SetRotationY(m_deltaAngle).SetPosition(m_position);

}
