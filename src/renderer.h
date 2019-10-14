#pragma once

#include "gl.h"
#include "resources.h"
#include "math.h"
#include "smallvec.h"
#include "decal.h"
#include "renderable.h"
#include "dynamic_buffer.h"
#include "buffer.h"

#include <algorithm>
#include <vector>

GLuint emptyVAO;

struct Camera
{
    glm::vec3 position;
    glm::mat4 view;
    glm::mat4 projection;
    glm::mat4 viewProjection;
    f32 fov;
    f32 nearPlane;
    f32 farPlane;
    f32 aspectRatio;
};

const u32 MAX_VIEWPORTS = 4;
struct ViewportLayout
{
    f32 fov;
    glm::vec2 scale;
    glm::vec2 offsets[MAX_VIEWPORTS];
};

ViewportLayout viewportLayout[MAX_VIEWPORTS] = {
    { 33, { 1.0f, 1.0f }, { { 0.0f, 0.0f } } },
    { 26, { 1.0f, 0.5f }, { { 0.0f, 0.0f }, { 0.0f, 0.5f } } },
    { 26, { 0.5f, 0.5f }, { { 0.0f, 0.0f }, { 0.5f, 0.0f }, { 0.0f, 0.5f } } },
    { 26, { 0.5f, 0.5f }, { { 0.0f, 0.0f }, { 0.5f, 0.0f }, { 0.0f, 0.5f }, { 0.5f, 0.5f } } },
};

struct Framebuffers
{
    GLuint mainFramebuffer;
    GLuint mainColorTexture;
    GLuint mainDepthTexture;

    GLuint finalColorTexture;
    GLuint finalFramebuffer;

    u32 msaaResolveFramebuffersCount;
    GLuint msaaResolveFromFramebuffers[MAX_VIEWPORTS];
    GLuint msaaResolveFramebuffers[MAX_VIEWPORTS];
    GLuint msaaResolveColorTexture;
    GLuint msaaResolveDepthTexture;

    SmallVec<GLuint> bloomFramebuffers;
    SmallVec<GLuint> bloomColorTextures;
    SmallVec<glm::ivec2> bloomBufferSize;

    GLuint shadowFramebuffer;
    GLuint shadowDepthTexture;

    GLuint cszFramebuffers[5];
    GLuint cszTexture;

    GLuint saoFramebuffer;
    GLuint saoTexture;
    GLuint saoBlurTexture;

    u32 renderWidth;
    u32 renderHeight;
};

struct FullscreenFramebuffers
{
    GLuint fullscreenTexture;
    GLuint fullscreenFramebuffer;
    GLuint fullscreenBlurTextures[2];
    GLuint fullscreenBlurFramebuffer;
};

struct WorldInfo
{
    glm::mat4 orthoProjection;
    glm::vec3 sunDirection;
    f32 time;
    glm::vec3 sunColor;
    f32 pad;
    glm::mat4 cameraViewProjection[MAX_VIEWPORTS];
    glm::mat4 cameraProjection[MAX_VIEWPORTS];
    glm::mat4 cameraView[MAX_VIEWPORTS];
    glm::vec4 cameraPosition[MAX_VIEWPORTS];
    glm::mat4 shadowViewProjectionBias[MAX_VIEWPORTS];
    glm::vec4 projInfo[MAX_VIEWPORTS];
    glm::vec4 projScale;
};

class RenderWorld
{
    friend class Renderer;

    u32 width, height;

    const char* name = "";
    WorldInfo worldInfo;
    Framebuffers fb;
    SmallVec<Camera, MAX_VIEWPORTS> cameras = { {} };
    DynamicBuffer worldInfoUBO = DynamicBuffer(sizeof(WorldInfo));
    DynamicBuffer worldInfoUBOShadow = DynamicBuffer(sizeof(WorldInfo));

    // TODO: calculate these based on render resolution
    u32 firstBloomDivisor = 2;
    u32 lastBloomDivisor = 16;

    struct QueuedRenderable
    {
        i32 priority;
        Renderable* renderable;
    };
    std::vector<QueuedRenderable> renderables;

