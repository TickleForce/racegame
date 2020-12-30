#pragma once

#include "../vehicle_data.h"
#include "../resources.h"

class VMini : public VehicleData
{
public:
    VMini()
    {
        name = "Mini";
        description = "Small and nimble. Quick acceleration, \nbut low top-speed.";
        price = 8400;
        weaponSlots = {
            { "FRONT WEAPON", WeaponType::FRONT_WEAPON, WeaponClass::HOOD1 },
            { "REAR WEAPON", WeaponType::REAR_WEAPON },
            { "PASSIVE ABILITY", WeaponType::SPECIAL_ABILITY },
        };

        loadModelData("model_mini");
        initStandardUpgrades();

        availableUpgrades.push_back({
            "AWD Conversion",
            "Converts the differential to all-wheel-drive\nto improve grip and acceleration.",
            g_res.getTexture("icon_drivetrain"),
            PerformanceUpgradeType::ALL_WHEEL_DRIVE,
            1,
            6000,
        });
    }

    void initTuning(VehicleConfiguration const& configuration, VehicleTuning& tuning) override
    {
        copySceneDataToTuning(tuning);

        tuning.maxHitPoints = 100;

        tuning.differential = PxVehicleDifferential4WData::eDIFF_TYPE_OPEN_FRONTWD;
        tuning.chassisMass = 1300;
        tuning.wheelMassFront = 20;
        tuning.wheelMassRear = 20;
        tuning.wheelDampingRate = 0.5f;
        tuning.wheelOffroadDampingRate = 33;
        tuning.frontToeAngle = 0;
        tuning.rearToeAngle = 0;
        tuning.trackTireFriction = 3.f;
        tuning.offroadTireFriction = 1.1f;

        tuning.rearTireGripPercent = 1.f;
        tuning.constantDownforce = 0.05f;
        tuning.forwardDownforce = 0.f;
        tuning.topSpeed = 29.5f;
        tuning.driftBoost = 0.f;

        tuning.maxEngineOmega = 800.f;
        tuning.peekEngineTorque = 980.f;
        tuning.engineDampingFullThrottle = 0.15f;
        tuning.engineDampingZeroThrottleClutchEngaged = 1.5f;
        tuning.engineDampingZeroThrottleClutchDisengaged = 0.6f;
        tuning.maxBrakeTorque = 4000.f;
        tuning.maxSteerAngle = radians(48.f);
        tuning.clutchStrength = 5.f;
        tuning.gearSwitchTime = 0.11f;
        tuning.autoBoxSwitchTime = 0.6f;
        tuning.gearRatios = { -5.8f, 0.f, 5.8f, 2.4f, 1.6f, 1.25f };
        tuning.finalGearRatio = 5.3f;

        tuning.suspensionMaxCompression = 0.1f;
        tuning.suspensionMaxDroop = 0.2f;
        tuning.suspensionSpringStrength = 26000.f;
        tuning.suspensionSpringDamperRate = 4000.f;

        tuning.camberAngleAtRest = -0.01f;
        tuning.camberAngleAtMaxDroop = 0.1f;
        tuning.camberAngleAtMaxCompression = -0.1f;

        tuning.frontAntiRollbarStiffness = 8000.f;
        tuning.rearAntiRollbarStiffness = 8000.f;
        tuning.ackermannAccuracy = 0.5f;
        tuning.centerOfMass = { 0.f, 0.f, -0.3f };

        for (auto& u : configuration.performanceUpgrades)
        {
            PerformanceUpgrade& upgrade = availableUpgrades[u.upgradeIndex];
            switch (upgrade.upgradeType)
            {
                case PerformanceUpgradeType::ENGINE:
                    tuning.peekEngineTorque += 10.f * u.upgradeLevel;
                    tuning.topSpeed += 1.f * u.upgradeLevel;
                    break;
                case PerformanceUpgradeType::TIRES:
                    tuning.trackTireFriction += 0.12f * u.upgradeLevel;
                    tuning.offroadTireFriction += 0.05f * u.upgradeLevel;
                    break;
                case PerformanceUpgradeType::ARMOR:
                    tuning.maxHitPoints += 12.f * u.upgradeLevel;
                    break;
                case PerformanceUpgradeType::SUSPENSION:
                    tuning.frontAntiRollbarStiffness += 500.f * u.upgradeLevel;
                    tuning.rearAntiRollbarStiffness += 500.f * u.upgradeLevel;
                    tuning.suspensionSpringStrength += 1000.f * u.upgradeLevel;
                    tuning.suspensionSpringDamperRate += 500.f * u.upgradeLevel;
                    break;
                case PerformanceUpgradeType::WEIGHT_REDUCTION:
                    tuning.chassisMass -= 70.f * u.upgradeLevel;
                    break;
                case PerformanceUpgradeType::ALL_WHEEL_DRIVE:
                    tuning.differential = PxVehicleDifferential4WData::eDIFF_TYPE_LS_4WD;
                    tuning.peekEngineTorque += 5.f;
                    break;
                default:
                    println("Unhandled upgrade: %s", upgrade.name);
                    break;
            }
        }
    }
};
