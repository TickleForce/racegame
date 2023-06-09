#pragma once

#include "../resources.h"
#include "../imgui.h"
#include "../vehicle_data.h"
#include "resource_editor.h"
#include "resource_manager.h"

class VehicleEditor : public ResourceEditor
{
    RenderWorld rw;
    VehicleConfiguration config;
    VehicleTuning tuning;

public:
    ~VehicleEditor()
    {
        rw.destroy();
    }

    void drawVehiclePreview(Renderer* renderer, RenderWorld& rw, VehicleData* v)
    {
        rw.setName("Vehicle Editor Preview");
        rw.setBloomForceOff(true);
        u32 w = 300, h = 200;
        rw.setSize(w, h);
        rw.setViewportCount(1);
        rw.addDirectionalLight(Vec3(-0.5f, 0.2f, -1.f), Vec3(1.0));
        rw.setViewportCamera(0, Vec3(8.f, -8.f, 10.f), Vec3(0.f, 0.f, 1.f), 1.f, 50.f, 30.f);

        Mesh* quadMesh = g_res.getModel("misc")->getMeshByName("Quad");
        drawSimple(&rw, quadMesh, &g_res.white, Mat4::scaling(Vec3(20.f)), Vec3(0.02f));

        v->render(&rw, Mat4::translation(Vec3(0, 0, tuning.getRestOffset())), nullptr, config, &tuning);
        renderer->addRenderWorld(&rw);

        ImGui::Image((void*)(uintptr_t)rw.getTexture()->handle, { (f32)w, (f32)h },
                { 1.f, 1.f }, { 0.f, 0.f });

        bool materialChanged = false;
        Vec3 col = hsvToRgb(v->defaultColorHsv.r, v->defaultColorHsv.g, v->defaultColorHsv.b);
        materialChanged |= ImGui::ColorEdit3("Default Color", (f32*)&col);
        materialChanged |= ImGui::SliderFloat("Default Shininess", &v->defaultShininess, 0.f, 1.f);
        config.cosmetics.paintShininess = v->defaultShininess;
        ImGui::ColorConvertRGBtoHSV(col.r, col.g, col.b,
                v->defaultColorHsv[0], v->defaultColorHsv[1], v->defaultColorHsv[2]);
        config.cosmetics.color = col;
        config.cosmetics.hsv = v->defaultColorHsv;

        if (materialChanged)
        {
            config.reloadMaterials();
        }

        ImGui::Columns(1);
    }

    void init(Resource* r) override
    {
        VehicleData* v = (VehicleData*)r;
        config.cosmetics.color = hsvToRgb(
                v->defaultColorHsv[0], v->defaultColorHsv[1], v->defaultColorHsv[2]);
        config.cosmetics.paintShininess = v->defaultShininess;
        v->initTuning(config, tuning);
    }

