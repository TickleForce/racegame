#pragma once

#include "editor_mode.h"
#include "../input.h"
#include "../renderer.h"
#include "../imgui.h"
#include "../scene.h"

class TerrainMode : public EditorMode
{
    bool canUseTerrainTool = false;
    f32 brushRadius = 8.f;
    f32 brushFalloff = 1.f;
    f32 brushStrength = 15.f;
    f32 brushStartZ = 0.f;
    Vec3 mousePosition;

    enum struct TerrainTool : i32
    {
        RAISE,
        PERTURB,
        FLATTEN,
        SMOOTH,
        ERODE,
        MATCH_TRACK,
        PAINT,
        MAX
    } terrainTool = TerrainTool::RAISE;
    i32 paintMaterialIndex = 2;

public:
    TerrainMode() : EditorMode("Terrain") {}

    void onUpdate(Scene* scene, Renderer* renderer, f32 deltaTime) override
    {
        bool isMouseClickHandled = ImGui::GetIO().WantCaptureMouse;
        bool isKeyboardHandled = ImGui::GetIO().WantCaptureKeyboard;

        if (!isKeyboardHandled && g_input.isKeyPressed(KEY_SPACE))
        {
            terrainTool = TerrainTool(((u32)terrainTool + 1) % (u32)TerrainTool::MAX);
        }

        if (!isMouseClickHandled && g_input.isMouseButtonPressed(MOUSE_LEFT))
        {
            canUseTerrainTool = true;
        }

        RenderWorld* rw = renderer->getRenderWorld();
        Vec3 rayDir = scene->getEditorCamera().getMouseRay(rw);
        Camera const& cam = scene->getEditorCamera().getCamera();

        if (g_input.isKeyDown(KEY_LCTRL) && g_input.getMouseScroll() != 0)
        {
            brushRadius = clamp(brushRadius + g_input.getMouseScroll(), 2.0f, 40.f);
        }

        const f32 step = 0.01f;
        Vec3 p = cam.position;
        u32 count = 50000;
        while (p.z > scene->terrain->getZ(Vec2(p)) && --count > 0)
        {
            p += rayDir * step;
        }
        mousePosition = p;

        if (!isMouseClickHandled && count > 0)
        {
            scene->terrain->setBrushSettings(brushRadius, brushFalloff, brushStrength, p);
            if (g_input.isMouseButtonPressed(MOUSE_LEFT))
            {
                brushStartZ = scene->terrain->getZ(Vec2(p));
                PxRaycastBuffer hit;
                if (scene->raycastStatic(cam.position, rayDir, 10000.f, &hit, COLLISION_FLAG_TRACK))
                {
                    brushStartZ = max(brushStartZ, hit.block.position.z - 0.06f);
                }
            }
            if (g_input.isMouseButtonDown(MOUSE_LEFT) && canUseTerrainTool)
            {
                switch (terrainTool)
                {
                case TerrainTool::RAISE:
                    scene->terrain->raise(Vec2(p), brushRadius, brushFalloff, brushStrength * deltaTime);
                    break;
                case TerrainTool::PERTURB:
                    scene->terrain->perturb(Vec2(p), brushRadius, brushFalloff, brushStrength * deltaTime);
                    break;
                case TerrainTool::FLATTEN:
                    scene->terrain->flatten(Vec2(p), brushRadius, brushFalloff, absolute(brushStrength) * deltaTime, brushStartZ);
                    break;
                case TerrainTool::SMOOTH:
                    scene->terrain->smooth(Vec2(p), brushRadius, brushFalloff, absolute(brushStrength) * deltaTime);
                    break;
                case TerrainTool::ERODE:
                    scene->terrain->erode(Vec2(p), brushRadius, brushFalloff, absolute(brushStrength) * deltaTime);
                    break;
                case TerrainTool::MATCH_TRACK:
                    scene->terrain->matchTrack(Vec2(p), brushRadius, brushFalloff, absolute(brushStrength) * deltaTime, scene);
                    break;
                case TerrainTool::PAINT:
                    scene->terrain->paint(Vec2(p), brushRadius, brushFalloff, absolute(brushStrength * 1.f) * deltaTime, paintMaterialIndex);
                    break;
                default:
                    break;
                }
            }
        }
        scene->terrain->regenerateMesh();
    }

