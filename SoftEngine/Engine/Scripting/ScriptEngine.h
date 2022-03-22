#pragma once

class Engine;
class Scene;
class Controller;
class ScriptLanguage;

class ScriptEngine
{
public:
	inline virtual ~ScriptEngine() {};

public:
	// call once after scene initialized
	virtual void SetupRuntime(Engine* engine, Scene* scene) = 0;

	// after setup runtime
	virtual void Run(const char* sourceCode) = 0;

	// alloc new memory, explicit delete
	virtual Controller* Parse(const char* sourceCode) = 0;

public:
	// just get, not alloc new memory
	virtual ScriptLanguage* GetLanguage() = 0;

	// script lang can have multiple version
	// enum versions by ScriptLanguage::EnumVersions()
	// eg: 
	// lua has lua 5, lua 2, luaJIT, ...
	// js has es5, es6, ts, ...
	// c++ (can use as script lang with dll module) has c++17, C++11, ...
	virtual void SetLanguageVersion(const char* desc) = 0;

public:
	// just frame update
	virtual void Update(Engine* engine, Scene* scene) = 0;

};