#include "CharacterScript.h"

#include "MainSystem/Rendering/Components/CameraTPP.h"

#include "MainSystem/Scripting/Components/FPPCameraScript.h"

#include "MainSystem/Physics/Components/CharacterController.h"
#include "MainSystem/Physics/Components/CharacterControllerCapsule.h"
#include "MainSystem/Physics/PhysicsSystem.h"
#include "MainSystem/Physics/Query/ActionPhysicsSweep.h"
#include "MainSystem/Physics/Shapes/PhysicsShapeCapsule.h"

#include "Common/Actions/ActionExecution.h"
#include "Common/Actions/ActionSequence.h"
#include "Common/Actions/ActionDelay.h"
#include "Common/Actions/ActionCallback.h"
#include "Common/Actions/ActionRepeatUntil.h"

#include "Input/Input.h"

#include "Math/Math.h"

#include "Graphics//Graphics.h"
#include "Graphics/DebugGraphics.h"

#include "imgui/imgui.h"

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

void CharacterScript::OnGUI()
{
	static Transform start = {};
	static Vec3 distance = Vec3(1,1,1);
	static float height = 1.0f;
	static float radius = 1.0f;
	static Vec3 euler = Vec3::ZERO;

	if (!m_testQueryShape)
	{
		m_testQueryShape = std::make_shared<PhysicsShapeCapsule>(height, radius, m_controller->GetShape(0)->GetFirstMaterial());
		m_testQueryShapeActionExecution = ActionExecution::New({});
	}

	ImGui::Begin("CharacterScript");
	
	if (ImGui::DragFloat3("Position", &start.Position()[0], 0.001f, -FLT_MAX, FLT_MAX))
	{

	}

	if (ImGui::DragFloat3("Rotation", &euler[0], 0.001f, -FLT_MAX, FLT_MAX))
	{
		start.Rotation() = Quaternion(euler);
	}

	if (ImGui::DragFloat3("Distance", &distance[0], 0.001f, -FLT_MAX, FLT_MAX))
	{
		
	}

	ImGui::End();

	auto debugGraphics = Graphics::Get()->GetDebugGraphics();
	if (debugGraphics)
	{
		debugGraphics->DrawCapsule(Capsule(
			start.Rotation().ToMat4().Right().Normal(),
			start.Position(),
			height, radius
		));

		debugGraphics->DrawCapsule(Capsule(
			start.Rotation().ToMat4().Right().Normal(),
			start.Position() + distance,
			height, radius
		));

		debugGraphics->DrawRay(start.Position(), distance);
	}

	m_testQueryShapeActionExecution->Update(0);

	m_testQueryShapeActionExecution->RunAction(
		Physics()->Sweep(
			[&, debugGraphics](const ActionPhysicsSweep* action, const PhysicsSweepResult& result)
			{
				if (!action->HitOrTouchAnything())
				{
					return;
				}

				if (debugGraphics)
				{
					if (result.hasBlock)
					{
						debugGraphics->DrawRay(result.block.position, result.block.normal);
					}
					
					for (auto& touch : result.touches)
					{
						debugGraphics->DrawRay(touch.position, touch.normal);
					}
				}
			},
			m_testQueryShape.get(),
			start,
			distance
		)
	);
}

