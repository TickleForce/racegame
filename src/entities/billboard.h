#pragma once

#include "../math.h"
#include "../entity.h"
#include "../resources.h"
#include "../scene.h"
#include "../game.h"
#include "../imgui.h"
#include "../model.h"

class Billboard : public PlaceableEntity
{
    Model* model = nullptr;
    ModelObject* billboardObject = nullptr;
    i64 billboardTextureGuid = 0;

public:
    Billboard()
    {
        model = g_res.getModel("billboard");
        billboardObject = model->getObjByName("BillboardSign");
        billboardTextureGuid = g_res.getTexture("billboard_1")->guid;
    }

    void applyDecal(class Decal& decal) override
    {
        for (auto& obj : model->objects)
        {
            decal.addMesh(&model->meshes[obj.meshIndex], obj.getTransform() * transform);
        }
    }

    void onCreate(class Scene* scene) override
    {
        updateTransform(scene);

        actor = g_game.physx.physics->createRigidStatic(
                PxTransform(convert(position), convert(rotation)));
        physicsUserData.entityType = ActorUserData::SELECTABLE_ENTITY;
        physicsUserData.entity = this;
        actor->userData = &physicsUserData;

        for (auto& obj : model->objects)
        {
            PxShape* collisionShape = PxRigidActorExt::createExclusiveShape(*actor,
                    PxTriangleMeshGeometry(model->meshes[obj.meshIndex].getCollisionMesh(),
                        PxMeshScale(convert(scale * obj.scale))), *g_game.physx.materials.generic);
            collisionShape->setQueryFilterData(
                    PxFilterData(COLLISION_FLAG_OBJECT, DECAL_SIGN, 0, DRIVABLE_SURFACE));
            collisionShape->setSimulationFilterData(PxFilterData(COLLISION_FLAG_OBJECT, -1, 0, 0));
            collisionShape->setLocalPose(convert(obj.getTransform()));
        }

        scene->getPhysicsScene()->addActor(*actor);
    }

    void onRender(RenderWorld* rw, Scene* scene, f32 deltaTime) override
    {
        drawSimple(rw, model->getMeshByName("billboard.BillboardSign"),
                g_res.getTexture(billboardTextureGuid), transform);
        if (scene->isBatched)
        {
            return;
        }
        for (auto& obj : model->objects)
        {
            if (obj.isVisible)
            {
                g_res.getMaterial(obj.materialGuid)->draw(rw, transform * obj.getTransform(),
                        &model->meshes[obj.meshIndex]);
            }
        }
    }

    void onBatch(Batcher& batcher) override
    {
        for (auto& obj : model->objects)
        {
            if (obj.isVisible)
            {
                batcher.add(g_res.getMaterial(obj.materialGuid),
                    transform * obj.getTransform(), &model->meshes[obj.meshIndex]);
            }
        }
    }

    void onPreview(RenderWorld* rw) override
    {
        rw->setViewportCamera(0, Vec3(3.f, 1.f, 4.f) * 4.5f,
                Vec3(0, 0, 3.5f), 1.f, 200.f, 32.f);
        transform = Mat4(1.f);
        drawSimple(rw, model->getMeshByName("billboard.BillboardSign"),
                g_res.getTexture(billboardTextureGuid), transform);
        for (auto& obj : model->objects)
        {
            if (obj.isVisible)
            {
                g_res.getMaterial(obj.materialGuid)->draw(rw, transform * obj.getTransform(),
                        &model->meshes[obj.meshIndex]);
            }
        }
    }

    void onEditModeRender(RenderWorld* rw, class Scene* scene, bool isSelected, u8 selectIndex) override
    {
        for (auto& obj : model->objects)
        {
            Material* mat = g_res.getMaterial(obj.materialGuid);
            Mat4 t = transform * obj.getTransform();
            Mesh* mesh = &model->meshes[obj.meshIndex];
            if (isSelected)
            {
                mat->drawHighlight(rw, t, mesh, selectIndex);
            }
            mat->drawPick(rw, t, mesh, entityCounterID);
        }
    }

    void showDetails(Scene* scene) override
    {
        g_res.iterateResourceType(ResourceType::TEXTURE, [&](Resource* res){
            Texture* tex = (Texture*)res;
            if (tex->name.find("billboard"))
            {
                f32 w = min(ImGui::GetColumnWidth(), 200.f);
                f32 h = w * 0.5f;
                ImGui::PushID(tex->name.data());
                if (ImGui::ImageButton((void*)(uintptr_t)tex->getPreviewHandle(), { w, h }))
                {
                    billboardTextureGuid = tex->guid;
                }
                ImGui::PopID();
            }
        });
    }

    void serializeState(Serializer& s) override
    {
        s.field(billboardTextureGuid);
        PlaceableEntity::serializeState(s);
    }

    Array<PropPrefabData> generatePrefabProps() override
    {
        Array<PropPrefabData> a;
        a.push({ PropCategory::NOT_NATURE, "Billboard" });
        return a;
    }

    const char* getName() const override { return "Billboard"; }
};