    void onEditorTabGui(Scene* scene, Renderer* renderer, f32 deltaTime) override
    {
        ImGui::Spacing();
        if (chooseResource(ResourceType::MATERIAL, scene->terrain->materialGuid, "Terrain Material",
                [](Resource* r){ return ((Material*)r)->materialType == MaterialType::TERRAIN; }))
        {
            scene->terrain->regenerateMaterial();
        }

        Vec2 terrainMin(scene->terrain->x1, scene->terrain->y1);
        Vec2 terrainMax(scene->terrain->x2, scene->terrain->y2);
        if (ImGui::InputFloat2("Terrain Min", (f32*)&terrainMin, "%.3f", ImGuiInputTextFlags_EnterReturnsTrue))
        {
            terrainMin = max(min(terrainMin, terrainMax - 10.f), Vec2(-400.f));
            scene->terrain->resize(terrainMin.x, terrainMin.y, terrainMax.x, terrainMax.y, true);
        }
        if (ImGui::InputFloat2("Terrain Max", (f32*)&terrainMax, "%.3f", ImGuiInputTextFlags_EnterReturnsTrue))
        {
            terrainMax = min(max(terrainMax, terrainMin + 10.f), Vec2(400.f));
            scene->terrain->resize(terrainMin.x, terrainMin.y, terrainMax.x, terrainMax.y, true);
        }

        ImGui::Gap();
        if (ImGui::BeginListBox("Terrain Tool", {0, 145}))
        {
            const char* toolNames[] = { "Raise / Lower", "Perturb", "Flatten", "Smooth", "Erode", "Match Track", "Paint" };
            for (i32 i=0; i<(i32)TerrainTool::MAX; ++i)
            {
                if (ImGui::Selectable(toolNames[i], i == (i32)terrainTool))
                {
                    terrainTool = (TerrainTool)i;
                }
            }
            ImGui::EndListBox();
        }

        ImGui::Gap();
        // TODO: Make brush settings per-tool
        ImGui::SliderFloat("Brush Radius", &brushRadius, 2.f, 40.f);
        ImGui::SliderFloat("Brush Falloff", &brushFalloff, 0.2f, 10.f);
        ImGui::SliderFloat("Brush Strength", &brushStrength, -30.f, 30.f);

        if (terrainTool == TerrainTool::PAINT)
        {
            ImGui::Text("Paint Material");
            for (i32 i=0; i<NUM_TERRAIN_LAYERS; ++i)
            {
                Material* m = scene->terrain->getMaterial();
                const u32 iconSize = 48;
                if (i > 0)
                {
                    ImGui::SameLine();
                }
                ImGui::PushID(i);
                bool isSelected = paintMaterialIndex == i;
                if (isSelected)
                {
                    const u32 selectedColor = 0x992299EE;
                    ImGui::PushStyleColor(ImGuiCol_Button, selectedColor);
                }
                if (ImGui::ImageButton((void*)(uintptr_t)scene->terrain->textures[i]->getPreviewTexture(),
                            ImVec2(iconSize, iconSize)))
                {
                    paintMaterialIndex = i;
                }
                if (isSelected)
                {
                    ImGui::PopStyleColor();
                }
                ImGui::PopID();
            }
        }

        ImGui::Gap();
        ImGui::Text("Mouse Position: %.3f, %.3f, %.3f", mousePosition.x, mousePosition.y, mousePosition.z);
    }

    void onBeginTest(Scene* scene) override
    {
        scene->terrain->regenerateCollisionMesh(scene);
    }

    void onSwitchFrom(Scene* scene) override
    {
        scene->terrain->regenerateCollisionMesh(scene);
    }
};
