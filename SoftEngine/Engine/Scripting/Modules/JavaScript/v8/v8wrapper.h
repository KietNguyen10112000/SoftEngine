#pragma once

#include <libplatform/libplatform.h>
#include <v8.h>

#ifdef __INTELLISENSE__
#pragma diag_suppress 458
#endif

#ifdef _WIN32
#include <Windows.h>
#endif // WIN32

#include <map>
#include <set>
#include <iostream>
#include <algorithm>

#include "Core/File.h"

extern void ___ConsoleClear(const v8::FunctionCallbackInfo<v8::Value>& args);

namespace v8wrapper
{

//recommend
constexpr int INTERNAL_FIELD_COUNT = 2;

inline std::map<void*, v8::Global<v8::ObjectTemplate>> g_cppToJsType;

inline std::set<v8::ObjectTemplate**> g_cppToJsTypeCache;

constexpr size_t HARD_CODED_JS_STRING_MAX_LENGTH = 8096;

//how many lines of code that you can do hard code :))
inline std::string g_hardCodedJsString;

//clean hard coded js string, ...
inline void CleanUp()
{
    g_hardCodedJsString.clear();
}

inline const char* ToCString(const v8::String::Utf8Value& value)
{
    return *value ? *value : "<string conversion failed>";
}

inline std::string ToStdString(v8::Local<v8::Value> value, v8::Isolate* isolate)
{
    v8::String::Utf8Value str(isolate, value);
    return *str ? *str : "<string conversion failed>";
}

inline bool V8TryCatch(v8::TryCatch& trycatch, v8::Isolate* isolate)
{
    if (trycatch.HasCaught())
    {
        auto context = isolate->GetCurrentContext();
        v8::Local<v8::Value> exception = trycatch.Exception();
        auto msg = trycatch.Message();

        v8::String::Utf8Value exception_str(isolate, exception);
        v8::String::Utf8Value loc_str(isolate, msg->GetSourceLine(context).ToLocalChecked());

        std::cerr << "Exception:\t" << *exception_str << "\n";
        std::cerr << "At line " << msg->GetLineNumber(context).ToChecked() << ":\t" << *loc_str << "\n";
        return -1;
    }
    return 0;
}

inline v8::Local<v8::String> JsString(const std::string& str, v8::Isolate* isolate)
{
    return v8::String::NewFromUtf8(isolate, str.c_str()).ToLocalChecked();
}

inline void V8Throw(v8::Isolate* isolate, const char* exceptionDesc)
{
    isolate->ThrowException(JsString(exceptionDesc, isolate));
}

template <typename T>
inline void RegisterCppJsClass(v8::Isolate* isolate, v8::Local<v8::FunctionTemplate> templ)
{
    auto instance = templ->InstanceTemplate();
    instance->SetInternalFieldCount(INTERNAL_FIELD_COUNT);

    g_cppToJsType.insert({ (void*)typeid(T).name(), v8::Global<v8::ObjectTemplate>(isolate, instance) });
}

template <typename T>
inline void RegisterCppJsClass(v8::Isolate* isolate, v8::Local<v8::ObjectTemplate>& templ)
{
    g_cppToJsType.insert({ (void*)typeid(T).name(), v8::Global<v8::ObjectTemplate>(isolate, templ) });
}

template <typename T>
inline void RegisterCppJsClass(v8::Isolate* isolate)
{
    auto context = isolate->GetCurrentContext();

    //v8::HandleScope scope(isolate);

    v8::Local<v8::FunctionTemplate> templ = v8::FunctionTemplate::New(isolate);
    auto instance = templ->InstanceTemplate();
    instance->SetInternalFieldCount(INTERNAL_FIELD_COUNT);

    auto clazz = templ->GetFunction(context).ToLocalChecked();
    clazz->SetName(JsString(typeid(T).name(), isolate));

    context->Global()->Set(context, JsString(typeid(T).name(), isolate), clazz);

    g_cppToJsType.insert({ (void*)typeid(T).name(), v8::Global<v8::ObjectTemplate>(isolate, templ->InstanceTemplate()) });
}

template<typename T>
using base_type = typename std::decay<T>::type;

//with out pointer, reference, const, ...
#define BaseTypeName(T) typename std::remove_const<typename std::remove_pointer<base_type<T>>::type>::type

template<typename T>
struct WeakCallBackInfoStruct
{
    v8::Persistent<v8::Value> persistent;
    T* ptr = 0;

