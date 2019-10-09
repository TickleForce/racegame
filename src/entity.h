#pragma once

#include "math.h"
#include "datafile.h"

enum struct SerializedEntityID
{
    TERRAIN,
    TRACK,
    STATIC_MESH,
    STATIC_DECAL,
    START,
    TREE,
    BOOSTER,
};

struct ActorUserData
{
    enum
    {
        TRACK,
        SCENERY,
        VEHICLE,
        ENTITY,
        SELECTABLE_ENTITY,
    };
    u32 entityType;
    union
    {
        class Vehicle* vehicle;
        class Entity* entity;
        class PlaceableEntity* placeableEntity;
    };
};

class Entity
{
    bool isMarkedForDeletion = false;

public:
    void destroy() { isMarkedForDeletion = true; }
    bool isDestroyed() { return isMarkedForDeletion; }

    virtual ~Entity() {}
    virtual DataFile::Value serialize() { return {}; }
    virtual void deserialize(DataFile::Value& data) {}
    virtual bool isPersistent() const { return false; }

    virtual void onCreate(class Scene* scene) {}
    virtual void onCreateEnd(class Scene* scene) {}
    virtual void onUpdate(class Renderer* renderer, class Scene* scene, f32 deltaTime) {}
    virtual void onRender(class Renderer* renderer, class Scene* scene, f32 deltaTime) {}

    virtual void applyDecal(class Decal& decal) {}

    virtual void onEditModeRender(class Renderer* renderer, class Scene* scene, bool isSelected) {}
};

class PlaceableEntity : public Entity
{
public:
    glm::vec3 position;
    glm::quat rotation = glm::identity<glm::quat>();
    glm::vec3 scale = glm::vec3(1.f);
    glm::mat4 transform;
    PxRigidActor* actor = nullptr;
    ActorUserData physicsUserData;

    virtual void updateTransform(class Scene* scene)
    {
        transform = glm::translate(glm::mat4(1.f), position)
            * glm::mat4_cast(rotation)
            * glm::scale(glm::mat4(1.f), scale);
        if (actor)
        {
            actor->setGlobalPose(PxTransform(convert(position), convert(rotation)));
            u32 numShapes = actor->getNbShapes();
            if (numShapes > 0)
            {
                for (u32 i=0; i<numShapes; ++i)
                {
                    PxShape* shape = nullptr;
                    actor->getShapes(&shape, 1, i);
                    PxTriangleMeshGeometry geom;
                    PxConvexMeshGeometry convexGeom;
                    if (shape->getTriangleMeshGeometry(geom))
                    {
                        geom.scale = convert(scale);
                        shape->setGeometry(geom);
                    }
                    else if (shape->getConvexMeshGeometry(convexGeom))
                    {
                        convexGeom.scale = convert(scale);
                        shape->setGeometry(convexGeom);
                    }
                }
            }
        }
    }
    virtual const char* getName() const { return "Unknown Entity"; }
    virtual void showDetails(Scene* scene) {}

    ~PlaceableEntity()
    {
        if (actor)
        {
            actor->release();
        }
    }

    bool isPersistent() const override { return true; }
};

