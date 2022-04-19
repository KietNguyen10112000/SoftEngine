#include "v8EngineWrapper.h"

#include "../JavaScriptLanguage.h"

#include "SoftWrapToV8.h"

#include "Engine/Engine.h"

//#include "Engine/Global.h"
//#include "Engine/Scene/Scene.h"

#define V8_ENTER_SCOPE()                                    \
v8::Isolate::Scope isolate_scope(m_isolate);                \
v8::HandleScope handle_scope(m_isolate);                    \
v8::Context::Scope context_scope(m_context.Get(m_isolate));

v8EngineWrapper::v8EngineWrapper()
{
	m_jsLang = JavaScriptLanguage::GetInstance();

    using namespace v8wrapper;

    const char* flags = "--expose_gc";
    v8::V8::SetFlagsFromString(flags);

    char buf[MAX_PATH];
    GetModuleFileNameA(nullptr, buf, MAX_PATH);

    // Initialize V8.
    v8::V8::InitializeICUDefaultLocation(buf);
    v8::V8::InitializeExternalStartupData(buf);
    m_platform = v8::platform::NewDefaultPlatform();
    v8::V8::InitializePlatform(m_platform.get());
    v8::V8::Initialize();

    m_createParams.array_buffer_allocator =
        v8::ArrayBuffer::Allocator::NewDefaultAllocator();

    m_isolate = v8::Isolate::New(m_createParams);

    v8::Isolate::Scope isolate_scope(m_isolate);
    // Create a stack-allocated handle scope.
    v8::HandleScope handle_scope(m_isolate);

    v8::Local<v8::ObjectTemplate> globalObj = v8::ObjectTemplate::New(m_isolate);
    v8wrapper::Initialize(m_isolate, globalObj);

    m_globalTempl = v8::Global<v8::ObjectTemplate>(m_isolate, globalObj);


    SoftWrapToV8(this, 0, 0);


    // Create a new context.
    v8::Local<v8::Context> context = v8::Context::New(m_isolate, 0, globalObj);
    m_context = v8::Global<v8::Context>(m_isolate, context);
}

v8EngineWrapper::~v8EngineWrapper()
{
    m_loopFuncs.clear();

    m_context.Reset();
    m_globalTempl.Reset();

    v8wrapper::Finalize(m_isolate);

    m_isolate->RequestGarbageCollectionForTesting(v8::Isolate::kFullGarbageCollection);

    // Dispose the isolate and tear down V8.
    m_isolate->Dispose();

    v8::V8::Dispose();
    v8::V8::ShutdownPlatform();

    delete m_createParams.array_buffer_allocator;
}

void v8EngineWrapper::SetupRuntime(Engine* engine, Scene* scene)
{
    v8::Isolate::Scope isolate_scope(m_isolate);
    v8::HandleScope handle_scope(m_isolate);

    v8wrapper::NewRunTime(m_isolate);
    v8::Local<v8::Context> context = v8::Context::New(m_isolate, 0, m_globalTempl.Get(m_isolate));
    m_context = v8::Global<v8::Context>(m_isolate, context);

    v8::Context::Scope context_scope(context);
    v8wrapper::CompileAndExec("console = ___console;", m_isolate);
}

void v8EngineWrapper::Run(const char* sourceCode)
{
    V8_ENTER_SCOPE();

    try
    {
        v8wrapper::CompileAndExec(sourceCode, m_isolate);
    }
    catch (const char* str)
    {
        std::cerr << str << "\n";
    }
    catch (const std::string& str)
    {
        std::cerr << str << "\n";
    }
    catch (...)
    {
        std::cerr << "Fatal error" << "\n";
    }

    v8wrapper::CleanUp();
}

Controller* v8EngineWrapper::Parse(const char* sourceCode)
{
	return nullptr;
}

ScriptLanguage* v8EngineWrapper::GetLanguage()
{
    return m_jsLang.get();
}

void v8EngineWrapper::SetLanguageVersion(const char* desc)
{
}

void v8EngineWrapper::Update(Engine* engine, Scene* scene)
{
    global.deltaTime = engine->FDeltaTime();

    V8_ENTER_SCOPE();

    auto context = m_context.Get(m_isolate);

    v8::Local<v8::Value> args[1] = { v8::Number::New(m_isolate, engine->FDeltaTime()) };
    for (auto& _func : m_loopFuncs)
    {
        auto func = _func.second.Get(m_isolate);
        func->Call(context, v8::Null(m_isolate), 1, args);
    }
}

void v8EngineWrapper::RunLoop(v8::Local<v8::Function> func)
{
    auto name = func->GetName();
    auto cName = v8wrapper::ToStdString(name, m_isolate);
    m_loopFuncs.insert({ cName, v8::Global<v8::Function>(m_isolate, func) });
}

void v8EngineWrapper::ClearLoop(v8::Local<v8::Function> func)
{
    auto name = func->GetName();
    auto cName = v8wrapper::ToStdString(name, m_isolate);

    auto it = m_loopFuncs.find(cName);

    if (it != m_loopFuncs.end())
    {
        m_loopFuncs.erase(it);
    }
}

void v8EngineWrapper::ClearAllLoops()
{
    m_loopFuncs.clear();
}
