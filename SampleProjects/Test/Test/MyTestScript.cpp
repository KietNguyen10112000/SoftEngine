#include "MyTestScript.h"

#include "Input/Input.h"

#include "MainSystem/Animation/AnimLayer/AnimBlendLayer.h"
#include "MainSystem/Animation/AnimLayer/AnimPlayerLayer.h"
#include "MainSystem/Animation/Components/AnimatorSkeletalArray.h"
#include "MainSystem/Animation/Utils/Animation.h"

#include "MainSystem/Rendering/Components/CameraTPP.h"
#include "MainSystem/Scripting/Components/FPPCameraScript.h"

#include "MainSystem/Physics/Components/CharacterController.h"

#include "Common/Actions/ActionExecution.h"
#include "Common/Actions/ActionCallback.h"
#include "Common/Actions/ActionDelay.h"
//#include "Common/Actions/ActionInterpolation.h"

void MyTestScript::OnStart()
{
	auto camera = GetScene()->FindObjectByIndexedName("#camera");
	m_camera = camera->GetComponent<CameraTPP>();
	m_fppCamScript = camera->GetComponent<FPPCameraScript>();

	Base::OnStart();

	m_animator = GetGameObject()->Children()[0]->GetComponent<AnimatorSkeletalArray>();
	m_character.Initialize(m_animator);
	m_character.SrcPlayer1->SetEnable(false);
	m_character.BlendLayer0->SetEnable(false);
	m_character.SrcPlayer0->SetAnimation(m_character.Animations.BreathingIdle, -1, -1);
	m_currentAnimation = m_character.Animations.BreathingIdle.get();

	m_actionExecution = ActionExecution::New({});
	m_cctRotationAction = ActionInterpolation<Quaternion>::New({});

	m_movingSpeed = 0.0f;
}

