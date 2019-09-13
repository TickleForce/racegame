#include "static_decal.h"
#include "../renderer.h"
#include "../scene.h"
#include "../game.h"

#include <typeinfo>

const char* textures[] = {
    "arrow",
    "arrow_left",
    "arrow_right"
};

StaticDecal::StaticDecal(u32 texIndex, glm::vec3 const& pos, u32 decalFilter)
{
    position = pos;
    scale = glm::vec3(16.f);
    rotation = glm::rotate(rotation, (f32)M_PI * 0.5f, glm::vec3(0, 1, 0));
    this->texIndex = texIndex;
    this->decalFilter = decalFilter;
}

void StaticDecal::onCreateEnd(Scene* scene)
{
    actor = g_game.physx.physics->createRigidStatic(PxTransform(convert(position), convert(rotation)));
    physicsUserData.entityType = ActorUserData::SELECTABLE_ENTITY;
    physicsUserData.placeableEntity = this;
    actor->userData = &physicsUserData;
    PxShape* collisionShape = PxRigidActorExt::createExclusiveShape(*actor,
            PxBoxGeometry(convert(scale * 0.5f)), *scene->genericMaterial);
    collisionShape->setQueryFilterData(PxFilterData(COLLISION_FLAG_SELECTABLE, 0, 0, 0));
    collisionShape->setSimulationFilterData(PxFilterData(0, 0, 0, 0));
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
                        glm::abs(glm::max(glm::vec3(0.01f), scale) * 0.5f))));
    }

    tex = g_resources.getTexture(textures[texIndex]);

    const u32 bufferSize = 32;
    PxOverlapHit hitBuffer[bufferSize];
    PxOverlapBuffer hit(hitBuffer, bufferSize);
    PxQueryFilterData filter;
    filter.flags |= PxQueryFlag::eSTATIC;
    //filter.flags |= PxQueryFlag::eDYNAMIC;
    filter.data = PxFilterData(0, decalFilter, 0, 0);
    decal.begin(transform);
    if (scene->getPhysicsScene()->overlap(PxBoxGeometry(convert(glm::abs(scale * 0.5f))),
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

void StaticDecal::onUpdate(Renderer* renderer, Scene* scene, f32 deltaTime)
{
    decal.setTexture(tex);
    renderer->add(&decal);
}

void StaticDecal::onEditModeRender(Renderer* renderer, Scene* scene, bool isSelected)
{
    BoundingBox decalBoundingBox{ glm::vec3(-0.5f), glm::vec3(0.5f) };
    if (isSelected)
    {
        scene->debugDraw.boundingBox(decalBoundingBox, transform, glm::vec4(1, 1, 1, 1));
    }
    else
    {
        scene->debugDraw.boundingBox(decalBoundingBox, transform, glm::vec4(1, 0.5f, 0, 1));
    }
}

DataFile::Value StaticDecal::serialize()
{
    DataFile::Value dict = DataFile::makeDict();
    dict["entityID"] = DataFile::makeInteger((i64)SerializedEntityID::STATIC_DECAL);
    dict["position"] = DataFile::makeVec3(position);
    dict["rotation"] = DataFile::makeVec4({ rotation.x, rotation.y, rotation.z, rotation.w });
    dict["scale"] = DataFile::makeVec3(scale);
    dict["texIndex"] = DataFile::makeInteger(texIndex);
    return dict;
}

void StaticDecal::deserialize(DataFile::Value& data)
{
    position = data["position"].vec3();
    glm::vec4 r = data["rotation"].vec4();
    rotation = glm::quat(r.w, r.x, r.y, r.z);
    scale = data["scale"].vec3();
    texIndex = data["texIndex"].integer(0);
}

void StaticDecal::showDetails()
{
}