    void onUpdate(Resource* r, ResourceManager* rm, Renderer* renderer, f32 deltaTime, u32 n) override
    {
        VehicleData* v = (VehicleData*)r;
        VehicleTuning& t = v->defaultTuning;

        bool isOpen = true;
        if (ImGui::Begin(tmpStr("Vehicle Properties###Vehicle Properties %i", n), &isOpen))
        {
            ImGui::PushItemWidth(210.f);
            ImGui::InputText("##Name", &v->name);
            ImGui::Guid(v->guid);
            ImGui::Gap();

            drawVehiclePreview(renderer, rw, v);

            bool modelChanged = chooseResource(ResourceType::MODEL, v->modelGuid, "Model", [](Resource* r){
                return ((Model*)r)->modelUsage == ModelUsage::VEHICLE;
            });
            if (modelChanged)
            {
                v->initTuning(config, tuning);
            }

            ImGui::Checkbox("Show in Car Lot", &v->showInCarLot);
            ImGui::InputText("Description", &v->description);
            ImGui::InputInt("Price", &v->price, 200, 1000);
            ImGui::InputFloatClamp("Hit Points", &t.maxHitPoints, 1.f, 1000.f, 5.f, 10.f, "%.0f");

            // TODO: add help markers
            ImGui::InputFloatClamp("Mass", &t.chassisMass, 1.f, 10000.f, 50.f, 100.f);
            ImGui::InputFloat3("Center of Mass", (f32*)&t.centerOfMass);
            ImGui::InputFloatClamp("Top Speed", &t.topSpeed, 1.f, 500.f, 0.5f, 2.f);
            ImGui::InputFloatClamp("Drift Boost", &t.forwardDownforce, 0.f, 5.f, 0.01f, 0.02f);

            const char* diffTypeNames =
                "Open FWD\0Open RWD\0Open 4WD\0Limited Slip FWD\0Limited Slip RWD\0Limited Slip 4WD\0";
            ImGui::Combo("Differential", (i32*)&t.differential, diffTypeNames);

            if (ImGui::TreeNode("Performance Upgrades"))
            {
                for (u32 upgradeIndex=0; upgradeIndex<v->availableUpgrades.size(); ++upgradeIndex)
                {
                    auto& upgrade = v->availableUpgrades[upgradeIndex];
                    if (ImGui::TreeNode(tmpStr("Upgrade %i - %s###Upgrade %i",
                                    upgradeIndex + 1, upgrade.name.data(), upgradeIndex + 1)))
                    {
                        ImGui::InputText("Name", &upgrade.name);
                        ImGui::InputText("Description", &upgrade.description);
                        chooseTexture(TextureType::COLOR, upgrade.iconGuid, "Icon");
                        ImGui::InputInt("Cost per Level", &upgrade.price, 100, 200);
                        ImGui::InputInt("Max Level", &upgrade.maxUpgradeLevel, 1, 2);

                        if (ImGui::TreeNode("Stat Changes"))
                        {
                            ImGui::InputFloatClamp("Mass", &upgrade.stats.chassisMassDiff,
                                    -10000.f, 10000.f, 50.f, 100.f);
                            ImGui::InputFloatClamp("Top Speed", &upgrade.stats.topSpeedDiff,
                                    -10000.f, 10000.f, 0.5f, 1.f);
                            ImGui::InputFloatClamp("Hit Points", &upgrade.stats.maxHitPointsDiff,
                                    -1000.f, 1000.f, 10.f, 100.f);
                            ImGui::InputFloatClamp("Wheel Damping", &upgrade.stats.wheelDampingRateDiff,
                                    -100.f, 100.f);
                            ImGui::InputFloatClamp("Wheel Offroad Damping", &upgrade.stats.wheelOffroadDampingRateDiff,
                                    -100.f, 100.f);
                            ImGui::InputFloatClamp("Tire Grip", &upgrade.stats.trackTireFrictionDiff,
                                    -100.f, 100.f);
                            ImGui::InputFloatClamp("Offroad Tire Grip", &upgrade.stats.offroadTireFrictionDiff,
                                    -100.f, 100.f);
                            ImGui::InputFloatClamp("Downforce", &upgrade.stats.constantDownforceDiff,
                                    -100.f, 100.f);
                            ImGui::InputFloatClamp("Forward Downforce", &upgrade.stats.forwardDownforceDiff,
                                    -100.f, 100.f);
                            ImGui::InputFloatClamp("Max Engine Omega", &upgrade.stats.maxEngineOmegaDiff,
                                    -1000.f, 1000.f);
                            ImGui::InputFloatClamp("Peak Engine Torque", &upgrade.stats.peakEngineTorqueDiff,
                                    -1000.f, 1000.f);
                            ImGui::InputFloatClamp("Anti-Rollbar Stiffness", &upgrade.stats.antiRollbarStiffnessDiff,
                                    -5000.f, 5000.f);
                            ImGui::InputFloatClamp("Spring Strength", &upgrade.stats.suspensionSpringStrengthDiff,
                                    -5000.f, 5000.f);
                            ImGui::InputFloatClamp("Spring Damper Rate", &upgrade.stats.suspensionSpringDamperRateDiff,
                                    -5000.f, 5000.f);
                            const char* diffTypeNames =
                                "Open FWD\0Open RWD\0Open 4WD\0Limited Slip FWD\0Limited Slip RWD\0Limited Slip 4WD\0None\0";
                            ImGui::Combo("Differential", (i32*)&upgrade.stats.differential, diffTypeNames);

                            ImGui::TreePop();
                        }

                        ImGui::TreePop();
                    }
                }
                if (ImGui::Button("Add Upgrade"))
                {
                    v->availableUpgrades.push({});
                }
                ImGui::SameLine();
                if (ImGui::Button("Remove Upgrade") && v->availableUpgrades.size() > 0)
                {
                    v->availableUpgrades.pop();
                }
                ImGui::TreePop();
            }

            if (ImGui::TreeNode("Weapon Slots"))
            {
                for (u32 slotIndex=0; slotIndex<v->weaponSlots.size(); ++slotIndex)
                {
                    auto& slot = v->weaponSlots[slotIndex];
                    if (ImGui::TreeNode(tmpStr("Weapon Slot %i - %s###Slot %i",
                                    slotIndex + 1, slot.name.data(), slotIndex + 1)))
                    {
                        const char* weaponTypeNames = "Front Weapon\0Rear Weapon\0Passive Ability\0";
                        ImGui::Combo(tmpStr("Weapon Type", slotIndex),
                                (i32*)&v->weaponSlots[slotIndex].weaponType, weaponTypeNames);
                        ImGui::InputText("Name", &slot.name);

                        ImGui::Columns(ARRAY_SIZE(slot.tagGroups));
                        for (u32 i=0; i<ARRAY_SIZE(slot.tagGroups); ++i)
                        {
                            ImGui::Text(tmpStr("Tag Group %i", i + 1));
                            for (u32 j=0; j<ARRAY_SIZE(slot.tagGroups[i].tags); ++j)
                            {
                                // TODO: add drop down to pick tags
                                ImGui::InputText(tmpStr("###%i%i", i, j), &slot.tagGroups[i].tags[j]);
                            }
                            ImGui::NextColumn();
                        }
                        ImGui::Columns(1);
                        ImGui::TreePop();
                    }
                }
                if (ImGui::Button("Add Slot") && v->weaponSlots.size() < v->weaponSlots.maximumSize())
                {
                    v->weaponSlots.push({});
                }
                ImGui::SameLine();
                if (ImGui::Button("Remove Slot") && v->weaponSlots.size() > 0)
                {
                    v->weaponSlots.pop();
                }
                ImGui::TreePop();
            }

            if (ImGui::TreeNode("Wheels"))
            {
                ImGui::InputFloatClamp("Wheel Damping", &t.wheelDampingRate, 0.f, 100.f);
                ImGui::InputFloatClamp("Wheel Offroad Damping", &t.wheelOffroadDampingRate, 0.f, 100.f);
                ImGui::InputFloatClamp("Grip", &t.trackTireFriction, 0.f, 20.f);
                ImGui::InputFloatClamp("Offroad Grip", &t.offroadTireFriction, 0.f, 20.f);
                ImGui::InputFloatClamp("Rear Grip Multiplier", &t.rearTireGripPercent, 0.f, 2.f);
                ImGui::InputFloatClamp("Wheel Mass Front", &t.wheelMassFront, 1.f, 100.f);
                ImGui::InputFloatClamp("Wheel Mass Rear", &t.wheelMassRear, 1.f, 100.f);
                ImGui::InputFloatClamp("Max Brake Torque", &t.engineDampingZeroThrottleClutchDisengaged, 0.f, 80000.f);
                ImGui::InputFloatClamp("Camber at Rest", &t.camberAngleAtRest, -2.f, 2.f);
                ImGui::InputFloatClamp("Camber at Max Droop", &t.camberAngleAtMaxDroop, -2.f, 2.f);
                ImGui::InputFloatClamp("Camber at Max Compression", &t.camberAngleAtMaxCompression, -2.f, 2.f);
                ImGui::InputFloatClamp("Front Toe Angle", &t.frontToeAngle, -2.f, 2.f);
                ImGui::InputFloatClamp("Rear Toe Angle", &t.rearToeAngle, -2.f, 2.f);
                ImGui::InputFloatClamp("Max Steer Angle", &t.maxSteerAngleDegrees, 0.f, 90.f);
                ImGui::InputFloatClamp("Ackermann Accuracy", &t.ackermannAccuracy, 0.f, 1.f);
                ImGui::TreePop();
            }
            if (ImGui::TreeNode("Suspension"))
            {
                ImGui::InputFloatClamp("Max Compression", &t.suspensionMaxCompression, 0.f, 10.f);
                ImGui::InputFloatClamp("Max Droop", &t.suspensionMaxDroop, 0.f, 10.f);
                ImGui::InputFloatClamp("Spring Strength", &t.suspensionSpringStrength, 100.f, 100000.f);
                ImGui::InputFloatClamp("Spring Damper Rate", &t.suspensionSpringDamperRate, 100.f, 10000.f);
                ImGui::InputFloatClamp("Anti-Rollbar Stiffness", &t.frontAntiRollbarStiffness, 0.f, 100000.f);
                t.rearAntiRollbarStiffness = t.frontAntiRollbarStiffness;
                ImGui::TreePop();
            }
            if (ImGui::TreeNode("Engine"))
            {
                ImGui::InputFloatClamp("Max Omega", &t.maxEngineOmega, 100.f, 5000.f);
                ImGui::InputFloatClamp("Peak Torque", &t.peakEngineTorque, 50.f, 5000.f);
                ImGui::InputFloatClamp("Damping Full Throttle", &t.engineDampingFullThrottle, 0.f, 500.f);
                ImGui::InputFloatClamp("Damping Zero Throttle With Clutch", &t.engineDampingZeroThrottleClutchEngaged, 0.f, 500.f);
                ImGui::InputFloatClamp("Damping Zero Throttle W/O Clutch", &t.engineDampingZeroThrottleClutchDisengaged, 0.f, 500.f);
                ImGui::TreePop();
            }
            if (ImGui::TreeNode("Downforce"))
            {
                ImGui::InputFloatClamp("Downforce", &t.constantDownforce, 0.f, 5.f);
                ImGui::InputFloatClamp("Forward Downforce", &t.forwardDownforce, 0.f, 5.f);
                ImGui::TreePop();
            }
            if (ImGui::TreeNode("Transmission"))
            {
                ImGui::InputFloatClamp("Final Gear Ratio", &t.finalGearRatio, 0.1f, 20.f);
                ImGui::InputFloatClamp("Clutch Strength", &t.clutchStrength, 0.1f, 100.f);
                ImGui::InputFloatClamp("Gear Switch Delay", &t.gearSwitchTime, 0.f, 2.f);
                ImGui::InputFloatClamp("Autobox Switch Delay", &t.autoBoxSwitchTime, 0.f, 2.f);
                const char* gearName[] = { "Reverse", "Neutral", "1st", "2nd", "3rd", "4th", "5th", "6th", "7th", "8th", "9th", "10th" };
                for (u32 i=0; i<t.gearRatios.size(); ++i)
                {
                    ImGui::SetNextItemWidth(164.f);
                    if (i == 0)
                    {
                        ImGui::InputFloatClamp(gearName[i], &t.gearRatios[i], -10.f, -0.1f);
                    }
                    else if (i == 1)
                    {
                        ImGui::InputFloatClamp(gearName[i], &t.gearRatios[i], 0.f, 0.f);
                    }
                    else
                    {
                        ImGui::InputFloatClamp(gearName[i], &t.gearRatios[i], 0.1f, 10.f);
                    }
                }
                if (ImGui::Button("Add Gear") && t.gearRatios.size() < t.gearRatios.maximumSize())
                {
                    t.gearRatios.push(0.f);
                }
                ImGui::SameLine();
                if (ImGui::Button("Remove Gear") && t.gearRatios.size() > 2)
                {
                    t.gearRatios.pop();
                }
                ImGui::TreePop();
            }

            ImGui::PopItemWidth();
        }
        ImGui::End();

        if (!isOpen)
        {
            rm->markClosed(r);
        }
    }
};

