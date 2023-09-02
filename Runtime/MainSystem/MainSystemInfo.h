#pragma once

NAMESPACE_BEGIN

class MainSystemInfo
{
public:
	constexpr static size_t COUNT = 8;

	constexpr static ID RENDERING_ID			= 0;
	constexpr static ID PHYSICS_ID				= 1;
	constexpr static ID SCRIPTING_ID			= 2;
	constexpr static ID AUDIO_ID				= 4;
};

NAMESPACE_END