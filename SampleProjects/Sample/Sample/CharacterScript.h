#pragma once

#include "Character.h"

#include "MainSystem/Scripting/Components/TPPCameraScript.h"

using namespace soft;

namespace soft
{
	class AnimatorSkeletalArray;
	class ActionExecution;
	class Action;
}

class CharacterScript : public TPPCameraScript
{
	using Base = TPPCameraScript;
protected:
	SCRIPT_DEFAULT_METHOD(CharacterScript);

	TRACEABLE_FRIEND();
	void Trace(Tracer* tracer)
	{
		Base::Trace(tracer);
	}

	struct STATE
	{
		enum ENUM
		{
			IDLE,
			MOVE,
			JUMP,
			TURN,
			RAGDOLL
		};
	};

protected:
	SharedPtr<ActionExecution> m_actionExecution;

	AnimatorSkeletalArray* m_animator = nullptr;
	Character m_character;

	float m_slowRunMovingSpeed = 5.0f;
	float m_fastRunMovingSpeed = 10.0f;

	float m_currentMovingSpeed = 0.0f;

	Vec3 m_characterForward = Vec3::ZERO; 
	Vec3 m_characterRight = Vec3::ZERO;
	Vec3 m_characterUp = Vec3::ZERO;

	Vec3 m_currentCameraToCharacterRight = Vec3::ZERO;
	Vec3 m_currentCameraToCharacterForward = Vec3::ZERO;

	Vec3 m_currentExpectedMovingDir = Vec3::ZERO;
	Vec3 m_lastExpectedMovingDir = Vec3::ZERO;

	STATE::ENUM m_currentBodyState = STATE::IDLE; 
	STATE::ENUM m_nextBodyState = STATE::IDLE;

	SharedPtr<Action> m_CCTRotationAction;

public:
	void OnStart() override;
	void OnUpdate(float dt) override;

private:
	void ControlMovement(float dt);
	void ControlAnim(float dt);

	bool IsBlockingControlCCT();

	// return true if current state of character is idling or moving
	bool IsPlayingMotionAnim();
	void ControlMotionAnim(float dt);

	void PlayAnimIdle(float dt);
	void PlayAnimRun(float dt);
	void PlayAnimTurnFromIdle(float dt);
};