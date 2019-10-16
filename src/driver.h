#pragma once

#include "math.h"
#include "vehicle_data.h"

struct ComputerDriverData
{
    std::string name;
    f32 drivingSkill; // [0,1] how optimal of a path the AI takes on the track
    f32 aggression;   // [0,1] how likely the AI is to go out of its way to attack other drivers
    f32 awareness;    // [0,1] how likely the AI is to attempt to avoid hitting other drivers and obstacles
    f32 fear;         // [0,1] how much the AI tries to evade other drivers
};

struct Driver
{
    u32 leaguePoints = 0;
    i32 credits = 10000;

    bool isPlayer = false;
    bool hasCamera = false;
    std::string playerName = "no-name";
    //ComputerDriverData aiDriverData;
    bool useKeyboard = false;
    u32 controllerID = 0;

    VehicleConfiguration vehicleConfig;
    VehicleTuning vehicleTuning;

    struct OwnedVehicle
    {
        i32 vehicleIndex;
        VehicleConfiguration vehicleConfig;
    };
    std::vector<OwnedVehicle> ownedVehicles;

    i32 vehicleIndex = -1;
    glm::vec3 vehicleColor = { 1.f, 1.f, 1.f };

    void updateTuning()
    {
        vehicleTuning = {};
        g_vehicles[vehicleIndex]->initTuning(vehicleConfig, vehicleTuning);
    }

    Driver(bool hasCamera, bool isPlayer, bool useKeyboard, i32 vehicleIndex,
            u32 colorIndex, u32 controllerID=0);

    Driver() = default;
    Driver(Driver&& other) = default;
    Driver& operator = (Driver&& other) = default;
};