    inline static void Destructor(const v8::WeakCallbackInfo<WeakCallBackInfoStruct<T>>& data)
    {
#ifdef _DEBUG
        std::cout << "C++ Object of \"" << typeid(T).name() << "\" deleted\n";
#endif // _DEBUG
        auto param = data.GetParameter();
        param->persistent.Reset();
        delete param->ptr;
        delete param;
    }
};

#define Int32ValueOf(v8Object, context) v8Object->ToInt32(context).ToLocalChecked()->Value()
#define Uint32ValueOf(v8Object, context) v8Object->ToUint32(context).ToLocalChecked()->Value()
#define Int64ValueOf(v8Object, context) v8Object->ToInteger(context).ToLocalChecked()->Value()
#define Uint64ValueOf(v8Object, context) v8Object->ToBigInt(context).ToLocalChecked()->Uint64Value();

#define DoubleValueOf(v8Object, context) v8Object->ToNumber(context).ToLocalChecked()->Value()


template<typename VarType, int InternalField = 0>
__forceinline VarType GetValueAs(v8::Local<v8::Value> value, v8::Local<v8::Context> context)
{
    if constexpr (std::is_same_v<VarType, int32_t>)
    {
        return Int32ValueOf(value, context);
    }
    else if constexpr (std::is_same_v<VarType, uint32_t>)
    {
        return Int32ValueOf(value, context);
    }
    else if constexpr (std::is_same_v<VarType, int64_t>)
    {
        return Int64ValueOf(value, context);
    }
    else if constexpr (std::is_same_v<VarType, uint64_t>)
    {
        //value->ToBigInt(context).ToLocalChecked()->Int64Value();
        return Uint64ValueOf(value, context);
    }
    else if constexpr (std::is_same_v<VarType, float>)
    {
        return (float)DoubleValueOf(value, context);
    }
    else if constexpr (std::is_same_v<VarType, double>)
    {
        return DoubleValueOf(value, context);
    }
    else if constexpr (
           std::is_same_v<VarType, const std::string&>
        || std::is_same_v<VarType, const std::string>
        || std::is_same_v<VarType, std::string>
        )
    {
        return ToStdString(value, context->GetIsolate());
    }
    else if constexpr (std::is_pointer_v<VarType>)
    {
        using BaseVarType = BaseTypeName(VarType);
        if constexpr (std::is_fundamental<BaseVarType>::value)
        {
            if constexpr (std::is_same<BaseVarType, char>::value)
            {
                v8::String::Utf8Value str(context->GetIsolate(), value);
                size_t cur = g_hardCodedJsString.size();

                auto cstr = ToCString(str);
                g_hardCodedJsString.append(cstr, str.length());
                g_hardCodedJsString.push_back('\0');
                return &g_hardCodedJsString[cur];
            }
            else
            {
                static_assert(0, "Can not wrap function with primitive data type pointer as parameter");
            }
        }
        else
        {
            auto obj = value->ToObject(context).ToLocalChecked();

            if (obj->InternalFieldCount() < INTERNAL_FIELD_COUNT) throw "Can not passing JS object as C++ parameter";

            void* ptr = obj->GetAlignedPointerFromInternalField(InternalField);

            auto id = (const char*)obj->GetAlignedPointerFromInternalField(InternalField + 1);

            if (id != typeid(BaseVarType).name())
            {
                //not allow polymorphism
                throw "Class mismatch: \'" + std::string(id) + "\' can not cast to \'" + std::string(typeid(BaseVarType).name()) + "\'";
            }

            return static_cast<VarType>(ptr);
        }
    }
    else// if constexpr (std::is_reference_v<VarType>)
    {
        using BaseVarType = BaseTypeName(VarType);

        auto obj = value->ToObject(context).ToLocalChecked();

        if (obj->InternalFieldCount() < INTERNAL_FIELD_COUNT) throw "Can not passing JS object as C++ parameter";

        void* ptr = obj->GetAlignedPointerFromInternalField(InternalField);

        auto id = (const char*)obj->GetAlignedPointerFromInternalField(InternalField + 1);

        if (id != typeid(BaseVarType).name())
        {
            //not allow polymorphism
            throw "Class mismatch: \'" + std::string(id) + "\' can not cast to \'" + std::string(typeid(BaseVarType).name()) + "\'";
        }

        return *static_cast<BaseVarType*>(ptr);
    }
}


#define MAKE_SETTER_FRIEND template<typename ClassType, typename VarType, VarType(ClassType::* member), int InternalField> \
friend void v8wrapper::Setter(v8::Local<v8::String>, v8::Local<v8::Value>, const v8::PropertyCallbackInfo<void>&);\
\
template<typename ClassType, auto(ClassType::* member), int InternalField> \
friend void v8wrapper::Setter(v8::Local<v8::String>, v8::Local<v8::Value>, const v8::PropertyCallbackInfo<void>&);\
\
template<auto* member, int InternalField> \
friend void v8wrapper::SetterGlobal(v8::Local<v8::Name>, v8::Local<v8::Value>,const v8::PropertyCallbackInfo<void>&);


#define MAKE_GETTER_FRIEND template<typename ClassType, typename VarType, VarType(ClassType::* member), int InternalField> \
friend void v8wrapper::Getter(v8::Local<v8::String>, const v8::PropertyCallbackInfo<v8::Value>&);\
\
template<typename ClassType, auto(ClassType::* member), int InternalField> \
friend void v8wrapper::Getter(v8::Local<v8::String>, const v8::PropertyCallbackInfo<v8::Value>&);\
\
template<class ClassType, auto(ClassType::* member), int InternalField> \
friend void v8wrapper::GetterClass(v8::Local<v8::String>, const v8::PropertyCallbackInfo<v8::Value>&);\
\
template<auto* member, int InternalField> \
friend void v8wrapper::GetterClassGlobal(v8::Local<v8::Name>, const v8::PropertyCallbackInfo<v8::Value>&);


template<typename ClassType, typename VarType,
    VarType(ClassType:: * member), int InternalField = 0>
void Setter(v8::Local<v8::String> property, v8::Local<v8::Value> value,
        const v8::PropertyCallbackInfo<void>& info)
{
    auto obj = info.This();
    //if (obj->InternalFieldCount() == 0) throw "";
    void* ptr = obj->GetAlignedPointerFromInternalField(InternalField);

    VarType& var = (static_cast<ClassType*>(ptr)->*member);
    auto context = info.GetIsolate()->GetCurrentContext();

    var = GetValueAs<VarType>(value, context);
}

template<typename ClassType,
    auto(ClassType:: * member), int InternalField = 0>
void Setter(v8::Local<v8::String> property, v8::Local<v8::Value> value,
        const v8::PropertyCallbackInfo<void>& info)
{
    auto obj = info.This();
    //if (obj->InternalFieldCount() == 0) throw "";
    void* ptr = obj->GetAlignedPointerFromInternalField(InternalField);

    auto& var = (static_cast<ClassType*>(ptr)->*member);
    auto context = info.GetIsolate()->GetCurrentContext();

    var = GetValueAs<std::remove_reference<decltype(var)>::type>(value, context);
}


template<typename ClassType, typename VarType,
    VarType(ClassType:: * member), int InternalField = 0>
void Getter(v8::Local<v8::String> property,
        const v8::PropertyCallbackInfo<v8::Value>& info) 
{
    auto obj = info.This();
    //if (obj->InternalFieldCount() == 0) throw "";
    void* ptr = obj->GetAlignedPointerFromInternalField(InternalField);

    VarType& value = static_cast<ClassType*>(ptr)->*member;
    info.GetReturnValue().Set(value);
}

template<class ClassType,
    auto(ClassType:: * member), int InternalField = 0>
void Getter(v8::Local<v8::String> property,
        const v8::PropertyCallbackInfo<v8::Value>& info) 
{
    auto obj = info.This();
    //if (obj->InternalFieldCount() == 0) throw "";
    void* ptr = obj->GetAlignedPointerFromInternalField(InternalField);

    auto& value = static_cast<ClassType*>(ptr)->*member;
    info.GetReturnValue().Set(value);
}

//new cpp object to JS
//return new ptr can save and delete yourself in C++ side
template<int InternalField = 0, typename T, typename ...Args>
T* NewCppObj(v8::Isolate* isolate, v8::Local<v8::Object>& toTarget, bool deleteWithJsObject, Args&&... args)
{
    T* p = new T(std::forward<Args>(args)...);

    if (deleteWithJsObject)
    {
        auto callback = new WeakCallBackInfoStruct<T>();
        callback->ptr = p;
        auto& obj = callback->persistent;

        obj.Reset(isolate, toTarget);

        obj.SetWeak(callback,
            WeakCallBackInfoStruct<T>::Destructor,
            v8::WeakCallbackType::kParameter
        );
    }

    toTarget->SetAlignedPointerInInternalField(InternalField, p);
    toTarget->SetAlignedPointerInInternalField(InternalField + 1, (void*)typeid(T).name());

    return p;
}

template<typename T, typename ...Args>
T* NewCppObj(v8::Isolate* isolate, v8::Local<v8::Object>& toTarget, bool deleteWithJsObject, Args&&... args)
{
    return NewCppObj<0, T, Args...>(isolate, toTarget, deleteWithJsObject, std::forward<Args>(args)...);
}


template<int InternalField = 0, typename T>
void NewCppObjFromPointer(v8::Isolate* isolate, v8::Local<v8::Object>& toTarget, T* ptr)
{
    toTarget->SetAlignedPointerInInternalField(InternalField, ptr);
    toTarget->SetAlignedPointerInInternalField(InternalField + 1, (void*)typeid(T).name());
}

//if target is a pointer, v8 will handle for you
//if target is not a pointer, the memories that target pointed to must exist until program exits
template<typename Target, int InternalField = 0, bool deleteWithJsObj = false>
v8::Local<v8::Value> NewCppJsObject(v8::Isolate* isolate, Target target)
{
    static v8::ObjectTemplate* templObj = 0;

    auto context = isolate->GetCurrentContext();

    using BaseTarget = typename std::remove_const<typename std::remove_pointer<base_type<Target>>::type>::type;

    const char* id = typeid(BaseTarget).name();

    if (templObj == 0)
    {
        auto it = g_cppToJsType.find((void*)id);

        if (it == g_cppToJsType.end())
        {
            std::string desc = id;
            desc = "C++ \"" + desc + "\" is not wrapped to JS";
            V8Throw(isolate, desc.c_str());
            return v8::Undefined(isolate);
        }

        auto& _templ = it->second;

        templObj = *(_templ.Get(isolate));
        g_cppToJsTypeCache.insert(&templObj);
        //v8::Local<v8::ObjectTemplate> templ = _templ.Get(isolate);
    }
    
    v8::ObjectTemplate*& templ = templObj;

    auto newJsObj = templ->NewInstance(context).ToLocalChecked();

    //using BaseType = BaseTypeName(Target);

    if constexpr (std::is_pointer_v<Target>)
    {
        newJsObj->SetAlignedPointerInInternalField(InternalField, target);
        if (deleteWithJsObj)
        {
            auto callback = new WeakCallBackInfoStruct<BaseTarget>();
            callback->ptr = target;
            auto& obj = callback->persistent;

            obj.Reset(isolate, newJsObj);

            obj.SetWeak(callback,
                WeakCallBackInfoStruct<BaseTarget>::Destructor,
                v8::WeakCallbackType::kParameter
            );
        }
    }
    else
    {
        /*auto newCppObj = NewCppObj<BaseTarget>(isolate, newJsObj, true);
        *newCppObj = target;*/
        newJsObj->SetAlignedPointerInInternalField(InternalField, &target);
    }

    newJsObj->SetAlignedPointerInInternalField(InternalField + 1, (void*)id);

    return newJsObj;
}

template<class ClassType,
    auto(ClassType::* member), int InternalField = 0>
void GetterClass(v8::Local<v8::String> property,
        const v8::PropertyCallbackInfo<v8::Value>& info)
{
    auto isolate = info.GetIsolate();
    auto context = isolate->GetCurrentContext();

    auto obj = info.This();
    //if (obj->InternalFieldCount() == 0) throw "";
    void* ptr = obj->GetAlignedPointerFromInternalField(InternalField);

    auto& value = static_cast<ClassType*>(ptr)->*member;

    using MemberVarType = typename std::remove_const<typename std::remove_pointer<base_type<decltype(value)>>::type>::type;

    if constexpr (std::is_fundamental<MemberVarType>::value)
    {
        info.GetReturnValue().Set(value);
    }
    else
    {
        info.GetReturnValue().Set(NewCppJsObject(isolate, value));
    }
}

template<auto* member, int InternalField = 0>
void GetterClassGlobal(v8::Local<v8::Name> property,
        const v8::PropertyCallbackInfo<v8::Value>& info)
{
    auto isolate = info.GetIsolate();
    auto context = isolate->GetCurrentContext();

    auto& value = *member;

    //auto v1 = &g_currentDir;
    //auto v2 = &value;

    using MemberVarType = typename std::remove_const<typename std::remove_pointer<base_type<decltype(value)>>::type>::type;

    if constexpr (std::is_fundamental<MemberVarType>::value)
    {
        info.GetReturnValue().Set(value);
    }
    else
    {
        info.GetReturnValue().Set(NewCppJsObject(isolate, &value));
    }
}

template<auto* member, int InternalField = 0>
void SetterGlobal(v8::Local<v8::Name> property, v8::Local<v8::Value> value,
        const v8::PropertyCallbackInfo<void>& info)
{
    auto& var = *member;
    auto context = info.GetIsolate()->GetCurrentContext();

    var = GetValueAs<std::remove_reference<decltype(var)>::type>(value, context);
}


template<size_t I = 0, typename... Tp>
void IterateOverTuple(std::tuple<Tp...>& t, const v8::FunctionCallbackInfo<v8::Value>& v8args, v8::Local<v8::Context> context)
{
    std::get<I>(t) = GetValueAs<std::tuple_element<I, std::tuple<Tp...> >::type>(v8args[I], context);
    if constexpr (I + 1 != sizeof...(Tp))
        IterateOverTuple<I + 1>(t, v8args, context);
}

template<size_t I, size_t TOTAL, typename... Tp>
auto GetTuple(const v8::FunctionCallbackInfo<v8::Value>& v8args, v8::Local<v8::Context> context)
{
    if constexpr (TOTAL == 0)
    {
        return std::tuple<>();
    }
    else
    {
        using tup = std::tuple<Tp...>;
        using _Ty = typename std::tuple_element<I, tup>::type;

        std::tuple<_Ty> val{ GetValueAs<_Ty>(v8args[I], context) };
        if constexpr (I + 1 != sizeof...(Tp))
        {
            auto r = GetTuple<I + 1, TOTAL, Tp ...>(v8args, context);
            return std::tuple_cat(val, r);
        }
        else
        {
            return val;
        }
    }
}

template<int InternalField = 0, typename ClassType, bool deleteWithJsObj, typename ...Args>
void Constructor(const v8::FunctionCallbackInfo<v8::Value>& v8args)
{
    auto isolate = v8args.GetIsolate();

    if (!v8args.IsConstructCall())
    {
        isolate->ThrowException(JsString("Cannot call C++ constructor as JS function", isolate));
        return;
    }
    
    //v8::HandleScope scope(isolate);
    auto context = isolate->GetCurrentContext();
    auto _this = (v8args.This());

    constexpr std::size_t numArgs = sizeof...(Args);

    if (v8args.Length() < numArgs)
    {
        isolate->ThrowException(JsString("Too few args for constructor", isolate));
        return;
    }

    std::tuple<base_type<Args> ...> args;

    if constexpr (numArgs != 0)
        IterateOverTuple(args, v8args, context);

    std::tuple<v8::Isolate*, v8::Local<v8::Object>&, bool> pre = { isolate,_this,deleteWithJsObj };
    auto all = std::tuple_cat(pre, args);

    std::apply(NewCppObj<InternalField, ClassType, Args...>, std::move(all));

    v8args.GetReturnValue().Set(_this);
}

template<typename ClassType, bool deleteWithJsObj, typename ...Args>
void Constructor(const v8::FunctionCallbackInfo<v8::Value>& v8args)
{
    return Constructor<0, ClassType, deleteWithJsObj, Args...>(v8args);
}

template<typename Class, int InternalField = 0>
__forceinline Class GetFromV8(v8::Local<v8::Object> obj)
{
    void* ptr = obj->GetAlignedPointerFromInternalField(InternalField);
    return static_cast<Class>(ptr);
}


template <typename ClassType, typename ReturnType, typename... Args>
struct WrapMemberFunctionPtr
{
    using _Ptr = ReturnType(ClassType::*)(Args...);

