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