#pragma once

#include "Engine/Scripting/ScriptLanguage.h"

#include <string>

class JavaScriptLanguage : public ScriptLanguage
{
public:
	std::string m_name;

public:
	JavaScriptLanguage();

public:
	// Inherited via ScriptLanguage
	virtual void EnumVersions(char* buffer, size_t bufferSize) override;

	virtual void GetName(char* buffer, size_t bufferSize) override;

};