    _Ptr ptr = 0;

    ReturnType operator()(ClassType* obj, Args&&... args)
    {
        return (obj->*ptr)(std::forward<Args>(args)...);
    }
};

template <typename ClassType, typename ReturnType, typename... Args>
struct WrapMemberFunctionPtrConst
{
    using _Ptr = ReturnType(ClassType::*)(Args...) const;

    _Ptr ptr = 0;

    ReturnType operator()(ClassType* obj, Args&&... args)
    {
        return (obj->*ptr)(std::forward<Args>(args)...);
    }
};


template<typename ...T, size_t... I>
auto makeReferencesHelper(std::tuple<T...>& t, std::index_sequence<I...>)
{
    return std::tie(*std::get<I>(t)...);
}

template<typename ...T>
auto makeReferences(std::tuple<T...>& t) {
    return makeReferencesHelper<T...>(t, std::make_index_sequence<sizeof...(T)>{});
}

template <template <typename, typename, typename...> typename WrapClass,
    typename ClassType, typename ReturnType, typename... Args>
    struct WrapMemberFunctionImpl
{
    using Ptr = typename WrapClass<ClassType, ReturnType, Args...>::_Ptr;

    template <int LambdaCallBackType, int InternalField, Ptr function>
    static void Call(const v8::FunctionCallbackInfo<v8::Value>& v8args)
    {
        if constexpr (LambdaCallBackType == 1)
        {
            v8::Local<v8::External> wrap = v8::Local<v8::External>::Cast(v8args.Data());
            auto* ptr = wrap->Value();
            auto _this = static_cast<ClassType*>(ptr);
            (*_this)(v8args);
        }
        else
        {
            auto isolate = v8args.GetIsolate();
            auto context = isolate->GetCurrentContext();
            auto v8this = (v8args.This());

            constexpr std::size_t numArgs = sizeof...(Args);

            if (v8args.Length() < numArgs)
            {
                isolate->ThrowException(JsString("Too few args for C++ member funtion call", isolate));
                return;
            }

            ClassType* _this;

            if constexpr (LambdaCallBackType == 2)
            {
                v8::Local<v8::External> wrap = v8::Local<v8::External>::Cast(v8args.Data());
                auto* ptr = wrap->Value();
                _this = static_cast<ClassType*>(ptr);
            }
            else
            {
                auto* ptr = v8this->GetAlignedPointerFromInternalField(0);
                _this = static_cast<ClassType*>(ptr);
            }


            /*std::tuple<base_type<Args> ...> args;

            if constexpr (numArgs != 0)
                IterateOverTuple(args, v8args, context);*/

            auto args = GetTuple<0, numArgs, Args ...>(v8args, context);

            std::tuple<ClassType*> pre = { _this };
            //std::tuple<ClassType*, base_type<Args> ...> all = std::tuple_cat(pre, args);
            auto all = std::tuple_cat(pre, args);

            if constexpr (std::is_same_v<ReturnType, void>)
            {
                std::apply(function, std::move(all));
                v8args.GetReturnValue().SetEmptyString();
            }
            else
            {
                ReturnType re = std::apply(function, std::move(all));

                using BaseReturnType = typename std::remove_const<typename std::remove_pointer<base_type<ReturnType>>::type>::type;
                //using BaseReturnType = typename base_type<ReturnType>::type;

                if constexpr (std::is_fundamental<BaseReturnType>::value)
                {
                    if constexpr (std::is_same_v<ReturnType, const char*>)
                    {
                        v8args.GetReturnValue().Set(JsString(re, isolate));
                    }
                    else
                    {
                        BaseReturnType ret = 0;

                        if constexpr (std::is_pointer_v<ReturnType>)
                        {
                            ret = *re;
                        }
                        else
                        {
                            ret = re;
                        }

                        if constexpr (std::is_same_v<ReturnType, int64_t>)
                        {
                            auto number = v8::BigInt::New(isolate, ret);
                            v8args.GetReturnValue().Set(number);
                        }
                        else if constexpr (std::is_same_v<ReturnType, uint64_t>)
                        {
                            auto number = v8::BigInt::NewFromUnsigned(isolate, ret);
                            v8args.GetReturnValue().Set(number);
                        }
                        else
                        {
                            v8args.GetReturnValue().Set(ret);
                        }
                    }

                }
                else
                {
                    //const char* id2 = typeid(ReturnType).name();

                    const char* id = typeid(BaseReturnType).name();

                    auto it = g_cppToJsType.find((void*)id);

                    if (it == g_cppToJsType.end())
                    {
                        std::string desc = id;
                        desc = "C++ \"" + desc + "\" is not wrapped to JS";
                        V8Throw(isolate, desc.c_str());
                        return;
                    }

                    auto& templ_ = it->second;

                    auto templ = templ_.Get(isolate);

                    auto newJsObj = templ->NewInstance(context).ToLocalChecked();

                    if constexpr (std::is_pointer_v<ReturnType>)
                    {
                        newJsObj->SetAlignedPointerInInternalField(0, re);
                        newJsObj->SetAlignedPointerInInternalField(1, (void*)typeid(BaseReturnType).name());
                    }
                    else
                    {
                        auto newCppObj = NewCppObj<BaseReturnType>(isolate, newJsObj, true);
                        *newCppObj = re;
                    }

                    v8args.GetReturnValue().Set(newJsObj);
                }
            }
        }
    }

};

template <typename ClassType, typename ReturnType, typename ... Args>
using WrapMemberFunction
= WrapMemberFunctionImpl<WrapMemberFunctionPtr, ClassType, ReturnType, Args...>;

template <typename ClassType, typename ReturnType, typename... Args>
using WrapMemberFunctionConst
= WrapMemberFunctionImpl<WrapMemberFunctionPtrConst, ClassType, ReturnType, Args... >;


template <typename ReturnType, typename... Args>
struct WrapFunction
{
    using Ptr = typename ReturnType(*)(Args...);

