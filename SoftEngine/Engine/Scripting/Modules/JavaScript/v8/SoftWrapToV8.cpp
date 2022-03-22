#include "SoftWrapToV8.h"

#include "v8EngineWrapper.h"

#include "Core/Resource.h"
#include "Core/Shader.h"

#include "Engine/Engine.h"
#include "Engine/Rendering/RenderingWorker.h"

namespace v8wrapper
{

class v8Resource
{
public:
	static void ReloadPixelShader(const v8::FunctionCallbackInfo<v8::Value>& args)
	{
		auto isolate = args.GetIsolate();

		if (args.Length() != 1 || !args[0]->IsString())
		{
			V8Throw(isolate, "Resource.ReloadPixelShader(path: string)");
			return;
		}
		
		auto path = ToStdString(args[0], isolate);

		auto wpath = StringToWString(path);

		auto* resource = Resource::Find(wpath);

		auto* ps = dynamic_cast<PixelShader*>(resource);

		auto engine = Global::engine;
		if (ps)
		{
			engine->GetRenderingWorker()->RunAsync(
				[=]() 
				{
					PixelShader* localPs = ps;
					localPs->~PixelShader();
					new (localPs) PixelShader(wpath, 0, 0);
					Resource::Release(&localPs);
				}, 
				RENDERING_TASK_HINT::RUN_AT_BEGIN_FRAME
			);
		}
		else
		{
			V8Throw(isolate, "Resource not found!");
		}
	};

};

}

void SoftWrapToV8(v8EngineWrapper* v8engine, Engine* softEngine, Scene* softScene)
{
	auto isolate = v8engine->m_isolate;
	auto& _globalTemple = v8engine->m_globalTempl;

	auto globalTemple = _globalTemple.Get(isolate);

	{
		v8::Local<v8::FunctionTemplate> resource = v8::FunctionTemplate::New(isolate);
		v8wrapper::AddFunction<&v8wrapper::v8Resource::ReloadPixelShader>(isolate, resource, "ReloadPixelShader");
		globalTemple->Set(isolate, "Resource", resource);
	}

}
