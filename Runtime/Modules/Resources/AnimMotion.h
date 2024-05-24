#pragma once

#include "Core/Memory/SmartPointers.h"

#include "Resource.h"

#include "MainSystem/Animation/Utils/KeyFrame.h"

NAMESPACE_BEGIN

class AnimMotion;

namespace ResourceUtils
{
	void LoadAnimMotion(String, void*, std::vector<Resource<AnimMotion>>&);
	void ExtractAnimMotionData(void*, AnimMotion*);
}

class API AnimMotion : public ResourceBase
{
private:
	friend class Animation;
	friend class AnimModel;
	friend void ResourceUtils::LoadAnimMotion(String, void*, std::vector<Resource<AnimMotion>>&);
	friend void ResourceUtils::ExtractAnimMotionData(void*, AnimMotion*);

	String m_name;
	float m_tickDuration = 0;
	float m_ticksPerSecond = 0;

	std::vector<KeyFrames> m_channels;

	std::vector<String> m_nodeNameEffectedByChannel;

public:
	// *.AnimMotion
	// usage resource::Load<AnimMotion>(<model file path> + "|" + <animation index>)
	//AnimMotion(String path, bool placeHolder = false);
protected:
	virtual int Load(const String& path) override;

private:
	int LoadFromFile(const String& path);

public:
	// the path where motion loaded from (eg: FBX, CDE, ...)
	String GetModelFilePath() const;

};

NAMESPACE_END