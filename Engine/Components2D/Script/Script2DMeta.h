#pragma once

#include "Core/TemplateUtils/TemplateUtils.h"

NAMESPACE_BEGIN

struct API Script2DMeta
{
	static Script2DMeta s_instance;
	inline static auto& Get()
	{
		return s_instance;
	}

	size_t onGUIVtbIdx				= INVALID_ID;
	size_t onCollideVtbIdx			= INVALID_ID;
	size_t onCollisionEnterVtbIdx	= INVALID_ID;
	size_t onCollisionExitVtbIdx	= INVALID_ID;
	size_t onUpdateVtbIdx			= INVALID_ID;
};

NAMESPACE_END