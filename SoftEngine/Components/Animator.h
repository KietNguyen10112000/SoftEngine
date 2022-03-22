#pragma once

#include <AnimModel3D.h>

#include <Engine/Engine.h>

template<typename _RenderBuffer>
class Animator
{
public:
	float m_time = 0;
	float m_durationRatio = 0;
	float m_durationTime = 0;

	struct TimeIndex
	{
		int s; //scaling
		int r; //rotation
		int t; //translation
	};

	//current scaling, rotation, translation time index
	//size == totalBone
	std::vector<TimeIndex> m_indices;

	//cpu side
	std::vector<Mat4x4> m_nodeLocalTransform;
	std::vector<Mat4x4> m_tempNodeLocalTransform;

	//update to gpu
	std::vector<Mat4x4> m_bones;
	std::vector<Mat4x4*> m_meshsLocalTransform;

	Animation* m_curAnimation = 0;

	AnimModel<_RenderBuffer>* m_model = 0;

public:
	inline Animator() {};
	Animator(AnimModel<_RenderBuffer>* model);
	inline void Free() { Resource::Release(&m_model); };

public:
	void GetScaleMatrix(Mat4x4& out, float time, std::vector<ScalingKeyFrame>& keyframe, int& index);
	void GetRotationMatrix(Mat4x4& out, float time, std::vector<RotaionKeyFrame>& keyframe, int& index);
	void GetTranslationMatrix(Mat4x4& out, float time, std::vector<TranslationKeyFrame>& keyframe, int& index);

	//slow
	template <typename T, typename Ret, Ret(*LerpFunc)(const Ret&, const Ret&, float)>
	Ret GetKey(float time, std::vector<T>& keyframe);

public:
	Mat4x4& Play(Engine* engine, bool* end = 0);
	Mat4x4& Play(Animation* animation, float t, bool* end = 0);
	inline void Render(IRenderer* renderer) { m_model->Render(renderer, m_meshsLocalTransform, m_bones); };

public:
	//return true on success
	bool SetAnimation(const std::string& name);
	bool SetAnimation(int index);

	//in sec
	void SetDuration(float duration);

	//reset the current animation (duration)
	void Reset(bool resetTimer = false);

	bool IsAABBCalculated();
	void ClearAABBPerAnimation();
	void CalculateAABBPerAnimation();

public:
	inline size_t AnimationCount() { return m_model->m_animations.size(); };

	inline size_t Duration() { return m_durationTime; };

	inline auto* CurrentAnimation() { return m_curAnimation; };
};

typedef Animator<_TBNRenderBuffer> TBNAnimator;

template<typename _RenderBuffer>
inline Animator<_RenderBuffer>::Animator(AnimModel<_RenderBuffer>* model)
{
	m_model = Resource::Get<AnimModel<_RenderBuffer>>(model);

	m_nodeLocalTransform.resize(model->m_nodes.size());
	m_tempNodeLocalTransform.resize(model->m_nodes.size());

	for (size_t i = 0; i < m_nodeLocalTransform.size(); i++)
	{
		m_nodeLocalTransform[i] = model->m_nodes[i].localTransform;
	}

	//m_bones.resize(MAX_BONE);

	m_bones.resize(model->m_bonesOffset.size());
	m_meshsLocalTransform.resize(model->m_renderBuf.size());

	m_time = 0;
	SetAnimation(0);

	auto curDurationTime = m_curAnimation->tickDuration / m_curAnimation->ticksPerSecond;
	m_durationTime = curDurationTime;
	SetDuration(curDurationTime);

	for (long long i = 0; i < m_nodeLocalTransform.size(); i++)
	{
		auto& curNode = m_model->m_nodes[i];
		if (curNode.numMeshs != 0)
		{
			//whole static mesh animation
			for (size_t j = 0; j < curNode.numMeshs; j++)
			{
				if(m_model->m_renderBuf[curNode.meshsIndex[j]].rplIndex == 0) 
					m_meshsLocalTransform[curNode.meshsIndex[j]] = &m_tempNodeLocalTransform[i];
			}
		}
	}
}

template<typename _RenderBuffer>
inline void Animator<_RenderBuffer>::GetScaleMatrix(Mat4x4& out, float time, std::vector<ScalingKeyFrame>& keyframe, int& index)
{
	if (keyframe.size() == 0)
	{
		out.SetIdentity();
		return;
	}
	else if (keyframe.size() == 1)
	{
		out.SetScale(keyframe[0].value);
		return;
	}

	if (time > keyframe.back().time)
	{
		index = keyframe.size() - 1;
	}
	else
	{
		while (keyframe[index].time < time)
		{
			index++;
		}
	}

	auto l0 = time - keyframe[index - 1].time;
	auto l1 = keyframe[index].time - keyframe[index - 1].time;
	out.SetScale(
		Lerp(
			keyframe[index - 1].value,
			keyframe[index].value,
			l0 / l1
		)
	);
}

