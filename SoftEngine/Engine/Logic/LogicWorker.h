#pragma once

#include "Core/MultiThreading/AsyncTasksRunner.h"
//#include "Core/MultiThreading/SynchronizableObject.h"

class RefCounted;

//proces input, physics, user script, ...
//everything relate to logic

class LOGIC_TASK_HINT
{
public:
    enum ENUM
    {
        NOTHING,

        //...

        COUNT
    };
};

class LogicWorker :
    public AsyncTasksRunner<LOGIC_TASK_HINT::COUNT, LOGIC_TASK_HINT::ENUM>
{
private:
    friend class Engine;
    friend class Input;

public:
    bool m_isRunning = 1;
    //size_t m_frameCount = 0;

    Engine* m_engine = 0;
    Input* m_input = 0;
    float m_fdeltaTime = 0;

    friend class SceneQueryContext;
    friend class SceneQueriedNode;

    using LightID = uint32_t;

    SceneQueryContext* m_queryContext = 0;

    struct Data;

    Data* m_data = 0;

public:
    LogicWorker(Engine* engine);
    ~LogicWorker();

public:
    void Update();
    void LastUpdate();
    //void UpdateRefCounted();

    void Idling();

    inline auto& FDeltaTime() { return m_fdeltaTime; };
    inline auto& IsRunning() { return m_isRunning; };

    inline auto GetSceneQueryContext() { return m_queryContext; };
};