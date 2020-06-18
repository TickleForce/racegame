#pragma once

#include "../weapon.h"
#include "../vehicle.h"
#include "../entities/projectile.h"

class WMachineGun : public Weapon
{
    f32 repeatTimer = 0.f;
    Mesh* mesh;
    Mesh* meshBarrel;
    f32 barrelSpin = 0.f;
    f32 barrelSpinSpeed = 0.f;

public:
    WMachineGun()
    {
        info.name = "Machine Gun";
        info.description = "Low damage but high rate of fire.";
        info.icon = g_res.getTexture("icon_mg");
        info.price = 1000;
        info.maxUpgradeLevel = 5;
        info.weaponType = WeaponType::FRONT_WEAPON;

        ammoUnitCount = 12;
        fireMode = FireMode::CONTINUOUS;

        loadModelData("weapon_minigun");
        Model* model = g_res.getModel("weapon_minigun");
        mesh = model->getMeshByName("minigun.Minigun");
        meshBarrel = model->getMeshByName("minigun.MinigunBarrel");
    }

    void update(Scene* scene, Vehicle* vehicle, bool fireBegin, bool fireHold,
            f32 deltaTime) override
    {
        barrelSpinSpeed = max(barrelSpinSpeed - deltaTime * 8.f, 0.f);
        barrelSpin += barrelSpinSpeed * deltaTime;

        if (!fireHold)
        {
            return;
        }

        if (ammo == 0)
        {
            if (fireBegin)
            {
                outOfAmmo(vehicle);
            }
            return;
        }

        barrelSpinSpeed = 12.f;

        if (repeatTimer > 0.f)
        {
            repeatTimer = max(repeatTimer - deltaTime, 0.f);
            return;
        }

        Mat4 transform = vehicle->getTransform();
        f32 minSpeed = 90.f;
        Vec3 vel = convert(vehicle->getRigidBody()->getLinearVelocity())
            + vehicle->getForwardVector() * minSpeed;
        if (length2(vel) < square(minSpeed))
        {
            vel = normalize(vel) * minSpeed;
        }
        Vec3 pos = transform * mountTransform * Vec4(projectileSpawnPoints[0], 1.f);
        scene->addEntity(new Projectile(pos,
                vel, zAxisOf(transform), vehicle->vehicleIndex, Projectile::BULLET));

        g_audio.playSound3D(g_res.getSound("mg2"),
                SoundType::GAME_SFX, vehicle->getPosition(), false,
                random(scene->randomSeries, 0.95f, 1.05f), 1.f,
                random(scene->randomSeries, -0.05f, 0.05f));

        ammo -= 1;

        repeatTimer = 0.09f;
    }

    void render(class RenderWorld* rw, Mat4 const& vehicleTransform,
            VehicleConfiguration const& config, VehicleData const& vehicleData) override
    {
        Material* mat = g_res.getMaterial("plastic");
        mat->draw(rw, vehicleTransform * mountTransform, mesh, 2);
        mat->draw(rw, vehicleTransform * mountTransform
                    * glm::translate(Mat4(1.f), Vec3(0.556007, 0, 0.397523f))
                    * glm::rotate(Mat4(1.f), barrelSpin, Vec3(1, 0, 0)), meshBarrel, 2);
    }
};
