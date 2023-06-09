#include "static_decal.h"
#include "../renderer.h"
#include "../scene.h"
#include "../game.h"
#include "../billboard.h"
#include "../imgui.h"

StaticDecal::StaticDecal()
{
    scale = Vec3(16.f);
    rotation *= Quat::rotationY(PI * 0.5f);
}

void StaticDecal::onCreateEnd(Scene* scene)
{
    actor = g_game.physx.physics->createRigidStatic(PxTransform(convert(position), convert(rotation)));
    physicsUserData.entityType = ActorUserData::SELECTABLE_ENTITY;
    physicsUserData.placeableEntity = this;
    actor->userData = &physicsUserData;

    PxShape* collisionShape = PxRigidActorExt::createExclusiveShape(*actor,
            PxBoxGeometry(convert(scale * 0.5f)), *g_game.physx.materials.generic);
    collisionShape->setQueryFilterData(PxFilterData(COLLISION_FLAG_SELECTABLE, 0, 0, 0));
    collisionShape->setSimulationFilterData(PxFilterData(0, 0, 0, 0));
    collisionShape->setFlag(PxShapeFlag::eSIMULATION_SHAPE, false);

    scene->getPhysicsScene()->addActor(*actor);

    updateTransform(scene);
}

void StaticDecal::updateTransform(Scene* scene)
{
    if (actor)
    {
        PlaceableEntity::updateTransform(scene);

        PxShape* shape = nullptr;
        actor->getShapes(&shape, 1);
        shape->setGeometry(PxBoxGeometry(convert(
                        absolute(max(Vec3(0.01f), scale) * 0.5f))));

        if (isDust)
        {
            shape->setQueryFilterData(PxFilterData(
                        COLLISION_FLAG_SELECTABLE | COLLISION_FLAG_DUST, 0, 0, 0));
        }
        else
        {
            shape->setQueryFilterData(PxFilterData(COLLISION_FLAG_SELECTABLE, 0, 0, 0));
        }
    }

    decal.setPriority(beforeMarking ? TransparentDepth::TRACK_DECAL :
            (isDust ? TransparentDepth::SAND_DECAL : TransparentDepth::TRACK_DECAL));

    const u32 bufferSize = 32;
    PxOverlapHit hitBuffer[bufferSize];
    PxOverlapBuffer hit(hitBuffer, bufferSize);
    PxQueryFilterData filter;
    filter.flags |= PxQueryFlag::eSTATIC;
    filter.data = PxFilterData(0, decalFilter, 0, 0);
    decal.begin(transform);
    if (scene->getPhysicsScene()->overlap(PxBoxGeometry(convert(absolute(scale * 0.5f))),
                PxTransform(convert(position), convert(rotation)), hit, filter))
    {
        for (u32 i=0; i<hit.getNbTouches(); ++i)
        {
            PxActor* actor = hit.getTouch(i).actor;
            ActorUserData* userData = (ActorUserData*)actor->userData;
            if (userData && (userData->entityType == ActorUserData::ENTITY
                        || userData->entityType == ActorUserData::SELECTABLE_ENTITY))
            {
                Entity* entity = (Entity*)userData->entity;
                entity->applyDecal(decal);
            }
        }
    }
    decal.end();
}

void StaticDecal::onRender(RenderWorld* rw, Scene* scene, f32 deltaTime)
{
    decal.draw(rw);
}

void StaticDecal::onPreview(RenderWorld* rw)
{
    rw->setViewportCamera(0, Vec3(0.f, 0.1f, 20.f), Vec3(0.f), 1.f, 200.f, 50.f);
    drawBillboard(rw, tex, Vec3(0, 0, 2.f), Vec4(1.f), 8.f, 0.f, false);
}

void StaticDecal::onEditModeRender(RenderWorld* rw, Scene* scene, bool isSelected, u8 selectIndex)
{
    BoundingBox decalBoundingBox{ Vec3(-0.5f), Vec3(0.5f) };
    if (isSelected)
    {
        scene->debugDraw.boundingBox(decalBoundingBox, transform, Vec4(1, 1, 1, 1));
    }
    else
    {
        scene->debugDraw.boundingBox(decalBoundingBox, transform, Vec4(1, 0.5f, 0, 1));
    }
}

void StaticDecal::showDetails(Scene* scene)
{
    u32 prevDecalFilter = decalFilter;

    bool affectsTrack = decalFilter & DECAL_TRACK;
    ImGui::Checkbox("Affects Track", &affectsTrack);
    bool affectsTerrain = decalFilter & DECAL_TERRAIN;
    ImGui::Checkbox("Affects Terrain", &affectsTerrain);
    bool affectsSigns = decalFilter & DECAL_SIGN;
    ImGui::Checkbox("Affects Signs", &affectsSigns);
    bool affectsRailings = decalFilter & DECAL_RAILING;
    ImGui::Checkbox("Affects Railings", &affectsRailings);
    bool affectsGround = decalFilter & DECAL_GROUND;
    ImGui::Checkbox("Affects Ground", &affectsGround);

    decalFilter = DECAL_PLACEHOLDER;
    if (affectsTrack)    decalFilter |= DECAL_TRACK;
    if (affectsTerrain)  decalFilter |= DECAL_TERRAIN;
    if (affectsSigns)    decalFilter |= DECAL_SIGN;
    if (affectsRailings) decalFilter |= DECAL_RAILING;
    if (affectsGround)   decalFilter |= DECAL_GROUND;

    if (prevDecalFilter != decalFilter)
    {
        updateTransform(scene);
    }

    if (ImGui::Checkbox("Before Marking", &beforeMarking))
    {
        updateTransform(scene);
    }
    if (ImGui::Checkbox("Dusty", &isDust))
    {
        updateTransform(scene);
    }
}

Array<PropPrefabData> StaticDecal::generatePrefabProps()
{
    Array<PropPrefabData> results;
    g_res.iterateResourceType(ResourceType::TEXTURE, [&](Resource* res){
        Texture* tex = (Texture*)res;
        if (tex->name.find("decal_"))
        {
            results.push({
                PropCategory::DECALS,
                tex->name,
                [tex](Entity* e) {
                    ((StaticDecal*)e)->setTexture(tex->guid);
                    ((StaticDecal*)e)->isDust = tex->name.find("dust") || tex->name.find("sand");
                }
            });
        }
    });
    return results;
}
