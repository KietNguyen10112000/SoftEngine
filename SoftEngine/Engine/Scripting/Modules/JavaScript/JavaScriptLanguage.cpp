#include "JavaScriptLanguage.h"

JavaScriptLanguage::JavaScriptLanguage()
{
	m_name = "JavaScript";
}

void JavaScriptLanguage::EnumVersions(char* buffer, size_t bufferSize)
{
	memcpy(buffer, "ES6;TypeScript", 14);
}

void JavaScriptLanguage::GetName(char* buffer, size_t bufferSize)
{
	auto num = std::min(bufferSize, m_name.size());
	memcpy(buffer, m_name.data(), m_name.size());
}
