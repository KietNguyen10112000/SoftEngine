#pragma once

#include "Animator.h"
#include "ANIMATION_TYPE.h"

#include "Resources/AnimModel.h"

NAMESPACE_BEGIN

class AnimatorSkeletalArray : public Animator
{
public:
	friend class AnimLayer;

	ID m_animationSystemId = 0;

	SharedPtr<AnimModel::AnimMeshRenderingBuffer> m_animMeshRenderingBuffer;

	// include both animMesh and static mesh
	Array<Handle<GameObject>> m_meshRendererObjs;

	std::vector<AnimLayer*> m_animLayers;

protected:
	TRACEABLE_FRIEND();
	inline void Trace(Tracer* tracer)
	{
		tracer->Trace(m_meshRendererObjs);
	}

public:
	COMPONENT_CLASS(AnimatorSkeletalArray);

	AnimatorSkeletalArray();
	~AnimatorSkeletalArray();

private:
	void InitAnimLayer(AnimLayer*);

protected:
	virtual void CloneFrom(Serializer* serializer, Serializable* another) override;

public:

	// Inherited via Animator
	virtual void OnComponentAdded() override;

	virtual void OnComponentRemoved() override;

	virtual void OnTransformChanged() override;

	virtual AABox GetGlobalAABB() override;

	virtual ID FindAnimation(const String& name) override;

	virtual void GetAnimationsName(std::vector<String>& output) const override;

	virtual void SetDuration(float sec) override;

	virtual void SetDuration(float sec, ID animationId) override;

	virtual float GetDuration() const override;

	virtual ID GetCurrentAnimationId() const override;

	virtual void Play(float startTransitTime, ID animationId, float startTime, float beginTime, float endTime, float blendTime) override;

	virtual void SetPause(bool pause) override;

	virtual void SetTime(float t) override;

	virtual void Serialize(Serializer* serializer);

	virtual void Deserialize(Serializer* serializer);

	virtual void CleanUp();

	virtual Handle<ClassMetadata> GetMetadata(size_t sign);

	virtual void OnPropertyChanged(const UnknownAddress& var, const Variant& newValue);

	void Update(Scene* scene, float dt);

	virtual void OnDrawDebug() override;

public:
	// create and push the new anim layer to back of process
	template <typename T, typename... Args>
	inline T* NewAnimLayer(Args&&... args)
	{
		auto ret = new T(std::forward<Args>(args)...);
		InitAnimLayer(ret);
		m_animLayers.push_back(ret);
		return ret;
	}

};


NAMESPACE_END