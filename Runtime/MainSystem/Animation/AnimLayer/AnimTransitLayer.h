#pragma once

#include "AnimLayer.h"
#include <queue>

NAMESPACE_BEGIN

class API AnimTransitLayer : public AnimLayer
{
protected:
	SERIALIZABLE_CLASS(AnimTransitLayer);

public:
	struct FadeState
	{
		Animation* animation = nullptr;
		float fadeTime;
		float startTime;
		float endTime;
	};

	std::queue<FadeState> m_queue;

	AnimLayer* m_input = nullptr;

	std::vector<Mat4> m_lastGlobalTransforms;
	std::vector<AABox> m_lastMeshesAABB;

	float m_transitTime = 0;
	float m_transitTotalTime = 0;

protected:
	// Inherited via AnimLayer
	void CloneFrom(Serializer* serializer, Serializable* another) override;

	void SerializeToBinary(Serializer* serializer, ByteStream& stream) const override;

	void DeserializeFromBinary(Serializer* serializer, const ByteStream& stream) override;

	void SerializeToJson(Serializer* serializer, json& j) const override;

	void DeserializeFromJson(Serializer* serializer, const json& j) override;

	Handle<ClassMetadata> GetMetadata(size_t sign) override;

	void OnPropertyChanged(const UnknownAddress& var, const Variant& newValue) override;

	void Initialize() override;
	void PrevRun(float dt) override;
	void Run(float dt) override;

public:
	virtual AnimLayer* GetOutput() override;

	void SetInput(AnimLayer* l);
	void FadeTo(float fadeTime, Animation* animation, float startTime, float endTime);
	void QueuedFadeTo(float fadeTime, Animation* animation, float startTime, float endTime);

	bool IsEndFade() const;
};

NAMESPACE_END