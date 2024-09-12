#pragma once

#include "Character.h"

#include "MainSystem/Scripting/Components/TPPCameraScript.h"

#ifdef IGNORE
#undef IGNORE
#endif
#include "MainSystem/Physics/Query/PhysicsQueryFilterCallback.h"

#include "Common/Actions/ActionExecution.h"
#include "Common/Actions/ActionSequence.h"
#include "Common/Actions/ActionDelay.h"
#include "Common/Actions/ActionCallback.h"

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
			MOVE_SLOW,
			MOVE_FAST,
			JUMP,
			FALL,
			LANDING,
			TURN,
			RAGDOLL
		};
	};

	class FallingSweepFilter : public PhysicsQueryFilterCallback
	{
	public:
		CharacterScript* m_script;
		inline FallingSweepFilter(CharacterScript* script) : m_script(script) {};

		// Inherited via PhysicsQueryFilterCallback
		PhysicsQueryHitType::ENUM PrevFilter(GameObject* obj, PhysicsShape* shape, PhysicsHitFlags& flags) override;
		PhysicsQueryHitType::ENUM PostFilter(GameObject* obj, PhysicsShape* shape, const PhysicsQueryHit& hit) override;
	};

	struct FallingSweepLocalState
	{
		bool stopSerialQuery = false;
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

	SharedPtr<ActionBase> m_switchRunSlowFastActionAnim;

	SharedPtr<ActionBase> m_switchBodyStateAction = nullptr;
	SharedPtr<ActionBase> m_modifyingMovingSpeedAction = nullptr;

	SharedPtr<ActionBase> m_fallingUpdateAction = nullptr;
	SharedPtr<FallingSweepFilter> m_fallingSweepFilter = std::make_shared<FallingSweepFilter>(this);

public:
	void OnStart() override;
	void OnUpdate(float dt) override;

private:
	void ControlMovement(float dt);
	void ControlAnim(float dt);

	bool IsBlockingControlCCT();
	bool IsAnimTransiting();

	// return true if current state of character is idling or moving
	bool IsPlayingMotionAnim();
	void ControlMotionAnim(float dt);

	void TimeoutTransitingBodyState(STATE::ENUM nextState, float timeout);

	template <typename Fn>
	void SetTimeout(float sec, const Fn& callback)
	{
		m_actionExecution->RunAction(
			ActionSequence::New({
				ActionDelay::New(sec),
				ActionCallback::New(callback)
			})
		);
	}

	void ModifyMovingSpeed(
		const ActionInterpolation<float>::KeyFrame& start,
		const ActionInterpolation<float>::KeyFrame& end);

	void PlayAnimIdle(float dt);
	void PlayAnimRun(float dt);
	void PlayAnimSwitchRunSlowFast(float dt);
	void PlayAnimTurnFromIdle(float dt);

	void PlayAnimJump(float dt);
	void FallingUpdate(float dt);
};