    template <int InternalField = 0, Ptr function>
    static void Call(const v8::FunctionCallbackInfo<v8::Value>& v8args)
    {
        auto isolate = v8args.GetIsolate();
        auto context = isolate->GetCurrentContext();
        auto v8this = (v8args.This());

        constexpr std::size_t numArgs = sizeof...(Args);

        if (v8args.Length() < numArgs)
        {
            isolate->ThrowException(JsString("Too few args for C++ funtion call", isolate));
            return;
        }

        /*std::tuple<base_type<Args> ...> args;

        if constexpr (numArgs != 0)
            IterateOverTuple(args, v8args, context);*/

        auto args = GetTuple<0, numArgs, Args ...>(v8args, context);

        if constexpr (std::is_same_v<ReturnType, void>)
        {
            std::apply(function, args);
            v8args.GetReturnValue().SetEmptyString();
        }
        else
        {
            ReturnType re = std::apply(function, args);

            using BaseReturnType = typename std::remove_const<typename std::remove_pointer<base_type<ReturnType>>::type>::type;
            //using BaseReturnType = typename base_type<ReturnType>::type;

            if constexpr (std::is_fundamental<BaseReturnType>::value)
            {
                if constexpr (std::is_same_v<ReturnType, const char*>)
                {
                    v8args.GetReturnValue().Set(JsString(re, isolate));
                }
                else
                {
                    BaseReturnType ret = 0;

                    if constexpr (std::is_pointer_v<ReturnType>)
                    {
                        ret = *re;
                    }
                    else
                    {
                        ret = re;
                    }

                    if constexpr (std::is_same_v<ReturnType, int64_t>)
                    {
                        auto number = v8::BigInt::New(isolate, ret);
                        v8args.GetReturnValue().Set(number);
                    }
                    else if constexpr (std::is_same_v<ReturnType, uint64_t>)
                    {
                        auto number = v8::BigInt::NewFromUnsigned(isolate, ret);
                        v8args.GetReturnValue().Set(number);
                    }
                    else
                    {
                        v8args.GetReturnValue().Set(ret);
                    }
                }

            }
            else
            {
                //const char* id2 = typeid(ReturnType).name();

                const char* id = typeid(BaseReturnType).name();

                auto it = g_cppToJsType.find((void*)id);

                if (it == g_cppToJsType.end())
                {
                    std::string desc = id;
                    desc = "C++ \"" + desc + "\" is not wrapped to JS";
                    V8Throw(isolate, desc.c_str());
                    return;
                }

                auto& _templ = it->second;

                auto templ = _templ.Get(isolate);

                auto newJsObj = templ->NewInstance(context).ToLocalChecked();

                if constexpr (std::is_pointer_v<ReturnType>)
                {
                    newJsObj->SetAlignedPointerInInternalField(0, re);
                    newJsObj->SetAlignedPointerInInternalField(1, (void*)typeid(BaseReturnType).name());
                }
                else
                {
                    auto newCppObj = NewCppObj<BaseReturnType>(isolate, newJsObj, true);
                    *newCppObj = re;
                }

                v8args.GetReturnValue().Set(newJsObj);
            }
        }
    }


};

template<typename T, T X>
struct ExtractType
{
};

template<typename T, T X, typename ReturnType, typename ClassType, typename ...Args >
struct ExtractType<ReturnType(ClassType::*)(Args...), X>
{
    using classType = ClassType;
    using returnType = ReturnType;
    //using args = Args;
    using Wrap = WrapMemberFunction<ClassType, ReturnType, Args...>;

    __forceinline static void Call(const v8::FunctionCallbackInfo<v8::Value>& info)
    {
        Wrap::Call<0, 0, X>(info);
    }

};

template<typename T, T X, typename ReturnType, typename ClassType, typename ...Args >
struct ExtractType<ReturnType(ClassType::*)(Args...) const, X>
{
    using classType = ClassType;
    using returnType = ReturnType;
    //using args = Args;
    using Wrap = WrapMemberFunctionConst<ClassType, ReturnType, Args...>;

    __forceinline static void Call(const v8::FunctionCallbackInfo<v8::Value>& info)
    {
        Wrap::Call<0, 0, X>(info);
    }

    __forceinline static void CallLambda1(const v8::FunctionCallbackInfo<v8::Value>& info)
    {
        Wrap::Call<1, 0, X>(info);
    }

    __forceinline static void CallLambda2(const v8::FunctionCallbackInfo<v8::Value>& info)
    {
        Wrap::Call<2, 0, X>(info);
    }
};

//for member variable setter, getter
template<typename T, T X, typename ReturnType, typename ClassType>
struct ExtractType<ReturnType(ClassType::*), X>
{
    using classType = ClassType;
    using returnType = ReturnType;

    template<typename Obj>
    static void Call(v8::Isolate* isolate, v8::Local<Obj>& instance, const char* jsSideMemberName)
    {
        instance->SetAccessor(JsString(jsSideMemberName, isolate), GetterClass<ClassType, X>, Setter<ClassType, X>);
    }

};

//for global variable setter, getter
template<typename T, T X, typename ReturnType>
struct ExtractType<ReturnType*, X>
{
    using returnType = ReturnType;

