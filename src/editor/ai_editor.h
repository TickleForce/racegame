#include "../resources.h"
#include "../imgui.h"
#include "../vinyl_pattern.h"
#include "resource_editor.h"
#include "resource_manager.h"

class AIEditor : public ResourceEditor
{
    struct VehicleRenderData
    {
        RenderWorld rw;
        VehicleTuning tuning;
    };
    Array<OwnedPtr<VehicleRenderData>> vehicleRenderData;

public:
    ~AIEditor()
    {
        for (auto& rd : vehicleRenderData)
        {
            rd->rw.destroy();
        }
    }

    void drawVehiclePreview(Renderer* renderer, RenderWorld& rw, VehicleTuning& tuning,
            AIVehicleConfiguration& ai)
    {
        rw.setName("AI Vehicle Preview");
        rw.setBloomForceOff(true);
        u32 rs = 160;
        rw.setSize(rs, rs);
        rw.setViewportCount(1);
        rw.addDirectionalLight(Vec3(-0.5f, 0.2f, -1.f), Vec3(1.0));
        rw.setViewportCamera(0, Vec3(8.f, -8.f, 10.f), Vec3(0.f, 0.f, 1.f), 1.f, 50.f, 30.f);

        Mesh* quadMesh = g_res.getModel("misc")->getMeshByName("Quad");
        drawSimple(&rw, quadMesh, &g_res.white, Mat4::scaling(Vec3(20.f)), Vec3(0.02f));

        const char* vehicleName = "None";
        VehicleData* vehicle = g_res.getVehicle(ai.vehicleGuid);
        if (vehicle)
        {
            vehicle->render(&rw, Mat4::translation(Vec3(0, 0, tuning.getRestOffset())),
                    nullptr, ai.config, &tuning, nullptr, false, false, Vec4(0));
            renderer->addRenderWorld(&rw);
            vehicleName = vehicle->name.data();
        }

        ImGui::Columns(2, nullptr, false);
        ImGui::SetColumnWidth(0, rs + 8);
        ImGui::Image((void*)(uintptr_t)rw.getTexture()->handle, { (f32)rs, (f32)rs },
                { 1.f, 1.f }, { 0.f, 0.f });
        ImGui::NextColumn();

        if (chooseResource(ResourceType::VEHICLE, ai.vehicleGuid, "Vehicle", [](Resource* r){
            return true;
        }))
        {
            VehicleData* v = (VehicleData*)g_res.getResource(ai.vehicleGuid);
            if (v)
            {
                v->initTuning(ai.config, tuning);
                ai.config.reloadMaterials();
            }
        }

        if (ImGui::TreeNodeEx("Paint", ImGuiTreeNodeFlags_DefaultOpen, "Paint"))
        {
            if (ImGui::ColorEdit3("Color", (f32*)&ai.config.cosmetics.color))
            {
                ai.config.reloadMaterials();
            }
            if (ImGui::SliderFloat("Shininess", &ai.config.cosmetics.paintShininess, 0.f, 1.f))
            {
                ai.config.reloadMaterials();
            }
            ImGui::TreePop();
        }

        for (u32 layerIndex=0; layerIndex<ARRAY_SIZE(ai.config.cosmetics.vinylGuids); ++layerIndex)
        {
            if (ImGui::TreeNodeEx(tmpStr("Vinyl Layer %i", layerIndex+1), 0, tmpStr("Vinyl Layer %i", layerIndex+1)))
            {
                const char* layerVinylName = "";
                i64 vinylGuid = ai.config.cosmetics.vinylGuids[layerIndex];
                if (vinylGuid && g_res.getResource(vinylGuid))
                {
                    layerVinylName = g_res.getResource(vinylGuid)->name.data();
                }
                else
                {
                    vinylGuid = 0;
                    ai.config.cosmetics.vinylGuids[layerIndex] = 0;
                    layerVinylName = "None";
                }
                if (ImGui::BeginCombo("Texture", layerVinylName))
                {
                    if (ImGui::Selectable("None###Absolutely Nothing"))
                    {
                        ai.config.cosmetics.vinylGuids[layerIndex] = 0;
                    }
                    if (vinylGuid == 0)
                    {
                        ImGui::SetItemDefaultFocus();
                    }
                    g_res.iterateResourceType(ResourceType::VINYL_PATTERN, [&](Resource* r) {
                        bool isSelected = r->guid == vinylGuid;
                        if (ImGui::Selectable(r->name.data()))
                        {
                            ai.config.cosmetics.vinylGuids[layerIndex] = r->guid;
                        }
                        if (isSelected)
                        {
                            ImGui::SetItemDefaultFocus();
                        }
                    });
                    ImGui::EndCombo();
                }
                ImGui::ColorEdit4("Color", (f32*)&ai.config.cosmetics.vinylColors[layerIndex]);
                ImGui::TreePop();
            }
        }

        if (vehicle)
        {
            if (ImGui::TreeNodeEx("Weapons", 0, "Weapons"))
            {
                for (u32 slotIndex=0; slotIndex<vehicle->weaponSlots.size(); ++slotIndex)
                {
                    i32 installedWeaponIndex = ai.config.weaponIndices[slotIndex];
                    const char* installedName =
                        installedWeaponIndex != -1 ? g_weapons[installedWeaponIndex].info.name : "None";
                    WeaponSlot slot = vehicle->weaponSlots[slotIndex];
                    if (ImGui::BeginCombo(slot.name.data(), installedName))
                    {
                        for (u32 i=0; i<g_weapons.size(); ++i)
                        {
                            auto& weapon = g_weapons[i];
                            if (weapon.info.weaponType != slot.weaponType || !slot.matchesWeapon(weapon.info))
                            {
                                continue;
                            }

                            bool isSelected = i == installedWeaponIndex;
                            if (ImGui::Selectable(weapon.info.name))
                            {
                                ai.config.weaponIndices[slotIndex] = i;
                                ai.config.weaponUpgradeLevel[slotIndex] = weapon.info.maxUpgradeLevel;
                            }
                            if (isSelected)
                            {
                                ImGui::SetItemDefaultFocus();
                            }
                        }
                        ImGui::EndCombo();
                    }
                }
                ImGui::TreePop();
            }
        }
        ImGui::Columns(1);
    }

