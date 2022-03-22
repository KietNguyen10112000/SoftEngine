#pragma once

class ScriptLanguage
{
public:
	inline virtual ~ScriptLanguage() {};

public:
	// eg:
	// lua 5 + lua 2 => buffer = "lua5;lua2;"
	virtual void EnumVersions(char* buffer, size_t bufferSize) = 0;

	virtual void GetName(char* buffer, size_t bufferSize) = 0;

};