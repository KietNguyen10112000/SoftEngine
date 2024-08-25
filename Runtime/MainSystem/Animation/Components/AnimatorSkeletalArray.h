#pragma once

#include "Animator.h"
#include "ANIMATION_TYPE.h"

#include "Resources/AnimModel.h"

NAMESPACE_BEGIN

class API AnimatorSkeletalArray : public AnimationComponent
{
public:
	friend class AnimLayer;
	friend class AnimCCTBufferLayer;
	friend class AnimationSystem;

	struct RIGID_BODY_PROXY_CONTROL_MODE
	{
		enum MODE
		{
			DISABLED,
			ANIMATOR_TO_RIGID_BODY,
			RIGID_BODY_TO_ANIMATOR
		};
	};

	Resource<AnimModel>	m_model3D;

	ID m_animationSystemId = 0;

	SharedPtr<AnimModel::AnimMeshRenderingBuffer> m_animMeshRenderingBuffer;

	// include both animMesh and static mesh
	Array<Handle<GameObject>> m_meshRendererObjs;

	Array<Handle<AnimLayer>> m_animLayers;
	AnimLayer* m_lastOutput = nullptr;

	bool m_isRunning = true;

	// to defer public results to RenderingSystem
	bool m_isEnableDeferPublicResults = false;
	bool m_padd[6];
	SharedPtr<AnimLayer> m_deferBufferLayer;
	SharedPtr<AnimLayer> m_deferBufferLayer2;

	CharacterController* m_cct = nullptr;
	Vec3 m_cctLockedUpDirection = Vec3::ZERO;
	Vec3 m_cctPrevPosition;
	Quaternion m_cctPrevRotation;
	Mat4 m_cctOffset;
	Mat4 m_rootOffset;
	Mat4 m_parentOffset;

	Array<Handle<GameObject>> m_rigidBodyProxy;
	std::vector<Mat4> m_rigidBodyAnimToPhysOffsets;
	std::vector<Mat4> m_rigidBodyPhysToAnimOffsets;
	RIGID_BODY_PROXY_CONTROL_MODE::MODE m_rigidBodyProxyControlMode = RIGID_BODY_PROXY_CONTROL_MODE::DISABLED;
	GameObject* m_pivotRigidBody = nullptr;
	float m_rigidBodyAABBScale = 1.01f;
	CharacterController* m_rigidBodyProxyCCT = nullptr;

protected:
	TRACEABLE_FRIEND();
	inline void Trace(Tracer* tracer)
	{
		tracer->Trace(m_meshRendererObjs);
		tracer->Trace(m_animLayers);
		tracer->Trace(m_rigidBodyProxy);
	}

public:
	COMPONENT_CLASS(AnimatorSkeletalArray);

	AnimatorSkeletalArray();
	~AnimatorSkeletalArray();

private:
	void InitAnimLayer(AnimLayer*);

	// Inherited via Animator
	void CloneFrom(Serializer* serializer, Serializable* another) override;
	void SerializeToBinary(Serializer* serializer, ByteStream& stream) const override;
	void DeserializeFromBinary(Serializer* serializer, const ByteStream& stream) override;
	void SerializeToJson(Serializer* serializer, json& j) const override;
	void DeserializeFromJson(Serializer* serializer, const json& j) override;
	Handle<ClassMetadata> GetMetadata(size_t sign) override;
	void OnPropertyChanged(const UnknownAddress& var, const Variant& newValue) override;

	void UpdateDataToRenderer(Scene* _scene, const std::vector<Mat4>& globalTransform, const std::vector<AABox>& meshesAABB);

	void ForwardCTTUpdateDataToRenderer(Scene* _scene, AnimLayer* last);
	void CopyDataToForwardCTTUpdateDataToRenderer(AnimLayer* last);
	void SetForwardCCTImpl(CharacterController* cct, const Vec3& lockUpDirection);

	void PublicResultToRigidBodies(Scene* _scene, AnimLayer* last);
	void FetchResultFromRigidBodies();

	void SetEnableDeferPublicResult(bool enable);
	void ResetDeferBufferLayer();

	void SetDiscardObjectTransformForRenderingObjects(bool discard);
	void SwitchBackTo_ANIMATOR_TO_RIGID_BODY_From_RIGID_BODY_TO_ANIMATOR();
	void SetRigidBodiesControlModeImpl(RIGID_BODY_PROXY_CONTROL_MODE::MODE mode);

	void CalculateAnimToPhysOffsets();

public:

	// Inherited via Animator
	virtual void OnComponentAdded() override;

	virtual void OnComponentRemoved() override;

	virtual void OnTransformChanged() override;

	virtual AABox GetGlobalAABB() override;

	void Update(Scene* scene, float dt);

	virtual void OnDrawDebug() override;

	void UpdateDataToRenderer(Scene* _scene, AnimLayer* last);

	void SetRunning(bool running);

	// forward root motion to cct
	void SetForwardCCT(CharacterController* cct, const Vec3& lockUpDirection = Vec3::ZERO);

	void SetRigidBodiesControlMode(RIGID_BODY_PROXY_CONTROL_MODE::MODE mode);

public:
	template <typename T, bool IS_EXTERN = false, typename... Args>
	inline Handle<T> NewAnimLayer(Args&&... args)
	{
		auto ret = mheap::New<T>(std::forward<Args>(args)...);
		InitAnimLayer(ret);

		if constexpr (!IS_EXTERN)
			m_animLayers.Push(ret);

		return ret;
	}

	inline auto* GetLastAnimLayerOutput() const
	{
		return m_lastOutput;
	}
};


NAMESPACE_END