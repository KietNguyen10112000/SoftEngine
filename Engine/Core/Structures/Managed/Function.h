#pragma once

#include "Core/TypeDef.h"

#include "Core/Memory/Trace.h"

#include <tuple>

NAMESPACE_BEGIN

#define DEFINE_TRACE(_args, prevExtCode, postExtCode)                                   \
template <typename _Elm>                                                                \
void TraceToElm(Tracer* tracer, _Elm& e)                                                \
{                                                                                       \
    if constexpr (std::is_base_of_v<Traceable<_Elm>, _Elm>)                             \
    {                                                                                   \
        tracer->Trace(e);                                                               \
    }                                                                                   \
}                                                                                       \
TRACEABLE_FRIEND();                                                                     \
void Trace(Tracer* tracer)                                                              \
{                                                                                       \
    prevExtCode;                                                                        \
    std::apply([&](auto&&... arg) {((TraceToElm(tracer, arg)), ...); }, _args);         \
    postExtCode;                                                                        \
}

namespace helper
{
    template <int... Is>
    struct index {};

    template <int N, int... Is>
    struct gen_seq : gen_seq<N - 1, N - 1, Is...> {};

    template <int... Is>
    struct gen_seq<0, Is...> : index<Is...> {};
}

class FunctionBase
{
public:
    virtual void Invoke() = 0;

};

template <typename _Fn, typename... _Ts>
class Function final : private Traceable<Function<_Fn, _Ts...>>, public FunctionBase
{
private:
    _Fn m_fn;
    std::tuple<_Ts...> m_args;

public:
    Function(_Fn fn, _Ts&&... args) : m_fn(fn), m_args(std::forward<_Ts>(args)...) {};

private:
    DEFINE_TRACE(m_args);

    template <int... Is>
    void func(std::tuple<_Ts...>& tup, helper::index<Is...>)
    {
        m_fn(std::get<Is>(tup)...);
    }

    void func(std::tuple<_Ts...>& tup)
    {
        func(tup, helper::gen_seq<sizeof...(_Ts)>{});
    }

public:
    virtual void Invoke() override
    {
        func(m_args);
    }

};

template <typename R, typename... Args>
class AsyncFunction final : private Traceable<AsyncFunction<R, Args...>>, public FunctionBase
{
private:
    class CallbackBase
    {
    public:
        virtual void Invoke(R) = 0;

    };

    template <typename _Fn, typename... _Ts>
    class Callback final : private Traceable<Callback<_Fn, _Ts...>>, public CallbackBase
    {
    private:
        _Fn m_fn;
        std::tuple<_Ts...> m_args;

    public:
        Callback(_Fn fn, _Ts&&... args) : m_fn(fn), m_args(std::forward<_Ts>(args)...) {};

    private:
        DEFINE_TRACE(m_args);

        template <int... Is>
        void func(R r, std::tuple<_Ts...>& tup, helper::index<Is...>)
        {
            m_fn(r, std::get<Is>(tup)...);
        }

        void func(R r, std::tuple<_Ts...>& tup)
        {
            func(r, tup, helper::gen_seq<sizeof...(_Ts)>{});
        }

    public:
        virtual void Invoke(R r) override
        {
            func(r, m_args);
        }

    };

    using Fn = R(*)(Args...);


    Handle<CallbackBase> m_callback = nullptr;

    Fn m_fn = nullptr;
    std::tuple<Args...> m_args;

public:
    AsyncFunction(Fn fn, Args&&... args)
        : m_fn(fn), m_args(std::forward<Args>(args)...) {};

private:
    DEFINE_TRACE(
        m_args,
        tracer->Trace(m_callback);
    );

    template <int... Is>
    R func(std::tuple<Args...>& tup, helper::index<Is...>)
    {
        return m_fn(std::get<Is>(tup)...);
    }

    R func(std::tuple<Args...>& tup)
    {
        return func(tup, helper::gen_seq<sizeof...(Args)>{});
    }

public:
    virtual void Invoke() override
    {
        auto r = func(m_args);
        if (m_callback.Get())
        {
            m_callback->Invoke(r);
        }
    }

    template <typename _Fn, typename... _Ts>
    auto Then(_Fn callback, _Ts&&... args)
    {
        m_callback = mheap::New<Callback<_Fn, _Ts...>>(callback, std::forward<_Ts>(args)...);
        return this;
    }
};

template <typename _Fn, typename... _Ts>
class AsyncFunctionVoidReturn final : private Traceable<AsyncFunctionVoidReturn<_Fn, _Ts...>>, public FunctionBase
{
private:
    Handle<FunctionBase> m_callback = nullptr;

    _Fn m_fn;
    std::tuple<_Ts...> m_args;

public:
    AsyncFunctionVoidReturn(_Fn fn, _Ts&&... args) : m_fn(fn), m_args(std::forward<_Ts>(args)...) {};

private:
    DEFINE_TRACE(
        m_args,
        tracer->Trace(m_callback);
    );

    template <int... Is>
    void func(std::tuple<_Ts...>& tup, helper::index<Is...>)
    {
        m_fn(std::get<Is>(tup)...);
    }

    void func(std::tuple<_Ts...>& tup)
    {
        func(tup, helper::gen_seq<sizeof...(_Ts)>{});
    }

public:
    virtual void Invoke() override
    {
        func(m_args);
        if (m_callback.Get())
        {
            m_callback->Invoke();
        }
    }

