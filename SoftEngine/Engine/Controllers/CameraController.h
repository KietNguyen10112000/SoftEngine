#pragma once

#include "Controller.h"

#include "Math/Math.h"

class Engine;

class CameraController : public Controller
{
public:
	bool m_change = 0;
	union
	{
		Mat4x4 m_view;
		Mat4x4 m_transform;
	};
	
	Mat4x4 m_proj;

	float m_rotateX = 0;
	float m_rotateY = 0;
	float m_rotateZ = 0;
	Vec3 m_position = {};

public:
	float m_speed = 0;
	float m_rotationSensi = 0.2f;

public:
	CameraController(const Vec3& position, const Vec3& focusPos,
		float fov, float dNear, float dFar, float aspectRatio, float speed = 20, float mouseSensi = 0.2f);

	~CameraController();

public:
	// Inherited via Controller
	

public:
	//inherite this to control
	virtual bool UpdateControl(Engine* engine);


	// Inherited via Controller
	virtual void Update(Engine* engine) override;


	virtual void WriteTo(SceneObject* object) override;


	virtual void WriteToShared(SceneSharedObject* object) override;


};