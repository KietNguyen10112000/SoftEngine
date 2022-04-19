#pragma once

#include "Core/MultiThreading/AsyncTasksRunner.h"
//#include "Core/MultiThreading/SynchronizableObject.h"


//render model, audio, ...
//everything relate to output will run in this thread

//HINT for logic thread call
//logic worker will call async tasks

class RENDERING_TASK_HINT
{
public:
    enum ENUM
    {
        RUN_AT_BEGIN_FRAME,
        RUN_BEFORE_POST_PROCESSING,
        RUN_BEFORE_PRESENT_TO_SCREEN,
        RUN_AT_END_FRAME,

        RUN_AUDIO,

        //...

        COUNT
    };
};

class RENDERING_MODE
{
public:
    enum MODE
    {
        REALTIME,
        MANUALLY_REFRESH
    };
};

class RenderingWorker :
    public AsyncTasksRunner<RENDERING_TASK_HINT::COUNT, RENDERING_TASK_HINT::ENUM>
{
private:
    friend class Engine;
    friend class IRenderer;

public:
    bool m_isRunning = 1;
    //size_t m_frameCount = 0;

    //not own
    Engine* m_engine = 0;
    float m_fdeltaTime = 0;

    //not own
    IRenderer* m_renderer = 0;

    friend class SceneQueryContext;
    friend class SceneQueriedNode;
    friend class IRenderableObject;
    friend class AABBRenderer;

    using LightID = uint32_t;

    SceneQueryContext* m_queryContext = 0;

    using NodeId = size_t;
    std::vector<NodeId> m_dataNodes;
    std::vector<IRenderableObject*> m_renderableObjects;
    std::vector<LightID> m_lightObjects;

    AABBRenderer* m_aabbRenderer = 0;

    RENDERING_MODE::MODE m_renderingMode = RENDERING_MODE::REALTIME;
    bool m_needReRender = true;

public:
    RenderingWorker(Engine* engine);
    ~RenderingWorker();

public:
    void Update();

    inline auto& FDeltaTime() { return m_fdeltaTime; };
    inline auto& IsRunning() { return m_isRunning; };


    inline auto& RenderingMode() { return m_renderingMode; };

    void Refresh();
};