void MyTestScript::OnUpdate(float dt)
{
	Base::OnUpdate(dt);

	const float idleToRunFadeTime = 0.3f;

	auto& globalTransform = GetGameObject()->GetCommittedGlobalTransform();

	bool needUpdateRotationToCTT = m_actionExecution->Contains(m_cctRotationAction);

	m_actionExecution->Update(dt);

	if (m_prevMotionDir != Vec3::ZERO && m_lastMotionDir != Vec3::ZERO)
	{
		if (AngleBetween(m_prevMotionDir, m_lastMotionDir) > 2 * PI / 3.0f)
		{
			m_lastMotionDir = Vec3::ZERO;
			m_lastMotion = Vec3::ZERO;
		}
	}

	bool moved = m_lastMotionDir.Length2() != 0;
	if (m_lastMotionDir.Length2() == 0 && m_movingSpeed != 0)
	{
		m_controller->Move(m_prevMovedMotionDir * m_movingSpeed * GetScene()->Dt());
		moved = true;
	}

	moved = moved && !IsBlockedMove();

	if (moved)
	{
		if (m_currentAnimation == m_character.Animations.BreathingIdle.get() && m_movingSpeed == 0.0f)
		{
			auto& lastMotionDir = m_lastMotionDir;
			auto angle = AngleBetween(lastMotionDir, globalTransform.Forward());
			if (angle > PI / 4.0f)
			{
				auto& right = globalTransform.Right();
				int dir = right.Dot(lastMotionDir) > 0 ? 1 : 0;

				assert(m_movingSpeed == 0.0f);

				PlayTurnAnimation(dir, angle);
			}
			else
			{
				//m_movingSpeed = 0.5f;

				//std::cout << "===>: " << m_movingSpeed << "\n";

				if (m_fadeStartDelay.get() == nullptr)
				{
					m_fadeStartSpeed = ActionInterpolation<float>::New(
						{
							{ 0.0f,0 },
							{ 15.0f, idleToRunFadeTime * 2.0f }
						},
						[&](const float& v)
						{
							m_movingSpeed = v;
						}
					);

					m_actionExecution->RunAction(
						m_fadeStartSpeed
					);

					m_character.TransitLayer0->FadeTo(
						AnimTransitLayer::TransitDirection::FORWARD,
						idleToRunFadeTime,
						m_character.Animations.FastRun,
						-1, -1
					);

					m_fadeStartDelay = ActionSequence::New(
						{
							ActionDelay::New(idleToRunFadeTime),
							ActionCallback::New(
								[&]()
								{
									m_currentAnimation = m_character.Animations.FastRun.get();
									m_fadeStartDelay = nullptr;
								}
							)
						}
					);
					m_actionExecution->RunAction(m_fadeStartDelay);
				}
				
				//m_currentAnimation = m_character.Animations.FastRun.get();
			}
		}
	}
	//else
	if (m_lastMotionDir.Length2() == 0)
	{
		if (m_currentAnimation == m_character.Animations.FastRun.get() && m_fadeStopDelay.get() == nullptr)
		{
			if (m_fadeStartSpeed && m_actionExecution->Contains(m_fadeStartSpeed))
			{
				m_actionExecution->StopAction(m_fadeStartSpeed);
			}

			m_fadeStartSpeed = nullptr;

			//m_movingSpeed = 0.0f;
			m_actionExecution->RunAction(
				ActionInterpolation<float>::New(
					{
						{ m_movingSpeed,0 },
						{ 0.0f, idleToRunFadeTime }
					},
					[&](const float& v)
					{
						m_movingSpeed = v;
					}
				)
			);

			m_character.TransitLayer0->FadeTo(
				AnimTransitLayer::TransitDirection::BACKWARD,
				idleToRunFadeTime,
				m_character.Animations.BreathingIdle,
				-1, -1
			);

			m_fadeStopDelay = ActionSequence::New(
				{
					ActionDelay::New(idleToRunFadeTime),
					ActionCallback::New(
						[&]()
						{
							m_currentAnimation = m_character.Animations.BreathingIdle.get();
							m_fadeStopDelay = nullptr;
							m_movingSpeed = 0.0f;
						}
					)
				}
			);

			m_actionExecution->RunAction(m_fadeStopDelay);

			m_actionExecution->StopAction(m_cctRotationAction);
		}
	}

	if (needUpdateRotationToCTT)
	{
		m_controller->CCTSetRotation(m_cctRotationAction->GetCurrentValue());
	}

	auto& transform = GetGameObject()->GetCommittedGlobalTransform();

	auto expectForward = m_lastMotion != Vec3::ZERO ? m_lastMotion.Normal() : Vec3::ZERO;
	// camera is not forward to the cct forward direction, rotate cct
	if (moved && m_lastMotion != Vec3::ZERO && m_cctDestRotation != expectForward) //&& !m_viewPoint.Cross(transform.Forward()).IsParallel(transform.Right())
	{
		if (m_cctRotationTurnAction)
		{
			m_actionExecution->StopAction(m_cctRotationTurnAction);
			m_cctRotationTurnAction = nullptr;
		}

		m_actionExecution->StopAction(m_cctRotationAction);
		m_cctRotationAction->Reset();

		auto& rotation = m_controller->CCTGetRotation();
		auto destRot = Mat4::Rotation(rotation) * Mat4::Rotation(Quaternion::RotationFromTo(transform.Forward().Normal(), expectForward));
		auto destUpDirection = destRot.Up().Normal();

		if (destUpDirection != Vec3::UP)
		{
			auto offset = Quaternion::RotationFromTo(destUpDirection, Vec3::UP);
			destRot = destRot * Mat4::Rotation(offset);
		}

		auto angle = std::abs(AngleBetween(transform.Forward(), destRot.Forward()));
		angle = std::isnan(angle) ? 0 : angle;
		const float angleSpeed = 3.0f * PI;

		m_cctRotationAction->AddKeyFrames(
			{
				{ rotation, 0 },
				{ destRot, angle / angleSpeed + 0.1f },
			}
		);
		m_actionExecution->RunAction(m_cctRotationAction);

		m_cctDestRotation = expectForward;
	}

	m_prevMotionDir = m_lastMotionDir;
	if (m_lastMotionDir.Length2() != 0)
	{
		m_prevMovedMotionDir = m_lastMotionDir;
	}

	/*if (Input()->IsKeyPressed('0'))
	{
		PlayTurnAnimation(0, m_testTurnAngle);
	}*/

	//std::cout << "CCTGetRotation: " << json(m_controller->CCTGetRotation()) << '\n';
}

