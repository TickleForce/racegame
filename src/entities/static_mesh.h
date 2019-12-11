#pragma once

#include "../math.h"
#include "../entity.h"
#include "../resources.h"
#include "../mesh_renderables.h"

class StaticMesh : public PlaceableEntity
{
    struct MeshItem
    {
        LitRenderable s;
        glm::mat4 transform;
        PxShape* shape;
    };
    SmallVec<MeshItem> meshes;
    u32 meshIndex;

public:
    StaticMesh* setup(u32 meshIndex=0, glm::vec3 const& position = {0, 0, 0},
            glm::vec3 const& scale = {1, 1, 1}, f32 zRotation=0.f);
    void applyDecal(class Decal& decal) override;
    void onCreate(class Scene* scene) override;
    void onRender(RenderWorld* rw, Scene* scene, f32 deltaTime) override;
    void onEditModeRender(RenderWorld* rw, class Scene* scene, bool isSelected) override;
    DataFile::Value serializeState() override;
    void deserializeState(DataFile::Value& data) override;
    void updateTransform(class Scene* scene) override;
};
