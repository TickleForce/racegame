#pragma once

#include "../math.h"
#include "../entity.h"
#include "../resources.h"

class StaticMesh : public PlaceableEntity
{
    i64 modelGuid = 0;
    Model* model = nullptr;

    struct Object
    {
        ModelObject* modelObject = nullptr;
        PxShape* shape = nullptr;
    };

    Array<Object> objects;

    void loadModel();

public:
    void applyDecal(class Decal& decal) override;
    void onCreate(class Scene* scene) override;
    void onRender(RenderWorld* rw, Scene* scene, f32 deltaTime) override;
    void onPreview(RenderWorld* rw) override;
    void onEditModeRender(RenderWorld* rw, class Scene* scene, bool isSelected, u8 selectIndex) override;
    void serializeState(Serializer& s) override;
    void updateTransform(class Scene* scene) override;
    Array<PropPrefabData> generatePrefabProps() override;
    const char* getName() const override { return model->name.data(); }
    void onBatch(class Batcher& batcher) override;
};
