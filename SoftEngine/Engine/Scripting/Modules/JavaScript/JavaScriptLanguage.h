#pragma once

#include "Engine/Scripting/ScriptLanguage.h"
#include "Core/Templates/Singleton.h"

#include <string>
#include <map>

class JavaScriptLanguage : public ScriptLanguage, public CSingleton<JavaScriptLanguage>
{
public:
	std::string m_name;
	//api call, api desc
	//eg: Soft.RunLoop => Soft.RunLoop(function: (deltaTime: float) => void)
	std::map<std::string, std::string> m_API;

public:
	JavaScriptLanguage();

public:
	// Inherited via ScriptLanguage
	virtual void EnumVersions(char* buffer, size_t bufferSize) override;

	virtual void GetName(char* buffer, size_t bufferSize) override;

	void AddAPI(const std::string& name, const std::string& desc);

	std::string GetAPIDesc(const std::string& name);
};