template<typename _RenderBuffer>
inline void Animator<_RenderBuffer>::GetRotationMatrix(Mat4x4& out, float time, std::vector<RotaionKeyFrame>& keyframe, int& index)
{
	if (keyframe.size() == 0)
	{
		out.SetIdentity();
		return;
	}
	else if (keyframe.size() == 1)
	{
		out.SetRotation(keyframe[0].value);
		return;
	}

	if (time > keyframe.back().time)
	{
		index = keyframe.size() - 1;
	}
	else
	{
		while (keyframe[index].time < time)
		{
			index++;
		}
	}

	auto l0 = time - keyframe[index - 1].time;
	auto l1 = keyframe[index].time - keyframe[index - 1].time;
	out.SetRotation(
		SLerp(
			keyframe[index - 1].value,
			keyframe[index].value,
			l0 / l1
		)
	);
}

template<typename _RenderBuffer>
inline void Animator<_RenderBuffer>::GetTranslationMatrix(Mat4x4& out, float time, std::vector<TranslationKeyFrame>& keyframe, int& index)
{
	if (keyframe.size() == 0)
	{
		out.SetIdentity();
		return;
	}
	else if (keyframe.size() == 1)
	{
		out.SetTranslation(keyframe[0].value);
		return;
	}

	if (time > keyframe.back().time)
	{
		index = keyframe.size() - 1;
	}
	else
	{
		while (keyframe[index].time < time)
		{
			index++;
		}
	}

	auto l0 = time - keyframe[index - 1].time;
	auto l1 = keyframe[index].time - keyframe[index - 1].time;
	out.SetTranslation(
		Lerp(
			keyframe[index - 1].value,
			keyframe[index].value,
			l0 / l1
		)
	);
}

template<typename _RenderBuffer>
template<typename T, typename Ret, Ret(*LerpFunc)(const Ret&, const Ret&, float)>
inline Ret Animator<_RenderBuffer>::GetKey(float time, std::vector<T>& keyframe)
{
	size_t i = 0;
	for (auto& key : keyframe)
	{
		if (key.time > time)
		{
			break;
		}
		i++;
	}

	if (i == 0) return keyframe[0].value;

	if (i == keyframe.size()) return keyframe.back().value;

	auto l0 = time - keyframe[i - 1].time;
	auto l1 = keyframe[i].time - keyframe[i - 1].time;

	return LerpFunc(
		keyframe[i - 1].value,
		keyframe[i].value,
		l0 / l1
	);
}


#define __ResetAnimation for (size_t i = 0; i < m_indices.size(); i++)\
{\
	m_indices[i].s = 1;\
	m_indices[i].r = 1;\
	m_indices[i].t = 1;\
}

template<typename _RenderBuffer>
inline Mat4x4& Animator<_RenderBuffer>::Play(Engine* engine, bool* end)
{
	m_time += engine->FDeltaTime();

	auto time = m_time * m_durationRatio * m_curAnimation->ticksPerSecond;

	if (time >= m_curAnimation->tickDuration)
	{
		m_time = 0;
		time = 0;
		for (size_t i = 0; i < m_indices.size(); i++)
		{
			m_indices[i].s = 1; 
			m_indices[i].r = 1; 
			m_indices[i].t = 1; 
		}
		if (end) *end = true;

		//return m_model->m_nodes[0].localTransform;
	}
	else
	{
		if (end) *end = false;
	}

	Mat4x4 scaling, rotation, translation;
	for (long long i = 0; i < m_curAnimation->channels.size(); i++)
	{
		auto& channel = m_curAnimation->channels[i];

		GetScaleMatrix(scaling, time, channel.scaling, m_indices[i].s);
		GetRotationMatrix(rotation, time, channel.rotation, m_indices[i].r);
		GetTranslationMatrix(translation, time, channel.translation, m_indices[i].t);

		assert(channel.nodeId != -1);
		//effects to node
		auto& curNode = m_model->m_nodes[channel.nodeId];
		assert(curNode.parentIndex < channel.nodeId);

		m_nodeLocalTransform[channel.nodeId] = (scaling * rotation) * translation;

	}

	for (long long i = 0; i < m_nodeLocalTransform.size(); i++)
	{
		auto& curNode = m_model->m_nodes[i];
		assert(curNode.parentIndex < i);
		if (curNode.parentIndex != -1)
		{
			m_tempNodeLocalTransform[i] = m_nodeLocalTransform[i] * m_tempNodeLocalTransform[curNode.parentIndex];
		}
		else
		{
			m_tempNodeLocalTransform[i] = m_nodeLocalTransform[i];
		}

		if (curNode.boneIndex != -1)
		{
			//bone
			m_bones[curNode.boneIndex] =
				m_model->m_bonesOffset[curNode.boneIndex] * m_tempNodeLocalTransform[i];
		}
	}

	return m_model->m_nodes[0].localTransform;

}