    template<typename Obj>
    static void Call(v8::Isolate* isolate, v8::Local<Obj>& instance, const char* jsSideMemberName)
    {
        if constexpr (std::is_same_v<Obj, v8::ObjectTemplate> || std::is_same_v<Obj, v8::FunctionTemplate>)
        {
            instance->SetNativeDataProperty(JsString(jsSideMemberName, isolate),
                GetterClassGlobal<X>, SetterGlobal<X>);
        }
        else
        {
            instance->SetAccessor(isolate->GetCurrentContext(), 
                JsString(jsSideMemberName, isolate), GetterClassGlobal<X>, SetterGlobal<X>);
        }
        
    }

};

template<typename T, T X, typename ReturnType, typename ...Args >
struct ExtractType<ReturnType(*)(Args...), X>
{
    using returnType = ReturnType;
    //using args = Args;
    using Wrap = WrapFunction<ReturnType, Args...>;

    __forceinline static void Call(const v8::FunctionCallbackInfo<v8::Value>& info)
    {
        Wrap::Call<0, X>(info);
    }

};


template<auto X>
void AddMemberFunction(v8::Isolate* isolate, v8::Local<v8::ObjectTemplate>& instance, const char* jsSideFuncName)
{
    using Ptr = decltype(X);
    instance->Set(isolate, jsSideFuncName, v8::FunctionTemplate::New(isolate, ExtractType<Ptr, X>::Call));
}

template<auto X, typename Obj>
void AddMemberVariable(v8::Isolate* isolate, v8::Local<Obj>& instance, const char* jsSideMemberName)
{
    using Ptr = decltype(X);
    ExtractType<Ptr, X>::template Call<Obj>(isolate, instance, jsSideMemberName);
}

//add function as static member function of clazz in js side
template<auto X, typename Obj>
void AddFunctionTempl(v8::Isolate* isolate, v8::Local<Obj>& clazz, const char* jsSideFuncName)
{
    using Ptr = decltype(X);

    if constexpr (std::is_same_v<Ptr, void(*)(const v8::FunctionCallbackInfo<v8::Value>&)>)
    {
        v8::Local<v8::FunctionTemplate> add_templ = v8::FunctionTemplate::New(isolate, X);
        clazz->Set(isolate, jsSideFuncName, add_templ);
    }
    else
    {
        v8::Local<v8::FunctionTemplate> add_templ = v8::FunctionTemplate::New(isolate, ExtractType<Ptr, X>::Call);
        clazz->Set(isolate, jsSideFuncName, add_templ);
    }
}

template<auto X, typename Obj>
void AddFunctionObj(v8::Isolate* isolate, v8::Local<Obj>& object, const char* jsSideFuncName)
{
    using Ptr = decltype(X);

    auto context = isolate->GetCurrentContext();

    if constexpr (std::is_same_v<Ptr, void(*)(const v8::FunctionCallbackInfo<v8::Value>&)>)
    {
        v8::Local<v8::Function> add_templ = v8::Function::New(context, X).ToLocalChecked();
        object->Set(context, JsString(jsSideFuncName, isolate), add_templ);
    }
    else
    {
        v8::Local<v8::Function> add_templ = v8::Function::New(context, ExtractType<Ptr, X>::Call).ToLocalChecked();
        object->Set(context, JsString(jsSideFuncName, isolate), add_templ);
    }
}

//add more function to js object
//like obj.funname = function(){ return 1; }; in js side
template<auto X, typename Obj>
void AddFunction(v8::Isolate* isolate, v8::Local<Obj>& object, const char* jsSideFuncName)
{
    if constexpr (std::is_same_v<Obj, v8::ObjectTemplate> || std::is_same_v<Obj, v8::FunctionTemplate>)
    {
        AddFunctionTempl<X, Obj>(isolate, object, jsSideFuncName);
    }
    else
    {
        AddFunctionObj<X, Obj>(isolate, object, jsSideFuncName);
    }
}

//template<void(*X)(const v8::FunctionCallbackInfo<v8::Value>&), typename Obj>
//void AddFunction(v8::Isolate* isolate, v8::Local<Obj>& object, const char* jsSideFuncName)
//{
//    auto context = isolate->GetCurrentContext();
//    v8::Local<v8::Function> add_templ = v8::Function::New(context, X).ToLocalChecked();
//    object->Set(context, JsString(jsSideFuncName, isolate), add_templ);
//}


template<class Signature>
struct signature_trait;

template<class R, class... Args>
struct signature_trait<R(Args...)>
{
    using type = std::tuple<R, Args...>;
};

template<class R, class... Args>
struct signature_trait<R(*)(Args...)>
{
    using type = std::tuple<R, Args...>;
};

template<class R, class U, class... Args>
struct signature_trait<R(U::*)(Args...)>
{
    using type = std::tuple<R, Args...>;
};

template<class R, class U, class... Args>
struct signature_trait<R(U::*)(Args...) const>
{
    using type = std::tuple<R, Args...>;
};

template<class Signature>
using signature_trait_t = typename signature_trait<Signature>::type;

template<class Signature1, class Signature2>
using is_same_signature = std::is_same<signature_trait_t<Signature1>,
    signature_trait_t<Signature2>>;

template<class Signature1, class Signature2>
inline constexpr bool is_same_signature_v =
is_same_signature<Signature1, Signature2>::value;



//add more function to js object
//like obj.funname = function(){ return 1; }; in js side
template<typename Lambda, typename Obj>
void AddLambdaFunctionObj(v8::Isolate* isolate, v8::Local<Obj>& object, const char* jsSideFuncName, Lambda& lambda)
{
    v8::Local<v8::ObjectTemplate> staticObjProto = v8::ObjectTemplate::New(isolate);

    using Ptr = decltype(&Lambda::operator());

    auto p = new Lambda(lambda);
    auto ext = v8::External::New(isolate, p);

    auto callback = new WeakCallBackInfoStruct<Lambda>();
    callback->ptr = p;
    auto& obj = callback->persistent;

    obj.Reset(isolate, ext);

    obj.SetWeak(callback,
        WeakCallBackInfoStruct<Lambda>::Destructor,
        v8::WeakCallbackType::kParameter
    );

    if constexpr (is_same_signature_v<Ptr, void(const v8::FunctionCallbackInfo<v8::Value>&)>)
    {
        //staticObjProto->SetCallAsFunctionHandler(ExtractType<Ptr, &Lambda::operator()>::CallLambda);
        staticObjProto->SetCallAsFunctionHandler(ExtractType<Ptr, &Lambda::operator()>::CallLambda1, ext);
    }
    else
    {
        staticObjProto->SetCallAsFunctionHandler(ExtractType<Ptr, &Lambda::operator()>::CallLambda2, ext);
    }

    auto context = isolate->GetCurrentContext();

    auto staticObj = staticObjProto->NewInstance(context).ToLocalChecked();

    object->Set(context, JsString(jsSideFuncName, isolate), staticObj);
}

//add function as static member function of clazz in js side
template<typename Lambda, typename Obj>
void AddLambdaFunctionTempl(v8::Isolate* isolate, v8::Local<Obj>& clazz, const char* jsSideFuncName, Lambda& lambda)
{
    v8::Local<v8::ObjectTemplate> staticObjProto = v8::ObjectTemplate::New(isolate);
    //staticObjProto->SetInternalFieldCount(1);

    using Ptr = decltype(&Lambda::operator());

    auto p = new Lambda(lambda);
    auto ext = v8::External::New(isolate, p);

    auto callback = new WeakCallBackInfoStruct<Lambda>();
    callback->ptr = p;
    auto& obj = callback->persistent;

    obj.Reset(isolate, ext);

    obj.SetWeak(callback,
        WeakCallBackInfoStruct<Lambda>::Destructor,
        v8::WeakCallbackType::kParameter
    );

    if constexpr (is_same_signature_v<Ptr, void(const v8::FunctionCallbackInfo<v8::Value>&)>)
    {
        staticObjProto->SetCallAsFunctionHandler(ExtractType<Ptr, &Lambda::operator()>::CallLambda1, ext);
    }
    else
    {
        staticObjProto->SetCallAsFunctionHandler(ExtractType<Ptr, &Lambda::operator()>::CallLambda2, ext);
    }

    clazz->Set(isolate, jsSideFuncName, staticObjProto);
}

template<typename Lambda, typename Obj>
void AddLambdaFunction(v8::Isolate* isolate, v8::Local<Obj>& clazz, const char* jsSideFuncName, Lambda& lambda)
{
    if constexpr (std::is_same_v<Obj, v8::ObjectTemplate> || std::is_same_v<Obj, v8::FunctionTemplate>)
    {
        AddLambdaFunctionTempl(isolate, clazz, jsSideFuncName, lambda);
    }
    else
    {
        AddLambdaFunctionObj(isolate, clazz, jsSideFuncName, lambda);
    }
}

//global or static variable
template<auto X, typename Obj>
void AddGlobalVariable(v8::Isolate* isolate, v8::Local<Obj>& instance, const char* jsSideMemberName)
{
    using Ptr = decltype(X);
    ExtractType<Ptr, X>::template Call<Obj>(isolate, instance, jsSideMemberName);
}

//end wrapper
//
//==============================================Some helper function===============================================================
// 
// 

struct FileTime
{
#ifndef _WIN32
    static_assert(0, "_SYSTEMTIME is win32 structure")
#endif // !_WIN32
    _SYSTEMTIME createTime;
    _SYSTEMTIME lastAccessTime;
    _SYSTEMTIME lastWriteTime;
};


struct ModuleInfo
{
    v8::Persistent<v8::Module> module;
    FileTime fileTime;