    Buffer tempRenderBuffer = Buffer(megabytes(4), 32);

    void setShadowMatrices(WorldInfo& worldInfo, WorldInfo& worldInfoShadow);

public:
    void add(Renderable* renderable)
    {
        renderables.push_back({ renderable->getPriority(), renderable });
    }
    template <typename T>
    T* push(T&& renderable)
    {
        u8* mem = tempRenderBuffer.bump(sizeof(T));
        new (mem) T(std::move(renderable));
        T* ptr = reinterpret_cast<T*>(mem);
        add(ptr);
        return ptr;
    }

    void setViewportCount(u32 viewports);
    u32 getViewportCount() const { return cameras.size(); }
    Camera& setViewportCamera(u32 index, glm::vec3 const& from, glm::vec3 const& to, f32 nearPlane=0.5f, f32 farPlane=500.f, f32 fov=0.f);
    Camera& getCamera(u32 index) { return cameras[index]; }

    //void addPointLight(glm::vec3 position, glm::vec3 color, f32 attenuation);
    //void addSpotLight(glm::vec3 position, glm::vec3 direction, glm::vec3 color, f32 innerRadius, f32 outerRadius, f32 attenuation);
    void addDirectionalLight(glm::vec3 direction, glm::vec3 color);

    void updateWorldTime(f64 time);
    void createFramebuffers(u32 width, u32 height);
    void render(Renderer* renderer, f32 deltaTime);
    void clear();
};

class Renderer
{
private:
    FullscreenFramebuffers fsfb = { 0 };
    RenderWorld renderWorld;

    std::map<std::string, u32> shaderHandleMap;
    std::vector<GLuint> loadedShaders[MAX_VIEWPORTS];

    struct QueuedRenderable2D
    {
        i32 priority;
        Renderable2D* renderable;
    };
    std::vector<QueuedRenderable2D> renderables2D;

    void glShaderSources(GLuint shader, std::string const& src, SmallVec<std::string> const& defines, u32 viewportCount);
    GLuint compileShader(std::string const& filename, SmallVec<std::string> defines, u32 viewportCount);

    void createFullscreenFramebuffers();

public:
    void add2D(Renderable2D* renderable, i32 priority=0)
    {
        renderables2D.push_back({ priority, renderable });
    }
    template <typename T>
    T* push2D(T&& renderable, i32 priority=0)
    {
        u8* mem = renderWorld.tempRenderBuffer.bump(sizeof(T));
        new (mem) T(std::move(renderable));
        T* ptr = reinterpret_cast<T*>(mem);
        add2D(ptr, priority);
        return ptr;
    }

    void init();
    void initShaders();
    void updateFramebuffers();
    void updateFullscreenFramebuffers();
    u32 loadShader(std::string const& filename, SmallVec<std::string> defines={}, std::string name="");
    u32 getShader(const char* name, i32 viewportCount=0) const;
    GLuint getShaderProgram(const char* name, i32 viewportCount=0) const;
    void render(f32 deltaTime);
    RenderWorld* getRenderWorld() { return &renderWorld; }

    size_t getTempRenderBufferSize() const { return renderWorld.tempRenderBuffer.pos; }
    std::string getDebugRenderList()
    {
        std::sort(renderWorld.renderables.begin(), renderWorld.renderables.end(), [&](auto& a, auto& b) {
            return a.priority < b.priority;
        });

        std::string result;
        std::string prev = str(renderWorld.renderables.front().priority, " - ",
                renderWorld.renderables.front().renderable->getDebugString());
        u32 count = 0;
        u32 items = 0;
        for (auto it = renderWorld.renderables.begin(); it != renderWorld.renderables.end(); ++it)
        {
            std::string t = str(it->priority, " - ", it->renderable->getDebugString());
            if (t != prev)
            {
                if (items != 0)
                {
                    result += '\n';
                }
                result += prev + " x " + std::to_string(count);
                prev = std::move(t);
                count = 1;
                ++items;
            }
            else
            {
                ++count;
            }

            if (it + 1 == renderWorld.renderables.end())
            {
                result += "\n" + prev + " x " + std::to_string(count);
            }
        }
        return result;
    }
};

