#include "AnimSkeletalArray.h"

NAMESPACE_BEGIN

AnimSkeletalArray::AnimSkeletalArray() : AnimationComponent(ANIMATION_TYPE_SKELETAL_ARRAY)
{
}

void AnimSkeletalArray::OnComponentAdded()
{
}

void AnimSkeletalArray::OnComponentRemoved()
{
}

void AnimSkeletalArray::OnTransformChanged()
{
}

AABox AnimSkeletalArray::GetGlobalAABB()
{
	return AABox();
}

void AnimSkeletalArray::Serialize(Serializer* serializer)
{
}

void AnimSkeletalArray::Deserialize(Serializer* serializer)
{
}

void AnimSkeletalArray::CleanUp()
{
}

Handle<ClassMetadata> AnimSkeletalArray::GetMetadata(size_t sign)
{
	return Handle<ClassMetadata>();
}

void AnimSkeletalArray::OnPropertyChanged(const UnknownAddress& var, const Variant& newValue)
{
}

NAMESPACE_END