    //not own, just point to std::string in g_modules
    const char* path = 0;

    inline static void Destructor(const v8::WeakCallbackInfo<ModuleInfo>& data)
    {
        auto param = data.GetParameter();
        param->module.Reset();
        param->fileTime = {};

        //#ifdef _DEBUG
        std::cout << "Module \'" << param->path << "\' removed.\n";
        //#endif 

    }
};

//path, module
inline std::map<std::string, ModuleInfo*> g_modules;
inline std::string g_currentDir = "";

inline std::string WStringToString(const std::wstring& wstr)
{
    std::string convertedStr(wstr.size() * 2, 'c');

#ifndef _WIN32
    static_assert(0, "WideCharToMultiByte() is win32 api")
#endif // !_WIN32

    int convertResult = WideCharToMultiByte(CP_UTF8, 0, wstr.data(), wstr.length(),
        convertedStr.data(), convertedStr.length(), NULL, NULL);
    if (convertResult >= 0)
    {
        std::string result(&convertedStr[0], &convertedStr[convertResult]);
        //convertedStr[convertResult] = '\0';

        if (result.empty()) return convertedStr;

        return result;
    }
    else
    {
        return "";
    }

}

inline std::wstring StringToWString(const std::string& str)
{
    std::wstring re;
    re.resize(str.size());
    for (size_t i = 0; i < str.length(); i++)
    {
        re[i] = str[i];
    }

    return re;
}

inline FileTime GetFileTime(std::wstring path)
{
    auto fHandle = CreateFile(path.c_str(), GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);

    if (fHandle == NULL) return {};

    _FILETIME create;
    _FILETIME lastAccess;
    _FILETIME lastWrite;
    GetFileTime(fHandle, &create, &lastAccess, &lastWrite);

    FileTime re;

    _SYSTEMTIME createTime;
    FileTimeToSystemTime(&create, &createTime);
    SystemTimeToTzSpecificLocalTime(0, &createTime, &re.createTime);

    _SYSTEMTIME lastAccessTime;
    FileTimeToSystemTime(&lastAccess, &lastAccessTime);
    SystemTimeToTzSpecificLocalTime(0, &lastAccessTime, &re.lastAccessTime);

    _SYSTEMTIME lastWriteTime;
    FileTimeToSystemTime(&lastWrite, &lastWriteTime);
    SystemTimeToTzSpecificLocalTime(0, &lastWriteTime, &re.lastWriteTime);

    CloseHandle(fHandle);

    return re;
}

inline std::string ReadFile(const std::string& path)
{
    std::fstream fin(path);
    auto t = fin.imbue(std::locale("en_US.utf8"));
    t = fin.imbue(std::locale("en_US.utf8"));

    fin.seekg(0, std::ios::end);
    size_t size = fin.tellg();

    if (size == std::string::npos) return "File not found";

    std::string ret(size, ' ');
    fin.seekg(0);
    fin.read(&ret[0], size);
    fin.close();

    return ret;
}

inline v8::MaybeLocal<v8::Module> DirectImportModuleFromFile(const std::string& path, v8::Isolate* isolate)
{
    v8::ScriptOrigin origin(JsString(path, isolate),
        v8::Integer::New(isolate, 0),             // line offset
        v8::Integer::New(isolate, 0),             // column offset
        v8::False(isolate),                       // is cross origin
        v8::Local<v8::Integer>(),                 // script id
        v8::Local<v8::Value>(),                   // source map URL
        v8::False(isolate),                       // is opaque
        v8::False(isolate),                       // is WASM
        v8::True(isolate));                       //is module


    std::string fileContent = ReadFile(path);

    v8::ScriptCompiler::Source source(JsString(fileContent, isolate), origin);

    v8::Local<v8::Module> module;
    if (!v8::ScriptCompiler::CompileModule(isolate, &source).ToLocal(&module))
    {
        return v8::MaybeLocal<v8::Module>();
    }

    return v8::MaybeLocal<v8::Module>(module);
}

inline std::string PopPath(const std::string& path)
{
    auto id = path.rfind('/', path.size() - 2);
    return path.substr(0, id);
}

inline std::string CombinePath(std::string curDir, std::string relativePath)
{
    if (relativePath.empty()) return curDir;
    std::string begin = relativePath.substr(0, 3);

    int count = 0;
    if (begin[0] == '.' && begin[1] == '/')
    {
        relativePath.erase(relativePath.begin(), relativePath.begin() + 2);
    }
    else
    {
        while (begin == "../")
        {
            count++;
            relativePath.erase(relativePath.begin(), relativePath.begin() + 3);
            begin = relativePath.substr(0, 3);
        }
    }


    if (count == 0)
    {
        if (relativePath.find(":/") != std::wstring::npos)
        {
            return relativePath;
        }
        else
        {
            while (*(relativePath.begin()) == L'/')
            {
                relativePath.erase(relativePath.begin());
            }
        }
    }

    for (size_t i = 0; i < count; i++)
    {
        if (curDir.back() == L':') break;
        size_t index = curDir.find_last_of('/');
        curDir.erase(curDir.begin() + index, curDir.end());
    }

    return curDir.back() == '/' ? curDir + relativePath : curDir + "/" + relativePath;

}

inline bool IsFileModified(const std::string& path, FileTime* in, FileTime* out)
{
    auto fileTime = GetFileTime(StringToWString(path));

    if (::memcmp(&fileTime.lastWriteTime, &in->lastWriteTime, sizeof(in->lastWriteTime)) != 0)
    {
        if (out) *out = fileTime;
        return true;
    }
    if (out) *out = fileTime;
    return false;
}


v8::MaybeLocal<v8::Module> Resolve(v8::Local<v8::Context>, v8::Local<v8::String>, v8::Local<v8::Module>);


inline v8::MaybeLocal<v8::Module> ImportModuleFromFile(std::string path, v8::Isolate* isolate)
{
    if (GetExtension(path) != "js")
    {
        auto newPath = path + "/index.js";

        if (FileExist(newPath))
        {
            path = newPath;
        }
        else
        {
            V8Throw(isolate, ("File \'" + path + "'\ doesn't exist").c_str());
            return v8::MaybeLocal<v8::Module>();
        }
    }
    else if (!FileExist(path))
    {
        //args.GetReturnValue().Set(v8::Undefined(isolate));
        V8Throw(isolate, ("File \'" + path + "'\ doesn't exist").c_str());
        return v8::MaybeLocal<v8::Module>();
    }

    auto it = g_modules.find(path);
    FileTime fileTime = {};

    if (it != g_modules.end())
    {
        if (!IsFileModified(path, &it->second->fileTime, &fileTime))
        {
#ifdef _DEBUG
            std::cout << "Module \'" << path << "\' cached.\n";
#endif

            v8::Local<v8::Module> ret = v8::Local<v8::Module>::New(isolate, it->second->module);
            return v8::MaybeLocal<v8::Module>(ret);
        }
    }


    auto context = isolate->GetCurrentContext();
    v8::Local<v8::Module> module;
    v8::Local<v8::Value> result;

    std::string preCurDir = g_currentDir;
    g_currentDir = PopPath(path);

    v8::MaybeLocal<v8::Module> temp = DirectImportModuleFromFile(path, isolate);

    if (!temp.ToLocal(&module))
    {
        goto Failed;
    }

    if (module->InstantiateModule(context, Resolve).IsNothing())
    {
        goto Failed;
    }

    if (module->Evaluate(context).ToLocal(&result))
    {
        goto Succeeded;
    }
    else
    {
        goto Failed;
    }

Failed:
    g_currentDir = preCurDir;
    return v8::MaybeLocal<v8::Module>();

Succeeded:
    if (it == g_modules.end())
    {
        auto mod = new ModuleInfo();

        mod->module.Reset(isolate, module);
        mod->module.SetWeak(mod, ModuleInfo::Destructor, v8::WeakCallbackType::kParameter);

        mod->fileTime = GetFileTime(StringToWString(path));

        auto _it = g_modules.insert({ path, mod });

        mod->path = _it.first->first.c_str();
    }
    else
    {
        it->second->module.Reset(isolate, module);
        //it->second->module.SetWeak(it->second, ModuleInfo::Destructor, v8::WeakCallbackType::kParameter);

        it->second->fileTime = fileTime;
    }

    g_currentDir = preCurDir;
    return v8::MaybeLocal<v8::Module>(module);
}

inline v8::MaybeLocal<v8::Module> Resolve(v8::Local<v8::Context> context, v8::Local<v8::String> specifier, v8::Local<v8::Module> referrer)
{
    auto isolate = context->GetIsolate();
    v8::String::Utf8Value utf8(isolate, specifier);

    std::string modPath = *utf8;
    auto fullPath = CombinePath(g_currentDir, modPath);

    //std::cout << "Resolving module \'" << fullPath << "\'\n";
    auto temp = ImportModuleFromFile(fullPath, isolate);

    if (temp.IsEmpty())
    {
        //isolate->ThrowException(JsString("Import \'" + fullPath + "\' error.", isolate));
        std::cerr << "Import \'" << fullPath << "\' error.\n";
    }

    return temp;
}

inline bool ImportModule(v8::Local<v8::Module> module, v8::Isolate* isolate)
{
    auto context = isolate->GetCurrentContext();

    if (module->InstantiateModule(context, Resolve).IsNothing())
    {
        return false;
    }

    if (module->Evaluate(context).IsEmpty())
    {
        return false;
    }
    else
    {
        return true;
    }

    std::cout << "===============================Import done===================================\n";
}

inline void Import(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    auto isolate = args.GetIsolate();

    if (args.Length() == 0)
    {
        V8Throw(isolate, "Function call require 1 parameter.");
        return;
    }

    auto path = ToStdString(args[0], isolate);
    path = CombinePath(g_currentDir, path);

    if (GetExtension(path) != "js")
    {
        auto newPath = path + "/index.js";

        if (FileExist(newPath))
        {
            path = newPath;
        }
        else
        {
            V8Throw(isolate, ("File \'" + path + "'\ doesn't exist").c_str());
            return;
        }
    }
    else if (!FileExist(path))
    {
        //args.GetReturnValue().Set(v8::Undefined(isolate));
        V8Throw(isolate, ("File \'" + path + "'\ doesn't exist").c_str());
        return;
    }

    auto it = g_modules.find(path);
    FileTime fileTime = {};

    if (it != g_modules.end())
    {
        if (!IsFileModified(path, &it->second->fileTime, &fileTime))
        {
#ifdef _DEBUG
            std::cout << "Module \'" << path << "\' cached.\n";
#endif
            v8::Local<v8::Module> ret = v8::Local<v8::Module>::New(isolate, it->second->module);
            args.GetReturnValue().Set(ret->GetModuleNamespace());
            return;
        }
    }

    g_currentDir = PopPath(path);

    v8::TryCatch trycatch(isolate);

    v8::ScriptOrigin origin(JsString(path, isolate),
        v8::Integer::New(isolate, 0),             // line offset
        v8::Integer::New(isolate, 0),             // column offset
        v8::False(isolate),                       // is cross origin
        v8::Local<v8::Integer>(),                 // script id
        v8::Local<v8::Value>(),                   // source map URL
        v8::False(isolate),                       // is opaque
        v8::False(isolate),                       // is WASM
        v8::True(isolate));                       //is module

    v8::ScriptCompiler::Source source(JsString(ReadFile(path), isolate), origin);

    v8::Local<v8::Module> module;
    if (!v8::ScriptCompiler::CompileModule(isolate, &source).ToLocal(&module))
    {
        //V8TryCatch(trycatch, isolate);
        //args.GetReturnValue().Set(v8::Undefined(isolate));
        V8Throw(isolate, "Compile module failed");
        return;
    }

    if (!ImportModule(module, isolate))
    {
        //V8TryCatch(trycatch, isolate);
        //args.GetReturnValue().Set(v8::Undefined(isolate));
        V8Throw(isolate, "Import module failed");
        return;
    }

    if (it == g_modules.end())
    {
        auto mod = new ModuleInfo();

        mod->module.Reset(isolate, module);
        mod->module.SetWeak(mod, ModuleInfo::Destructor, v8::WeakCallbackType::kParameter);

        mod->fileTime = GetFileTime(StringToWString(path));

        auto _it = g_modules.insert({ path, mod });

        mod->path = _it.first->first.c_str();
    }
    else
    {
        it->second->module.Reset(isolate, module);
        //it->second->module.SetWeak(it->second, ModuleInfo::Destructor, v8::WeakCallbackType::kParameter);

        it->second->fileTime = fileTime;
    }

    auto target = module->GetModuleNamespace();
    args.GetReturnValue().Set(target);
}

inline void ClearModulesCache(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    g_modules.clear();
}

inline void GC(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    args.GetIsolate()->RequestGarbageCollectionForTesting(v8::Isolate::kFullGarbageCollection);
}

inline void Print(const v8::FunctionCallbackInfo<v8::Value>& args, std::ostream& out)
{
    auto isolate = args.GetIsolate();
    auto context = isolate->GetCurrentContext();
    auto v8this = args.This();

    for (int i = 0; i < args.Length(); i++)
    {
        auto a = args[i];
        v8::Local<v8::Value> strObj;

        if (a->IsUndefined())
        {
            strObj = v8::Undefined(isolate);
        }
        else if (a->IsNull())
        {
            strObj = v8::Null(isolate);
        }
        else
        {
            auto maybe = args[i]->ToObject(context);

            if (maybe.IsEmpty()) continue;

            auto cur = maybe.ToLocalChecked();

            auto toString = cur->Get(context, JsString("toString", isolate)).ToLocalChecked();

            if (!toString->IsFunction()) continue;
            auto callable = v8::Function::Cast(*toString);

            auto ret = callable->Call(context, cur, 0, 0);

            if (!ret.IsEmpty())
            {
                strObj = ret.ToLocalChecked();
            }
        }

        v8::String::Utf8Value str(isolate, strObj);
        const char* cstr = ToCString(str);
        out << cstr;
    }
    out << "\n";
    out.flush();
}

inline void PrintStdErr(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    Print(args, std::cerr);
}

inline void PrintStdOut(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    Print(args, std::cout);
}

inline void CppToJsString(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    //std::cout << "CppToJsString() called\n";
    auto re = GetFromV8<std::string*>(args.This());
    args.GetReturnValue().Set(JsString(re->c_str(), args.GetIsolate()));
}

template <typename Obj>
void WrapCppStdString(v8::Isolate* isolate, v8::Local<Obj> global)
{
    v8::Local<v8::FunctionTemplate> templ = v8::FunctionTemplate::New(isolate, Constructor<std::string, true, const char*>);
    RegisterCppJsClass<std::string>(isolate, templ);

    auto instance = templ->InstanceTemplate();
    instance->Set(isolate, "toString", v8::FunctionTemplate::New(isolate, CppToJsString));

    if constexpr (std::is_same_v<Obj, v8::ObjectTemplate> || std::is_same_v<Obj, v8::FunctionTemplate>)
    {
        global->Set(isolate, "StdString", templ);
    }
    else
    {
        auto context = isolate->GetCurrentContext();
        auto clazz = templ->GetFunction(context).ToLocalChecked();
        global->Set(context, JsString("StdString", isolate), clazz);
    }
    
}

template <typename Obj, typename TT>
void WrapClass(v8::Isolate* isolate, v8::Local<Obj> global, v8::Local<TT> templ, const char* name)
{
    if constexpr (std::is_same_v<Obj, v8::ObjectTemplate> || std::is_same_v<Obj, v8::FunctionTemplate>)
    {
        global->Set(isolate, name, templ);
    }
    else
    {
        auto context = isolate->GetCurrentContext();
        auto clazz = templ->GetFunction(context).ToLocalChecked();
        global->Set(context, JsString(name, isolate), clazz);
    }
}

inline auto CompileAndExec(v8::Local<v8::String> source, v8::Isolate* isolate)
{
    auto context = isolate->GetCurrentContext();

    //v8::Context::Scope context_scope(context);
    //v8::HandleScope handle_scope(context->GetIsolate());
    v8::TryCatch trycatch(isolate);

    auto maybecomplete = v8::Script::Compile(context, source);

    if (maybecomplete.IsEmpty())
    {
        if (V8TryCatch(trycatch, isolate))
        {
            return -1;
        }
    }

    v8::Local<v8::Script> script = maybecomplete.ToLocalChecked();

    // Run the script to get the result.

    auto maybelocal = script->Run(context);

    if (!maybelocal.IsEmpty())
    {
        v8::Local<v8::Value> result = maybelocal.ToLocalChecked();
        if (result.IsEmpty())
        {
            if (V8TryCatch(trycatch, isolate))
            {
                return -1;
            }
        }
    }
    else
    {
        if (V8TryCatch(trycatch, isolate))
        {
            return -1;
        }
    }

    return 0;
}

inline auto CompileAndExec(const char* _source, v8::Isolate* isolate)
{
    v8::Local<v8::String> source =
        v8::String::NewFromUtf8(isolate, _source,
            v8::NewStringType::kNormal)
        .ToLocalChecked();
    // Compile the source code.
    return CompileAndExec(source, isolate);
}

inline void Initialize(v8::Isolate* isolate, v8::Local<v8::ObjectTemplate> templ)
{
    g_hardCodedJsString.reserve(HARD_CODED_JS_STRING_MAX_LENGTH);

    auto t = std::wcin.imbue(std::locale("en_US.utf8"));
    t = std::wcout.imbue(std::locale("en_US.utf8"));
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);