template<typename _RenderBuffer>
inline Mat4x4& Animator<_RenderBuffer>::Play(Animation* animation, float t, bool* end)
{
	auto time = t;

	Mat4x4 scaling, rotation, translation;
	for (long long i = 0; i < animation->channels.size(); i++)
	{
		auto& channel = animation->channels[i];

		Vec3 sKey = GetKey<ScalingKeyFrame, Vec3, Lerp>(time, channel.scaling);
		scaling.SetScale(sKey);
		Vec4 rKey = GetKey<RotaionKeyFrame, Vec4, SLerp>(time, channel.rotation);
		rotation.SetRotation(rKey);
		Vec3 tKey = GetKey<TranslationKeyFrame, Vec3, Lerp>(time, channel.translation);
		translation.SetTranslation(tKey);

		assert(channel.nodeId != -1);
		//effects to node
		auto& curNode = m_model->m_nodes[channel.nodeId];
		assert(curNode.parentIndex < channel.nodeId);

		m_nodeLocalTransform[channel.nodeId] = (scaling * rotation) * translation;
	}

	for (long long i = 0; i < m_nodeLocalTransform.size(); i++)
	{
		auto& curNode = m_model->m_nodes[i];
		assert(curNode.parentIndex < i);
		if (curNode.parentIndex != -1)
		{
			m_tempNodeLocalTransform[i] = m_nodeLocalTransform[i] * m_tempNodeLocalTransform[curNode.parentIndex];
		}
		else
		{
			m_tempNodeLocalTransform[i] = m_nodeLocalTransform[i];
		}

		if (curNode.boneIndex != -1)
		{
			//bone
			m_bones[curNode.boneIndex] =
				m_model->m_bonesOffset[curNode.boneIndex] * m_tempNodeLocalTransform[i];
		}
	}

	return m_model->m_nodes[0].localTransform;
}

template<typename _RenderBuffer>
inline bool Animator<_RenderBuffer>::SetAnimation(const std::string& name)
{
	int i = 0;
	for (auto& anim : m_model->m_animations)
	{
		if (anim.name == name)
		{
			SetAnimation(i);
			return true;
		}
		i++;
	}
	return false;
}

template<typename _RenderBuffer>
inline bool Animator<_RenderBuffer>::SetAnimation(int index)
{
	if (index < 0 || index >= m_model->m_animations.size()) return false;
	m_curAnimation = &m_model->m_animations[index];
	m_indices.clear();
	m_indices.resize(m_curAnimation->channels.size(), { 1,1,1 });
	return true;
}

template<typename _RenderBuffer>
inline void Animator<_RenderBuffer>::SetDuration(float duration)
{
	if (duration <= 0) return;
	m_time = m_time * (duration / m_durationTime);
	m_durationTime = duration;
	m_durationRatio = (m_curAnimation->tickDuration / m_curAnimation->ticksPerSecond) / m_durationTime;
}

template<typename _RenderBuffer>
inline void Animator<_RenderBuffer>::Reset(bool resetTimer)
{
	m_durationTime = m_curAnimation->tickDuration / m_curAnimation->ticksPerSecond;
	m_durationRatio = 1;

	if (resetTimer)
	{
		m_time = 0;
		for (size_t i = 0; i < m_indices.size(); i++)
		{
			m_indices[i].s = 1;
			m_indices[i].r = 1;
			m_indices[i].t = 1;
		}
	}
}

template<typename _RenderBuffer>
inline bool Animator<_RenderBuffer>::IsAABBCalculated()
{
	for (auto& animation : m_model->m_animations)
	{
		if (animation.meshAABBs.size() != m_model->m_renderBuf.size()) return false;
	}
	return true;
}

template<typename _RenderBuffer>
inline void Animator<_RenderBuffer>::ClearAABBPerAnimation()
{
	for (auto& animation : m_model->m_animations)
	{
		animation.meshAABBs.resize(0);
	}
}

template<typename _RenderBuffer>
inline void Animator<_RenderBuffer>::CalculateAABBPerAnimation()
{
	auto GetAABBKeyFrameTimes = [](Animation* animation) -> std::set<float>
	{
		std::set<float> times;

		for (auto& keyframe : animation->channels)
		{
			for (auto& key : keyframe.scaling)
			{
				times.insert(key.time);
			}

			for (auto& key : keyframe.rotation)
			{
				times.insert(key.time);
			}

			for (auto& key : keyframe.translation)
			{
				times.insert(key.time);
			}
		}

		return times;
	};

	size_t i = 0;
	for (auto& animation : m_model->m_animations)
	{
		auto times = GetAABBKeyFrameTimes(&animation);

		SetAnimation(i);

		for (auto& t : times)
		{
			Play(&animation, t);
			m_model->CalculateAABB(&animation, i, t, m_meshsLocalTransform, m_bones);
		}
		
		i++;
	}

	SetAnimation(0);
	Reset(true);
}