Handle<ClassMetadata> MyTestScript::GetMetadata(size_t sign)
{
	auto meta = Base::GetMetadata(sign + 1);
	meta->SetName(GetClassName());

	meta->AddProperty(Accessor::For("TestTurnAngle", m_testTurnAngle, this));
	meta->AddProperty(Accessor::For("EnableRootMotion", m_enableRootMotion, this));

	return meta;
}

void MyTestScript::PlayTurnAnimation(int direction, float angle)
{
	assert(angle >= 0 && angle <= PI);

	if (m_cctTurnAction.get() == nullptr)
	{
		m_cctTurnAction = ActionSequence::New({});
	}

	if (m_actionExecution->Contains(m_cctTurnAction) || (m_cctRotationTurnAction && m_actionExecution->Contains(m_cctRotationTurnAction)))
	{
		return;
	}

	SharedPtr<Animation>* pAnimation = nullptr;
	float base = 0;
	float animationTimeScale = 1.0f;
	if (angle <= PI / 2.0f)
	{
		pAnimation = direction == 0 ? &m_character.Animations.TurnLeft90FromIdle : &m_character.Animations.TurnRight90FromIdle;
		base = PI / 2.0f;

		animationTimeScale = 0.4f;
	}
	else
	{
		pAnimation = direction == 0 ? &m_character.Animations.TurnLeft180FromIdle : &m_character.Animations.TurnRight180FromIdle;
		base = PI;

		animationTimeScale = 0.4f;
	}

	auto& animation = *pAnimation;
	auto turnFullTime = animation->GetTickDuration() / animation->GetTicksPerSecond();

	std::vector<SharedPtr<ActionBase>> actions;

	auto turnAngleTime = turnFullTime * angle / base;
	turnAngleTime *= animationTimeScale;
	turnFullTime *= animationTimeScale;

	actions.push_back(
		ActionCallback::New(
			[&, turnFullTime]()
			{
				m_animator->SetForwardCCT(m_controller, Vec3::UP);
				m_character.TransitLayer0->FadeTo(AnimTransitLayer::TransitDirection::FORWARD, 0.15f, animation, -1, -1);
				m_character.SrcPlayer0->SetDuration(turnFullTime);
			}
		)
	);

	actions.push_back(ActionDelay::New(turnAngleTime - 0.016f));

	//if (!callback)
	{
		actions.push_back(
			ActionCallback::New(
				[&]()
				{
					m_animator->SetForwardCCT(nullptr);
					m_character.TransitLayer0->FadeTo(AnimTransitLayer::TransitDirection::FORWARD, 0.15f, m_character.Animations.BreathingIdle, -1, -1);
				}
			)
		);

		//{
		//	// destination rotation of animation can be not algined to up direction, let fix it
		//	actions.push_back(
		//		ActionCallback::New(
		//			[&, turnAngleTime, pAnimation]()
		//			{
		//				auto& animation = *pAnimation;
		//				const auto& currentRotation = m_controller->CCTGetRotation();
		//				auto destRotation = Mat4::Rotation(currentRotation);
		//				//auto destUpDirection = destRotation.Transform(Vec3::UP).Normal();
		//				auto destUpDirection = destRotation.Up().Normal();

		//				if (destUpDirection != Vec3::UP)
		//				{
		//					auto offset = Quaternion::RotationFromTo(destUpDirection, Vec3::UP);
		//					auto lastRotation = Mat4::Rotation(currentRotation) * Mat4::Rotation(offset);

		//					m_cctRotationTurnAction = ActionInterpolation<Quaternion>::New(
		//						{
		//							{ currentRotation,0 },
		//							{ lastRotation,0.15f }
		//						},
		//						[&](const Quaternion& v)
		//						{
		//							m_controller->CCTSetRotation(v);
		//						}
		//					);

		//					m_actionExecution->RunAction(m_cctRotationTurnAction);
		//				}
		//			}
		//		)
		//	);
		//}

		actions.push_back(ActionDelay::New(0.1f));
	}

	m_cctTurnAction->StopAllActions();
	m_cctTurnAction->RunActions(actions);
	m_actionExecution->RunAction(m_cctTurnAction);
}

bool MyTestScript::IsBlockedMove()
{
	bool isPlayingTurnAnimation = 
		(m_cctTurnAction && m_actionExecution->Contains(m_cctTurnAction)) 
		|| (m_cctRotationTurnAction && m_actionExecution->Contains(m_cctRotationTurnAction));

	return isPlayingTurnAnimation;
}
