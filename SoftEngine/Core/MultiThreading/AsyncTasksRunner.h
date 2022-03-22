#pragma once

#include <vector>
#include <functional>
#include <mutex>

#ifndef _ASSERT
static_assert("define assert");
#endif // !_ASSERT


template <typename size_t TASK_HINT_COUNT, typename HintEnum>
class AsyncTasksRunner
{
public:
    template <typename T>
    struct Task
    {
        T callback;
    };

    using LambdaTaskCallback = std::function<void()>;
    using LambdaTask = Task<LambdaTaskCallback>;

    using DefaultTaskCallback = void(*)(void*);
    struct DefaultTask : public Task<DefaultTaskCallback>
    {
        //where you store data to call in callback
        void* opaque;
    };

    using LambdaTaskList = std::vector<LambdaTask>;
    using DefaultTaskList = std::vector<DefaultTask>;

    std::mutex m_mutex;
    LambdaTaskList m_lambdaTasks[TASK_HINT_COUNT];
    DefaultTaskList m_defaultTasks[TASK_HINT_COUNT];

    LambdaTaskList m_copylambdaTasks[TASK_HINT_COUNT];
    DefaultTaskList m_copydefaultTasks[TASK_HINT_COUNT];

public:
    inline void RunAsync(LambdaTaskCallback callback, HintEnum hint)
    {
#ifdef _DEBUG
        _ASSERT(hint < TASK_HINT_COUNT);
#endif // _DEBUG

        m_mutex.lock();
        m_lambdaTasks[hint].push_back({ callback });
        m_mutex.unlock();
    };

    inline void RunAsync(DefaultTaskCallback callback, HintEnum hint, void* opaque = 0)
    {
#ifdef _DEBUG
        _ASSERT(hint < TASK_HINT_COUNT);
#endif // _DEBUG
        DefaultTask task;
        task.callback = callback;
        task.opaque = opaque;

        m_mutex.lock();
        m_defaultTasks[hint].push_back(task);
        m_mutex.unlock();
    };

    inline void RunSynch(HintEnum hint)
    {
#ifdef _DEBUG
        _ASSERT(hint < TASK_HINT_COUNT);
#endif // _DEBUG

        auto& ltasks = m_lambdaTasks[hint];
        auto cltasks = m_copylambdaTasks[hint];

        auto& dtasks = m_defaultTasks[hint];
        auto cdtasks = m_copydefaultTasks[hint];


        // do copy
        m_mutex.lock();
        
        for (auto& task : ltasks)
        {
            //task.callback();
            cltasks.push_back(task);
        }
        ltasks.clear();

        for (auto& task : dtasks)
        {
            //task.callback(task.opaque);
            cdtasks.push_back(task);
        }
        dtasks.clear();

        m_mutex.unlock();


        // run
        for (auto& task : cltasks)
        {
            task.callback();
        }
        cltasks.clear();

        for (auto& task : cdtasks)
        {
            task.callback(task.opaque);
        }
        cdtasks.clear();

    };
};