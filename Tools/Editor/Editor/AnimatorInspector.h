#pragma once

#include "ComponentInspectorBase.h"

namespace soft
{
	class RigidBody;
	class ClassMetadata;
	class PhysicsShape;
	class GameObject;
	class Joint;
	class AnimatorSkeletalArray;
}

using namespace soft;

class AnimatorInspector : public ComponentInspectorBase
{
public:
	ClassMetadata* m_metadata = nullptr;
	AnimatorSkeletalArray* m_animator = nullptr;

	float m_currentAlpha = 0.498f;

	AnimatorInspector(AnimatorSkeletalArray* animator, ClassMetadata* meta);

	// Inherited via ComponentInspectorBase
	void OnBeginInspecting() override;
	void OnEndInspecting() override;
	void Inspect();

};