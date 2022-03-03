#pragma once

#include "Controller.h"

#include "Math/Math.h"

class Engine;

class CameraController : public Controller
{
public:
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

	float m_speed = 0;
	float m_rotationSensi = 0.2f;

public:
	CameraController(const Vec3& position, const Vec3& focusPos,
		float fov, float dNear, float dFar, float aspectRatio, float speed = 20);

	~CameraController();

public:
	// Inherited via Controller
	virtual bool Update(SceneQueryContext* context, 
		SceneQueriedNode* node, const Mat4x4& globalTransform, Engine* engine) override;

	virtual void CallMethod(SceneQueryContext* context, SceneQueriedNode* node, const Mat4x4& globalTransform, 
		int methodId, void* args, int argc, Engine* engine) override;

public:
	//inherite this to control
	virtual bool UpdateControl(Engine* engine);

};