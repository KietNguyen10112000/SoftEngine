#include "ScriptSystem2D.h"

#include "Core/TemplateUtils/TemplateUtils.h"

#include "Components2D/Script/Script2D.h"
#include "Components2D/Physics/Physics2D.h"

#include "Objects2D/GameObject2D.h"

#include "Engine/Engine.h"

#include "Components2D/Script/Script2DMeta.h"

#include <iostream>

NAMESPACE_BEGIN

Script2DMeta Script2DMeta::s_instance = {};

ScriptSystem2D::ScriptSystem2D(Scene2D* scene) : SubSystem2D(scene, Script2D::COMPONENT_ID)
{
	if (Script2DMeta::Get().onGUIVtbIdx == INVALID_ID)
	{
		Script2DMeta::Get().onGUIVtbIdx						= VTableIndex<Script2D>(&Script2D::OnGUI);
		Script2DMeta::Get().onCollideVtbIdx					= VTableIndex<Script2D>(&Script2D::OnCollide);
		Script2DMeta::Get().onCollisionEnterVtbIdx			= VTableIndex<Script2D>(&Script2D::OnCollisionEnter);
		Script2DMeta::Get().onCollisionExitVtbIdx			= VTableIndex<Script2D>(&Script2D::OnCollisionExit);
		Script2DMeta::Get().onUpdateVtbIdx					= VTableIndex<Script2D>(&Script2D::OnUpdate);
	}

	auto f = &Script2D::OnUpdate;
	auto p = (void*&)f;

	m_querySession = scene->NewQuerySession();
}

ScriptSystem2D::~ScriptSystem2D()
{
	m_scene->DeleteQuerySession(m_querySession);
}

void ScriptSystem2D::PrevIteration(float dt)
{
}

void ScriptSystem2D::Iteration(float dt)
{
	auto onCollideVtbIdx		= Script2DMeta::Get().onCollideVtbIdx;
	auto onCollisionEnterVtbIdx = Script2DMeta::Get().onCollisionEnterVtbIdx;
	auto onCollisionExitVtbIdx	= Script2DMeta::Get().onCollisionExitVtbIdx;

	for (auto& script : m_onCollide)
	{
		auto physics = script->GetObject()->GetComponentRaw<Physics2D>();
		auto& pairs = physics->CollisionPairs();
		auto& prevPairs = physics->PrevCollisionPairs();
		auto& curPairs = pairs;
		
		if (script->m_overriddenVtbIdx.test(onCollisionEnterVtbIdx)
			|| script->m_overriddenVtbIdx.test(onCollideVtbIdx))
		{
			for (auto& pair : prevPairs)
			{
				if (!pair->result.HasCollision()) continue;

				auto another = pair->GetAnotherOf(physics);

				//if (!another) continue;
				assert(another != nullptr);

				assert(another->m_collisionPairEnterCount == 0);
				another->m_collisionPairEnterCount = 1;
			}

			for (auto& pair : curPairs)
			{
				if (!pair->result.HasCollision()) continue;

				auto another = pair->GetAnotherOf(physics);

				//if (!another) continue;
				assert(another != nullptr);

				if (another->m_collisionPairEnterCount == 1 && (pair->cacheA == 0 || pair->cacheB == 0))
				{
					pair->result.penetration = 0;
					script->OnCollisionExit(another->GetObject(), *pair);
					if (script->GetObject()->IsFloating())
					{
						another->m_collisionPairEnterCount = 0;
						continue;
					}
				}

				if (another->m_collisionPairEnterCount != 1)
				{
					script->OnCollisionEnter(another->GetObject(), *pair);
					if (script->GetObject()->IsFloating())
					{
						another->m_collisionPairEnterCount = 0;
						continue;
					}
				}

				another->m_collisionPairEnterCount = 0;
			}

			for (auto& pair : prevPairs)
			{
				if (!pair->result.HasCollision()) continue;

				auto another = pair->GetAnotherOf(physics);

				//if (!another) continue;
				assert(another != nullptr);

				if (another->m_collisionPairEnterCount == 1)
				{
					script->OnCollisionExit(another->GetObject(), *pair);
					if (script->GetObject()->IsFloating())
					{
						another->m_collisionPairEnterCount = 0;
						continue;
					}
				}

				another->m_collisionPairEnterCount = 0;
			}
		}

		if (script->m_overriddenVtbIdx.test(onCollideVtbIdx))
		{
			for (auto& pair : pairs)
			{
				auto another = pair->GetAnotherOf(physics);

				assert(another != nullptr);

				if (/*another && */ pair->result.HasCollision())
				{
					script->OnCollide(another->GetObject(), *pair);

					if (script->GetObject()->IsFloating())
					{
						break;
					}
				}
			}
		}
	}

	for (auto& obj : m_onUpdate)
	{
		if (obj->IsFloating())
		{
			continue;
		}
		GameObject2D::PostTraversal(
			obj,
			[=](GameObject2D* obj) 
			{
				auto script = obj->GetComponentRaw<Script2D>();
				if (script)
				{
					script->OnUpdate(dt);
				}
			}
		);
	}
}

void ScriptSystem2D::PostIteration(float dt)
{
}

