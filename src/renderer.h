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

const u32 MAX_VIEWPORTS = 4;
struct ViewportLayout
{
    f32 fov;
    glm::vec2 scale;
    glm::vec2 offsets[MAX_VIEWPORTS];
    //glm::vec2 trackPos;
    //f32 trackScale;
};

ViewportLayout viewportLayout[MAX_VIEWPORTS] = {
    { 33, { 1.0f, 1.0f }, { { 0.0f, 0.0f } } },
    { 26, { 1.0f, 0.5f }, { { 0.0f, 0.0f }, { 0.0f, 0.5f } } },
    { 26, { 0.5f, 0.5f }, { { 0.0f, 0.0f }, { 0.5f, 0.0f }, { 0.0f, 0.5f } } },
    { 26, { 0.5f, 0.5f }, { { 0.0f, 0.0f }, { 0.5f, 0.0f }, { 0.0f, 0.5f }, { 0.5f, 0.5f } } },
};

struct RenderTextureItem
{
    u32 renderHandle;
    glm::mat4 transform;
};

class Renderer
{
public:
    SDL_Window* initWindow(const char* name, u32 width, u32 height);
    u32 loadMesh(Mesh const& mesh);
    u32 loadTexture(Texture const& texture, u8* data, size_t size);
    void render(f32 deltaTime);

    void addPointLight(glm::vec3 position, glm::vec3 color, f32 attenuation);
    void addSpotLight(glm::vec3 position, glm::vec3 direction, glm::vec3 color, f32 innerRadius, f32 outerRadius, f32 attenuation);
    void addDirectionalLight(glm::vec3 direction, glm::vec3 color);

    void drawMesh(Mesh const& mesh, glm::mat4 const& worldTransform);
    void drawMesh(u32 renderHandle, glm::mat4 const& worldTransform);

    void setViewportCount(u32 viewports);
    Camera& setViewportCamera(u32 index, glm::vec3 const& from, glm::vec3 const& to, f32 near=0.5f, f32 far=500.f);

    void drawLine(glm::vec3 const& p1, glm::vec3 const& p2,
            glm::vec4 const& c1 = glm::vec4(1), glm::vec4 const& c2 = glm::vec4(1));
    void drawQuad2D(u32 texture, glm::vec2 p1, glm::vec2 p2, glm::vec2 t1, glm::vec2 t2,
            glm::vec3 color, f32 alpha=1.f, bool colorShader=true);
    //void drawQuad2D(u32 texture, glm::vec2 p, f32 angle=0.f, glm::vec3 color=glm::vec3(1.f), f32 alpha=1.f);

    u32 renderTexture(std::vector<RenderTextureItem> const& items, u32 width, u32 height);
};
