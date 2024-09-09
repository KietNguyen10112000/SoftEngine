#include "CharacterScript.h"

#include "MainSystem/Rendering/Components/CameraTPP.h"

#include "MainSystem/Scripting/Components/FPPCameraScript.h"

#include "MainSystem/Physics/Components/CharacterController.h"
#include "MainSystem/Physics/PhysicsSystem.h"

#include "Common/Actions/ActionExecution.h"
#include "Common/Actions/ActionSequence.h"
#include "Common/Actions/ActionDelay.h"
#include "Common/Actions/ActionCallback.h"

#include "Input/Input.h"

#include "Math/Math.h"

#include "AnimationUtils.h"

void CharacterScript::OnStart()
{
	auto camera = GetScene()->FindObjectByIndexedName("#camera");
	m_camera = camera->GetComponent<CameraTPP>();
	m_fppCamScript = camera->GetComponent<FPPCameraScript>();

	m_camera->SetTarget(GetGameObject());

	Base::OnStart();

	m_animator = GetGameObject()->Children()[0]->GetComponent<AnimatorSkeletalArray>();
	m_character.Initialize(m_animator);

	m_actionExecution = ActionExecution::New({});
}

void CharacterScript::OnUpdate(float dt)
{
	UpdateCameraDefault(dt);

	if (m_camera && !m_camera->IsTPPEnabled())
	{
		return;
	}

	m_actionExecution->Update(dt);

	ControlMovement(dt);
	ControlAnim(dt);
}

void CharacterScript::ControlMovement(float dt)
{
	{
		auto temp = Mat4::Rotation(m_controller->CCTGetRotation());
		m_characterForward = temp.Forward().Normal();
		m_characterRight = temp.Right().Normal();
		m_characterUp = temp.Up().Normal();
	}

	auto right = Vec3::UP.Cross(-m_viewPoint.Normal());
	right.Normalize();
	auto forward = right.Cross(Vec3::UP);
	forward.Normalize();

	m_currentCameraToCharacterRight = right;
	m_currentCameraToCharacterForward = forward;

	Vec3 motion = { 0,0,0 };
	if (Input()->IsKeyDown('W'))
	{
		motion += forward;
	}

	if (Input()->IsKeyDown('S'))
	{
		motion -= forward;
	}

	if (Input()->IsKeyDown('A'))
	{
		motion -= right;
	}

	if (Input()->IsKeyDown('D'))
	{
		motion += right;
	}

	if (motion != Vec3::ZERO)
	{
		m_lastExpectedMovingDir = motion.Normalize();
		m_currentExpectedMovingDir = motion;
	}
	else
	{
		m_currentExpectedMovingDir = Vec3::ZERO;
	}

	if (m_currentMovingSpeed != 0.0f && !IsBlockingControlCCT())
	{
		m_controller->Move(m_currentMovingSpeed * dt * m_characterForward);
	}

	if (m_currentMovingSpeed != 0.0f && !IsBlockingControlCCT())
	{
		if (!m_characterForward.Equals(m_lastExpectedMovingDir, 0.0001f))
		{
			auto angle = std::abs(AngleBetween(m_characterForward, m_lastExpectedMovingDir));
			const float angleSpeed = 1.0f * PI;

			auto& cctRotation = m_controller->CCTGetRotation();
			auto cctRotationMat = Mat4::Rotation(cctRotation);
			auto up = cctRotationMat.Up().Normal();
			auto destRotation = Mat4({
				Vec4(up.Cross(m_lastExpectedMovingDir).Normal(), 0.0f),
				Vec4(up, 0.0f),
				Vec4(m_lastExpectedMovingDir, 0.0f),
				Vec4(0,0,0,1.0f)
			});

			m_controller->CCTSetRotation(SLerp(cctRotation, destRotation, std::clamp((angleSpeed * dt) / angle, 0.0f, 1.0f)));
		}
	}
}

void CharacterScript::ControlAnim(float dt)
{
	if (IsPlayingMotionAnim())
	{
		ControlMotionAnim(dt);
	}
}

