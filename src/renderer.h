#pragma once

#include "resources.h"
#include "math.h"

#include <SDL2/SDL.h>
#include <vector>

struct Camera
{
    glm::vec3 position;
    glm::mat4 view;
    glm::mat4 projection;
    glm::mat4 viewProjection;
    f32 fov;
    f32 near;
    f32 far;
    f32 aspectRatio;
};

struct ViewportLayout
{
    glm::vec2 offset;
    glm::vec2 scale;
    f32 fov;
};

const u32 MAX_VIEWPORTS = 4;
ViewportLayout viewportLayout[MAX_VIEWPORTS][MAX_VIEWPORTS] = {
    {
        { { 0.0f, 0.0f }, { 1.0f, 1.0f }, 33 }
    },
    {
        { { 0.0f, 0.0f }, { 1.0f, 0.5f }, 26 },
        { { 0.0f, 0.5f }, { 1.0f, 0.5f }, 26 },
    },
    {
#if 0
        { { 0.0f, 0.0f }, { 0.5f, 0.5f }, 26 },
        { { 0.5f, 0.0f }, { 0.5f, 0.5f }, 26 },
        { { 0.0f, 0.5f }, { 0.5f, 0.5f }, 26 },

#else
        { { 0.0f, 0.0f }, { 0.5f, 0.55f }, 26 },
        { { 0.5f, 0.0f }, { 0.5f, 0.55f }, 26 },
        { { 0.0f, 0.55f },{ 0.6f, 0.45f }, 26 },
#endif
    },
    {
        { { 0.0f, 0.0f }, { 0.5f, 0.5f }, 26 },
        { { 0.5f, 0.0f }, { 0.5f, 0.5f }, 26 },
        { { 0.0f, 0.5f }, { 0.5f, 0.5f }, 26 },
        { { 0.5f, 0.5f }, { 0.5f, 0.5f }, 26 },
    },
};

class Renderer
{
public:
    SDL_Window* initWindow(const char* name, u32 width, u32 height);
    u32 loadMesh(Mesh const& mesh);
    u32 loadTexture(Texture const& texture);
    void render(f32 deltaTime);

    void addPointLight(glm::vec3 position, glm::vec3 color, f32 attenuation);
    void addSpotLight(glm::vec3 position, glm::vec3 direction, glm::vec3 color, f32 innerRadius, f32 outerRadius, f32 attenuation);
    void addDirectionalLight(glm::vec3 direction, glm::vec3 color);

    void drawMesh(Mesh const& mesh, glm::mat4 const& worldTransform);
    void drawMesh(u32 renderHandle, glm::mat4 const& worldTransform);

    void setBackgroundColor(glm::vec3 color);

    void setViewportCount(u32 viewports);
    Camera& setViewportCamera(u32 index, glm::vec3 const& from, glm::vec3 const& to, f32 near=0.5f, f32 far=500.f);
};
