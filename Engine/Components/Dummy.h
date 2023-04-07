#pragma once

#include "SubSystemComponent.h"

#include "Core/Pattern/Singleton.h"

NAMESPACE_BEGIN

namespace dummy
{

class SubSystemComponent : public ::soft::SubSystemComponent
{
public:
	virtual void OnComponentAdded() {};
	virtual void OnComponentRemoved() {};
	virtual void OnComponentAddedToScene() {};
	virtual void OnComponentRemovedFromScene() {};
	virtual void SetAsMain() {};
	virtual void SetAsExtra() {};
	virtual void ResolveConflict() {};
	virtual bool IsConflict() {};
};

}

NAMESPACE_END