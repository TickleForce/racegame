#pragma once

#include "misc.h"
#include <SDL2/SDL.h>
#include <glad/glad.h>

const u32 MAX_BUFFERED_FRAMES = 3;
using ShaderHandle = u32;

namespace RenderFlags
{
    enum
    {
        BACKFACE_CULL,
        //WIREFRAME,
        DEPTH_READ,
        DEPTH_WRITE,
    };
};

struct ShaderDefine
{
    const char* name = "";
    const char* value = "";
};

ShaderHandle getShaderHandle(const char* name, SmallArray<ShaderDefine> const& defines={},
        u32 renderFlags=RenderFlags::DEPTH_READ | RenderFlags::DEPTH_WRITE, f32 depthOffset=0.f);

namespace TransparentDepth
{
    enum
    {
        TRACK_DECAL = -13000,
        TIRE_MARKS = -12000,
        OIL_GLUE = -11000,
        FLAT_SPLINE = -10000,
        SAND_DECAL = -9000,
        DEBUG_LINES = 0,
        PARTICLE_SYSTEM = 10000,
        BILLBOARD = 20000,
        OVERLAY = 1000000,
    };
}
