#pragma once

#include "../math.h"
#include "../entity.h"
#include "../resources.h"

class Mine : public Entity
{
    Mat4 transform;
    u32 instigator;
    Model* model;
    f32 aliveTime = 0.f;

public:
    Mine(Mat4 const& transform, u32 instigator)
        : transform(transform), instigator(instigator)
    {
        model = g_res.getModel("mine");
        // TODO: add collision mesh so that AI can avoid it
    }

    void onUpdate(RenderWorld* rw, Scene* scene, f32 deltaTime) override;
    void onRender(RenderWorld* rw, Scene* scene, f32 deltaTime) override;
};
