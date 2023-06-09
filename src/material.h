#pragma once

#include "math.h"
#include "datafile.h"
#include "resource.h"
#include "gl.h"

enum struct MaterialType
{
    NORMAL = 0,
    TERRAIN = 1,
    TRACK = 2,
};

struct TerrainLayer
{
    i64 colorTextureGuid = 0;
    i64 normalTextureGuid = 0;
    f32 textureScale = 0.1f;
    f32 fresnelBias = -0.1f;
    f32 fresnelScale = 0.08f;
    f32 fresnelPower = 2.5f;

    bool isOffroad = false;

    void serialize(Serializer& s)
    {
        s.field(colorTextureGuid);
        s.field(normalTextureGuid);
        s.field(textureScale);
        s.field(fresnelBias);
        s.field(fresnelScale);
        s.field(fresnelPower);
        s.field(isOffroad);
    }
};

class Material : public Resource
{
public:
    MaterialType materialType = MaterialType::NORMAL;

    bool isCullingEnabled = true;
    bool castsShadow = true;
    bool isVisible = true;
    bool isDepthReadEnabled = true;
    bool isDepthWriteEnabled = true;
    bool displayWireframe = false;
    bool useVertexColors = false;
    f32 depthOffset = 0.f;
    f32 windAmount = 0.f;

    bool isTransparent = false;
    f32 contactFadeDistance = 0.f;
    f32 alphaCutoff = 0.f;
    f32 shadowAlphaCutoff = 0.f;

    Vec3 color = { 1, 1, 1 };
    Vec3 emit = { 0, 0, 0 };
    f32 emitPower = 0.f;

    f32 specularPower = 50.f;
    f32 specularStrength = 0.f;
    Vec3 specularColor = { 1, 1, 1 };

    f32 fresnelScale = 0.f;
    f32 fresnelPower = 2.5f;
    f32 fresnelBias = -0.2f;

    f32 reflectionStrength = 0.f;
    f32 reflectionLod = 1.f;
    f32 reflectionBias = 0.2f;

    i64 colorTexture = 0;
    i64 normalMapTexture = 0;

    TerrainLayer terrainLayers[4];

    void serialize(Serializer& s) override
    {
        Resource::serialize(s);

        s.field(materialType);

        s.field(isCullingEnabled);
        s.field(castsShadow);
        s.field(isVisible);
        s.field(isDepthReadEnabled);
        s.field(isDepthWriteEnabled);
        s.field(useVertexColors);
        s.field(depthOffset);

        s.field(isTransparent);
        s.field(alphaCutoff);
        s.field(shadowAlphaCutoff);
        s.field(windAmount);

        s.field(color);
        s.field(emit);
        s.field(emitPower);

        s.field(specularPower);
        s.field(specularStrength);
        s.field(specularColor);

        s.field(fresnelScale);
        s.field(fresnelPower);
        s.field(fresnelBias);

        s.field(reflectionStrength);
        s.field(reflectionLod);
        s.field(reflectionBias);

        switch (materialType)
        {
            case MaterialType::NORMAL:
                s.field(colorTexture);
                s.field(normalMapTexture);
                break;
            case MaterialType::TRACK:
                break;
            case MaterialType::TERRAIN:
                s.field(terrainLayers);
                break;
        }
    }

    ShaderHandle colorShaderHandle = 0;
    ShaderHandle depthShaderHandle = 0;
    ShaderHandle shadowShaderHandle = 0;
    ShaderHandle pickShaderHandle = 0;
    GLuint textureColorHandle = 0;
    GLuint textureNormalHandle = 0;

    void loadShaderHandles(SmallArray<ShaderDefine> additionalDefines={});
    void draw(class RenderWorld* rw, Mat4 const& transform, struct Mesh* mesh, u8 stencil=0);
    void drawPick(class RenderWorld* rw, Mat4 const& transform, struct Mesh* mesh, u32 pickValue);
    void drawHighlight(class RenderWorld* rw, Mat4 const& transform, struct Mesh* mesh,
            u8 stencil, u8 cameraIndex=0);
    void drawVehicle(class RenderWorld* rw, Mat4 const& transform, struct Mesh* mesh, u8 stencil,
            Vec4 const& shield, i64 wrapTextureGuids[3], Vec4 wrapColor[3]);
};

void drawSimple(RenderWorld* rw, Mesh* mesh, struct Texture* tex, Mat4 const& transform,
        Vec3 const& color=Vec3(1.f), Vec3 const& emit=Vec3(0.f));
void drawWireframe(RenderWorld* rw, Mesh* mesh, Mat4 const& transform, Vec4 color);
void drawOverlay(RenderWorld* rw, Mesh* mesh, Mat4 const& transform,
        Vec3 const& color=Vec3(1.f), i32 priorityOffset=0, bool onlyDepth=false);