    AddFunction<&Import>(isolate, templ, "Import");
    AddFunction<&ClearModulesCache>(isolate, templ, "ClearModulesCache");
    AddFunction<&GC>(isolate, templ, "GC");
    AddFunction<&PrintStdOut>(isolate, templ, "print");

    v8::Local<v8::ObjectTemplate> globalConsole = v8::ObjectTemplate::New(isolate);

    AddFunction<&PrintStdOut>(isolate, globalConsole, "log");
    AddFunction<&PrintStdErr>(isolate, globalConsole, "warn");
    AddFunction<&PrintStdErr>(isolate, globalConsole, "error");

    
    AddFunction<&___ConsoleClear>(isolate, globalConsole, "clear");

    templ->Set(isolate, "___console", globalConsole);

    WrapCppStdString(isolate, templ);

    wchar_t cpath[MAX_PATH] = {};
    GetCurrentDirectory(MAX_PATH, cpath);
    std::wstring wcpath = cpath;
    g_currentDir = WStringToString(wcpath);
    std::replace(g_currentDir.begin(), g_currentDir.end(), '\\', '/');
}

inline void Initialize(v8::Isolate* isolate, v8::Local<v8::Object> global)
{
    g_hardCodedJsString.reserve(HARD_CODED_JS_STRING_MAX_LENGTH);

    auto t = std::wcin.imbue(std::locale("en_US.utf8"));
    t = std::wcout.imbue(std::locale("en_US.utf8"));
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);

    AddFunction<&Import>(isolate, global, "Import");
    AddFunction<&GC>(isolate, global, "GC");
    AddFunction<&PrintStdOut>(isolate, global, "print");

    v8::Local<v8::FunctionTemplate> globalConsole = v8::FunctionTemplate::New(isolate);

    AddFunction<&PrintStdOut>(isolate, globalConsole, "log");
    AddFunction<&PrintStdErr>(isolate, globalConsole, "warn");
    AddFunction<&PrintStdErr>(isolate, globalConsole, "error");

    auto context = isolate->GetCurrentContext();

    auto clazz = globalConsole->GetFunction(context).ToLocalChecked();
    global->Set(context, JsString("___console", isolate), clazz);

    WrapCppStdString(isolate, global);

    wchar_t cpath[MAX_PATH] = {};
    GetCurrentDirectory(MAX_PATH, cpath);
    std::wstring wcpath = cpath;
    g_currentDir = WStringToString(wcpath);
    std::replace(g_currentDir.begin(), g_currentDir.end(), '\\', '/');
}

