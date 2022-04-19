#include "SoftWrapToV8.h"

#include "v8EngineWrapper.h"

#include "Core/Resource.h"
#include "Core/Shader.h"

#include "Engine/Engine.h"
#include "Engine/Rendering/RenderingWorker.h"
#include "Engine/Logic/LogicWorker.h"

#include "Engine/Scene/Scene.h"

#include "Engine/Scripting/Modules/JavaScript/v8/v8EngineWrapper.h"

#include "Engine/Scripting/Modules/JavaScript/JavaScriptLanguage.h"

#include "Engine/UI/ImGuiCommon.h"

#include <filesystem>

//#include <winapifamily.h>

namespace v8wrapper
{

inline void JsAddAPI(const std::string& name, const std::string& desc)
{
	JavaScriptLanguage::GetInstancePtr()->AddAPI(name, desc);
}

inline std::string JsGetAPI(const std::string& name)
{
	return JavaScriptLanguage::GetInstancePtr()->GetAPIDesc(name);
}

class v8Resource
{
public:

#define RELOAD_SHADER(shaderType)										\
	auto* l_##shaderType = dynamic_cast<shaderType*>(resource);			\
	if (l_##shaderType)													\
	{																	\
		engine->GetRenderingWorker()->RunAsync(							\
			[=]()														\
			{															\
				shaderType* local_##shaderType = l_##shaderType;		\
				local_##shaderType->~shaderType();						\
				new (local_##shaderType) shaderType(wpath, 0, 0);		\
				Resource::Release(&local_##shaderType);					\
			},															\
			RENDERING_TASK_HINT::RUN_AT_BEGIN_FRAME						\
		);																\
		engine->GetRenderingWorker()->Refresh();						\
		std::cout << #shaderType " \'"									\
				<< WStringToString(wpath) << "\' reloaded";				\
		return;															\
	}

	static void ReloadShader(const v8::FunctionCallbackInfo<v8::Value>& args)
	{
		auto isolate = args.GetIsolate();

		if (args.Length() != 1 || !args[0]->IsString())
		{
			V8Throw(isolate, JsGetAPI("Resource.ReloadShader").c_str());
			return;
		}
		
		auto path = ToStdString(args[0], isolate);

		auto wpath = StringToWString(path);

		auto* resource = Resource::Find(wpath);

		auto engine = Global::engine;

		RELOAD_SHADER(VertexShader);
		RELOAD_SHADER(GeometryShader);
		//RELOAD_SHADER(HullShader);
		//RELOAD_SHADER(DomainShader);
		RELOAD_SHADER(PixelShader);

		std::cerr << "Resource not found!";
	};

#undef RELOAD_SHADER

};

#define V8_CHECK_ARGS(index, what, apiName)																	\
if (args.Length() <= index || !args[index]->what())															\
{																											\
	V8Throw(isolate,																						\
		("Arguments[" #index "]." #what "() == true\nAPI description: " + JsGetAPI(apiName)).c_str());		\
	return;																									\
}

#define V8_CHECK_ARGS_LENGTH(length, apiName)																\
if (args.Length() < length)																					\
{																											\
	V8Throw(isolate, JsGetAPI(apiName).c_str());															\
	return;																									\
}

#define V8_CHECK_VALUE(value, condition, apiName)															\
if (!value->condition()) { V8Throw(isolate, JsGetAPI(apiName).c_str()); return; }


#define V8_CHECK_GLOBAL_SCENE_OPERATOR(scene, ...)															\
if (scene->IsUpdating())																					\
{																											\
	V8Throw(isolate, "Global scene operator is not allowed in frame scope. " #__VA_ARGS__);					\
	return;																									\
}

class v8Soft
{
public:
	static void RunLoop(const v8::FunctionCallbackInfo<v8::Value>& args)
	{
		auto isolate = args.GetIsolate();
		V8_CHECK_ARGS(0, IsFunction, "Soft.RunLoop");
		auto func = v8::Local<v8::Function>::Cast(args[0]);
		auto v8 = dynamic_cast<v8EngineWrapper*>(Global::engine->CurrentScene()->GetScriptEngine());
		v8->RunLoop(func);
	};

	static void ClearLoop(const v8::FunctionCallbackInfo<v8::Value>& args)
	{
		auto isolate = args.GetIsolate();
		V8_CHECK_ARGS(0, IsFunction, "Soft.ClearLoop");
		auto func = v8::Local<v8::Function>::Cast(args[0]);
		auto v8 = dynamic_cast<v8EngineWrapper*>(Global::engine->CurrentScene()->GetScriptEngine());
		v8->ClearLoop(func);
	};

	static void ClearAllLoops(const v8::FunctionCallbackInfo<v8::Value>& args)
	{
		auto isolate = args.GetIsolate();
		V8_CHECK_ARGS_LENGTH(0, "Soft.ClearAllLoops");
		auto v8 = dynamic_cast<v8EngineWrapper*>(Global::engine->CurrentScene()->GetScriptEngine());
		v8->ClearAllLoops();
	};

	static void SetCurrentDir(const v8::FunctionCallbackInfo<v8::Value>& args)
	{
		auto isolate = args.GetIsolate();
		V8_CHECK_ARGS(0, IsString, "Soft.SetCurrentDir");

		auto str = ToStdString(args[0], isolate);
		if (std::filesystem::is_directory(str))
		{
			g_currentDir = str;
			Resource::CurrentDirectory() = StringToWString(str);

			args.GetReturnValue().Set(true);
			return;
		}

		args.GetReturnValue().Set(false);
	};

};

// you are working on LogicWorker
template <class T>
class v8RefHandle
{
public:
	static_assert(std::is_base_of_v<RefCounted, T>);
	//inline static v8::Global<v8::ObjectTemplate> ctorFunc;
	inline static v8::ObjectTemplate* ctorFunc;

	T* m_ref = 0;

public:
	v8RefHandle()
	{
		m_ref = Global::engine->CurrentScene()->RefCountedGet(new T());
	};

	v8RefHandle(T* ref)
	{
		m_ref = Global::engine->CurrentScene()->RefCountedGet(ref);
	};

	~v8RefHandle()
	{
		Global::engine->CurrentScene()->RefCountedRelease(m_ref);
	};

	template <auto method, typename ...Args>
	auto Method(Args&&... args)
	{
		return method(std::forward<Args>(args)...);
	};

	inline auto Ptr()
	{
		return m_ref;
	};

//public:
//	static void Config(const v8::FunctionCallbackInfo<v8::Value>& args)
//	{
//		V8_CHECK_ARGS_LENGTH(2, "Soft.ClearAllLoops");
//	};

};

using v8SceneNode = v8RefHandle<SceneNode>;

class _v8SceneNode
{
public:
	static void ToString(const v8::FunctionCallbackInfo<v8::Value>& args)
	{
		auto isolate = args.GetIsolate();
		auto context = isolate->GetCurrentContext();
		auto _this = args.This();

		auto current = GetValueAs<v8SceneNode*>(_this, context);

		std::string ret;

		ret += SceneNode::NODE_TYPE_DESC[current->Ptr()->Type()];

		args.GetReturnValue().Set(JsString(ret.c_str(), isolate));
	};

	static void GetterTransform(v8::Local<v8::String> property,
		const v8::PropertyCallbackInfo<v8::Value>& info)
	{
		auto isolate = info.GetIsolate();
		auto context = isolate->GetCurrentContext();
		auto _this = info.This();

		auto scene = Global::engine->CurrentScene();
		V8_CHECK_GLOBAL_SCENE_OPERATOR(scene);

		auto handle = GetValueAs<v8SceneNode*>(_this, context);
		auto sceneNode = handle->Ptr();
		Mat4x4& mat = sceneNode->Transform();
		float* ptr = &mat.m[0][0];

		auto jsArr = v8::Array::New(isolate, 16);

		for (size_t i = 0; i < 16; i++)
		{
			jsArr->Set(context, i, v8::Number::New(isolate, ptr[i]));
		}

		info.GetReturnValue().Set(jsArr);
	};

	static void SetterTransform(v8::Local<v8::String> property, v8::Local<v8::Value> value,
		const v8::PropertyCallbackInfo<void>& info)
	{
		auto isolate = info.GetIsolate();
		auto context = isolate->GetCurrentContext();
		auto _this = info.This();

		auto scene = Global::engine->CurrentScene();
		V8_CHECK_GLOBAL_SCENE_OPERATOR(scene);

		//"set SceneNode.transform = [Array number]"
		V8_CHECK_VALUE(value, IsArray, "SceneNode.transform");
		
		auto handle = GetValueAs<v8SceneNode*>(_this, context);
		auto sceneNode = handle->Ptr();
		Mat4x4& mat = sceneNode->Transform();
		float* ptr = &mat.m[0][0];

		auto jsArr = v8::Local<v8::Array>::Cast(value);

		for (size_t i = 0; i < 16; i++)
		{
			ptr[i] = GetValueAs<float>(jsArr->Get(context, i).ToLocalChecked(), context);
		}

		sceneNode->ChangeState();
		Global::engine->GetRenderingWorker()->Refresh();
	};

};



class v8Scene
{
public:
	static void NewSceneNode(const v8::FunctionCallbackInfo<v8::Value>& args)
	{
		auto isolate = args.GetIsolate();
		auto context = isolate->GetCurrentContext();
		V8WRAPPER_TRY_GET_REGISTED_CPP_JS_CLASS_RAW_PTR(v8wrapper::v8SceneNode, v8wrapper::v8SceneNode::ctorFunc);

		auto jsSceneNode = v8SceneNode::ctorFunc->NewInstance(context).ToLocalChecked();
		auto ref = v8wrapper::NewCppObj<v8SceneNode>(isolate, jsSceneNode, true);

		args.GetReturnValue().Set(jsSceneNode);
	};

	static void EnumerateNodes(const v8::FunctionCallbackInfo<v8::Value>& args)
	{
		auto isolate = args.GetIsolate();
		auto context = isolate->GetCurrentContext();
		V8WRAPPER_TRY_GET_REGISTED_CPP_JS_CLASS_RAW_PTR(v8wrapper::v8SceneNode, v8wrapper::v8SceneNode::ctorFunc);


		auto scene = Global::engine->CurrentScene();
		V8_CHECK_GLOBAL_SCENE_OPERATOR(scene);

		std::vector<SceneNode*> nodes;

		scene->Filter(nodes);

		auto arr = v8::Array::New(isolate, nodes.size());

		uint32_t i = 0;
		for (auto& node : nodes)
		{
			auto jsSceneNode = v8SceneNode::ctorFunc->NewInstance(context).ToLocalChecked();
			auto ref = v8wrapper::NewCppObj<v8SceneNode>(isolate, jsSceneNode, true, node);
			arr->Set(context, i, jsSceneNode);
			i++;
		}

		scene->FreeNodes(nodes);

		args.GetReturnValue().Set(arr);
	};

};

template <typename T>
class _StdVector
{
public:
	inline static void ToString(const v8::FunctionCallbackInfo<v8::Value>& args)
	{
		auto isolate = args.GetIsolate();
		auto _this = args.This();
		auto p = _this->GetAlignedPointerFromInternalField(0);

		std::string ret = "[";

		std::vector<T>* buffer = (std::vector<T>*)p;
		std::vector<T>& vector = *buffer;

		if constexpr (std::is_same_v<std::string, T> || std::is_same_v<const char*, T> || std::is_same_v<char*, T>)
		{
			for (size_t i = 0; i < vector.size(); i++)
			{
				ret = ret + "\'" + vector[i] + "\', ";
			}
		}
		else
		{
			for (size_t i = 0; i < vector.size(); i++)
			{
				ret = ret + std::to_string(vector[i]) + ", ";
			}
		}


		if (ret.size() > 2)
		{
			ret.resize(ret.size() - 2);
		}

		ret += "]";

		args.GetReturnValue().Set(v8wrapper::JsString(ret.c_str(), isolate));
	}
};


class _Std
{
public:
	template <typename T>
	inline static void ToVector(const v8::FunctionCallbackInfo<v8::Value>& args)
	{
		auto isolate = args.GetIsolate();

		if (args.Length() != 1)
		{
			v8wrapper::V8Throw(isolate, "Too few args");
			return;
		}

		auto input = args[0];

		if (!input->IsArray())
		{
			v8wrapper::V8Throw(isolate, "Args must be \'Array\'");
			return;
		}

		auto context = isolate->GetCurrentContext();

		auto jsArr = v8::Local<v8::Array>::Cast<v8::Value>(input);

		auto length = jsArr->Length();

		std::vector<T>* buffer = new std::vector<T>();
		std::vector<T>& vector = *buffer;
		vector.resize(length);

		for (size_t i = 0; i < length; i++)
		{
			auto maybeelm = jsArr->Get(context, i);

			if (maybeelm.IsEmpty()) continue;

			auto elm = maybeelm.ToLocalChecked();

			if constexpr (std::is_same_v<std::string, T>)
			{
				vector[i] = v8wrapper::GetValueAs<const char*>(elm, context);
			}
			else if constexpr (std::is_fundamental_v<T>)
			{
				if (!elm->IsNumber()) continue;
				vector[i] = v8wrapper::GetValueAs<T>(elm, context);
			}
			else
			{
				vector[i] = v8wrapper::GetValueAs<T>(elm, context);
			}

		}

		auto ret = v8wrapper::NewCppJsObject<std::vector<T>*, 0, true>(isolate, buffer);

		args.GetReturnValue().Set(ret);
	}

	static void ToString(const v8::FunctionCallbackInfo<v8::Value>& args)
	{
		auto isolate = args.GetIsolate();

		std::string output;
		if (args.Length() != 0)
		{
			auto input = args[0];
			output = ToStdString(input, isolate);
		}

		//let v8 handle (new std::string()) for you
		auto ret = v8wrapper::NewCppJsObject(isolate, new std::string(output));
		args.GetReturnValue().Set(ret);
	}

public:
	template <typename T>
	inline static void WrapVector(v8::Isolate* isolate, v8::Local<v8::ObjectTemplate>& global, const char* jsSideFuncName)
	{
		v8::Local<v8::FunctionTemplate> templ = v8::FunctionTemplate::New(isolate,
			v8wrapper::Constructor<std::vector<T>, true>);
		v8wrapper::RegisterCppJsClass<std::vector<T>>(isolate, templ);
		auto ins = templ->InstanceTemplate();
		v8wrapper::AddFunction<&_StdVector<T>::ToString>(isolate, ins, "toString");
		v8wrapper::WrapClass(isolate, global, templ, jsSideFuncName);
	}

};

}

void ___ConsoleClear(const v8::FunctionCallbackInfo<v8::Value>& args)
{
#if defined(IMGUI)
	extern ::ImGuiCommon::Console g_imGuiConsole;
	g_imGuiConsole.ClearOutput();
#endif
}

void SoftWrapToV8(v8EngineWrapper* v8engine, Engine* softEngine, Scene* softScene)
{
	auto isolate = v8engine->m_isolate;
	auto& _globalTemple = v8engine->m_globalTempl;

	auto globalTemple = _globalTemple.Get(isolate);

	{
		//object Std
		using namespace v8wrapper;
		//==========================Std class=======================================================================
		_Std::WrapVector<const char*>(isolate, globalTemple, "StdVectorCString");
		_Std::WrapVector<std::string>(isolate, globalTemple, "StdVectorString");
		_Std::WrapVector<int32_t>(isolate, globalTemple, "StdVectorInt32");
		_Std::WrapVector<int64_t>(isolate, globalTemple, "StdVectorInt64");
		_Std::WrapVector<float>(isolate, globalTemple, "StdVectorFloat");
		_Std::WrapVector<double>(isolate, globalTemple, "StdVectorDouble");

		//==========================object Std method================================================================
		v8::Local<v8::ObjectTemplate> _std = v8::ObjectTemplate::New(isolate);
		v8wrapper::AddFunction<&_Std::ToVector<const char*>>(isolate, _std, "ToVectorCString");
		v8wrapper::AddFunction<&_Std::ToVector<std::string>>(isolate, _std, "ToVectorString");
		v8wrapper::AddFunction<&_Std::ToVector<int32_t>>(isolate, _std, "ToVectorInt32");
		v8wrapper::AddFunction<&_Std::ToVector<int64_t>>(isolate, _std, "ToVectorInt64");
		v8wrapper::AddFunction<&_Std::ToVector<float>>(isolate, _std, "ToVectorFloat");
		v8wrapper::AddFunction<&_Std::ToVector<double>>(isolate, _std, "ToVectorDouble");

		v8wrapper::AddFunction<&_Std::ToString>(isolate, _std, "ToString");

		globalTemple->Set(isolate, "Std", _std);
	}


	{
		//object Resource
		v8::Local<v8::ObjectTemplate> resource = v8::ObjectTemplate::New(isolate);

		v8wrapper::AddFunction<&v8wrapper::v8Resource::ReloadShader>(isolate, resource, "ReloadShader");
		v8wrapper::JsAddAPI("Resource.ReloadShader", "Resource.ReloadPixelShader(path: string): void");

		globalTemple->Set(isolate, "Resource", resource);
	}

	{
		//object Soft
		v8::Local<v8::ObjectTemplate> soft = v8::ObjectTemplate::New(isolate);

		v8wrapper::AddGlobalVariable<(float*)&(v8EngineWrapper::global)>(isolate, soft, "DeltaTime");
		v8wrapper::JsAddAPI("Soft.DeltaTime", "Soft.DeltaTime: float");

		v8wrapper::AddGlobalVariable<&v8wrapper::g_currentDir>(isolate, soft, "CurrentDir");
		v8wrapper::JsAddAPI("Soft.CurrentDir", "Soft.CurrentDir: string; // (get, shouldn't set directly)");

		v8wrapper::AddFunction<&v8wrapper::Import>(isolate, soft, "Import");
		v8wrapper::JsAddAPI("Soft.Import", "Soft.Import(path: string): any");


		v8wrapper::AddFunction<&v8wrapper::v8Soft::SetCurrentDir>(isolate, soft, "SetCurrentDir");
		v8wrapper::JsAddAPI("Soft.SetCurrentDir", "Soft.SetCurrentDir(path: string): boolean");


		v8wrapper::AddFunction<&v8wrapper::v8Soft::RunLoop>(isolate, soft, "RunLoop");
		v8wrapper::JsAddAPI("Soft.RunLoop", "Soft.RunLoop(function: (deltaTime: float) => void): void");
		v8wrapper::AddFunction<&v8wrapper::v8Soft::ClearLoop>(isolate, soft, "ClearLoop");
		v8wrapper::JsAddAPI("Soft.ClearLoop", "Soft.ClearLoop(function: (deltaTime: float) => void): void");
		v8wrapper::AddFunction<&v8wrapper::v8Soft::ClearAllLoops>(isolate, soft, "ClearAllLoops");
		v8wrapper::JsAddAPI("Soft.ClearAllLoops", "Soft.ClearAllLoops(): void");


		globalTemple->Set(isolate, "Soft", soft);
	}

	{
		//object Scene
		v8::Local<v8::ObjectTemplate> scene = v8::ObjectTemplate::New(isolate);

		v8wrapper::AddFunction<&v8wrapper::v8Scene::EnumerateNodes>(isolate, scene, "EnumerateNodes");

		globalTemple->Set(isolate, "Scene", scene);


		//object SceneNode
		//constructor function
		//like in js side:
		//
		// //this function still call as contructor
		// function Scene
		// {
		//		this.some = 1
		// }
		//
		v8::Local<v8::FunctionTemplate> sceneNodeFunc = v8::FunctionTemplate::New(isolate);
		v8wrapper::RegisterCppJsClass<v8wrapper::v8SceneNode>(isolate, sceneNodeFunc);
		auto sceneNodeClazz = sceneNodeFunc->InstanceTemplate();

		v8wrapper::AddFunction<v8wrapper::_v8SceneNode::ToString>(isolate, sceneNodeClazz, "toString");

		// sceneNode.transform = blah ... (array-like object)
		sceneNodeClazz->SetAccessor(v8::String::NewFromUtf8(isolate, "transform").ToLocalChecked(),
			v8wrapper::_v8SceneNode::GetterTransform, v8wrapper::_v8SceneNode::SetterTransform);
		v8wrapper::JsAddAPI("SceneNode.transform", "[get, set] SceneNode.transform: number[]");

		v8wrapper::WrapClass(isolate, globalTemple, sceneNodeClazz, "SceneNode");
	}

}