    void init(Resource* r) override
    {
        AIDriverData& ai = *(AIDriverData*)r;
        for (auto& v : ai.vehicles)
        {
            vehicleRenderData.push(new VehicleRenderData());
            auto vehicle = g_res.getVehicle(v.vehicleGuid);
            if (vehicle)
            {
                vehicle->initTuning(v.config, vehicleRenderData.back()->tuning);
            }
        }
    }

    void onUpdate(Resource* r, ResourceManager* rm, Renderer* renderer, f32 deltaTime, u32 n) override
    {
        AIDriverData& ai = *(AIDriverData*)r;

        bool isOpen = true;
        if (ImGui::Begin(tmpStr("AI Driver Properties###AI Driver Properties %i", n), &isOpen))
        {
            ImGui::InputText("##Name", &ai.name);
            ImGui::Guid(ai.guid);
            ImGui::Gap();
            ImGui::Checkbox("Used in Championship", &ai.usedForChampionshipAI);
            ImGui::SliderFloat("Driving Skill", &ai.drivingSkill, 0.f, 1.f);
            ImGui::HelpMarker("How optimal of a path the AI takes on the track.");
            ImGui::SliderFloat("Aggression", &ai.aggression, 0.f, 1.f);
            ImGui::HelpMarker("How often the AI will go out of its way to attack other drivers.");
            ImGui::SliderFloat("Awareness", &ai.awareness, 0.f, 1.f);
            ImGui::HelpMarker("How much the AI will attempt to avoid hitting other drivers and obstacles.");
            ImGui::SliderFloat("Fear", &ai.fear, 0.f, 1.f);
            ImGui::HelpMarker("How much the AI will try to evade other drivers.");

            ImGui::Gap();

            for (u32 i=0; i<ai.vehicles.size();)
            {
                bool isOpen = ImGui::TreeNodeEx(tmpStr("Vehicle %i", i+1), ImGuiTreeNodeFlags_DefaultOpen);
                if (ImGui::BeginPopupContextItem())
                {
                    if (ImGui::MenuItem("Duplicate"))
                    {
                        auto data = Serializer::toDict(ai.vehicles[i]);
                        ai.vehicles.push({});
                        Serializer::fromDict(data, ai.vehicles.back());
                        vehicleRenderData.push(new VehicleRenderData());
                        VehicleData* vehicle = g_res.getVehicle(ai.vehicles.back().vehicleGuid);
                        if (vehicle)
                        {
                            vehicle->initTuning(
                                    ai.vehicles.back().config, vehicleRenderData.back()->tuning);
                        }
                    }
                    if (ImGui::MenuItem("Delete"))
                    {
                        ai.vehicles.erase(ai.vehicles.begin() + i);
                        vehicleRenderData[i]->rw.destroy();
                        vehicleRenderData.erase(vehicleRenderData.begin() + i);
                        ImGui::EndPopup();
                        if (isOpen)
                        {
                            ImGui::TreePop();
                        }
                        continue;
                    }
                    ImGui::EndPopup();
                }
                if (isOpen)
                {
                    drawVehiclePreview(renderer, vehicleRenderData[i]->rw,
                            vehicleRenderData[i]->tuning, ai.vehicles[i]);
                    ImGui::TreePop();
                }
                ++i;
            }

            // TODO: auto-filter out invalid vehicle indices and upgrade indices
            if (ImGui::Button("Add Vehicle"))
            {
                ai.vehicles.push({});
                ai.vehicles.back().purchaseActions.push({ PurchaseActionType::WEAPON, 0, 3 });
                ai.vehicles.back().purchaseActions.push({ PurchaseActionType::PERFORMANCE, 0, 1 });
                ai.vehicles.back().purchaseActions.push({ PurchaseActionType::WEAPON, 0, 5 });
                ai.vehicles.back().purchaseActions.push({ PurchaseActionType::PERFORMANCE, 2, 1 });
                ai.vehicles.back().purchaseActions.push({ PurchaseActionType::WEAPON, 1, 2 });
                ai.vehicles.back().purchaseActions.push({ PurchaseActionType::PERFORMANCE, 2, 2 });
                ai.vehicles.back().purchaseActions.push({ PurchaseActionType::PERFORMANCE, 0, 2 });
                vehicleRenderData.push(new VehicleRenderData());
            }
        }
        ImGui::End();

        if (!isOpen)
        {
            rm->markClosed(r);
        }
    }
};

