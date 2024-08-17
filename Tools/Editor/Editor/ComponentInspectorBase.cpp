#include "ComponentInspectorBase.h"

#include "Scene/GameObject.h"
#include "MainSystem/Rendering/Components/RenderingComponent.h"

void ComponentInspectorBase::SetOpacityForObject(GameObject* o, float alpha)
{
	if (o->GetComponentRaw<RenderingComponent>())
	{
		o->GetComponentRaw<RenderingComponent>()->SetOpacity(alpha);
		return;
	}

	for (auto& c : o->Children())
	{
		SetOpacityForObject(c, alpha);
	}
}