bool CharacterScript::IsBlockingControlCCT()
{
	return m_currentBodyState == STATE::TURN || m_nextBodyState == STATE::TURN
		|| (m_nextBodyState == STATE::IDLE && m_currentBodyState != STATE::IDLE)
		|| (m_nextBodyState == STATE::JUMP || m_currentBodyState == STATE::JUMP);
}

bool CharacterScript::IsAnimTransiting()
{
	return m_nextBodyState != m_currentBodyState;
}

bool CharacterScript::IsPlayingMotionAnim()
{
	return 
		m_currentBodyState == STATE::IDLE
		|| m_currentBodyState == STATE::MOVE_SLOW
		|| m_currentBodyState == STATE::MOVE_FAST
		|| m_currentBodyState == STATE::JUMP;
}

void CharacterScript::ControlMotionAnim(float dt)
{
	if (m_currentExpectedMovingDir != Vec3::ZERO)
	{
		PlayAnimTurnFromIdle(dt);
	}

	if (!IsBlockingControlCCT() && !IsAnimTransiting() && m_currentExpectedMovingDir != Vec3::ZERO && m_currentMovingSpeed == 0.0f && m_currentBodyState == STATE::IDLE)
	{
		// player request moving character from idling state

		PlayAnimRun(dt);
	}

	if (!IsBlockingControlCCT() /*&& !IsAnimTransiting()*/ && m_currentExpectedMovingDir == Vec3::ZERO && m_currentMovingSpeed != 0.0f 
		&& (m_currentBodyState == STATE::MOVE_SLOW || m_currentBodyState == STATE::MOVE_FAST) 
		&& (m_nextBodyState == STATE::MOVE_SLOW || m_nextBodyState == STATE::MOVE_FAST))
	{
		// player request stoping character from moving state

		PlayAnimIdle(dt);
	}

	if (!IsBlockingControlCCT() && !IsAnimTransiting()
		&& (m_currentBodyState == STATE::MOVE_SLOW || m_currentBodyState == STATE::MOVE_FAST))
	{
		// player request moving character from idling state

		PlayAnimSwitchRunSlowFast(dt);
	}

	if (!IsBlockingControlCCT() && !IsAnimTransiting() && Input()->IsKeyDown(KEYBOARD::SPACE)
		&& (m_currentBodyState == STATE::IDLE || m_currentBodyState == STATE::MOVE_SLOW || m_currentBodyState == STATE::MOVE_FAST))
	{
		// player request jumping character

		PlayAnimJump(dt);
	}
}

void CharacterScript::TimeoutTransitingBodyState(STATE::ENUM nextState, float timeout)
{
	if (m_switchBodyStateAction)
	{
		m_actionExecution->StopAction(m_switchBodyStateAction);
		m_switchBodyStateAction = nullptr;
	}

	m_nextBodyState = nextState;
	m_actionExecution->RunAction(
		m_switchBodyStateAction = ActionSequence::New({
			ActionDelay::New(timeout),
			ActionCallback::New([&, nextState]()
				{
					m_currentBodyState = nextState;
					m_switchBodyStateAction = nullptr;
				}
			)
		})
	);
}