Handle<ClassMetadata> CharacterScript::GetMetadata(size_t sign)
{
	auto meta = mheap::New<ClassMetadata>(GetClassName(), this);

	

	return meta;
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
			const float angleSpeed = 1.6f * PI;

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

	if (IsBlockingControlCCT() && (m_currentBodyState == STATE::JUMP || m_currentBodyState == STATE::FALL))
	{
		m_fallingMovingMotion = Vec3::ZERO;
		if (motion != Vec3::ZERO)
		{
			m_fallingMovingMotion = 6.0f * motion;
			m_controller->Move(m_fallingMovingMotion * dt);
		}

		if (!m_characterForward.Equals(m_lastExpectedMovingDir, 0.0001f))
		{
			auto angle = std::abs(AngleBetween(m_characterForward, m_lastExpectedMovingDir));
			const float angleSpeed = 2.0f * PI;

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
		|| (m_nextBodyState == STATE::JUMP || m_currentBodyState == STATE::JUMP)
		|| (m_nextBodyState == STATE::FALL || m_currentBodyState == STATE::FALL);
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

void CharacterScript::ModifyMovingSpeed(const ActionInterpolation<float>::KeyFrame& start, const ActionInterpolation<float>::KeyFrame& end)
{
	if (m_modifyingMovingSpeedAction)
	{
		m_actionExecution->StopAction(m_modifyingMovingSpeedAction);
		m_modifyingMovingSpeedAction = nullptr;
	}

	auto endValue = end.value;
	m_actionExecution->RunAction(
		m_modifyingMovingSpeedAction = ActionInterpolation<float>::New(
			{
				start,
				end
			},
			[&, endValue](const float& v)
			{
				m_currentMovingSpeed = v;
				if (m_currentMovingSpeed == endValue)
				{
					m_modifyingMovingSpeedAction = nullptr;
				}
			}
		)
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

		ModifyMovingSpeed({ instantSpeed, 0.0f }, { 0.0f, transitTime });
		TimeoutTransitingBodyState(STATE::IDLE, transitTime);
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

		SetTimeout(transitTime, 
			[&]()
			{
				auto playerLayer = dynamic_cast<AnimPlayerLayer*>(m_character.Blend01->GetMainLayer());
				assert(playerLayer);
				playerLayer->SetAnimation(m_character.Animations.IdleCarefully, -1, -1);
			}
		);

		ModifyMovingSpeed({ instantSpeed, 0.0f }, { 0.0f, transitTime });
		TimeoutTransitingBodyState(STATE::IDLE, transitTime + MIN_FRAMETIME);
	}
}

void CharacterScript::PlayAnimRun(float dt)
{
	float transitTime = 0.15f;

	m_character.Transit0->FadeTo(AnimTransitLayer::TransitDirection::FORWARD, transitTime, m_character.Animations.RunSlow, -1, -1);

	ModifyMovingSpeed({ 0.0f, 0.0f }, { m_slowRunMovingSpeed, transitTime });
	TimeoutTransitingBodyState(STATE::MOVE_SLOW, transitTime);
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

			ModifyMovingSpeed({ m_currentMovingSpeed, 0.0f }, { m_fastRunMovingSpeed, transitTime });
		}

		if (!Input()->IsKeyDown(KEYBOARD::LSHIFT) && m_currentBodyState == STATE::MOVE_FAST)
		{
			// switch to slow run
			float transitTime = 0.5f;

			m_switchRunSlowFastActionAnim = AnimationUtils::Transit(m_actionExecution, m_character.Blend01, m_character.Animations.RunFast, m_character.Animations.RunSlow, transitTime);

			TimeoutTransitingBodyState(STATE::MOVE_SLOW, transitTime + MIN_FRAMETIME);

			ModifyMovingSpeed({ m_currentMovingSpeed, 0.0f }, { m_slowRunMovingSpeed, transitTime });
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
			duration = 0.35f - transitTime;
			coeff = angle / (PI / 2.0f);
		}
		else
		{
			turnAnimation = dir == 0 ? m_character.Animations.IdleTurnLeft180 : m_character.Animations.IdleTurnRight180;
			duration = 0.7f - transitTime;
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
	if (m_fallingUpdateAction)
	{
		return;
	}

	if (m_currentBodyState == STATE::MOVE_SLOW && !IsAnimTransiting())
	{
		// jump from idle

		auto& g = GetScene()->GetPhysicsSystem()->GetGravity();
		
		const float transitTime = 0.15f;
		m_character.Transit0->FadeTo(AnimTransitLayer::TransitDirection::BACKWARD, transitTime, m_character.Animations.RunJumpInPlace, -1, -1);

		m_nextBodyState = STATE::JUMP;
		m_currentBodyState = STATE::JUMP;

		constexpr static float jumpUpVelocityLength = 6.0f;

		SetTimeout(transitTime,
			[&]()
			{
				//m_animator->SetForwardCCT(m_controller, m_characterUp);
				auto srcPlayer = dynamic_cast<AnimPlayerLayer*>(m_character.Transit0->GetInput());
				if (srcPlayer)
				{
					srcPlayer->SetEnableRootMotion(true, true, false);
				}
				
				m_controller->CCTSetAdditionRotationEnabled(true);
				m_controller->CCTApplyVelocity(/*m_characterForward * 6.0f +*/  Vec3(0, jumpUpVelocityLength, 0));
			}
		);

		const float reachedTopTime = 0.377f + transitTime;

		SetTimeout(reachedTopTime,
			[&]()
			{
				auto srcPlayer = dynamic_cast<AnimPlayerLayer*>(m_character.Transit0->GetInput());
				if (srcPlayer)
				{
					srcPlayer->SetEnableRootMotion(true, true, true);
				}
				//m_animator->SetForwardCCT(nullptr);
			}
		);

		SetTimeout(reachedTopTime - 0.15f,
			[&]()
			{
				m_character.Transit0->FadeTo(AnimTransitLayer::TransitDirection::FORWARD, 0.6f, m_character.Animations.FallIdle, -1, -1);
			}
		);

		m_currentMovingSpeed = 0.0f;
		float t0 = jumpUpVelocityLength / g.Length() + transitTime;
		TimeoutTransitingBodyState(STATE::FALL, t0);
		SetTimeout(t0, 
			[&]() 
			{
				m_actionExecution->RunAction(
					m_fallingUpdateAction = ActionRepeatUntil::New(
						ActionCallback::New(
							[&]()
							{
								FallingUpdate(GetScene()->Dt());
							}
						),
						nullptr
					)
				);
			}
		);
	}

	if (m_currentBodyState == STATE::MOVE_FAST && !IsAnimTransiting())
	{
		// jump from slow run
	}

	if (m_currentBodyState == STATE::IDLE)
	{
		// jump from fast run
	}
}

PhysicsQueryHitType::ENUM CharacterScript::FallingSweepFilter::PrevFilter(GameObject* obj, PhysicsShape* shape, PhysicsHitFlags& flags)
{
	if (obj->GetRoot() == m_script->GetGameObject()->GetRoot())
	{
		return PhysicsQueryHitType::ENUM::IGNORE;
	}

	return PhysicsQueryHitType::ENUM::BLOCK;
}

PhysicsQueryHitType::ENUM CharacterScript::FallingSweepFilter::PostFilter(GameObject* obj, PhysicsShape* shape, const PhysicsQueryHit& hit)
{
	return PhysicsQueryHitType::ENUM();
}

void CharacterScript::FallingUpdate(float dt)
{
	std::vector<Vec3> points;
	points.reserve(10);
	const float dtSample = 0.16f;
	//const size_t numSamples = 100;

	float deltaHeight = 0.0f;
	auto g = m_controller->GetGravity();
	auto velocity = m_controller->GetVelocity() + m_fallingMovingMotion;

	Transform start = Transform::FromTransformMatrix(GetGameObject()->GetCommittedGlobalTransform());
	auto startRotationMat = start.Rotation().ToMat4();
	start.Rotation() = Mat4(
		Vec4(startRotationMat.Up(), 0.0f),
		Vec4(startRotationMat.Forward(), 0.0f),
		Vec4(startRotationMat.Right(), 0.0f),
		Vec4(0.0f, 0.0f, 0.0f, 1.0f)
	);

	auto startPosition = start.Position();
	auto pos = start.Position();

	points.push_back(pos);

	while (deltaHeight < 2.0f)
	{
		pos += velocity * dtSample;

		if (velocity.Length() > 0.0001f)
			points.push_back(pos);

		velocity += g * dtSample;

		deltaHeight = startPosition.y - pos.y;

		//assert(deltaHeight >= 0);
	}

	if (points.size() > 1)
	{
		auto physicsSystem = GetScene()->GetPhysicsSystem();
		auto shape = m_controller->GetShape(0);

		auto localState = std::make_shared<FallingSweepLocalState>();

		auto serialId = Physics()->BeginSerialQuery(
			[&, localState](PhysicsSystem* sys, const ActionPhysicsQuery* prev, const ActionPhysicsQuery*) -> bool
			{
				auto sweepPrev = (const ActionPhysicsSweep*)prev;
				if (localState->stopSerialQuery || (sweepPrev && sweepPrev->HitOrTouchAnything()))
				{
					localState->stopSerialQuery = true;
					return false;
				}
				return true;
			}
		);

		for (size_t i = 0; i < points.size() - 1; i++)
		{
			auto& begin = points[i];
			auto& end = points[i + 1];

			Transform transform = start;
			transform.Position() = begin;

			assert(begin != end);

			m_actionExecution->RunAction(
				Physics()->SerialSweep(
					serialId,
					[&](const ActionPhysicsSweep* action, const PhysicsSweepResult& result)
					{
						if (!action->HitOrTouchAnything())
						{
							return;
						}

						LandingUpdate(GetScene()->Dt(), result);
					},
					shape, transform, end - begin, m_fallingSweepFilter
				)
			);
		}

		Physics()->EndSerialQuery(serialId);
	}

	//auto debugGraphics = Graphics::Get()->GetDebugGraphics();
	//if (debugGraphics)
	//{
	//	if (points.size() > 1)
	//	{
	//		auto cct = ((CharacterControllerCapsule*)m_controller);

	//		//std::cout << "Num points: " << points.size() << '\n';
	//		for (size_t i = 0; i < points.size() - 1; i++)
	//		{
	//			auto& begin = points[i];
	//			auto& end = points[i + 1];

	//			if (begin != end)
	//			{
	//				debugGraphics->DrawLineSegment(begin, end);
	//			}

	//			debugGraphics->DrawCapsule(
	//				Capsule(start.Rotation().ToMat4().Right().Normal(), begin, cct->CCTGetHeight(), cct->CCTGetRadius())
	//			);
	//		}
	//	}
	//}
}

void CharacterScript::LandingUpdate(float dt, const PhysicsSweepResult& result)
{
	//std::cout << "Will touch ground\n";

	auto& pos = GetGameObject()->GetCommittedGlobalTransform().Position();
	auto g = m_controller->GetGravity();

	float nearestGroundDistance = INFINITY;
	float distanceFromUpAxis = INFINITY;
	int nearestGroundIdx = -1;

	Vec3 touchPos;
	Vec3 touchNormal;

	auto FnMinDistance = [&](const PhysicsSweepHit& touch)
	{
		auto d = std::abs(touch.position.y - pos.y);//.Length();

		// d must be greater than half body height because we are sweeping by body capsule with origin is in center of capsule
		if (/*d > 1.0f &&*/ d < nearestGroundDistance)
		{
			bool isGround = g.Dot(touch.normal) < -0.00001f;
			if (isGround)
			{
				nearestGroundIdx = &touch - result.touches.data();
				nearestGroundDistance = d;

				distanceFromUpAxis = (Vec3(touch.position.x, 0, touch.position.z) - Vec3(pos.x, 0, pos.z)).Length();
				touchPos = touch.position;
				touchNormal = touch.normal;
			}
		}
	};

	if (result.hasBlock)
	{
		FnMinDistance(result.block);
	}

	for (auto& touch : result.touches)
	{
		FnMinDistance(touch);
	}

	assert(m_currentBodyState == STATE::LANDING || m_currentBodyState == STATE::FALL);

	if (nearestGroundIdx != -1)
	{
		// has ground

		//std::cout << "Has ground\n";

		auto r = ((CharacterControllerCapsule*)m_controller)->CCTGetRadius();

		//std::cout << "distanceFromUpAxis: " << distanceFromUpAxis << "\n";
		auto debugGraphics = Graphics::Get()->GetDebugGraphics();
		if (debugGraphics)
		{
			debugGraphics->DrawRay(touchPos, touchNormal);
		}

		if (nearestGroundDistance <= 2.0f /*&& distanceFromUpAxis <= r * 0.5f*/ && m_currentBodyState == STATE::FALL)
		{
			m_controller->CCTSetAdditionRotationEnabled(false);
			//m_controller->CCTApplyVelocity(m_currentExpectedMovingDir * 2.0f);

			m_nextBodyState = STATE::LANDING;
			m_currentBodyState = STATE::LANDING;

			m_character.Transit0->FadeTo(AnimTransitLayer::TransitDirection::FORWARD, 0.3f, m_character.Animations.FallSoftLanding, -1, -1);

			auto duration = m_character.Animations.FallSoftLanding->GetDuration();
			SetTimeout(duration - 0.45f,
				[&]()
				{
					m_character.Transit0->FadeTo(AnimTransitLayer::TransitDirection::BACKWARD, 0.15f, m_character.Animations.IdleCarefully, -1, -1);
					TimeoutTransitingBodyState(STATE::IDLE, 0.15f);

					m_actionExecution->StopAction(m_fallingUpdateAction);
					m_fallingUpdateAction = nullptr;
				}
			);
		}
	}
	else
	{
		// doesn't have ground

		//std::cout << "Doesn't have ground\n";

		if (m_currentBodyState == STATE::LANDING)
		{
			// switch back to falling

		}
	}
}
