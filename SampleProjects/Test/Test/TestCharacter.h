#pragma once

#include "MainSystem/Animation/AnimLayer/AnimPlayerLayer.h"
#include "MainSystem/Animation/AnimLayer/AnimBlendLayer.h"
#include "MainSystem/Animation/AnimLayer/AnimTransitLayer.h"
#include "MainSystem/Animation/AnimLayer/AnimJointLayer.h"
#include "MainSystem/Animation/AnimLayer/AnimMixLayer.h"
#include "MainSystem/Animation/Components/AnimatorSkeletalArray.h"
#include "MainSystem/Animation/Utils/Animation.h"

namespace soft
{
	class AnimatorSkeletalArray;
	class AnimPlayerLayer;
	class AnimBlendLayer;
	class AnimTransitLayer;
	class AnimMixLayer;
	class AnimJointLayer;
	class Animation;
}

using namespace soft;

struct TestCharacter
{

	struct AnimationID
	{

		constexpr static ID FastRun = 0;
		constexpr static ID BreathingIdle = 1;
		constexpr static ID TurnRight90FromIdle = 2;
		constexpr static ID TurnLeft90FromIdle = 3;
		constexpr static ID TurnRight180FromIdle = 4;
		constexpr static ID TurnLeft180FromIdle = 5;

	};

	struct _Animations
	{

		SharedPtr<Animation> FastRun;
		SharedPtr<Animation> BreathingIdle;
		SharedPtr<Animation> TurnRight90FromIdle;
		SharedPtr<Animation> TurnLeft90FromIdle;
		SharedPtr<Animation> TurnRight180FromIdle;
		SharedPtr<Animation> TurnLeft180FromIdle;

	};

	_Animations Animations;

	struct SrcPlayer0
	{
		constexpr static ID ID = 0;
	};

	AnimPlayerLayer* SrcPlayer0 = nullptr;


	struct TransitLayer0
	{
		constexpr static ID ID = 2;
	};

	AnimTransitLayer* TransitLayer0 = nullptr;


	struct SrcPlayer1
	{
		constexpr static ID ID = 1;
	};

	AnimPlayerLayer* SrcPlayer1 = nullptr;


	struct BlendLayer0
	{
		constexpr static ID ID = 3;
	};

	AnimBlendLayer* BlendLayer0 = nullptr;


	struct MixLayer0
	{
		constexpr static ID ID = 4;
	};

	AnimMixLayer* MixLayer0 = nullptr;


	inline void Initialize(AnimatorSkeletalArray* animator)
	{
		Animations.FastRun = animator->m_model3D->GetAnimation(0);
		Animations.BreathingIdle = animator->m_model3D->GetAnimation(1);
		Animations.TurnRight90FromIdle = animator->m_model3D->GetAnimation(2);
		Animations.TurnLeft90FromIdle = animator->m_model3D->GetAnimation(3);
		Animations.TurnRight180FromIdle = animator->m_model3D->GetAnimation(4);
		Animations.TurnLeft180FromIdle = animator->m_model3D->GetAnimation(5);


		SrcPlayer0 = (AnimPlayerLayer*)(animator->m_animLayers[0].Get());

		TransitLayer0 = (AnimTransitLayer*)(animator->m_animLayers[2].Get());

		SrcPlayer1 = (AnimPlayerLayer*)(animator->m_animLayers[1].Get());

		BlendLayer0 = (AnimBlendLayer*)(animator->m_animLayers[3].Get());

		MixLayer0 = (AnimMixLayer*)(animator->m_animLayers[4].Get());

	}

};