void CharacterScript::PlayAnimIdle(float dt)
{
	if (!IsAnimTransiting())
	{
		float transitTime = 0.15f;
		float instantSpeed = 2.0f;
		if (m_currentBodyState == STATE::MOVE_FAST)
		{
			transitTime = 0.3f;
			instantSpeed = 5.0f;
		}

		m_character.Transit0->FadeTo(AnimTransitLayer::TransitDirection::BACKWARD, transitTime, m_character.Animations.IdleCarefully, -1, -1);

		m_nextBodyState = STATE::IDLE;
		m_actionExecution->RunAction(
			ActionInterpolation<float>::New(
				{
					{ instantSpeed, 0.0f },
					{ 0.0f, transitTime }
				},
				[&](const float& v)
				{
					m_currentMovingSpeed = v;
					if (m_currentMovingSpeed == 0)
					{
						m_currentBodyState = STATE::IDLE;
					}
				}
			)
		);
	}
	else if (m_nextBodyState != STATE::IDLE)
	{
		const float TRANSIT_TIME_FROM_SLOW = 0.15f;
		const float TRANSIT_TIME_FROM_FAST = 0.3f;

		const float SPEED_FROM_SLOW = 2.0f;
		const float SPEED_FROM_FAST = 5.0f;

		auto srcPlayer0 = dynamic_cast<AnimPlayerLayer*>(m_character.Blend01->GetInput(0));

		auto currentBlendFactor = m_character.Blend01->GetBlendFactor();

		float srcTransitTime, destTransitTime, srcInstantSpeed, destInstantSpeed; int dir;
		if (srcPlayer0->GetAnimation() == m_character.Animations.RunSlow && m_currentBodyState == STATE::MOVE_SLOW)
		{
			// blend from 0->1, slow->fast
			srcTransitTime = TRANSIT_TIME_FROM_SLOW;
			destTransitTime = TRANSIT_TIME_FROM_FAST;
			srcInstantSpeed = SPEED_FROM_SLOW;
			destInstantSpeed = SPEED_FROM_FAST;
			dir = 0;
		}
		else if (srcPlayer0->GetAnimation() != m_character.Animations.RunSlow && m_currentBodyState == STATE::MOVE_SLOW)
		{
			// blend from 1->0, slow->fast
			srcTransitTime = TRANSIT_TIME_FROM_SLOW;
			destTransitTime = TRANSIT_TIME_FROM_FAST;
			srcInstantSpeed = SPEED_FROM_SLOW;
			destInstantSpeed = SPEED_FROM_FAST;
			currentBlendFactor = 1 - currentBlendFactor;
			dir = 0;
		}
		else if (srcPlayer0->GetAnimation() == m_character.Animations.RunSlow && m_currentBodyState != STATE::MOVE_SLOW)
		{
			// blend from 1->0, fast->slow
			srcTransitTime = TRANSIT_TIME_FROM_FAST;
			destTransitTime = TRANSIT_TIME_FROM_SLOW;
			srcInstantSpeed = SPEED_FROM_FAST;
			destInstantSpeed = SPEED_FROM_SLOW;
			currentBlendFactor = 1 - currentBlendFactor;
			dir = 1;
		}
		else if (srcPlayer0->GetAnimation() != m_character.Animations.RunSlow && m_currentBodyState != STATE::MOVE_SLOW)
		{
			// blend from 0->1, fast->slow
			srcTransitTime = TRANSIT_TIME_FROM_FAST;
			destTransitTime = TRANSIT_TIME_FROM_SLOW;
			srcInstantSpeed = SPEED_FROM_FAST;
			destInstantSpeed = SPEED_FROM_SLOW;
			dir = 1;
		}

		auto transitTime = Lerp(srcTransitTime, destTransitTime, currentBlendFactor);
		auto instantSpeed = Lerp(srcInstantSpeed, destInstantSpeed, currentBlendFactor);

		m_character.Transit0->FadeTo(AnimTransitLayer::TransitDirection::BACKWARD, transitTime, m_character.Animations.IdleCarefully, -1, -1);

		//AnimationUtils::TransitChangeDirection(m_actionExecution, m_character.Blend01, transitTime, dir);
		m_character.Blend01->StartBlending(
			std::make_shared<FunctionLinear1D>(0, m_character.Blend01->GetBlendFactor()),
			0, transitTime
		);

		m_actionExecution->StopAction(m_switchRunSlowFastActionAnim);
		m_actionExecution->StopAction(m_switchRunSlowFastActionSpeed);

		SetTimeout(transitTime, 
			[&]()
			{
				auto playerLayer = dynamic_cast<AnimPlayerLayer*>(m_character.Blend01->GetMainLayer());
				assert(playerLayer);
				playerLayer->SetAnimation(m_character.Animations.IdleCarefully, -1, -1);
			}
		);

		m_actionExecution->RunAction(
			ActionInterpolation<float>::New(
				{
					{ instantSpeed, 0.0f },
					{ 0.0f, transitTime }
				},
				[&](const float& v)
				{
					m_currentMovingSpeed = v;
				}
			)
		);

		TimeoutTransitingBodyState(STATE::IDLE, transitTime + MIN_FRAMETIME);
	}
}

