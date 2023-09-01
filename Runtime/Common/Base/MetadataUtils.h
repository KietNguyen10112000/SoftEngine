#pragma once

#include "Core/TypeDef.h"
#include "Core/Structures/String.h"
#include "Core/Pattern/Singleton.h"

#include <string>
#include <vector>
#include <string_view>

NAMESPACE_BEGIN

class API MetadataParser : public Singleton<MetadataParser>
{
public:
	std::vector<std::string_view> m_values;

	static void Initialize();
	static void Finalize();

	// ignore all decorators
	std::vector<std::string_view>& GetValueChain(const std::string_view& editFormat, const String& input);

};

NAMESPACE_END