#pragma once

#include "AnimLayer.h"

NAMESPACE_BEGIN

class AnimBlendLayer : public AnimLayer
{
protected:
	SERIALIZABLE_CLASS(AnimBlendLayer);

	// Inherited via AnimLayer
	void Serialize(Serializer* serializer) override;

	void Deserialize(Serializer* serializer) override;

	void CleanUp() override;

	Handle<ClassMetadata> GetMetadata(size_t sign) override;

	void OnPropertyChanged(const UnknownAddress& var, const Variant& newValue) override;

	void Run(float dt) override;

public:
	void SetInput(AnimLayer* l1, AnimLayer* l2);

};

NAMESPACE_END