void ScriptSystem2D::AddSubSystemComponent(SubSystemComponent2D* comp)
{
	auto root = comp->GetObject()->GetRoot();
	auto script = (Script2D*)comp;
	script->InitializeClassMetaData();

	//if (IsOverridden<&Script2D::OnGUI, Script2D>(g_originScriptVTable, script))
	if (script->m_overriddenVtbIdx.test(Script2DMeta::Get().onGUIVtbIdx))
	{
		assert(script->m_onGUIId == INVALID_ID && "script added twices");
		script->m_onGUIId = m_onGUI.size();
		m_onGUI.push_back(script);
	}

	//if (IsOverridden<&Script2D::OnCollide, Script2D>(g_originScriptVTable, script))
	if (script->m_overriddenVtbIdx.test(Script2DMeta::Get().onCollideVtbIdx)
		|| script->m_overriddenVtbIdx.test(Script2DMeta::Get().onCollisionEnterVtbIdx)
		|| script->m_overriddenVtbIdx.test(Script2DMeta::Get().onCollisionExitVtbIdx))
	{
		assert(script->m_onCollideId == INVALID_ID && "script added twices");
		script->m_onCollideId = m_onCollide.size();
		m_onCollide.push_back(script);
	}

	//if (IsOverridden<&Script2D::OnUpdate, Script2D>(g_originScriptVTable, script))
	if (script->m_overriddenVtbIdx.test(Script2DMeta::Get().onUpdateVtbIdx))
	{
		if (root->m_subSystemID[COMPONENT_ID] != INVALID_ID)
		{
			assert(0);
		}
		root->m_subSystemID[COMPONENT_ID] = m_onUpdate.size();
		m_onUpdate.push_back(root);
	}
}

void ScriptSystem2D::RemoveSubSystemComponent(SubSystemComponent2D* comp)
{
	auto root = comp->GetObject()->GetRoot();
	auto script = (Script2D*)comp;

	if (script->m_onGUIId != INVALID_ID)
	{
		STD_VECTOR_ROLL_TO_FILL_BLANK(m_onGUI, script->m_onGUIId, back->m_onGUIId);
		script->m_onGUIId = INVALID_ID;
	}

	if (script->m_onCollideId != INVALID_ID)
	{
		STD_VECTOR_ROLL_TO_FILL_BLANK(m_onCollide, script->m_onCollideId, back->m_onCollideId);
		script->m_onCollideId = INVALID_ID;
	}

	if (root->m_subSystemID[COMPONENT_ID] != INVALID_ID)
	{
		STD_VECTOR_ROLL_TO_FILL_BLANK(m_onUpdate, root->m_subSystemID[COMPONENT_ID], back->m_subSystemID[COMPONENT_ID]);
		root->m_subSystemID[COMPONENT_ID] = INVALID_ID;
	}
}

Ray2DQueryInfo* ScriptSystem2D::RayQuery(const Vec2& begin, const Vec2& end, size_t sortLevel)
{
	assert(begin != end);

	auto minX = std::min(begin.x, end.x);
	auto maxX = std::max(begin.x, end.x);
	auto minY = std::min(begin.y, end.y);
	auto maxY = std::max(begin.y, end.y);

	AARect region({ minX, minY }, { maxX - minX, maxY - minY });

	assert(region.IsValid());

	m_rayQueryInfo.Clear();
	m_querySession->Clear();
	m_scene->AABBStaticQueryAARect(region, m_querySession);
	m_scene->AABBDynamicQueryAARect(region, m_querySession);

	Ray2D ray(begin, end);
	for (auto& object : *m_querySession)
	{
		auto comp = object->GetComponentRaw<Physics2D>();
		if (comp)
		{
			auto& collider = comp->Collider();
			auto beginIdx = m_rayQueryInfo.points.size();	
			collider->RayQuery(object->GlobalTransformMatrix(), ray, m_rayQueryInfo);
			auto endIdx = m_rayQueryInfo.points.size();

			if (beginIdx != endIdx)
			{
				m_rayQueryInfo.objectResult.push_back({ object, beginIdx, endIdx });
			}
		}
	}

	if (sortLevel & Ray2DQueryInfo::SORT_MASK::SORT_POINT)
	{
		auto* head = m_rayQueryInfo.points.data();
		for (auto& obj : m_rayQueryInfo.objectResult)
		{
			std::sort(head + obj.idxBegin, head + obj.idxEnd,
				[](const Ray2DQueryResult::PointOnRay& p1, const Ray2DQueryResult::PointOnRay& p2)
				{
					return p1.length2 < p2.length2;
				}
			);

			obj.closestPoint = head + obj.idxBegin;
		}
	}
	else if (sortLevel & Ray2DQueryInfo::SORT_MASK::SORT_OBJECT)
	{
		auto* head = m_rayQueryInfo.points.data();
		for (auto& obj : m_rayQueryInfo.objectResult)
		{
			float minLength2 = INFINITY;
			size_t idx = 0;
			for (size_t i = obj.idxBegin; i < obj.idxEnd; i++)
			{
				auto& p = *(head + i);
				if (minLength2 > p.length2)
				{
					minLength2 = p.length2;
					idx = i;
				}
			}

			assert(minLength2 != INFINITY);
			obj.closestPoint = head + idx;
		}
	}


	if (sortLevel & Ray2DQueryInfo::SORT_MASK::SORT_OBJECT)
	{
		std::sort(m_rayQueryInfo.objectResult.begin(), m_rayQueryInfo.objectResult.end(),
			[](const Ray2DQueryInfo::Object2DResult& p1, const Ray2DQueryInfo::Object2DResult& p2)
			{
				return p1.closestPoint->length2 < p2.closestPoint->length2;
			}
		);
	}

	return m_rayQueryInfo.objectResult.size() != 0 ? &m_rayQueryInfo : nullptr;
}

NAMESPACE_END
