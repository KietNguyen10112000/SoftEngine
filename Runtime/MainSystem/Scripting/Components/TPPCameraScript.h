#pragma once
#include "Script.h"

NAMESPACE_BEGIN

class CharacterController;
class CameraTPP;
class FPPCameraScript;

class API TPPCameraScript : public Script
{
private:
	using Base = Script;
	SCRIPT_DEFAULT_METHOD(TPPCameraScript);

protected:
	TRACEABLE_FRIEND();
	inline void Trace(Tracer* tracer)
	{
		Base::Trace(tracer);

		tracer->Trace(m_camera);
		tracer->Trace(m_fppCamScript);
	}

	CharacterController* controller = nullptr;

	float m_motionY = 0;
	float m_prevPosY1 = 0;
	float m_prevPosY2 = 0;

public:
	Handle<CameraTPP> m_camera;
	Handle<FPPCameraScript> m_fppCamScript;

	Handle<RigidBodyDynamic> m_testBody;

	float m_rotateX = 0;
	//float m_rotateY = 0;
	float m_rotationSensi = 0.25f;
	Vec3 m_viewPoint = Vec3(5, 5, 5);



protected:
	virtual void OnStart() override;

	virtual void OnUpdate(float dt) override;

	Handle<ClassMetadata> GetMetadata(size_t sign) override;

};

//class TestScript2 : public Script
//{
//private:
//	using Base = Script;
//	SCRIPT_DEFAULT_METHOD(TestScript2);
//
//protected:
//	TRACEABLE_FRIEND();
//	inline void Trace(Tracer* tracer)
//	{
//		Base::Trace(tracer);
//	}
//
//	float m_A = 1;
//	float m_a = 0;
//	float m_speed = PI / 3;
//
//protected:
//	//virtual void OnStart() override;
//
//	virtual void OnUpdate(float dt) override;
//
//	Handle<ClassMetadata> GetMetadata(size_t sign) override;
//
//};

NAMESPACE_END