void CharacterScript::PlayAnimRun(float dt)
{
	float transitTime = 0.15f;

	m_character.Transit0->FadeTo(AnimTransitLayer::TransitDirection::FORWARD, transitTime, m_character.Animations.RunSlow, -1, -1);
	m_actionExecution->RunAction(
		ActionInterpolation<float>::New(
			{
				{ 0.0f, 0.0f },
				{ m_slowRunMovingSpeed, transitTime }
			},
			[&](const float& v)
			{
				m_currentMovingSpeed = v;
				if (m_currentMovingSpeed == m_slowRunMovingSpeed)
				{
					m_currentBodyState = STATE::MOVE_SLOW;
				}
			}
		)
	);
	m_nextBodyState = STATE::MOVE_SLOW;
}

void CharacterScript::PlayAnimSwitchRunSlowFast(float dt)
{
	// mode 1: hold LSHIFT to sprint
	{
		if (Input()->IsKeyDown(KEYBOARD::LSHIFT) && m_currentBodyState == STATE::MOVE_SLOW)
		{
			// switch to fast run
			float transitTime = 0.5f;

			m_switchRunSlowFastActionAnim = AnimationUtils::Transit(m_actionExecution, m_character.Blend01, m_character.Animations.RunSlow, m_character.Animations.RunFast, transitTime);

			TimeoutTransitingBodyState(STATE::MOVE_FAST, transitTime + MIN_FRAMETIME);

			// iteratively increase both's duration to match RunFast duration
			m_switchRunSlowFastActionSpeed = ActionInterpolation<float>::New(
				{
					{ m_currentMovingSpeed, 0.0f },
					{ m_fastRunMovingSpeed, transitTime },
				},
				[&](const float& v)
				{
					m_currentMovingSpeed = v;
				}
			);
			m_actionExecution->RunAction(m_switchRunSlowFastActionSpeed);
		}

		if (!Input()->IsKeyDown(KEYBOARD::LSHIFT) && m_currentBodyState == STATE::MOVE_FAST)
		{
			// switch to slow run
			float transitTime = 0.5f;

			m_switchRunSlowFastActionAnim = AnimationUtils::Transit(m_actionExecution, m_character.Blend01, m_character.Animations.RunFast, m_character.Animations.RunSlow, transitTime);

			TimeoutTransitingBodyState(STATE::MOVE_SLOW, transitTime + MIN_FRAMETIME);

			// iteratively increase both's duration to match RunFast duration
			m_switchRunSlowFastActionSpeed = ActionInterpolation<float>::New(
				{
					{ m_currentMovingSpeed, 0.0f },
					{ m_slowRunMovingSpeed, transitTime },
				},
				[&](const float& v)
				{
					m_currentMovingSpeed = v;
				}
			);
			m_actionExecution->RunAction(m_switchRunSlowFastActionSpeed);
		}
	}
}

