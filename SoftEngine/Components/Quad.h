#pragma once

#include <Buffer.h>
#include <Math/Math.h>


//must delete yourself
inline auto NewScreenRectangle(float uMax = 1, float vMax = 1)
{
    struct __TempVertex
    {
        Vec3 position;
        Vec2 textcoord;
    };
    __TempVertex vert[6] =
    {
        //first triangle
        {{-1, -1, 0}, {0, vMax}},
        {{-1, 1, 0}, {0, 0}},
        {{1, 1, 0}, {uMax, 0}},

        //2nd triangle
        {{1, 1, 0}, {uMax, 0}},
        {{1, -1, 0}, {uMax, vMax}},
        {{-1, -1, 0}, {0, vMax}}

    };
    return new VertexBuffer(vert, 6, sizeof(__TempVertex));
}


inline static VertexBuffer* g_screenVertexBuffer = 0;
inline static size_t g_screenVertexBufferRefCount = 0;
//remember ReleaseScreenVertexBuffer()
inline auto GetScreenVertexBuffer()
{
    struct __TempVertex
    {
        Vec3 position;
        Vec2 textcoord;
    };

    if (g_screenVertexBuffer == 0)
    {
        float uMax = 1;
        float vMax = 1;
        
        __TempVertex vert[6] =
        {
            //first triangle
            {{-1, -1, 0}, {0, vMax}},
            {{-1, 1, 0}, {0, 0}},
            {{1, 1, 0}, {uMax, 0}},

            //2nd triangle
            {{1, 1, 0}, {uMax, 0}},
            {{1, -1, 0}, {uMax, vMax}},
            {{-1, -1, 0}, {0, vMax}}

        };

        g_screenVertexBuffer = new VertexBuffer(vert, 6, sizeof(__TempVertex));
    }

    g_screenVertexBufferRefCount++;
    return g_screenVertexBuffer;
}

inline auto ReleaseScreenVertexBuffer()
{
    g_screenVertexBufferRefCount--;

    if (g_screenVertexBufferRefCount == 0) delete g_screenVertexBuffer;
}