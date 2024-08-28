#pragma once

#include "ComponentInspectorBase.h"

#include "Core/Memory/Memory.h"
#include "Core/Structures/Managed/Array.h"

#include "AnimatorEditorTab.h"

namespace soft
{
	class RigidBody;
	class ClassMetadata;
	class PhysicsShape;
	class GameObject;
	class Joint;
	class AnimatorSkeletalArray;
	class AnimLayer;
}

using namespace soft;

class AnimatorInspector : public ComponentInspectorBase
{
public:
	struct ModelNode
	{
		ModelNode* parent = nullptr;
		std::vector<ModelNode*> children;

		ID nodeIdx = INVALID_ID;
		bool isSelected = false;
		bool isTryingExpand = true;
		bool isOpen = false;
		bool isHovering = false;

		template <typename Fn>
		inline void ForEach(Fn fn)
		{
			fn(this);

			for (auto& child : children)
			{
				child->ForEach(fn);
			}
		}

		inline auto& Children()
		{
			return children;
		}
	};

	ClassMetadata* m_metadata = nullptr;
	AnimatorSkeletalArray* m_animator = nullptr;

	float m_currentAlpha = 0.498f;

	byte m_isEnableTPose = 0;
	bool m_renderPoseBasises = false;

	Handle<AnimatorEditorTab::TPoseLayer> m_tposeLayer;

	ModelNode* m_root = nullptr;
	std::vector<ModelNode*> m_modelNodes;

	TRACEABLE_FRIEND();
	inline void Trace(Tracer* tracer)
	{
		tracer->Trace(m_tposeLayer);
	}

public:
	AnimatorInspector(AnimatorSkeletalArray* animator, ClassMetadata* meta);
	~AnimatorInspector();

	// Inherited via ComponentInspectorBase
	void OnBeginInspecting() override;
	void OnEndInspecting() override;
	void Inspect();

private:
	void BuildModelHierarchy();
	void RenderModelNodeHierarchy(void (*)(ModelNode*, void*), void* userPtr);
	void RenderModelNodeHierarchyImpl(void (*)(ModelNode*, void*), void* userPtr, ModelNode*, void* outRect);

	void CalculateAnimToPhysOffsets();
	void MakeRigidBodySkeleton();
	void DrawDebugSkeletonBasises();
	void DrawDebugSkeleton();

	void RematchRigidBodiesWithTPose();

public:
	static void CopyRigidBoiesData(AnimatorSkeletalArray* dest, AnimatorSkeletalArray* src);

};