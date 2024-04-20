#include "AnimMotion.h"

NAMESPACE_BEGIN

AnimMotion::AnimMotion(String path, bool placeholder) : ResourceBase(path)
{
	if (placeholder)
	{
		return;
	}

	LoadFromFile(path);
}

void AnimMotion::LoadFromFile(const String& path)
{
}

String AnimMotion::GetModelFilePath()
{
	auto ret = GetPath();

	std::string_view str = ret.c_str();
	auto idx = str.find_last_of('|');

	assert(idx != std::string_view::npos);

	return String(ret.c_str(), idx);
}

NAMESPACE_END