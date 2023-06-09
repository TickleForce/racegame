#pragma once

#include "../weapon.h"
#include "../vehicle.h"
#include "../entities/mine.h"

class WExplosiveMine : public Weapon
{
public:
    WExplosiveMine()
    {
        info.name = "Exploding Mine";
        info.description = "It explodes";
        info.icon = g_res.getTexture("icon_mine");
        info.price = 1250;
        info.maxUpgradeLevel = 4;
        info.weaponType = WeaponType::REAR_WEAPON;
    }

    void update(Scene* scene, Vehicle* vehicle, bool fireBegin, bool fireHold,
            f32 deltaTime) override
    {
        if (!fireBegin)
        {
            return;
        }

        if (ammo == 0)
        {
            outOfAmmo(vehicle);
            return;
        }

        PxRaycastBuffer hit;
        Vec3 down = convert(vehicle->getRigidBody()->getGlobalPose().q.getBasisVector2() * -1.f);
        if (!scene->raycastStatic(vehicle->getPosition(), down, 2.f, &hit,
                    COLLISION_FLAG_TERRAIN | COLLISION_FLAG_TRACK))
        {
            g_audio.playSound(g_res.getSound("nono"), SoundType::GAME_SFX);
            return;
        }

        Vec3 pos = convert(hit.block.position);
        Vec3 up = convert(hit.block.normal);
        Mat4 m(1.f);
        m[0] = Vec4(vehicle->getForwardVector(), m[0].w);
        m[1] = Vec4(normalize(
                    cross(up, Vec3(m[0]))), m[1].w);
        m[2] = Vec4(normalize(
                cross(Vec3(m[0]), Vec3(m[1]))), m[2].w);
        scene->addEntity(new Mine(Mat4::translation(pos) * m, vehicle->vehicleIndex));
        g_audio.playSound3D(g_res.getSound("thunk"), SoundType::GAME_SFX,
                vehicle->getPosition(), false, 1.f, 0.9f);

        ammo -= 1;
    }

    bool shouldUse(Scene* scene, Vehicle* vehicle) override
    {
        return vehicle->isOnTrack && vehicle->getAI()->aggression > 0.f
                && ammo > 0 && vehicle->getForwardSpeed() > 10.f
                && irandom(scene->randomSeries, 0, 100 + (i32)((1.f - vehicle->getAI()->aggression) * 100)) < 2;
    }
};
