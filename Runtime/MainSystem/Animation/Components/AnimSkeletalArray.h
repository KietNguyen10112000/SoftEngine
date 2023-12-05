#pragma once

#include "AnimationComponent.h"
#include "ANIMATION_TYPE.h"

NAMESPACE_BEGIN

class AnimSkeletalArray : public AnimationComponent
{
protected:
	TRACEABLE_FRIEND();
	inline void Trace(Tracer* tracer)
	{
	}

public:
	COMPONENT_CLASS(AnimSkeletalArray);

	AnimSkeletalArray();

	// Inherited via AnimationComponent
	virtual void OnComponentAdded() override;

	virtual void OnComponentRemoved() override;

	virtual void OnTransformChanged() override;

	virtual AABox GetGlobalAABB() override;

	virtual void Serialize(Serializer* serializer);

	virtual void Deserialize(Serializer* serializer);

	virtual void CleanUp();

	virtual Handle<ClassMetadata> GetMetadata(size_t sign);

	virtual void OnPropertyChanged(const UnknownAddress& var, const Variant& newValue);

};

NAMESPACE_END