    template <typename _Fn, typename... _Ts>
    auto Then(_Fn callback, _Ts&&... args)
    {
        m_callback = mheap::New<Function<_Fn, _Ts...>>(callback, std::forward<_Ts>(args)...);
        return this;
    }
};

template <typename T, typename R, typename... Args>
class AsyncMemberFunction final : private Traceable<AsyncMemberFunction<T, R, Args...>>, public FunctionBase
{
private:
    class CallbackBase
    {
    public:
        virtual void Invoke(R) = 0;

    };

    template <typename _Fn, typename... _Ts>
    class Callback final : private Traceable<Callback<_Fn, _Ts...>>, public CallbackBase
    {
    private:
        _Fn m_fn;
        std::tuple<_Ts...> m_args;

    public:
        Callback(_Fn fn, _Ts&&... args) : m_fn(fn), m_args(std::forward<_Ts>(args)...) {};

    private:
        DEFINE_TRACE(m_args);

        template <int... Is>
        void func(R r, std::tuple<_Ts...>& tup, helper::index<Is...>)
        {
            m_fn(r, std::get<Is>(tup)...);
        }

        void func(R r, std::tuple<_Ts...>& tup)
        {
            func(r, tup, helper::gen_seq<sizeof...(_Ts)>{});
        }

    public:
        virtual void Invoke(R r) override
        {
            func(r, m_args);
        }

    };

    using Fn = R(T::*)(Args...);


    Handle<CallbackBase> m_callback = nullptr;

    T* m_obj = nullptr;
    Fn m_fn = nullptr;
    std::tuple<Args...> m_args;

public:
    AsyncMemberFunction(T* obj, Fn fn, Args&&... args)
        : m_obj(obj), m_fn(fn), m_args(std::forward<Args>(args)...) {};

private:
    DEFINE_TRACE(
        m_args,
        tracer->Trace(m_callback);
    );

    template <int... Is>
    R func(std::tuple<Args...>& tup, helper::index<Is...>)
    {
        return (m_obj->*m_fn)(std::get<Is>(tup)...);
    }

    R func(std::tuple<Args...>& tup)
    {
        return func(tup, helper::gen_seq<sizeof...(Args)>{});
    }

public:
    virtual void Invoke() override
    {
        auto r = func(m_args);
        if (m_callback.Get())
        {
            m_callback->Invoke(r);
        }
    }

    template <typename _Fn, typename... _Ts>
    auto Then(_Fn callback, _Ts&&... args)
    {
        m_callback = mheap::New<Callback<_Fn, _Ts...>>(callback, std::forward<_Ts>(args)...);
        return this;
    }
};

template <typename T, typename... Args>
class AsyncMemberFunctionVoidReturn final : private Traceable<AsyncMemberFunctionVoidReturn<T, Args...>>, public FunctionBase
{
private:
    using Fn = void (T::*)(Args...);

    Handle<FunctionBase> m_callback = nullptr;

    T* m_obj = nullptr;
    Fn m_fn = nullptr;
    std::tuple<Args...> m_args;

public:
    AsyncMemberFunctionVoidReturn(T* obj, Fn fn, Args&&... args)
        : m_obj(obj), m_fn(fn), m_args(std::forward<Args>(args)...) {};

private:
    DEFINE_TRACE(
        m_args,
        tracer->Trace(m_callback);
    );

    template <int... Is>
    void func(std::tuple<Args...>& tup, helper::index<Is...>)
    {
        (m_obj->*m_fn)(std::get<Is>(tup)...);
    }

    void func(std::tuple<Args...>& tup)
    {
        func(tup, helper::gen_seq<sizeof...(Args)>{});
    }

public:
    virtual void Invoke() override
    {
        func(m_args);
        if (m_callback.Get())
        {
            m_callback->Invoke();
        }
    }

    template <typename _Fn, typename... _Ts>
    auto Then(_Fn callback, _Ts&&... args)
    {
        m_callback = mheap::New<Function<_Fn, _Ts...>>(callback, std::forward<_Ts>(args)...);
        return this;
    }
};


template <typename T, typename R, typename... Args>
auto MakeAsyncMemberFunction(T* obj, R (T::* fn)(Args...), Args&&... args)
{
    return mheap::New<AsyncMemberFunction<T, R, Args...>>(obj, fn, std::forward<Args>(args)...);
}

template <typename T, typename... Args>
auto MakeAsyncMemberFunction(T* obj, void (T::* fn)(Args...), Args&&... args)
{
    return mheap::New<AsyncMemberFunctionVoidReturn<T, Args...>>(obj, fn, std::forward<Args>(args)...);
}

//template <typename R, typename... Args>
//auto MakeAsyncFunction_(R (*fn)(Args...), Args&&... args)
//{
//    return new AsyncFunction(fn, std::forward<Args>(args)...);
//}

// to capture variable, using args
template <typename Fn, typename... Args>
auto MakeAsyncFunction(Fn fn, Args&&... args)
{
    using return_type = std::invoke_result_t<Fn, Args...>;
    //return_type (*_fn)(Args...) = fn;

    if constexpr (std::is_same_v<return_type, void>)
    {
        return mheap::New<AsyncFunctionVoidReturn<Fn, Args...>>(fn, std::forward<Args>(args)...);
    }
    else
    {
        return mheap::New<AsyncFunction<Fn, return_type, Args...>>(fn, std::forward<Args>(args)...);
    }
}

#undef DEFINE_TRACE

NAMESPACE_END