#pragma once

#include "../resources.h"
#include "../imgui.h"
#include "../vinyl_pattern.h"
#include "resource_editor.h"
#include "resource_manager.h"

class VinylPatternEditor : public ResourceEditor
{
public:
    void init(Resource* r) override
    {
    }

    void onUpdate(Resource* r, ResourceManager* rm, Renderer* renderer, f32 deltaTime, u32 n) override
    {
        VinylPattern& p = *(VinylPattern*)r;

        bool isOpen = true;
        bool dirty = false;
        if (ImGui::Begin(tmpStr("Vinyl Pattern Properties###Vinyl Pattern Properties %i", n), &isOpen))
        {
            dirty |= ImGui::InputText("##Name", &p.name);
            ImGui::Guid(p.guid);
            dirty |= ImGui::Checkbox("Useable by Player", &p.canBeUsedByPlayer);
            dirty |= rm->chooseTexture(TextureType::COLOR, p.colorTextureGuid, "Color Texture");
            //rm->chooseTexture(TextureType::NORMAL_MAP, p.normalTextureGuid, "Normal Map");
            //rm->chooseTexture(TextureType::COLOR, p.normalTextureGuid, "Shininess Map");
        }
        ImGui::End();

        if (!isOpen)
        {
            rm->markClosed(r);
        }

        if (dirty)
        {
            rm->markDirty(r->guid);
        }
    }
};

