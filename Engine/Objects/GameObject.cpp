#include "GameObject.h"

#include "SubSystems/SubSystem.h"

#include "Objects/Scene/Scene.h"

NAMESPACE_BEGIN

void GameObject::InvokeAddRootComponentToSubSystem(SubSystemComponent* comp, const ID COMPONENT_ID)
{
	m_scene->GetSubSystem(COMPONENT_ID)->AddSubSystemComponent(comp, COMPONENT_ID);
}

void GameObject::InvokeRemoveRootComponentFromSubSystem(SubSystemComponent* comp, const ID COMPONENT_ID)
{
	m_scene->GetSubSystem(COMPONENT_ID)->RemoveSubSystemComponent(comp, COMPONENT_ID);
}

bool GameObject::RecalculateAABB()
{
	auto oriLooserAABB = m_aabb;
	GameObject::PostTraversal(this,
		[](GameObject* object)
		{
			bool first = true;
			AABox localAABB;
			object->ForEachSubSystemComponents(
				[&](Handle<SubSystemComponent>& comp)
				{
					if (first)
					{
						localAABB = comp->GetLocalAABB();
						first = false;
						return;
					}

					auto aabb = comp->GetLocalAABB();
					localAABB.Joint(aabb);
				}
			);

			auto& globalAABB = localAABB;
			object->ForEachChildren(
				[&](GameObject* child)
				{
					globalAABB.Joint(child->m_aabb);
				}
			);

			object->m_aabb = globalAABB;

			if (object->IsTransformMat4())
			{
				object->m_aabb.Transform(object->m_transform.GetUpToDateReadHead()->mat);
			}
			else
			{
				object->m_aabb.Transform(object->m_transform.GetUpToDateReadHead()->transform.ToTransformMatrix());
			}
		}
	);

	m_tiedAABB = m_aabb;

	auto jointed = oriLooserAABB.MakeJointed(m_aabb);
	if (jointed == oriLooserAABB)
	{
		m_aabb = oriLooserAABB;
		return false;
	}

	auto iter = m_scene->GetIterationCount();
	auto diter = iter - m_lastChangeAABB;
	m_lastChangeAABB = iter;
	if (diter < m_minChangeAABB)
	{
		m_aabbGap += m_aabbDGap;
	}

	if (diter > m_maxChangeAABB)
	{
		m_aabbGap -= m_aabbDGap;
	}

	m_aabbGap = std::min(std::max(0.0f, m_aabbGap), 10.0f);

	// make looser aabb
	auto dims = m_aabb.GetDimensions();
	//auto l = dims.Length();
	//auto d = dims / l;

	// looser random from 15% to 20% of diagonal
	//l = l * (1 + Random::RangeFloat(5 / 100.0f, 60 / 100.0f));
	//Vec3(l,l,l) * Random::RangeFloat(20 / 100.0f, 50 / 100.0f)
	//auto v = Random::RangeFloat(5, 10);
	m_aabb = AABox(m_aabb.GetCenter(), dims + Vec3(m_aabbGap, m_aabbGap, m_aabbGap));
	return true;
}
NAMESPACE_END