void CharacterScript::PlayAnimTurnFromIdle(float dt)
{
	float transitTime = 0.15f;

	auto angle = std::abs(AngleBetween(m_characterForward, m_currentExpectedMovingDir));
	if (angle >= PI / 3.0f && m_nextBodyState == m_currentBodyState && (m_currentBodyState == STATE::IDLE))
	{
		//std::cout << angle << "\n";

		auto& right = Mat4::Rotation(m_controller->CCTGetRotation()).Right();
		int dir = right.Dot(m_currentExpectedMovingDir) > 0 ? 1 : 0;

		SharedPtr<Animation> turnAnimation;
		float duration = 0.0f;
		float coeff = 0.0f;
		if (angle <= PI / 2.0f)
		{
			turnAnimation = dir == 0 ? m_character.Animations.IdleTurnLeft90 : m_character.Animations.IdleTurnRight90;
			duration = 0.5f - transitTime;
			coeff = angle / (PI / 2.0f);
		}
		else
		{
			turnAnimation = dir == 0 ? m_character.Animations.IdleTurnLeft180 : m_character.Animations.IdleTurnRight180;
			duration = 1.0f - transitTime;
			coeff = angle / (PI);
		}

		// play turn animation
		//if ()
		{
			m_character.Transit0->FadeTo(AnimTransitLayer::TransitDirection::FORWARD, transitTime, turnAnimation, -1, -1);

			// let animator control this cct
			m_animator->SetForwardCCT(m_controller, m_characterUp);

			auto srcPlayer = dynamic_cast<AnimPlayerLayer*>(m_character.Transit0->GetInput());
			assert(srcPlayer);

			srcPlayer->SetDuration(duration);

			TimeoutTransitingBodyState(STATE::TURN, 0.15f);

			SetTimeout(duration * coeff - MIN_FRAMETIME,
				[&]()
				{
					m_animator->SetForwardCCT(nullptr);

					m_character.Transit0->FadeTo(AnimTransitLayer::TransitDirection::FORWARD, 0.15f,
						m_character.Animations.IdleCarefully, -1, -1);

					TimeoutTransitingBodyState(STATE::IDLE, 0.15f);
				}
			);
		}
	}
}

void CharacterScript::PlayAnimJump(float dt)
{
	if (m_currentBodyState == STATE::IDLE)
	{
		// jump from idle

		/*m_character.Transit0->FadeTo(AnimTransitLayer::TransitDirection::BACKWARD, 0.15f, m_character.Animations.RunJumpInPlace, -1, -1);

		SetTimeout(0.5f - 0.15f,
			[&]()
			{
				m_character.Transit0->FadeTo(AnimTransitLayer::TransitDirection::FORWARD, 0.6f, m_character.Animations.FallIdle, -1, -1);
			}
		);

		SetTimeout(1.5f,
			[&]()
			{
				m_nextBodyState = STATE::IDLE;
				m_currentBodyState = STATE::IDLE;
				m_character.Transit0->FadeTo(AnimTransitLayer::TransitDirection::FORWARD, 0.15f, m_character.Animations.IdleCarefully, -1, -1);
			}
		);*/
		
		float transitTime = 0.15f;
		m_character.Transit0->FadeTo(AnimTransitLayer::TransitDirection::BACKWARD, transitTime, m_character.Animations.RunJumpInPlace, -1, -1);
		//dynamic_cast<AnimPlayerLayer*>(m_character.Transit0->GetInput())->SetAnimation(m_character.Animations.RunJumpInPlace, -1, -1);
		m_animator->SetForwardCCT(m_controller, m_characterUp);
		m_controller->SetGravity({ 0,0,0 });
		m_controller->CCTApplyVelocity({ 0,10,0 });

		m_nextBodyState = STATE::JUMP;
		m_currentBodyState = STATE::JUMP;

		const float reachedTopTime = 0.5f;
		TimeoutTransitingBodyState(STATE::FALL, reachedTopTime);

		SetTimeout(reachedTopTime,
			[&]()
			{
				m_controller->SetGravity(GetGameObject()->GetScene()->GetPhysicsSystem()->GetGravity());
				m_animator->SetForwardCCT(nullptr);
			}
		);

		SetTimeout(reachedTopTime - 0.15f,
			[&]()
			{
				m_character.Transit0->FadeTo(AnimTransitLayer::TransitDirection::FORWARD, 0.6f, m_character.Animations.FallIdle, -1, -1);
			}
		);

		SetTimeout(3.5f,
			[&]()
			{
				m_nextBodyState = STATE::IDLE;
				m_currentBodyState = STATE::IDLE;
				m_character.Transit0->FadeTo(AnimTransitLayer::TransitDirection::FORWARD, 0.15f, m_character.Animations.IdleCarefully, -1, -1);
			}
		);
	}

	if (m_currentBodyState == STATE::MOVE_SLOW)
	{
		// jump from slow run
	}

	if (m_currentBodyState == STATE::MOVE_FAST)
	{
		// jump from fast run
	}
}
