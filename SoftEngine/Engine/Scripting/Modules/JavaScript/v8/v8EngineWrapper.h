#pragma once

#include "Engine/Scripting/ScriptEngine.h"

#define V8_DLL
#include "v8Wrapper.h"


class v8EngineWrapper : public ScriptEngine
{
public:
	friend class JavaScriptLanguage;

	std::shared_ptr<JavaScriptLanguage> m_jsLang;

	std::unique_ptr<v8::Platform> m_platform;

	v8::Isolate::CreateParams m_createParams;
	v8::Isolate* m_isolate = 0;

	v8::Global<v8::Context> m_context;
	v8::Global<v8::ObjectTemplate> m_globalTempl;

	std::map<std::string, v8::Global<v8::Function>> m_loopFuncs;

	struct Global
	{
		//in sec
		float deltaTime;
		float time;
	};

	inline static Global global = {};

public:
	v8EngineWrapper();
	~v8EngineWrapper();

public:
	// Inherited via ScriptEngine
	virtual void SetupRuntime(Engine* engine, Scene* scene) override;

	virtual void Run(const char* sourceCode) override;

	virtual Controller* Parse(const char* sourceCode) override;

	virtual ScriptLanguage* GetLanguage() override;

	virtual void SetLanguageVersion(const char* desc) override;

	virtual void Update(Engine* engine, Scene* scene) override;

public:
	void RunLoop(v8::Local<v8::Function> func);
	void ClearLoop(v8::Local<v8::Function> func);
	void ClearAllLoops();

};