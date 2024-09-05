#include "CharacterScript.h"

#include "MainSystem/Rendering/Components/CameraTPP.h"

#include "MainSystem/Scripting/Components/FPPCameraScript.h"

#include "MainSystem/Physics/Components/CharacterController.h"

#include "Common/Actions/ActionExecution.h"
#include "Common/Actions/ActionSequence.h"
#include "Common/Actions/ActionDelay.h"
#include "Common/Actions/ActionCallback.h"

#include "Input/Input.h"

#include "Math/Math.h"

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
	return m_currentBodyState == STATE::TURN || m_nextBodyState == STATE::TURN;
}

bool CharacterScript::IsPlayingMotionAnim()
{
	return 
		m_currentBodyState == STATE::IDLE
		|| m_currentBodyState == STATE::MOVE
		|| m_currentBodyState == STATE::JUMP;
}

void CharacterScript::ControlMotionAnim(float dt)
{
	if (m_currentExpectedMovingDir != Vec3::ZERO)
	{
		PlayAnimTurnFromIdle(dt);
	}

	if (!IsBlockingControlCCT() && m_currentExpectedMovingDir != Vec3::ZERO && m_currentMovingSpeed == 0.0f && m_currentBodyState == STATE::IDLE)
	{
		// player request moving character from idling state

		PlayAnimIdle(dt);
	}

	if (!IsBlockingControlCCT() && m_currentExpectedMovingDir == Vec3::ZERO && m_currentMovingSpeed != 0.0f && m_currentBodyState == STATE::MOVE)
	{
		// player request stoping character from moving state

		PlayAnimRun(dt);
	}
}

void CharacterScript::PlayAnimIdle(float dt)
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
					m_currentBodyState = STATE::MOVE;
				}
			}
		)
	);
	m_nextBodyState = STATE::MOVE;
}

void CharacterScript::PlayAnimRun(float dt)
{
	float transitTime = 0.15f;

	m_character.Transit0->FadeTo(AnimTransitLayer::TransitDirection::BACKWARD, transitTime, m_character.Animations.IdleCarefully, -1, -1);
	m_actionExecution->RunAction(
		ActionInterpolation<float>::New(
			{
				{ 2.0f, 0.0f },
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

	m_nextBodyState = STATE::IDLE;
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

			m_actionExecution->RunAction(
				ActionSequence::New({
					ActionDelay::New(0.15f),
					ActionCallback::New([&]()
						{
							m_currentBodyState = STATE::TURN;
						}
					)
					})
			);

			m_actionExecution->RunAction(
				ActionSequence::New({
					ActionDelay::New(duration * coeff - 0.016f),
					ActionCallback::New([&]()
						{
							m_animator->SetForwardCCT(nullptr);

							m_character.Transit0->FadeTo(AnimTransitLayer::TransitDirection::FORWARD, 0.15f,
								m_character.Animations.IdleCarefully, -1, -1);

							m_nextBodyState = STATE::IDLE;
							m_actionExecution->RunAction(
								ActionSequence::New({
									ActionDelay::New(0.15f),
									ActionCallback::New([&]()
										{
											m_currentBodyState = STATE::IDLE;
										}
									)
								})
							);
						}
					)
					})
			);

			m_nextBodyState = STATE::TURN;
		}
	}
}
