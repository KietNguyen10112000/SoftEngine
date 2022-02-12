#pragma once

#include <IObject.h>

#include <Engine/Engine.h>

#include <Logger.h>

class FPPCamera : public ICamera
{
protected:
	float m_rotateX = 0;
	float m_rotateY = 0;
	float m_rotateZ = 0;
	Vec3 m_position = {};

	float m_speed = 0;
	float m_rotationSensi = 0.2f;

public:
	inline FPPCamera(const Vec3& position, const Vec3& focusPos, 
		float fov, float dNear, float dFar, float aspectRatio, float speed = 20);
	inline virtual ~FPPCamera() {};

public:
	// Inherited via ICamera
	inline virtual void Update(Engine* engine) override;

	//override this method to control camera in game
	inline virtual void UpdateControl(Engine* engine);

public:
	inline virtual Vec3 GetPosition() override { return m_position; };

	inline virtual std::wstring ToString() override;

public:
	inline auto& Speed() { return m_speed; };
	inline auto& RotationSensitivity() { return m_rotationSensi; };

};

inline FPPCamera::FPPCamera(const Vec3& position, const Vec3& focusPos, 
	float fov, float dNear, float dFar, float aspectRatio, float speed)
{
	m_proj.SetPerspectiveFovLH(fov, dNear, dFar, aspectRatio);

	m_transform.SetLookAtLH(position, focusPos, { 0,1,0 });

	m_mvp = m_transform * m_proj;

	m_position = position;

	m_speed = speed;

	Vec3 direction = (focusPos - position).Normalize();
	m_rotateX = asin(direction.y);
	m_rotateY = atan2(direction.x, direction.z);

	m_frustum.FromProjectionMatrix(m_proj);
}

inline void FPPCamera::Update(Engine* engine)
{
	UpdateControl(engine);

	m_transform.SetIdentity();
	m_transform *= GetRotationYMatrix(m_rotateY);

	auto rwd = m_transform.GetRightwardDir().Normalize();

	m_transform *= GetRotationMatrix(rwd, -m_rotateX);

	if (m_rotateZ != 0)
	{
		auto fwd = m_transform.GetForwardDir().Normalize();
		m_transform *= GetRotationMatrix(fwd, m_rotateZ);
	}

	m_transform *= GetTranslationMatrix(m_position);

	m_mvp = GetInverse(m_transform) * m_proj;
}

inline void FPPCamera::UpdateControl(Engine* engine)
{
	if (engine->Input()->GetKey('W'))
	{
		m_position = m_position + m_transform.GetForwardDir().Normalize() * m_speed * engine->FDeltaTime();
	}
	if (engine->Input()->GetKey('S'))
	{
		m_position = m_position - m_transform.GetForwardDir().Normalize() * m_speed * engine->FDeltaTime();
	}
	if (engine->Input()->GetKey('A'))
	{
		m_position = m_position - m_transform.GetRightwardDir().Normalize() * m_speed * engine->FDeltaTime();
	}
	if (engine->Input()->GetKey('D'))
	{
		m_position = m_position + m_transform.GetRightwardDir().Normalize() * m_speed * engine->FDeltaTime();
	}

	if (engine->Input()->IsMouseMove())
	{
		int* delta = engine->Input()->GetDeltaMousePos();
		m_rotateY += delta[0] * engine->FDeltaTime() * m_rotationSensi;
		m_rotateX -= delta[1] * engine->FDeltaTime() * m_rotationSensi;

		m_rotateX = max(min(m_rotateX, PI / 2.0f), -PI / 2.0f);
	}
}

inline std::wstring FPPCamera::ToString()
{
	return L"View Matrix:\n" + ::ToString(m_transform)
		+ L"\nTransform Matrix:\n" + ::ToString(GetInverse(m_transform))
		+ L"\nPosition:\n" + ::ToString(m_position)
		+ L"\nForward Dir:\n" + ::ToString(m_transform.GetForwardDir().Normalize())
		+ L"\nRotate Y: " + std::to_wstring(m_rotateY)
		+ L"\nRotate X: " + std::to_wstring(m_rotateX);
}
