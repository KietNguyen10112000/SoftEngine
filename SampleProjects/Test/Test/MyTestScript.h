#pragma once

#include "MainSystem/Scripting/Components/Script.h"
#include "MainSystem/Scripting/Components/TPPCameraScript.h"

#include "Common/Actions/ActionInterpolation.h"
#include "Common/Actions/ActionSequence.h"

#include "TestCharacter.h"

using namespace soft;

namespace soft
{
	class AnimatorSkeletalArray;
	class AnimBlendLayer;
	class Animation;
	class ActionExecution;
}

class MyTestScript : public TPPCameraScript
{
	using Base = TPPCameraScript;
protected:
	SCRIPT_DEFAULT_METHOD(MyTestScript);
	TRACEABLE_FRIEND();
	inline void Trace(Tracer* tracer)
	{
		Base::Trace(tracer);
		tracer->Trace(m_animator);
	}

protected:
	Handle<AnimatorSkeletalArray> m_animator;
	TestCharacter m_character;

	SharedPtr<ActionExecution> m_actionExecution;

	Animation* m_currentAnimation = nullptr;
	SharedPtr<ActionSequence> m_fadeStopDelay = nullptr;
	SharedPtr<ActionSequence> m_fadeStartDelay = nullptr;
	SharedPtr<ActionBase> m_fadeStartSpeed = nullptr;
	Vec3 m_prevMotionDir = Vec3::ZERO;
	Vec3 m_prevMovedMotionDir = Vec3::ZERO;

	SharedPtr<ActionInterpolation<Quaternion>> m_cctRotationAction;
	Vec3 m_cctDestRotation = Vec3::ZERO;

	SharedPtr<ActionSequence> m_cctTurnAction;
	SharedPtr<ActionInterpolation<Quaternion>> m_cctRotationTurnAction;
	float m_testTurnAngle = PI / 2.0f + PI / 3.0f;
	bool m_enableRootMotion = false;

public:
	virtual void OnStart() override;
	virtual void OnUpdate(float dt) override;

	Handle<ClassMetadata> GetMetadata(size_t sign) override;

	// direction: 0-left, 1-right
	void PlayTurnAnimation(int direction, float angle);

	bool IsBlockedMove();

};

