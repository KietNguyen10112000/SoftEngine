#pragma once
#include <vector>

class IRenderer;

class PostProcessor
{
public:
	enum POST_PROCESSING
	{
		LIGHT_BLUR,
		LIGHT_SCATTERING,
		COUNT
	};

protected:
	std::vector<POST_PROCESSING> m_list;

public:
	inline virtual ~PostProcessor() {};

public:
	inline virtual void Add(POST_PROCESSING type) { m_list.push_back(type); };

	inline virtual void Apply() = 0;

};
