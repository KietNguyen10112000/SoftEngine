#pragma once

#include "Core/Memory/SmartPointers.h"

#include "Resource.h"

#include "MainSystem/Animation/Utils/KeyFrame.h"

NAMESPACE_BEGIN

class AnimMotion;

namespace ResourceUtils
{
	void LoadAnimMotion(String, void*, std::vector<Resource<AnimMotion>>&);
}

class AnimMotion : public ResourceBase
{
private:
	friend class Animation;
	friend class AnimModel;
	friend void ResourceUtils::LoadAnimMotion(String, void*, std::vector<Resource<AnimMotion>>&);

	String m_name;
	float m_tickDuration = 0;
	float m_ticksPerSecond = 0;

	std::vector<KeyFrames> m_channels;

	std::map<String, ID> m_nodeNameEffectedByChannel;

public:
	// *.AnimMotion
	AnimMotion(String path, bool placeholder = false);

private:
	void LoadFromFile(const String& path);

public:
	// the path where motion loaded from (eg: FBX, CDE, ...)
	String GetModelFilePath();

};

NAMESPACE_END