#include "AnimLayer.h"

#include "MainSystem/Animation/AnimationSystem.h"
#include "MainSystem/MainSystemTaskPacking.h"

#include "Scene/Scene.h"
#include "Scene/GameObject.h"

NAMESPACE_BEGIN

void AnimLayer::SetEnabled(bool enable)
{
	MAIN_SYSTEM_TASK_IMPL_COMMON_1(GetComponent(),
		AnimationSystem, AsyncTaskRunner, enable,
		{
			self->SetEnabledImpl(enable);
		}
	);
}

NAMESPACE_END