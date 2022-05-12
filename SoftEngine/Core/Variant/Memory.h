#pragma once

#include <vector>

#if defined(MEMORY_USE_PRINTER)
#include <string>
#endif

#include <cassert>

///
/// //to use Memory::ToString()
/// #define MEMORY_USE_PRINTER 1 
/// 
/// usage example:
/// 
/// void Function()
/// {
///	    Memory m1 = Memory::New<Mat4x4>();
///	    auto& mat = m1.As<Mat4x4>();
/// 
///	    // use mat as Mat4x4 here
///	    mat.SetIdentity();
///	    ...
/// 
///	    // destructor of Mat4x4 called automatically
///	    auto& vec = m1.Malloc<Vec3>();
/// }
/// 
/// // destructor of Vec3 called automatically
/// 
class Memory
{
public:
    using ThisClass = Memory;

private:
    std::vector<int8_t> m_mem;
    void (*m_destructor)(void*) = 0;

#if defined(MEMORY_USE_PRINTER)
    std::string(*m_printer)(void*) = 0;
#endif // _DEBUG

#if defined(_DEBUG)
    const char* m_currentClassName;

#define DEBUG_CHECK() assert(m_currentClassName == typeid(T).name())

#else

#define DEBUG_CHECK()

#endif // _DEBUG

public:
    ~Memory()
    {
        CallDestructor();
    };

private:
    inline bool IsUnderlyingBufferTooLarge()
    {
        // 128KB
        return m_mem.capacity() > 128 * 1024;
    };

    inline void CallDestructor()
    {
        if (m_destructor) m_destructor(this);
        m_destructor = 0;
    };

    inline void _Malloc(size_t size)
    {
        m_mem.reserve(size);
        m_mem.resize(size);
        if (IsUnderlyingBufferTooLarge()) ShrinkToFit();
    }

public:
    // alloc this memory as a type, automatic call destructor of type when Memory deleted
    template <typename T, typename... Args>
    inline T& Malloc(Args&&... args)
    {
        CallDestructor();
        _Malloc(sizeof(T));


        m_destructor = [](void* ptr)
        {
            ThisClass* _this = reinterpret_cast<ThisClass*>(ptr);
            T& obj = _this->As<T>();
            obj.~T();
        };


#if defined(MEMORY_USE_PRINTER)
        m_printer = [](void* ptr)
        {
            ThisClass* _this = reinterpret_cast<ThisClass*>(ptr);
            T& obj = _this->As<T>();
            std::stringstream ss;
            ss << obj;
            return ss.str();
        };
#endif


#if defined(_DEBUG)
        m_currentClassName = typeid(T).name();
#endif // _DEBUG


        new (Ptr<T>()) T(std::forward<Args>(args)...);

        return As<T>();
    };

    template <typename T, typename... Args>
    inline T* MallocArray(size_t arraySize, Args&&... args)
    {
        assert(arraySize != 0);

        CallDestructor();
        _Malloc(sizeof(T) * arraySize);


        m_destructor = [](void* ptr)
        {

            ThisClass* _this = reinterpret_cast<ThisClass*>(ptr);
            T* objs = _this->AsArray<T>();
            size_t count = _this->GetArraySize<T>();

            for (size_t i = 0; i < count; i++)
            {
                auto& obj = objs[i];
                obj.~T();
            }
        };


#if defined(MEMORY_USE_PRINTER)
        m_printer = [](void* ptr)
        {
            std::stringstream ss;

            ss << "[";

            ThisClass* _this = reinterpret_cast<ThisClass*>(ptr);
            T* objs = _this->AsArray<T>();
            size_t count = _this->GetArraySize<T>();

            for (size_t i = 0; i < count; i++)
            {
                auto& obj = objs[i];
                ss << obj << ", ";
            }

            std::string str = ss.str();
            str.resize(str.size() - 2);
            str += ']';

            return str;
        };
#endif


#if defined(_DEBUG)
        m_currentClassName = typeid(T).name();
#endif // _DEBUG


        T* objs = AsArray<T>();
        size_t count = GetArraySize<T>();

        for (size_t i = 0; i < count; i++)
        {
            auto& obj = objs[i];
            new (&obj) T(std::forward<Args>(args)...);
        }

        return AsArray<T>();
    };

    template <typename T, typename C, typename... Args>
    inline void ForEach(C forEachCall, Args&&... args)
    {
        DEBUG_CHECK();

        T* objs = AsArray<T>();
        size_t count = GetArraySize<T>();

        for (size_t i = 0; i < count; i++)
        {
            auto& obj = objs[i];
            forEachCall(obj, i, std::forward<Args>(args)...);
        }
    };

public:
    // helper constructor
    template <typename T, typename... Args>
    inline static Memory New(Args&&... args)
    {
        Memory mem;
        mem.Malloc<T>(std::forward<Args>(args)...);
        return mem;
    };

    template <typename T, typename... Args>
    inline static Memory NewArray(size_t arraySize, Args&&... args)
    {
        Memory mem;
        mem.MallocArray<T>(arraySize, std::forward<Args>(args)...);
        return mem;
    };


public:
    template <typename T>
    inline T& As()
    {
        DEBUG_CHECK();
        return *reinterpret_cast<T*>(m_mem.data());
    };

    template <typename T>
    inline T* Ptr()
    {
        DEBUG_CHECK();
        return reinterpret_cast<T*>(m_mem.data());
    };

    template <typename T>
    inline T* AsArray()
    {
        DEBUG_CHECK();
        return reinterpret_cast<T*>(m_mem.data());
    };

    template <typename T>
    inline size_t GetArraySize()
    {
        DEBUG_CHECK();
        return m_mem.size() / sizeof(T);
    };

    inline size_t Size()
    {
        return m_mem.size();
    };

    inline void ShrinkToFit()
    {
        m_mem.shrink_to_fit();
    };

#if defined(MEMORY_USE_PRINTER)
    std::string ToString()
    {
        return m_printer(this);
    };
#endif

#ifdef DEBUG_CHECK
#undef DEBUG_CHECK
#endif // DEBUG_CHECK

    
};