inline void Finalize(v8::Isolate* isolate)
{
    g_hardCodedJsString.clear();
    g_cppToJsType.clear();
    //g_modules.clear();
}

inline void NewRunTime(v8::Isolate* isolate)
{
    g_hardCodedJsString.clear();
    //g_cppToJsType.clear();
    g_modules.clear();
    for (auto& ptr : g_cppToJsTypeCache)
    {
        *ptr = 0;
    }
}

}

//for cache
#define V8WRAPPER_TRY_GET_REGISTED_CPP_JS_CLASS_RAW_PTR(className, output)  \
if(!output) {const char* _id = typeid(className).name();                    \
auto _it = v8wrapper::g_cppToJsType.find((void*)_id);                       \
if (_it == v8wrapper::g_cppToJsType.end())                                  \
{                                                                           \
    assert(0);                                                              \
}                                                                           \
auto& _templ = _it->second;                                                 \
auto _templObj = *(_templ.Get(isolate));                                    \
v8wrapper::g_cppToJsTypeCache.insert(&_templObj); output = _templObj;}


#ifndef V8_LIB_DIR
#define V8_LIB_DIR ""
#endif 

#ifdef V8_DLL
#pragma comment(lib, V8_LIB_DIR "v8.dll.lib")
#pragma comment(lib, V8_LIB_DIR "v8_libbase.dll.lib")
#pragma comment(lib, V8_LIB_DIR "v8_libplatform.dll.lib")
//#pragma comment(lib, V8_LIB_DIR "zlib.dll.lib")
#endif


#ifdef V8_MONOLITH_STATIC
#pragma comment(lib, V8_LIB_DIR "winmm.lib") 
#pragma comment(lib, V8_LIB_DIR "dbghelp.lib") 
#pragma comment(lib, V8_LIB_DIR "shlwapi.lib")
#pragma comment(lib, V8_LIB_DIR "v8_monolith.lib") 
#endif

#ifdef V8_STATIC
#pragma comment(lib, V8_LIB_DIR "winmm.lib") 
#pragma comment(lib, V8_LIB_DIR "dbghelp.lib") 
#pragma comment(lib, V8_LIB_DIR "shlwapi.lib")
#pragma comment(lib, V8_LIB_DIR "v8_compiler.lib") 
#pragma comment(lib, V8_LIB_DIR "v8_libbase.lib") 
#pragma comment(lib, V8_LIB_DIR "v8_snapshot.lib") 
#pragma comment(lib, V8_LIB_DIR "v8_libplatform.lib") 
#pragma comment(lib, V8_LIB_DIR "v8_libsampler.lib") 
#pragma comment(lib, V8_LIB_DIR "v8_init.lib") 
#pragma comment(lib, V8_LIB_DIR "v8_initializers.lib") 
#pragma comment(lib, V8_LIB_DIR "v8_zlib.lib") 
#pragma comment(lib, V8_LIB_DIR "icudata.lib") 
#pragma comment(lib, V8_LIB_DIR "icutools.lib") 
#pragma comment(lib, V8_LIB_DIR "icuucx.lib") 
#pragma comment(lib, V8_LIB_DIR "icui18n.lib") 
#pragma comment(lib, V8_LIB_DIR "v8_base_without_compiler.lib") 
#endif


#ifdef V8_MONOLITH_DLL
#pragma comment(lib, "v8_monolith.lib")
#endif
