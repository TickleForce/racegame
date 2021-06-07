constexpr f32 REFERENCE_HEIGHT = 1080.f;
f32 convertSize(f32 size)
{
    return floorf(size * (g_game.windowHeight / REFERENCE_HEIGHT));
}

Vec2 convertSize(Vec2 size)
{
    return Vec2(convertSize(size.x), convertSize(size.y));
}

i32 Menu::didChangeSelectionX()
{
    i32 result = (i32)(g_input.isKeyPressed(KEY_RIGHT, true) || didSelect())
                - (i32)g_input.isKeyPressed(KEY_LEFT, true);

    for (auto& pair : g_input.getControllers())
    {
        i32 tmpResult = pair.value.isButtonPressed(BUTTON_DPAD_RIGHT) -
                        pair.value.isButtonPressed(BUTTON_DPAD_LEFT);
        if (!tmpResult)
        {
            if (repeatTimer == 0.f)
            {
                f32 xaxis = pair.value.getAxis(AXIS_LEFT_X);
                if (xaxis < -JOYSTICK_DEADZONE)
                {
                    tmpResult = -1;
                    repeatTimer = 0.1f;
                }
                else if (xaxis > JOYSTICK_DEADZONE)
                {
                    tmpResult = 1;
                    repeatTimer = 0.1f;
                }
            }
        }
        if (tmpResult)
        {
            result = tmpResult;
            break;
        }
    }

    return result;
}

i32 Menu::didChangeSelectionY()
{
    i32 result = (i32)g_input.isKeyPressed(KEY_DOWN, true)
                - (i32)g_input.isKeyPressed(KEY_UP, true);

    for (auto& pair : g_input.getControllers())
    {
        i32 tmpResult = pair.value.isButtonPressed(BUTTON_DPAD_DOWN) -
                        pair.value.isButtonPressed(BUTTON_DPAD_UP);
        if (!tmpResult)
        {
            if (repeatTimer == 0.f)
            {
                f32 yaxis = pair.value.getAxis(AXIS_LEFT_Y);
                if (yaxis < -JOYSTICK_DEADZONE)
                {
                    tmpResult = -1;
                    repeatTimer = 0.1f;
                }
                else if (yaxis > JOYSTICK_DEADZONE)
                {
                    tmpResult = 1;
                    repeatTimer = 0.1f;
                }
            }
        }
        if (tmpResult)
        {
            result = tmpResult;
            break;
        }
    }

    return result;
}

f32 didMoveX()
{
    f32 result = (f32)g_input.isKeyDown(KEY_RIGHT) - (f32)g_input.isKeyDown(KEY_LEFT);
    for (auto& pair : g_input.getControllers())
    {
        result += (f32)pair.value.isButtonDown(BUTTON_DPAD_RIGHT) -
                  (f32)pair.value.isButtonDown(BUTTON_DPAD_LEFT);
        result += pair.value.getAxis(AXIS_LEFT_X);
    }
    return result;
}

Widget* Menu::addBackgroundBox(Vec2 pos, Vec2 size, f32 alpha, bool scaleOut)
{
    Widget w;
    w.pos = pos;
    w.size = size;
    w.fadeInScale = 1.f;
    w.onRender = [alpha, scaleOut](Widget& w, bool){
        Vec2 center = Vec2(g_game.windowWidth, g_game.windowHeight) * 0.5f;
        f32 scale = scaleOut ? w.fadeInScale : 1.f;
        Vec2 size = convertSize(w.size) * scale;
        Vec2 pos = convertSize(w.pos) + center - size * 0.5f;
        ui::rectBlur(-1000, nullptr, pos, size, Vec4(Vec3(0.f), alpha), w.fadeInAlpha);
    };
    w.flags = 0;
    widgets.push(w);
    return &widgets.back();
}

Widget* Menu::addLogic(std::function<void()> onUpdate)
{
    Widget w;
    w.flags = 0;
    w.onRender = [onUpdate](Widget& w, bool isSelected){ onUpdate(); };
    widgets.push(w);
    return &widgets.back();
}

Widget* Menu::addLogic(std::function<void(Widget&)> onUpdate)
{
    Widget w;
    w.flags = 0;
    w.onRender = [onUpdate](Widget& w, bool isSelected){ onUpdate(w); };
    widgets.push(w);
    return &widgets.back();
}

void drawSelectableBox(Widget& w, bool isSelected, bool isEnabled,
        Vec4 color=Vec4(0,0,0,0.8f), Vec4 unselectedBorderColor=COLOR_NOT_SELECTED)
{
    f32 borderSize = convertSize(isSelected ? 5 : 2);
    Vec4 borderColor = mix(unselectedBorderColor, COLOR_SELECTED, w.hover);
    Vec2 center = Vec2(g_game.windowWidth, g_game.windowHeight) * 0.5f;
    Vec2 size = convertSize(w.size) * w.fadeInScale;
    Vec2 pos = convertSize(w.pos) + center - size * 0.5f;
    ui::rectBlur(ui::BORDER, &g_res.white,
                pos - borderSize, size + borderSize * 2,
                borderColor, (0.5f + w.hover * 0.5f) * w.fadeInAlpha);
    if (!isEnabled)
    {
        color.a = 0.4f;
    }
    Vec3 col = mix(Vec3(color), Vec3(1.f),
            (sinf(w.hoverTimer * 4.f) + 1.f)*0.5f*w.hover*0.04f);
    ui::rectBlur(ui::BG, &g_res.white, pos, size,
            Vec4(col, color.a), w.fadeInAlpha);
}

Widget* Menu::addButton(const char* text, const char* helpText, Vec2 pos, Vec2 size,
        std::function<void()> onSelect, u32 flags, Texture* image, std::function<bool()> isEnabled)
{
    Font* font = &g_res.getFont("font", (u32)convertSize(38));

    Widget button;
    button.helpText = helpText;
    button.pos = pos;
    button.size = size;
    button.onSelect = ::move(onSelect);
    button.onRender = [image, font, text, isEnabled](Widget& btn, bool isSelected){
        bool enabled = isEnabled();
        drawSelectableBox(btn, isSelected, enabled);
        Vec2 center = Vec2(g_game.windowWidth, g_game.windowHeight) * 0.5f;
        Vec2 size = convertSize(btn.size) * btn.fadeInScale;
        Vec2 pos = convertSize(btn.pos) + center - size * 0.5f;
        if (enabled)
        {
            btn.flags = btn.flags & ~WidgetFlags::CANNOT_ACTIVATE;
        }
        else
        {
            btn.flags |= WidgetFlags::CANNOT_ACTIVATE;
        }
        if (image)
        {
            f32 imageSize = convertSize(64) * btn.fadeInScale;
            f32 textOffsetX = imageSize + convertSize(40);
#if 1
            ui::text(font, text, pos + Vec2(textOffsetX, size.y * 0.5f),
                        Vec3(1.f), (isSelected ? 1.f : 0.5f) * btn.fadeInAlpha, btn.fadeInScale,
                        HAlign::LEFT, VAlign::CENTER);
#else
            ui::text(font, text, pos + size * 0.5f,
                        Vec3(1.f), (isSelected ? 1.f : 0.5f) * btn.fadeInAlpha, btn.fadeInScale,
                        HAlign::CENTER, VAlign::CENTER));
#endif
            ui::rectBlur(ui::IMAGE, image, pos + Vec2(convertSize(20), size.y*0.5f-imageSize*0.5f),
                    Vec2(imageSize), Vec4(1.f), btn.fadeInAlpha);
        }
        else
        {
            f32 alpha = (enabled ? (isSelected ? 1.f : 0.5f) : 0.25f) * btn.fadeInAlpha;
            ui::text(font, text, pos + size * 0.5f,
                    Vec3(1.f), alpha, btn.fadeInScale, HAlign::CENTER, VAlign::CENTER);
        }
    };
    button.fadeInScale = 0.7f;
    button.flags = WidgetFlags::SELECTABLE |
                   WidgetFlags::NAVIGATE_VERTICAL |
                   WidgetFlags::NAVIGATE_HORIZONTAL | flags;
    widgets.push(button);
    return &widgets.back();
}

Widget* Menu::addImageButton(const char* text, const char* helpText, Vec2 pos, Vec2 size,
        std::function<void()> onSelect, u32 flags, Texture* image, f32 imageMargin,
        std::function<ImageButtonInfo(bool isSelected)> getInfo)
{
    Font* font = &g_res.getFont("font", (u32)convertSize(22));

    Widget button;
    button.helpText = helpText;
    button.pos = pos;
    button.size = size;
    button.onSelect = ::move(onSelect);
    button.onRender = [=](Widget& btn, bool isSelected){
        auto info = getInfo(isSelected);
#if 0
        drawSelectableBox(btn, isSelected, info.isEnabled, {0,0,0,0.8f},
                info.isHighlighted ? Vec4(0.05f,0.05f,1.f,1.f) : COLOR_NOT_SELECTED);
#else
        drawSelectableBox(btn, isSelected, info.isEnabled, {0,0,0,0.8f},
                info.isHighlighted ? COLOR_SELECTED : COLOR_NOT_SELECTED);
#endif
        Vec2 center = Vec2(g_game.windowWidth, g_game.windowHeight) * 0.5f;
        Vec2 imageSize = convertSize(btn.size - imageMargin) * btn.fadeInScale;
        Vec2 pos = convertSize(btn.pos) + center - imageSize * 0.5f;
        f32 textOffsetY1 = convertSize(btn.pos.y) + center.y - (convertSize(btn.size.y) * 0.5f
            - convertSize(16)) * btn.fadeInScale;
        f32 textOffsetY2 = convertSize(btn.pos.y) + center.y + (convertSize(btn.size.y) * 0.5f
            - convertSize(16)) * btn.fadeInScale;

        f32 alpha = (info.isEnabled ? (isSelected ? 1.f : 0.8f) : 0.25f) * btn.fadeInAlpha;
        if (info.flipImage)
        {
            ui::rectUVBlur(ui::IMAGE, image, pos, imageSize, {1,1}, {0,0}, Vec4(1.f), alpha);
        }
        else
        {
            ui::rectBlur(ui::IMAGE, image, pos, imageSize, Vec4(1.f), alpha);
        }

        ui::text(font, text, {pos.x + imageSize.x*0.5f, textOffsetY1},
                    Vec3(1.f), alpha, btn.fadeInScale, HAlign::CENTER, VAlign::CENTER);

        if (info.maxUpgradeLevel == 0 || info.upgradeLevel < info.maxUpgradeLevel)
        {
            ui::text(font, info.bottomText, {pos.x + imageSize.x*0.5f, textOffsetY2},
                    Vec3(1.f), alpha, btn.fadeInScale, HAlign::CENTER, VAlign::CENTER);
        }

        if (info.maxUpgradeLevel > 0)
        {
            Texture* tex1 = g_res.getTexture("button");
            Texture* tex2 = g_res.getTexture("button_glow");
            f32 size = convertSize(16);
            f32 sep = convertSize(20);
            for (i32 i=0; i<info.maxUpgradeLevel; ++i)
            {
                Vec2 pos = center + convertSize(btn.pos) - (convertSize({btn.size.x, 0}) * 0.5f
                    - Vec2(0, sep * info.maxUpgradeLevel * 0.5f) + Vec2(0, sep * i)
                    - convertSize({ 12, -10 })) * btn.fadeInScale;
                ui::rectBlur(ui::IMAGE, tex1, pos - size*0.5f * btn.fadeInScale,
                            Vec2(size * btn.fadeInScale), Vec4(1.f), btn.fadeInAlpha);
            }
            for (i32 i=0; i<info.upgradeLevel; ++i)
            {
                Vec2 pos = center + convertSize(btn.pos) - (convertSize({btn.size.x, 0}) * 0.5f
                    - Vec2(0, sep * info.maxUpgradeLevel * 0.5f) + Vec2(0, sep * i)
                    - convertSize({ 12, -10 })) * btn.fadeInScale;
                ui::rectBlur(ui::IMAGE, tex2, pos - size*0.5f * btn.fadeInScale,
                            Vec2(size * btn.fadeInScale), Vec4(1.f), btn.fadeInAlpha);
            }
        }
    };
    button.fadeInScale = 0.7f;
    button.flags = WidgetFlags::SELECTABLE |
                   WidgetFlags::NAVIGATE_VERTICAL |
                   WidgetFlags::NAVIGATE_HORIZONTAL | flags;
    widgets.push(button);
    return &widgets.back();
}

Widget* Menu::addSlider(const char* text, Vec2 pos, Vec2 size, u32 flags,
            std::function<void(f32)> onValueChanged, std::function<SliderInfo()> getInfo)
{
    Font* font = &g_res.getFont("font", (u32)convertSize(28));

    Widget w;
    w.pos = pos;
    w.size = size;
    w.flags = WidgetFlags::SELECTABLE | WidgetFlags::NAVIGATE_VERTICAL | flags;
    w.onSelect = []{};
    w.onRender = [=](Widget& w, bool isSelected) {
        drawSelectableBox(w, isSelected, true, {0,0,0,0.5f});

        Vec2 center = Vec2(g_game.windowWidth, g_game.windowHeight) * 0.5f;
        Vec2 size = convertSize({w.size.x, w.size.y * 0.5f}) * w.fadeInScale;
        Vec2 pos = convertSize(w.pos) + center - Vec2(size.x * 0.5f, 0.f);

        auto info = getInfo();

        f32 alpha = (isSelected ? 1.f : 0.6f) * w.fadeInAlpha;
        ui::text(font, text, pos + Vec2(size.x * 0.5f, -convertSize(8)),
                    Vec3(1.f), alpha, w.fadeInScale, HAlign::CENTER, VAlign::BOTTOM);
        ui::rectBlur(ui::IMAGE, info.tex, pos, size, info.color1, info.color2, w.fadeInAlpha);

        if (isSelected && w.fadeInAlpha > 0.5f)
        {
            f32 dir = didMoveX();
            if (dir != 0.f)
            {
                f32 t = (info.val - info.min) / (info.max - info.min);
                t = clamp(t + dir * g_game.realDeltaTime * 0.5f, 0.f, 1.f);
                onValueChanged(lerp(info.min, info.max, t));
            }
            if (g_input.isMouseButtonDown(MOUSE_LEFT))
            {
                Vec2 mousePos = g_input.getMousePosition();
                f32 localX = mousePos.x - pos.x;
                f32 t = clamp(localX / size.x, 0.f, 1.f);
                onValueChanged(lerp(info.min, info.max, t));
            }
        }

        f32 t = (info.val - info.min) / (info.max - info.min);
        f32 handleWidth = convertSize(6);
        ui::rectBlur(ui::IMAGE, &g_res.white, pos + Vec2(t * size.x - handleWidth / 2, 0.f),
                Vec2(handleWidth, size.y), Vec4(0.1f, 0.1f, 0.1f, 1.f), w.fadeInAlpha);
    };
    w.fadeInScale = 0.7f;
    widgets.push(w);
    return &widgets.back();
}

Widget* Menu::addSelector2(const char* text, const char* helpText, Vec2 pos, Vec2 size,
        u32 flags, std::function<SelectorInfo()> getInfo,
        std::function<void(i32 valueIndex)> onValueChanged)
{
    Font* font = &g_res.getFont("font", (u32)convertSize(28));
    Font* fontBig = &g_res.getFont("font", (u32)convertSize(34));

    Widget w;
    w.pos = pos;
    w.size = size;
    w.flags = WidgetFlags::SELECTABLE | WidgetFlags::NAVIGATE_VERTICAL | flags;
    w.onSelect = []{};
    w.onRender = [=](Widget& w, bool isSelected) {
        drawSelectableBox(w, isSelected, true, {0,0,0,0.5f});

        Vec2 center = Vec2(g_game.windowWidth, g_game.windowHeight) * 0.5f;
        Vec2 size = convertSize({w.size.x, w.size.y * 0.5f}) * w.fadeInScale;
        Vec2 pos = convertSize(w.pos) + center - Vec2(size.x * 0.5f, 0.f);

        auto info = getInfo();

        f32 alpha = (isSelected ? 1.f : 0.6f) * w.fadeInAlpha;
        ui::text(font, text, pos + Vec2(size.x * 0.5f, -convertSize(8)),
                    Vec3(1.f), alpha, w.fadeInScale, HAlign::CENTER, VAlign::BOTTOM);
        ui::text(fontBig, info.selectedItemText, pos + size * 0.5f, Vec3(1.f), alpha, w.fadeInScale,
                HAlign::CENTER, VAlign::CENTER);

        if (isSelected)
        {
            Texture* cheveron = g_res.getTexture("cheveron");
            f32 cheveronSize = convertSize(32);
            f32 offset = convertSize(150.f);
            ui::rectBlur(ui::ICON, cheveron,
                    pos + Vec2(size.x*0.5f - offset - cheveronSize, size.y*0.5f - cheveronSize*0.5f),
                    Vec2(cheveronSize), Vec4(1.f), w.fadeInAlpha * w.hover);
            ui::rectUVBlur(ui::ICON, cheveron,
                    pos + Vec2(size.x*0.5f + offset, size.y*0.5f - cheveronSize*0.5f),
                    Vec2(cheveronSize), {1,0}, {0,1}, Vec4(1.f), w.fadeInAlpha * w.hover);

            i32 dir = didChangeSelectionX();
            if (dir)
            {
                g_audio.playSound(g_res.getSound("click"), SoundType::MENU_SFX);
            }
            if (g_input.isMouseButtonPressed(MOUSE_LEFT))
            {
                Vec2 mousePos = g_input.getMousePosition();
                f32 x = convertSize(pos.x + size.x * 0.5f);
                dir = mousePos.x - x < 0 ? -1 : 1;
            }
            if (dir)
            {
                onValueChanged(info.currentIndex + dir);
            }
        }
    };
    w.fadeInScale = 0.7f;
    widgets.push(w);
    return &widgets.back();
}

Widget* Menu::addSelector(const char* text, const char* helpText, Vec2 pos, Vec2 size,
        SmallArray<Str32> values, i32 valueIndex,
        std::function<void(i32 valueIndex)> onValueChanged)
{
    Font* font = &g_res.getFont("font", (u32)convertSize(34));

    Widget w;
    w.helpText = helpText;
    w.pos = pos;
    w.size = size;
    w.onSelect = []{};
    w.onRender = [=](Widget& w, bool isSelected) mutable {
        Vec2 center = Vec2(g_game.windowWidth, g_game.windowHeight) * 0.5f;
        Vec2 size = convertSize(w.size) * w.fadeInScale;
        Vec2 pos = convertSize(w.pos) + center - size * 0.5f;
        if (isSelected)
        {
            f32 borderSize = convertSize(isSelected ? 5 : 2);
            Vec4 borderColor = mix(COLOR_NOT_SELECTED, COLOR_SELECTED, w.hover);
            ui::rectBlur(ui::BORDER, &g_res.white, pos - borderSize, size + borderSize * 2,
                        borderColor, (0.5f + w.hover * 0.5f) * w.fadeInAlpha);
        }
        ui::rectBlur(ui::BG, &g_res.white, pos, size,
                Vec4(Vec3((sinf(w.hoverTimer * 4.f) + 1.f)*0.5f*w.hover*0.04f), 0.8f),
                w.fadeInAlpha);
        ui::text(font, text,
                    pos + Vec2(convertSize(20.f), size.y * 0.5f),
                    Vec3(1.f), (isSelected ? 1.f : 0.5f) * w.fadeInAlpha, w.fadeInScale,
                    HAlign::LEFT, VAlign::CENTER);
        ui::text(font, tmpStr("%s", values[valueIndex].data()),
                    pos + Vec2(size.x * 0.75f, size.y * 0.5f),
                    Vec3(1.f), (isSelected ? 1.f : 0.5f) * w.fadeInAlpha, w.fadeInScale,
                    HAlign::CENTER, VAlign::CENTER);

        if (isSelected)
        {
            Texture* cheveron = g_res.getTexture("cheveron");
            f32 cheveronSize = convertSize(36);
            f32 offset = convertSize(150.f);
            ui::rectBlur(ui::ICON, cheveron,
                    pos + Vec2(size.x*0.75f - offset - cheveronSize, size.y*0.5f - cheveronSize*0.5f),
                    Vec2(cheveronSize), Vec4(1.f), w.fadeInAlpha * w.hover);
            ui::rectUVBlur(ui::ICON, cheveron,
                    pos + Vec2(size.x*0.75f + offset, size.y*0.5f - cheveronSize*0.5f),
                    Vec2(cheveronSize), {1,0}, {0,1}, Vec4(1.f), w.fadeInAlpha * w.hover);

            i32 dir = didChangeSelectionX();
            if (dir)
            {
                g_audio.playSound(g_res.getSound("click"), SoundType::MENU_SFX);
            }
            if (g_input.isMouseButtonPressed(MOUSE_LEFT))
            {
                Vec2 mousePos = g_input.getMousePosition();
                f32 x = convertSize(pos.x + size.x * 0.75f);
                dir = mousePos.x - x < 0 ? -1 : 1;
            }
            if (dir)
            {
                valueIndex += dir;
                if (valueIndex < 0)
                {
                    valueIndex = (i32)values.size() - 1;
                }
                if (valueIndex >= (i32)values.size())
                {
                    valueIndex = 0;
                }
                onValueChanged(valueIndex);
            }
        }
    };
    w.fadeInScale = 0.7f;
    w.flags = WidgetFlags::SELECTABLE |
              WidgetFlags::NAVIGATE_VERTICAL;
    widgets.push(w);
    return &widgets.back();
}

Widget* Menu::addHelpMessage(Vec2 pos)
{
    Widget w;
    w.pos = pos;
    w.onRender = [this](Widget& widget, bool isSelected) {
        if (selectedWidget->helpText)
        {
            Font* font = &g_res.getFont("font", (u32)convertSize(26));
            Vec2 textSize = font->stringDimensions(selectedWidget->helpText) + convertSize({50,30});
            Vec2 center = Vec2(g_game.windowWidth, g_game.windowHeight) * 0.5f;
            Vec2 size = textSize * widget.fadeInScale;
            Vec2 pos = convertSize(widget.pos) + center - size * 0.5f;
            f32 alpha = selectedWidget->hover * widget.fadeInAlpha;
            ui::rectBlur(ui::BG, &g_res.white, pos, size, Vec4(0,0,0,0.5f), alpha);
            ui::text(font, selectedWidget->helpText,
                        pos + size * 0.5f, Vec3(1.f), alpha, widget.fadeInScale,
                        HAlign::CENTER, VAlign::CENTER);
        }
    };
    w.flags = 0;
    widgets.push(w);
    return &widgets.back();
}

Widget* Menu::addLabel(std::function<const char*()> getText, Vec2 pos, Font* font,
        HAlign halign, VAlign valign, Vec3 const& color, u32 flags)
{
    Widget w;
    w.pos = pos;
    w.onRender = [=](Widget& widget, bool isSelected) {
        Vec2 center = Vec2(g_game.windowWidth, g_game.windowHeight) * 0.5f;
        ui::text(font, getText(), center + convertSize(w.pos), color,
                    widget.fadeInAlpha, widget.fadeInScale, halign, valign);
    };
    w.flags = flags;
    widgets.push(w);
    return &widgets.back();
}

Widget* Menu::addTitle(const char* text, Vec2 pos)
{
    Font* font = &g_res.getFont("font_bold", (u32)convertSize(88));
    Widget w;
    w.pos = pos;
    w.onRender = [text, font](Widget& widget, bool isSelected){
        Vec2 center = Vec2(g_game.windowWidth, g_game.windowHeight) * 0.5f;
        Vec2 pos = convertSize(widget.pos) + center;
        Vec2 boxSize = convertSize({5000, 90});
        ui::rectBlur(-500, &g_res.white,
                pos-boxSize*0.5f, boxSize, Vec4(0,0,0,0.5f), widget.fadeInAlpha);
        ui::text(font, text, pos, Vec3(1.f),
                widget.fadeInAlpha, widget.fadeInScale, HAlign::CENTER, VAlign::CENTER);
    };
    w.flags = 0;
    widgets.push(w);
    return &widgets.back();
}
void Menu::showGraphicsSettingsMenu()
{
    reset();
    addTitle("Graphics Settings");
    addBackgroundBox({0,0}, {880, 780});

    // TODO: detect aspect ratio and show resolutions accordingly
    static Vec2i resolutions[] = {
        { 960, 540 },
        { 1024, 576 },
        { 1280, 720 },
        { 1366, 768 },
        { 1600, 900 },
        { 1920, 1080 },
        { 2560, 1440 },
        { 3840, 2160 },
    };
    SmallArray<Str32> resolutionNames;
    i32 valueIndex = 2;
    for (i32 i=0; i<(i32)ARRAY_SIZE(resolutions); ++i)
    {
        if (resolutions[i].x == (i32)tmpConfig.graphics.resolutionX &&
            resolutions[i].y == (i32)tmpConfig.graphics.resolutionY)
        {
            valueIndex = i;
        }
        resolutionNames.push(tmpStr("%i x %i", resolutions[i].x, resolutions[i].y));
    }

    Vec2 size(750, 50);
    f32 y = -300;
    selectedWidget = addSelector("Resolution", "The internal render resolution.", { 0, y },
        size, resolutionNames, valueIndex, [this](i32 valueIndex){
            tmpConfig.graphics.resolutionX = resolutions[valueIndex].x;
            tmpConfig.graphics.resolutionY = resolutions[valueIndex].y;
        });
    y += size.y + 10;

    addSelector("Fullscreen", "Run the game in fullscreen or window.", { 0, y }, size, { "OFF", "ON" }, tmpConfig.graphics.fullscreen,
            [this](i32 valueIndex){ tmpConfig.graphics.fullscreen = (bool)valueIndex; });
    y += size.y + 10;

    addSelector("V-Sync", "Lock the frame rate to the monitor's refresh rate.",
            { 0, y }, size, { "OFF", "ON" }, tmpConfig.graphics.vsync,
            [this](i32 valueIndex){ tmpConfig.graphics.vsync = (bool)valueIndex; });
    y += size.y + 10;

    /*
    f32 maxFPS = tmpConfig.graphics.maxFPS;
    g_gui.slider("Max FPS", 30, 300, maxFPS);
    tmpConfig.graphics.maxFPS = (u32)maxFPS;
    */

    static u32 shadowMapResolutions[] = { 0, 1024, 2048, 4096 };
    SmallArray<Str32> shadowQualityNames = { "Off", "Low", "Medium", "High" };
    i32 shadowQualityIndex = 0;
    for (i32 i=0; i<(i32)ARRAY_SIZE(shadowMapResolutions); ++i)
    {
        if (shadowMapResolutions[i] == tmpConfig.graphics.shadowMapResolution)
        {
            shadowQualityIndex = i;
            break;
        }
    }
    addSelector("Shadows", "The quality of shadows cast by objects in the world.", { 0, y },
        size, shadowQualityNames, shadowQualityIndex, [this](i32 valueIndex){
            tmpConfig.graphics.shadowsEnabled = valueIndex > 0;
            tmpConfig.graphics.shadowMapResolution = shadowMapResolutions[valueIndex];
        });
    y += size.y + 10;

    SmallArray<Str32> ssaoQualityNames = { "Off", "Normal", "High" };

    addSelector("SSAO", "Darkens occluded areas of the world for improved realism.", { 0, y }, size,
        ssaoQualityNames, (i32)tmpConfig.graphics.ssaoQuality, [this](i32 valueIndex){
            tmpConfig.graphics.ssaoQuality = (ConfigLevel)valueIndex;
        });
    y += size.y + 10;

    addSelector("Bloom", "Adds a glowy light-bleeding effect to bright areas of the world.", { 0, y }, size, { "OFF", "ON" }, tmpConfig.graphics.bloomEnabled,
            [this](i32 valueIndex){ tmpConfig.graphics.bloomEnabled = (bool)valueIndex; });
    y += size.y + 10;

    i32 aaIndex = 0;
    static u32 aaLevels[] = { 0, 2, 4, 8 };
    SmallArray<Str32> aaLevelNames = { "Off", "2x MSAA", "4x MSAA", "8x MSAA" };
    for (i32 i=0; i<(i32)ARRAY_SIZE(aaLevels); ++i)
    {
        if (aaLevels[i] == tmpConfig.graphics.msaaLevel)
        {
            aaIndex = i;
            break;
        }
    }
    addSelector("Anti-Aliasing", "Improves image quality by smoothing jagged edges.", { 0, y }, size,
        aaLevelNames, aaIndex, [this](i32 valueIndex){
            tmpConfig.graphics.msaaLevel = aaLevels[valueIndex];
        });
    y += size.y + 10;

    addSelector("Sharpen", "Sharpen the final image in a post-processing pass.",
            { 0, y }, size, { "OFF", "ON" }, tmpConfig.graphics.sharpenEnabled,
            [this](i32 valueIndex){ tmpConfig.graphics.sharpenEnabled = (bool)valueIndex; });
    y += size.y + 10;

    addSelector("Potato Mode", "Disables basic graphics features to reduce system requirements.",
            { 0, y }, size, { "OFF", "ON" }, !tmpConfig.graphics.fogEnabled, [this](i32 valueIndex){
        tmpConfig.graphics.pointLightsEnabled = !(bool)valueIndex;
        tmpConfig.graphics.motionBlurEnabled = !(bool)valueIndex;
        tmpConfig.graphics.fogEnabled = !(bool)valueIndex;
    });
    y += size.y + 10;

    y += 20;
    addHelpMessage({0, y});

    Vec2 buttonSize(280, 60);
    addButton("APPLY", nullptr, { -290, 350 }, buttonSize, [this]{
        g_game.config = tmpConfig;
        g_game.renderer->updateSettingsVersion();
        g_res.iterateResourceType(ResourceType::TEXTURE, [](Resource* r) {
            ((Texture*)r)->onUpdateGlobalTextureSettings();
        });
        SDL_SetWindowFullscreen(g_game.window, g_game.config.graphics.fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
        SDL_GL_SetSwapInterval(g_game.config.graphics.vsync ? 1 : 0);
        g_game.config.save();
        showSettingsMenu();
    }, WidgetFlags::FADE_OUT);

    addButton("DEFAULTS", nullptr, { 0, 350 }, buttonSize, [this]{
        Config defaultConfig;
        tmpConfig.graphics = defaultConfig.graphics;
        Widget* previousSelectedWidget = selectedWidget;
        showGraphicsSettingsMenu();
        selectedWidget = previousSelectedWidget;
        for (auto& w : widgets)
        {
            w.fadeInAlpha = 1.f;
            w.fadeInScale = 1.f;
            fadeInTimer = 1920.f;
        }
    }, 0);

    addButton("CANCEL", nullptr, { 290, 350 }, buttonSize, [this]{
        showSettingsMenu();
    }, WidgetFlags::FADE_OUT | WidgetFlags::BACK);
}

void Menu::showRaceResults()
{
#if 0
    // generate fake race results
    Scene* scene = g_game.currentScene.get();
    scene->getRaceResults().clear();
    RandomSeries series = randomSeed();
    for (u32 i=0; i<10; ++i)
    {
        g_game.state.drivers.push(Driver(i==0, i==0, i==0, 0, i%2, i));
    }
    for (u32 i=0; i<10; ++i)
    {
        RaceResult result;
        result.driver = &g_game.state.drivers[i];
        result.finishedRace = true;
        result.placement = i;
        result.statistics.accidents = irandom(series, 0, 10);
        result.statistics.destroyed = irandom(series, 0, 10);
        result.statistics.frags = irandom(series, 0, 50);
        result.statistics.bonuses.push(RaceBonus{ "SMALL BONUS", 100 });
        result.statistics.bonuses.push(RaceBonus{ "MEDIUM BONUS", 400 });
        result.statistics.bonuses.push(RaceBonus{ "BIG BONUS", 500 });
        scene->getRaceResults().push(result);
    }
#endif
    reset();
    updateVehiclePreviews();

    static f32 columnOffset[] = { 32, 90, 225, 335, 420, 490, 590 };
    static const char* columnTitle[] = {
        "NO.",
        "DRIVER",
        "ACCIDENTS",
        "DESTROYED",
        "FRAGS",
        "BONUS",
        "CREDITS EARNED"
    };

    static const Vec2 boxSize{1040, 840};

    Widget w;
    w.fadeInScale = 0.5f;
    w.flags = 0;
    w.pos = Vec2(0,0);
    w.onRender = [this](Widget& w, bool isSelected){
        Vec2 center = Vec2(g_game.windowWidth, g_game.windowHeight) * 0.5f;
        ui::rectBlur(-1000, nullptr, center-convertSize(boxSize)*0.5f * w.fadeInScale,
                convertSize(boxSize) * w.fadeInScale, Vec4(Vec3(0.f), 0.8f), w.fadeInAlpha);

        Font* titlefont = &g_res.getFont("font_bold", (u32)convertSize(26));
        Font* font = &g_res.getFont("font", (u32)convertSize(26));
        Font* bigfont = &g_res.getFont("font_bold", (u32)convertSize(55));

        ui::text(bigfont, "RACE RESULTS", center + Vec2(0, -boxSize.y/2 + 40) * w.fadeInScale, Vec3(1.f),
                w.fadeInAlpha, w.fadeInScale, HAlign::CENTER, VAlign::CENTER);

        u32 maxColumn = g_game.state.gameMode == GameMode::CHAMPIONSHIP
            ? ARRAY_SIZE(columnTitle) : ARRAY_SIZE(columnTitle) - 1;

        for (u32 i=0; i<maxColumn; ++i)
        {
            Vec2 pos = (-boxSize / 2 + 20) + Vec2(columnOffset[i] * 1.5f, 70);
            ui::text(titlefont, columnTitle[i], center + convertSize(pos) * w.fadeInScale,
                    Vec3(1.f), w.fadeInAlpha, w.fadeInScale, HAlign::CENTER, VAlign::CENTER);
        }

        for (u32 i=0; i<10; ++i)
        {
            auto& row = g_game.currentScene->getRaceResults()[i];
            Font* f = row.driver->isPlayer ? titlefont : font;
            Vec3 color = row.driver->isPlayer ? COLOR_SELECTED.rgb : Vec3(1.f);

            f32 rowHeight = 50;
            f32 rowSep = 65;
            Vec2 pos = (-boxSize / 2 + 20) + Vec2(0, i * rowSep + 115);
            Vec2 boxP = center + convertSize(Vec2(0, pos.y) - Vec2(boxSize.x, rowHeight) * 0.5f) * w.fadeInScale;
            Vec2 boxS = Vec2(boxSize.x, rowHeight) * w.fadeInScale;
            ui::rectBlur(ui::BG, nullptr, boxP, boxS, Vec4(Vec3(0.1f), 1.f), w.fadeInAlpha * 0.2f);

            Vec2 p = center + convertSize(pos + Vec2(columnOffset[0] * 1.5f, 0.f)) * w.fadeInScale;
            ui::text(f, tmpStr("%i", row.placement + 1), p, Vec3(1.f), w.fadeInAlpha, w.fadeInScale,
                    HAlign::CENTER, VAlign::CENTER);

            f32 iconSize = 60;
            i32 driverIndex = g_game.state.drivers.findIndexIf([&row](Driver& d) { return &d == row.driver; });
            p = center + convertSize(pos + Vec2(columnOffset[0] * 1.5f + 20, -iconSize/2)) * w.fadeInScale;
            ui::rectUVBlur(ui::IMAGE, vehiclePreviews[driverIndex].getTexture(), p,
                    convertSize(Vec2(iconSize)) * w.fadeInScale, Vec2(0,1), Vec2(1,0), Vec4(1.f),
                    w.fadeInAlpha);

            p = center + convertSize(pos + Vec2(columnOffset[1] * 1.5f, 0.f)) * w.fadeInScale;
            ui::text(f, tmpStr("%s", row.driver->playerName.data()), p, color, w.fadeInAlpha, w.fadeInScale,
                    HAlign::LEFT, VAlign::CENTER);

            p = center + convertSize(pos + Vec2(columnOffset[2] * 1.5f, 0.f)) * w.fadeInScale;
            ui::text(f, tmpStr("%i", row.statistics.accidents), p, Vec3(1.f), w.fadeInAlpha, w.fadeInScale,
                    HAlign::CENTER, VAlign::CENTER);

            p = center + convertSize(pos + Vec2(columnOffset[3] * 1.5f, 0.f)) * w.fadeInScale;
            ui::text(f, tmpStr("%i", row.statistics.destroyed), p, Vec3(1.f), w.fadeInAlpha, w.fadeInScale,
                    HAlign::CENTER, VAlign::CENTER);

            p = center + convertSize(pos + Vec2(columnOffset[4] * 1.5f, 0.f)) * w.fadeInScale;
            ui::text(f, tmpStr("%i", row.statistics.frags), p, Vec3(1.f), w.fadeInAlpha, w.fadeInScale,
                    HAlign::CENTER, VAlign::CENTER);

            p = center + convertSize(pos + Vec2(columnOffset[5] * 1.5f, 0.f)) * w.fadeInScale;
            ui::text(f, tmpStr("%i", row.getBonus()), p, Vec3(1.f), w.fadeInAlpha, w.fadeInScale,
                    HAlign::CENTER, VAlign::CENTER);

            if (g_game.state.gameMode == GameMode::CHAMPIONSHIP)
            {
                p = center + convertSize(pos + Vec2(columnOffset[6] * 1.5f, 0.f)) * w.fadeInScale;
                ui::text(f, tmpStr("%i", row.getCreditsEarned()), p, Vec3(1.f), w.fadeInAlpha, w.fadeInScale,
                    HAlign::CENTER, VAlign::CENTER);
            }
        }
    };
    widgets.push(move(w));

    selectedWidget = addButton("OKAY", nullptr, Vec2(0, boxSize.y/2-50), Vec2(200, 50), [this]{
        for (auto& r : g_game.currentScene->getRaceResults())
        {
            r.driver->lastPlacement = r.placement;
        }

        g_audio.playSound(g_res.getSound("close"), SoundType::MENU_SFX);
        reset();
        if (g_game.state.gameMode == GameMode::CHAMPIONSHIP)
        {
            showChampionshipStandings();
            for (auto& row : g_game.currentScene->getRaceResults())
            {
                row.driver->credits += row.getCreditsEarned();
                row.driver->leaguePoints += row.getLeaguePointsEarned();
            }
            ++g_game.state.currentRace;
        }
        else
        {
            mode = MAIN_MENU;
            g_game.currentScene->isCameraTourEnabled = true;
        }
    }, WidgetFlags::FADE_OUT | WidgetFlags::BACK);
}

void Menu::showChampionshipStandings()
{
    static const Vec2 boxSize{800, 830};

    reset();

    Widget w;
    w.fadeInScale = 0.5f;
    w.flags = 0;
    w.pos = Vec2(0,0);
    w.onRender = [this](Widget& w, bool isSelected){
        Vec2 center = Vec2(g_game.windowWidth, g_game.windowHeight) * 0.5f;
        ui::rectBlur(-1000, nullptr, center-convertSize(boxSize)*0.5f * w.fadeInScale,
                convertSize(boxSize) * w.fadeInScale, Vec4(Vec3(0.f), 0.8f), w.fadeInAlpha);

        Font* boldfont = &g_res.getFont("font_bold", (u32)convertSize(26));
        Font* pointsfont = &g_res.getFont("font_bold", (u32)convertSize(36));
        Font* font = &g_res.getFont("font", (u32)convertSize(26));
        Font* bigfont = &g_res.getFont("font_bold", (u32)convertSize(55));

        ui::text(bigfont, "STANDINGS", center + Vec2(0, -boxSize.y/2 + 40) * w.fadeInScale, Vec3(1.f),
                w.fadeInAlpha, w.fadeInScale, HAlign::CENTER, VAlign::CENTER);

        SmallArray<Driver*, 10> sortedDrivers;
        for(auto& driver : g_game.state.drivers)
        {
            sortedDrivers.push(&driver);
        }
        sortedDrivers.sort([](Driver* a, Driver* b) { return a->leaguePoints > b->leaguePoints; });

        for (u32 i=0; i<10; ++i)
        {
            Driver* driver = sortedDrivers[i];
            Font* f = driver->isPlayer ? boldfont : font;
            Vec3 color = driver->isPlayer ? COLOR_SELECTED.rgb : Vec3(1.f);

            f32 rowHeight = 50;
            f32 rowSep = 65;
            Vec2 pos = (-boxSize / 2 + 20) + Vec2(0, i * rowSep + 90);
            Vec2 boxP = center + convertSize(Vec2(0, pos.y) - Vec2(boxSize.x, rowHeight) * 0.5f) * w.fadeInScale;
            Vec2 boxS = Vec2(boxSize.x, rowHeight) * w.fadeInScale;
            ui::rectBlur(ui::BG, nullptr, boxP, boxS, Vec4(Vec3(0.1f), 1.f), w.fadeInAlpha * 0.2f);

            Vec2 p = center + convertSize(pos + Vec2(40, 0)) * w.fadeInScale;
            ui::text(f, tmpStr("%i", i+1), p, Vec3(1.f), w.fadeInAlpha, w.fadeInScale,
                    HAlign::CENTER, VAlign::CENTER);

            f32 iconSize = 60;
            p = center + convertSize(pos + Vec2(40 + 20, -iconSize/2)) * w.fadeInScale;
            ui::rectUVBlur(ui::IMAGE, vehiclePreviews[driver - g_game.state.drivers.begin()].getTexture(), p,
                    convertSize(Vec2(iconSize)) * w.fadeInScale, Vec2(0,1), Vec2(1,0), Vec4(1.f),
                    w.fadeInAlpha);

            p = center + convertSize(pos + Vec2(150, 0)) * w.fadeInScale;
            ui::text(f, tmpStr("%s", driver->playerName.data()), p, color, w.fadeInAlpha, w.fadeInScale,
                    HAlign::LEFT, VAlign::CENTER);

            p = center + convertSize(pos + Vec2(600, 0)) * w.fadeInScale;
            ui::text(pointsfont, tmpStr("%i", driver->leaguePoints), p, color, w.fadeInAlpha, w.fadeInScale,
                    HAlign::CENTER, VAlign::CENTER);
        }
    };
    widgets.push(move(w));

    u32 flags = WidgetFlags::FADE_OUT | WidgetFlags::BACK;
    if (g_game.currentScene->guid != g_res.getTrackGuid(championshipTracks[g_game.state.currentRace]))
    {
        flags |= WidgetFlags::FADE_TO_BLACK;
    }
    selectedWidget = addButton("OKAY", nullptr, Vec2(0, boxSize.y/2-50), Vec2(200, 50), [this]{
        g_audio.playSound(g_res.getSound("close"), SoundType::MENU_SFX);
        showChampionshipMenu();
        RandomSeries series = randomSeed();
        if (g_game.currentScene->guid != g_res.getTrackGuid(championshipTracks[g_game.state.currentRace]))
        {
            for (auto& driver : g_game.state.drivers)
            {
                if (!driver.isPlayer)
                {
                    driver.aiUpgrades(series);
                }
            }
            g_game.saveGame();
            g_game.changeScene(championshipTracks[g_game.state.currentRace]);
        }
    }, flags);
}

void Menu::createWeaponsMenu(WeaponSlot const& slot, i32& weaponIndex, u32& upgradeLevel)
{
	resetTransient();

    Vec2 buttonSize(450, 75);
    selectedWidget = addButton("BACK", nullptr, {280, 350-buttonSize.y*0.5f}, buttonSize, [this]{
        garage.driver->vehicleConfig = garage.previewVehicleConfig;
        createMainGarageMenu();
    }, WidgetFlags::FADE_OUT_TRANSIENT | WidgetFlags::BACK | WidgetFlags::TRANSIENT);

    selectedWidget = addButton("SELL", "Sell the currently equipped item for half the original value.",
        {280, 350-buttonSize.y*0.5f-buttonSize.y-12}, buttonSize, [this, &weaponIndex, &upgradeLevel]{
        garage.driver->credits += g_weapons[weaponIndex].info.price / 2;
        upgradeLevel -= 1;
        // TODO: use different sound for selling
        g_audio.playSound(g_res.getSound("airdrill"), SoundType::MENU_SFX);
        if (upgradeLevel == 0)
        {
            weaponIndex = -1;
        }
    }, WidgetFlags::TRANSIENT, nullptr, [&upgradeLevel]{
        return upgradeLevel > 0;
    });

    Vec2 size(150, 150);
    f32 x = 280-buttonSize.x*0.5f + size.x*0.5f;
    f32 y = -400 + size.y * 0.5f;
    f32 gap = 6;
    u32 buttonsPerRow = 3;

    i32 buttonCount = 0;
    for (i32 i=0; i<(i32)g_weapons.size(); ++i)
    {
        auto& weapon = g_weapons[i];
        if (weapon.info.weaponType != slot.weaponType || !slot.matchesWeapon(weapon.info))
        {
            continue;
        }
        Vec2 pos = { x + (buttonCount % buttonsPerRow) * (size.x + gap),
                          y + (buttonCount / buttonsPerRow) * (size.y + gap) };
        ++buttonCount;
        Widget* w = addImageButton(weapon.info.name, weapon.info.description, pos, size,
            [i, this, &weaponIndex, &upgradeLevel]{
            u32 weaponUpgradeLevel = (weaponIndex == i) ? upgradeLevel: 0;
            bool isPurchasable = garage.driver->credits >= g_weapons[i].info.price
                && weaponUpgradeLevel < g_weapons[i].info.maxUpgradeLevel
                && (weaponIndex == -1 || weaponIndex == i);
            if (isPurchasable)
            {
                // TODO: Play sound that is more appropriate for a weapon
                g_audio.playSound(g_res.getSound("airdrill"), SoundType::MENU_SFX);
                garage.driver->credits -= g_weapons[i].info.price;
                weaponIndex = i;
                upgradeLevel += 1;
            }
            else
            {
                g_audio.playSound(g_res.getSound("nono"), SoundType::MENU_SFX);
            }
        }, WidgetFlags::TRANSIENT, weapon.info.icon, 48,
            [i, this, &weaponIndex, &upgradeLevel](bool isSelected) -> ImageButtonInfo {
            u32 weaponUpgradeLevel = (weaponIndex == i) ? upgradeLevel: 0;
            bool isPurchasable = garage.driver->credits >= g_weapons[i].info.price
                && weaponUpgradeLevel < g_weapons[i].info.maxUpgradeLevel
                && (weaponIndex == -1 || weaponIndex == i);
            return ImageButtonInfo{
                isPurchasable, weaponIndex == i, (i32)g_weapons[i].info.maxUpgradeLevel,
                (i32)weaponUpgradeLevel, tmpStr("%i", g_weapons[i].info.price) };
        });

        if (i == 0)
        {
            selectedWidget = w;
        }
    }
}

void Menu::updateVehiclePreviews()
{
    Mesh* quadMesh = g_res.getModel("misc")->getMeshByName("Quad");
    for (u32 driverIndex=0; driverIndex<10; ++driverIndex)
    {
        RenderWorld& rw = vehiclePreviews[driverIndex];
        rw.setName(tmpStr("Vehicle Icon %i", driverIndex));
        u32 vehicleIconSize = (u32)convertSize(150);
        rw.setSize(vehicleIconSize, vehicleIconSize);
        drawSimple(&rw, quadMesh, &g_res.white, Mat4::scaling(Vec3(20.f)), Vec3(0.02f));
        Driver& driver = g_game.state.drivers[driverIndex];
        if (driver.getVehicleData())
        {
            VehicleTuning t = driver.getTuning();
            driver.getVehicleData()->render(&rw, Mat4::translation(Vec3(0, 0, t.getRestOffset())),
                nullptr, *driver.getVehicleConfig(), &t);
        }
        rw.setViewportCount(1);
        rw.addDirectionalLight(Vec3(-0.5f, 0.2f, -1.f), Vec3(1.0));
        rw.setViewportCamera(0, Vec3(8.f, -8.f, 10.f), Vec3(0.f, 0.f, 1.f), 1.f, 50.f, 30.f);
        g_game.renderer->addRenderWorld(&rw);
    }
}

void Menu::showChampionshipMenu()
{
    updateVehiclePreviews();
    reset();

    i32 playerIndex = 0;
    for (auto& driver : g_game.state.drivers)
    {
        if (driver.isPlayer)
        {
            Widget w;
            w.helpText = "Buy, sell, or upgrade your vehicle";
            w.pos = { 55.f, -135.f + playerIndex * 170 };
            w.size = { 450, 150 };
            w.fadeInScale = 0.7f;
            w.onRender = [this, &driver, playerIndex](Widget& w, bool isSelected){
                Vec2 center = Vec2(g_game.windowWidth, g_game.windowHeight) * 0.5f;
                Vec2 size = convertSize(w.size) * w.fadeInScale;
                Vec2 pos = convertSize(w.pos) + center - size * 0.5f;
                f32 borderSize = convertSize(isSelected ? 5 : 2);
                Vec4 borderColor = mix(COLOR_NOT_SELECTED, COLOR_SELECTED, w.hover);
                ui::rectBlur(ui::BORDER, &g_res.white, pos-borderSize, size+borderSize*2,
                            borderColor, (0.5f + w.hover * 0.5f) * w.fadeInAlpha);
                ui::rectBlur(ui::BG, &g_res.white, pos, size,
                        Vec4(Vec3((sinf(w.hoverTimer * 4.f) + 1.f)*0.5f*w.hover*0.04f), 0.8f),
                        w.fadeInAlpha);

                f32 iconSize = (u32)convertSize(w.size.y) * w.fadeInScale;
                ui::rectUVBlur(ui::IMAGE, vehiclePreviews[playerIndex].getTexture(), pos,
                        Vec2(iconSize), {1,1}, {0,0}, Vec4(1.f), w.fadeInAlpha);

                f32 textAlpha = (isSelected ? 1.f : 0.5f) * w.fadeInAlpha;
                f32 margin = convertSize(15.f);
                Font* font = &g_res.getFont("font", (u32)convertSize(34));
                ui::text(font, tmpStr("%s's Garage", driver.playerName.data()),
                            pos + Vec2(iconSize + margin, margin),
                            Vec3(1.f), textAlpha, w.fadeInScale);

                Font* fontSmall = &g_res.getFont("font", (u32)convertSize(28));
                ui::text(fontSmall, tmpStr("Credits: %i", driver.credits),
                            pos + Vec2(iconSize + margin, margin + convertSize(35.f)),
                            Vec3(1.f), textAlpha, w.fadeInScale);

            };
            w.onSelect = [playerIndex, this]{
                garage.driver = &g_game.state.drivers[playerIndex];
                garage.previewVehicle = garage.driver->getVehicleData();
                garage.previewVehicleConfig = *garage.driver->getVehicleConfig();
                garage.previewTuning = garage.driver->getTuning();
                garage.currentStats = garage.previewTuning.computeVehicleStats();
                garage.upgradeStats = garage.currentStats;
                showGarageMenu();
            };
            w.fadeInScale = 0.7f;
            w.flags = WidgetFlags::SELECTABLE |
                        WidgetFlags::NAVIGATE_VERTICAL |
                        WidgetFlags::NAVIGATE_HORIZONTAL |
                        WidgetFlags::FADE_OUT;
            widgets.push(w);

            ++playerIndex;
        }
    }

    f32 x = 500.f;
    f32 y = -170.f;
    Vec2 size(400, 80);
    selectedWidget = addButton("BEGIN RACE", "Start the next race.", {x,y}, size, [this]{
        reset();
        Scene* scene = g_game.changeScene(championshipTracks[g_game.state.currentRace]);
        scene->startRace();
    }, WidgetFlags::FADE_OUT | WidgetFlags::FADE_TO_BLACK, g_res.getTexture("icon_flag"));
    y += 100;

    addButton("STANDINGS", "View the current championship standings.", {x,y}, size, [this]{
        showChampionshipStandings();
    }, WidgetFlags::FADE_OUT, g_res.getTexture("icon_stats"));
    y += 100;

    addButton("QUIT", "Return to main menu.", {x,y}, size, [this]{
        showMainMenu();
    }, WidgetFlags::FADE_OUT);
    y += 100;

    addBackgroundBox({0,-425+90}, {1920, 180}, 0.5f, false);
    Font* bigFont = &g_res.getFont("font_bold", (u32)convertSize(110));
    Widget* label = addLabel([]{ return tmpStr("League %c", (char)('A' + g_game.state.currentLeague)); },
            {0, -370}, bigFont, HAlign::CENTER, VAlign::CENTER);
    Font* mediumFont = &g_res.getFont("font", (u32)convertSize(60));
    addLabel([]{ return tmpStr("Race %u/10", g_game.state.currentRace + 1); }, {0, -290}, mediumFont,
            HAlign::CENTER, VAlign::CENTER);

    addLogic([label]{
        Vec2 center = Vec2(g_game.windowWidth, g_game.windowHeight) * 0.5f;
        u32 trackPreviewSize = (u32)convertSize(380);
        g_game.currentScene->updateTrackPreview(g_game.renderer.get(), trackPreviewSize);
        f32 size = trackPreviewSize * label->fadeInScale;
        Vec2 trackPreviewPos = center + convertSize({ -450, -350 }) - size * 0.5f;
        Texture* trackTex = g_game.currentScene->getTrackPreview2D().getTexture();
        ui::rectUV(ui::IMAGE, trackTex, trackPreviewPos,
                Vec2(size), {0,1}, {1,0}, Vec3(1.f), label->fadeInAlpha);
    });
}

void Menu::createMainGarageMenu()
{
	resetTransient();

    Texture* iconbg = g_res.getTexture("iconbg");
    Vec2 size(450, 75);
    f32 x = 280;
    f32 y = -400 + size.y * 0.5f;
    f32 gap = 12;
    selectedWidget = addButton("PERFORMANCE", "Upgrades to enhance your vehicle's performance.", {x,y}, size, [this]{
        createPerformanceMenu();
    }, WidgetFlags::TRANSIENT | WidgetFlags::FADE_OUT_TRANSIENT, g_res.getTexture("icon_engine"));
    y += size.y + gap;

    addButton("COSMETICS", "Change your vehicle's appearance with decals or paint.", {x,y}, size, [this]{
        createCosmeticsMenu();
    }, WidgetFlags::TRANSIENT | WidgetFlags::FADE_OUT_TRANSIENT, g_res.getTexture("icon_spraycan"));
    y += size.y + gap;

    for (u32 i=0; i<garage.driver->getVehicleData()->weaponSlots.size(); ++i)
    {
        i32 installedWeapon = garage.previewVehicleConfig.weaponIndices[i];
        WeaponSlot& slot = garage.driver->getVehicleData()->weaponSlots[i];
        addButton(slot.name.data(), "Install or upgrade a weapon.", {x,y}, size, [this, slot, i]{
            createWeaponsMenu(slot,
                    garage.previewVehicleConfig.weaponIndices[i],
                    garage.previewVehicleConfig.weaponUpgradeLevel[i]);
        }, WidgetFlags::TRANSIENT | WidgetFlags::FADE_OUT_TRANSIENT,
            installedWeapon == -1 ? iconbg : g_weapons[installedWeapon].info.icon);
        y += size.y + gap;
    }

    addButton("CAR LOT", "Buy a new vehicle!", {x,y}, size, [this]{
        //createCarLotMenu();
    }, WidgetFlags::TRANSIENT | WidgetFlags::FADE_OUT_TRANSIENT);
    y += size.y + gap;

    addButton("DONE", nullptr, {x, 350-size.y*0.5f}, size, [this]{
        if (garage.initialCarSelect)
        {
            garage.playerIndex += 1;
            i32 playerCount = 0;
            for (auto& driver : g_game.state.drivers)
            {
                if (driver.isPlayer)
                {
                    ++playerCount;
                }
            }
            if (garage.playerIndex >= playerCount)
            {
                garage.initialCarSelect = false;
                showChampionshipMenu();
            }
            else
            {
                showInitialCarLotMenu(garage.playerIndex);
            }
        }
        else
        {
            showChampionshipMenu();
        }
    }, WidgetFlags::FADE_OUT | WidgetFlags::BACK | WidgetFlags::TRANSIENT);
}

void Menu::createPerformanceMenu()
{
	resetTransient();

    Vec2 buttonSize(450, 75);
    addButton("BACK", nullptr, {280, 350-buttonSize.y*0.5f}, buttonSize, [this]{
        garage.previewVehicleConfig = *garage.driver->getVehicleConfig();
        garage.upgradeStats = garage.currentStats;
        createMainGarageMenu();
    }, WidgetFlags::FADE_OUT_TRANSIENT | WidgetFlags::BACK | WidgetFlags::TRANSIENT);

    Vec2 size(150, 150);
    f32 x = 280-buttonSize.x*0.5f + size.x*0.5f;
    f32 y = -400 + size.y * 0.5f;
    f32 gap = 6;
    u32 buttonsPerRow = 3;

    static i32 previewUpgradeIndex;
    previewUpgradeIndex = -1;
    for (i32 i=0; i<(i32)garage.driver->getVehicleData()->availableUpgrades.size(); ++i)
    {
        auto& upgrade = garage.driver->getVehicleData()->availableUpgrades[i];
        Vec2 pos = { x + (i % buttonsPerRow) * (size.x + gap),
                          y + (i / buttonsPerRow) * (size.y + gap) };
        Widget* w = addImageButton(upgrade.name, upgrade.description, pos, size, [i, this]{
            auto& upgrade = garage.driver->getVehicleData()->availableUpgrades[i];
            auto currentUpgrade = garage.driver->getVehicleConfig()->performanceUpgrades.findIf(
                    [i](auto& u) { return u.upgradeIndex == i; });
            bool isEquipped = !!currentUpgrade;
            i32 upgradeLevel = 0;
            i32 price = upgrade.price;
            if (isEquipped)
            {
                upgradeLevel = currentUpgrade->upgradeLevel;
                price = upgrade.price * (upgradeLevel + 1);
            }
            bool isPurchasable = garage.driver->credits >= price && upgradeLevel < upgrade.maxUpgradeLevel;
            if (isPurchasable)
            {
                g_audio.playSound(g_res.getSound("airdrill"), SoundType::MENU_SFX);
                garage.driver->credits -= price;
                garage.driver->getVehicleConfig()->addUpgrade(i);
                garage.previewTuning = garage.driver->getTuning();
                garage.currentStats = garage.previewTuning.computeVehicleStats();
                garage.upgradeStats = garage.currentStats;

                auto currentUpgrade = garage.driver->getVehicleConfig()->performanceUpgrades.findIf(
                        [i](auto& u) { return u.upgradeIndex == i; });
                if (currentUpgrade->upgradeLevel < upgrade.maxUpgradeLevel)
                {
                    garage.previewVehicleConfig = *garage.driver->getVehicleConfig();
                    garage.previewVehicleConfig.addUpgrade(i);
                    g_res.getVehicle(garage.previewVehicleGuid)->initTuning(garage.previewVehicleConfig, garage.previewTuning);
                    garage.upgradeStats = garage.previewTuning.computeVehicleStats();
                }
            }
            else
            {
                g_audio.playSound(g_res.getSound("nono"), SoundType::MENU_SFX);
            }
        }, WidgetFlags::TRANSIENT, upgrade.icon, 48, [i, this](bool isSelected) -> ImageButtonInfo {
            auto& upgrade = garage.driver->getVehicleData()->availableUpgrades[i];
            auto currentUpgrade = garage.driver->getVehicleConfig()->performanceUpgrades.findIf(
                    [i](auto& u) { return u.upgradeIndex == i; });
            bool isEquipped = !!currentUpgrade;
            i32 upgradeLevel = 0;
            i32 price = upgrade.price;
            if (isEquipped)
            {
                upgradeLevel = currentUpgrade->upgradeLevel;
                price = upgrade.price * (upgradeLevel + 1);
            }

            if (isSelected && upgradeLevel < upgrade.maxUpgradeLevel && previewUpgradeIndex != i)
            {
                previewUpgradeIndex = i;
                garage.previewVehicleConfig = *garage.driver->getVehicleConfig();
                garage.previewVehicleConfig.addUpgrade(i);
                g_res.getVehicle(garage.previewVehicleGuid)->initTuning(garage.previewVehicleConfig, garage.previewTuning);
                garage.upgradeStats = garage.previewTuning.computeVehicleStats();
            }

            bool isPurchasable = garage.driver->credits >= price && upgradeLevel < upgrade.maxUpgradeLevel;
            return ImageButtonInfo{
                isPurchasable, false, upgrade.maxUpgradeLevel, upgradeLevel, tmpStr("%i", price) };
        });

        if (i == 0)
        {
            selectedWidget = w;
        }
    }
}

void Menu::createCosmeticsMenu()
{
	resetTransient();

    Vec2 size(450, 70);
    f32 x = 280;
    f32 y = -400 + size.y * 0.5f;
    f32 gap = 12;

    static f32 hue;
    static f32 saturation;
    static f32 value;
    hue = garage.previewVehicleConfig.cosmetics.hsv.x;
    saturation = garage.previewVehicleConfig.cosmetics.hsv.y;
    value = garage.previewVehicleConfig.cosmetics.hsv.z;

    selectedWidget = addSlider("PAINT HUE", {x,y}, size, WidgetFlags::TRANSIENT, [this](f32 val){
        hue = val;
        garage.previewVehicleConfig.cosmetics.hsv.x = hue;
        garage.previewVehicleConfig.cosmetics.color = srgb(hsvToRgb(hue, saturation, value));
        garage.previewVehicleConfig.reloadMaterials();
        garage.driver->getVehicleConfig()->cosmetics.hsv = garage.previewVehicleConfig.cosmetics.hsv;
        garage.driver->getVehicleConfig()->cosmetics.color = garage.previewVehicleConfig.cosmetics.color;
        garage.driver->getVehicleConfig()->reloadMaterials();
    }, []{
        return SliderInfo{
            Vec4(1.f),
            Vec4(1.f),
            hue,
            g_res.getTexture("hues"),
            0.01f,
            0.99f,
        };
    });
    y += size.y + gap;

    addSlider("PAINT SATURATION", {x,y}, size, WidgetFlags::TRANSIENT, [this](f32 val){
        saturation = val;
        garage.previewVehicleConfig.cosmetics.hsv.y = saturation;
        garage.previewVehicleConfig.cosmetics.color = srgb(hsvToRgb(hue, saturation, value));
        garage.previewVehicleConfig.reloadMaterials();
        garage.driver->getVehicleConfig()->cosmetics.hsv = garage.previewVehicleConfig.cosmetics.hsv;
        garage.driver->getVehicleConfig()->cosmetics.color = garage.previewVehicleConfig.cosmetics.color;
        garage.driver->getVehicleConfig()->reloadMaterials();
    }, []{
        return SliderInfo{
            Vec4(hsvToRgb(hue, 0.f, value), 1.f),
            Vec4(hsvToRgb(hue, 1.f, value), 1.f),
            saturation,
            &g_res.white,
            0.f,
            0.99f,
        };
    });
    y += size.y + gap;

    addSlider("PAINT BRIGHTNESS", {x,y}, size, WidgetFlags::TRANSIENT, [this](f32 val){
        value = val;
        garage.previewVehicleConfig.cosmetics.hsv.z = value;
        garage.previewVehicleConfig.cosmetics.color = srgb(hsvToRgb(hue, saturation, value));
        garage.previewVehicleConfig.reloadMaterials();
        garage.driver->getVehicleConfig()->cosmetics.hsv = garage.previewVehicleConfig.cosmetics.hsv;
        garage.driver->getVehicleConfig()->cosmetics.color = garage.previewVehicleConfig.cosmetics.color;
        garage.driver->getVehicleConfig()->reloadMaterials();
    }, []{
        return SliderInfo{
            Vec4(hsvToRgb(hue, saturation, 1.f), 1.f),
            Vec4(hsvToRgb(hue, saturation, 1.f), 1.f),
            value,
            g_res.getTexture("brightness_gradient"),
            0.02f,
            0.98f,
        };
    });
    y += size.y + gap;

    addSlider("PAINT SHININESS", {x,y}, size, WidgetFlags::TRANSIENT, [this](f32 val){
        garage.previewVehicleConfig.cosmetics.paintShininess = val;
        garage.previewVehicleConfig.reloadMaterials();
        garage.driver->getVehicleConfig()->cosmetics.paintShininess = garage.previewVehicleConfig.cosmetics.paintShininess;
        garage.driver->getVehicleConfig()->reloadMaterials();
    }, [&]{
        return SliderInfo{
            Vec4(1.f),
            Vec4(1.f),
            garage.previewVehicleConfig.cosmetics.paintShininess,
            g_res.getTexture("slider"),
            0.f,
            1.f
        };
    });
    y += size.y + gap;

    y += size.y * 0.5f + gap;
    for (i32 i=0; i<3; ++i)
    {
        Vec2 layerButtonSize = Vec2(size.x, size.y * 0.75f);
        //Vec2 layerButtonSize = size;
        const char* text[3] = { "VINYL LAYER 1", "VINYL LAYER 2", "VINYL LAYER 3" };
        addButton(text[i], "Add graphics to your vehicle.", {x,y}, layerButtonSize, [this,i]{
            i32 layerIndex = i;
            resetTransient();
            createCosmeticLayerMenu(layerIndex);
        }, WidgetFlags::TRANSIENT | WidgetFlags::FADE_OUT_TRANSIENT);
        y+= layerButtonSize.y + gap;
    }

    addLogic([this]{
        if (g_input.isKeyPressed(KEY_F12))
        {
            DataFile::Value data = DataFile::makeDict();
            Serializer s(data, false);
            garage.driver->serialize(s);
            StrBuf buf;
            data.debugOutput(buf, 0, false);
            println("========== DRIVER DEBUG OUTPUT ==========");
            println(buf.data());
        }
    });

    Vec2 buttonSize(450, 75);
    addButton("BACK", nullptr, {280, 350-buttonSize.y*0.5f}, buttonSize, [this]{
        createMainGarageMenu();
    }, WidgetFlags::FADE_OUT_TRANSIENT | WidgetFlags::BACK | WidgetFlags::TRANSIENT);

}

void Menu::createCosmeticLayerMenu(i32 layerIndex)
{
    Vec2 size(450, 70);
    f32 x = 280;
    f32 y = -400 + size.y * 0.5f;
    f32 gap = 12;

    static f32 hue;
    static f32 saturation;
    static f32 value;
    static f32 alpha;
    static i32 vinylIndex;
    hue = garage.previewVehicleConfig.cosmetics.vinylColorsHSV[layerIndex].x;
    saturation = garage.previewVehicleConfig.cosmetics.vinylColorsHSV[layerIndex].y;
    value = garage.previewVehicleConfig.cosmetics.vinylColorsHSV[layerIndex].z;
    alpha = garage.previewVehicleConfig.cosmetics.vinylColors[layerIndex].a;

    static VinylPattern none;
    none.name = "None";
    none.guid = 0;
    none.colorTextureGuid = 0;
    static Array<VinylPattern*> vinyls;
    vinyls.clear();
    vinylIndex = 0;
    g_res.iterateResourceType(ResourceType::VINYL_PATTERN, [](Resource* r) {
        vinyls.push((VinylPattern*)r);
    });
    vinyls.sort([](VinylPattern* a, VinylPattern* b){ return a->name < b->name; });
    vinyls.insert(vinyls.begin(), &none);

    for (u32 i=0; i<vinyls.size(); ++i)
    {
        if (vinyls[i]->guid == garage.previewVehicleConfig.cosmetics.vinylGuids[layerIndex])
        {
            vinylIndex = i;
            break;
        }
    }

    selectedWidget = addSelector2("LAYER IMAGE", "Add a thing to your vehicle.", {x,y}, size,
        WidgetFlags::TRANSIENT, []{
        return SelectorInfo{vinylIndex, vinyls[vinylIndex]->name.data()};
    }, [this, layerIndex](i32 valueIndex){
        vinylIndex = valueIndex;
        if (vinylIndex < 0)
        {
            vinylIndex = vinyls.size() - 1;
        }
        if (vinylIndex >= (i32)vinyls.size())
        {
            vinylIndex = 0;
        }
        garage.previewVehicleConfig.cosmetics.vinylGuids[layerIndex] = vinyls[vinylIndex]->guid;
        garage.driver->getVehicleConfig()->cosmetics.vinylGuids[layerIndex] =
            garage.previewVehicleConfig.cosmetics.vinylGuids[layerIndex];
    });
    y += size.y + gap;

    addSlider("LAYER HUE", {x,y}, size, WidgetFlags::TRANSIENT, [this, layerIndex](f32 val){
        hue = val;
        garage.previewVehicleConfig.cosmetics.vinylColorsHSV[layerIndex].x = hue;
        garage.previewVehicleConfig.cosmetics.vinylColors[layerIndex]
            = Vec4(srgb(hsvToRgb(hue, saturation, value)), alpha);
        garage.previewVehicleConfig.reloadMaterials();
        garage.driver->getVehicleConfig()->cosmetics.vinylColorsHSV[layerIndex]
            = garage.previewVehicleConfig.cosmetics.vinylColorsHSV[layerIndex];
        garage.driver->getVehicleConfig()->cosmetics.vinylColors[layerIndex]
            = garage.previewVehicleConfig.cosmetics.vinylColors[layerIndex];
        garage.driver->getVehicleConfig()->reloadMaterials();
    }, []{
        return SliderInfo{
            Vec4(1.f),
            Vec4(1.f),
            hue,
            g_res.getTexture("hues"),
            0.f,
            1.f,
        };
    });
    y += size.y + gap;

    addSlider("LAYER SATURATION", {x,y}, size, WidgetFlags::TRANSIENT, [this, layerIndex](f32 val){
        saturation = val;
        garage.previewVehicleConfig.cosmetics.vinylColorsHSV[layerIndex].y = saturation;
        garage.previewVehicleConfig.cosmetics.vinylColors[layerIndex]
            = Vec4(srgb(hsvToRgb(hue, saturation, value)), alpha);
        garage.previewVehicleConfig.reloadMaterials();
        garage.driver->getVehicleConfig()->cosmetics.vinylColorsHSV[layerIndex]
            = garage.previewVehicleConfig.cosmetics.vinylColorsHSV[layerIndex];
        garage.driver->getVehicleConfig()->cosmetics.vinylColors[layerIndex]
            = garage.previewVehicleConfig.cosmetics.vinylColors[layerIndex];
        garage.driver->getVehicleConfig()->reloadMaterials();
    }, []{
        return SliderInfo{
            Vec4(hsvToRgb(hue, 0.f, value), 1.f),
            Vec4(hsvToRgb(hue, 1.f, value), 1.f),
            saturation,
            &g_res.white,
            0.f,
            1.f,
        };
    });
    y += size.y + gap;

    addSlider("LAYER BRIGHTNESS", {x,y}, size, WidgetFlags::TRANSIENT, [this, layerIndex](f32 val){
        value = val;
        garage.previewVehicleConfig.cosmetics.vinylColorsHSV[layerIndex].z = value;
        garage.previewVehicleConfig.cosmetics.vinylColors[layerIndex]
            = Vec4(srgb(hsvToRgb(hue, saturation, value)), alpha);
        garage.previewVehicleConfig.reloadMaterials();
        garage.driver->getVehicleConfig()->cosmetics.vinylColorsHSV[layerIndex]
            = garage.previewVehicleConfig.cosmetics.vinylColorsHSV[layerIndex];
        garage.driver->getVehicleConfig()->cosmetics.vinylColors[layerIndex]
            = garage.previewVehicleConfig.cosmetics.vinylColors[layerIndex];
        garage.driver->getVehicleConfig()->reloadMaterials();
    }, []{
        return SliderInfo{
            Vec4(hsvToRgb(hue, saturation, 1.f), 1.f),
            Vec4(hsvToRgb(hue, saturation, 1.f), 1.f),
            value,
            g_res.getTexture("brightness_gradient"),
            0.f,
            1.f,
        };
    });
    y += size.y + gap;

    addSlider("LAYER TRANSPARENCY", {x,y}, size, WidgetFlags::TRANSIENT, [this, layerIndex](f32 val){
        alpha = val;
        garage.previewVehicleConfig.cosmetics.vinylColors[layerIndex].a = val;
        garage.previewVehicleConfig.reloadMaterials();
        garage.driver->getVehicleConfig()->cosmetics.vinylColors[layerIndex].a
            = garage.previewVehicleConfig.cosmetics.vinylColors[layerIndex].a;
        garage.driver->getVehicleConfig()->reloadMaterials();
    }, [&]{
        return SliderInfo{
            Vec4(1.f),
            Vec4(1.f),
            alpha,
            g_res.getTexture("alpha_gradient"),
            0.f,
            1.f
        };
    });
    y += size.y + gap;

    Vec2 buttonSize(450, 75);
    addButton("BACK", nullptr, {280, 350-buttonSize.y*0.5f}, buttonSize, [this]{
        createCosmeticsMenu();
    }, WidgetFlags::FADE_OUT_TRANSIENT | WidgetFlags::BACK | WidgetFlags::TRANSIENT);
}

void Menu::createCarLotMenu()
{
    Vec2 buttonSize(450, 75);
    addButton("DONE", nullptr, {280, 350-buttonSize.y*0.5f}, buttonSize, [this]{
        garage.previewVehicle = garage.driver->getVehicleData();
        garage.previewVehicleConfig = *garage.driver->getVehicleConfig();
        garage.previewTuning = garage.driver->getTuning();
        garage.currentStats = garage.previewTuning.computeVehicleStats();
        garage.upgradeStats = garage.currentStats;
        createMainGarageMenu();
    }, WidgetFlags::FADE_OUT_TRANSIENT | WidgetFlags::BACK | WidgetFlags::TRANSIENT, nullptr, [this]{
        return !!garage.driver->getVehicleData();
    });

    addButton("BUY CAR", nullptr, {280, 350-buttonSize.y*0.5f - buttonSize.y - 12}, buttonSize, [this]{
        i32 totalCost = garage.previewVehicle->price - garage.driver->getVehicleValue();
        garage.driver->credits -= totalCost;
        garage.driver->vehicleGuid = garage.previewVehicle->guid;
        garage.driver->vehicleConfig = garage.previewVehicleConfig;
    }, WidgetFlags::TRANSIENT, nullptr, [this]{
        i32 totalCost = garage.previewVehicle->price - garage.driver->getVehicleValue();
        return garage.previewVehicle->guid != garage.driver->vehicleGuid &&
            garage.driver->credits >= totalCost;
    });

    {
        Widget w;
        w.flags = WidgetFlags::TRANSIENT;
        w.onRender = [this](Widget& w, bool) {
            Font* fontBold = &g_res.getFont("font_bold", (u32)convertSize(24));
            Font* font = &g_res.getFont("font", (u32)convertSize(24));

            Vec2 pos = Vec2(-260, -150);
            Vec2 center = Vec2(g_game.windowWidth, g_game.windowHeight) * 0.5f;

            if (garage.driver->getVehicleData())
            {
                ui::text(fontBold,
                    tmpStr("PRICE: %i", garage.previewVehicle->price),
                    center + convertSize(pos), Vec3(1.f),
                    w.fadeInAlpha, 1.f, HAlign::LEFT, VAlign::TOP);
            }
            else
            {
                if (garage.driver->getVehicleData() != garage.previewVehicle)
                {
                    ui::text(font,
                        tmpStr("PRICE: %i", garage.previewVehicle->price),
                        center + convertSize(pos), Vec3(0.6f),
                        w.fadeInAlpha, 1.f, HAlign::LEFT, VAlign::TOP);

                    ui::text(font,
                        tmpStr("TRADE: %i", garage.driver->getVehicleValue()),
                        center + convertSize(pos + Vec2(0, 24)), Vec3(0.6f),
                        w.fadeInAlpha, 1.f, HAlign::LEFT, VAlign::TOP);

                    ui::text(fontBold,
                        tmpStr("TOTAL: %i", garage.previewVehicle->price - garage.driver->getVehicleValue()),
                        center + convertSize(pos + Vec2(0, 48)), Vec3(1.f),
                        w.fadeInAlpha, 1.f, HAlign::LEFT, VAlign::TOP);
                }
            }
        };
        widgets.push(w);
    }

    Vec2 size(150, 150);
    f32 x = 280-buttonSize.x*0.5f + size.x*0.5f;
    f32 y = -400 + size.y * 0.5f;
    f32 gap = 6;
    u32 buttonsPerRow = 3;

    static Array<RenderWorld> carLotRenderWorlds(vehicles.size());
    for (i32 i=0; i<(i32)vehicles.size(); ++i)
    {
        RenderWorld& rw = carLotRenderWorlds[i];

        Mesh* quadMesh = g_res.getModel("misc")->getMeshByName("Quad");
        rw.setName("Garage");
        rw.setBloomForceOff(true);
        rw.setSize((u32)convertSize(size.x)*2, (u32)convertSize(size.y)*2);
        drawSimple(&rw, quadMesh, &g_res.white, Mat4::scaling(Vec3(20.f)), Vec3(0.02f));

        VehicleConfiguration vehicleConfig;
        auto& vd = vehicles[i];
        vehicleConfig.cosmetics.color =
            srgb(hsvToRgb(vd->defaultColorHsv.x, vd->defaultColorHsv.y, vd->defaultColorHsv.z));
        vehicleConfig.cosmetics.hsv = vd->defaultColorHsv;

        VehicleTuning tuning;
        vehicles[i]->initTuning(vehicleConfig, tuning);
        f32 vehicleAngle = 0.f;
        Mat4 vehicleTransform = Mat4::rotationZ(vehicleAngle);
        vehicles[i]->render(&rw, Mat4::translation(Vec3(0, 0, tuning.getRestOffset())) *
            vehicleTransform, nullptr, vehicleConfig, &tuning);
        rw.setViewportCount(1);
        rw.addDirectionalLight(Vec3(-0.5f, 0.2f, -1.f), Vec3(1.0));
        rw.setViewportCamera(0, Vec3(8.f, -8.f, 10.f), Vec3(0.f, 0.f, 1.f), 1.f, 50.f, 30.f);
        g_game.renderer->addRenderWorld(&rw);

        Vec2 pos = Vec2(x + (i % buttonsPerRow) * (size.x + gap),
                        y + (i / buttonsPerRow) * (size.y + gap));
        Widget* w = addImageButton(vehicles[i]->name.data(), vehicles[i]->description.data(), pos, size, [i,this]{
            garage.previewVehicle = vehicles[i];
            if (garage.previewVehicle->guid == garage.driver->vehicleGuid)
            {
                garage.previewVehicleConfig = *garage.driver->getVehicleConfig();
                garage.previewTuning = garage.driver->getTuning();
                garage.currentStats = garage.previewTuning.computeVehicleStats();
                garage.upgradeStats = garage.currentStats;
            }
            else
            {
                garage.previewVehicleConfig = VehicleConfiguration{};
                auto vd = garage.previewVehicle;
                garage.previewVehicleConfig.cosmetics.color =
                    srgb(hsvToRgb(vd->defaultColorHsv.x, vd->defaultColorHsv.y, vd->defaultColorHsv.z));
                garage.previewVehicleConfig.cosmetics.hsv = vd->defaultColorHsv;
                garage.previewVehicleConfig.reloadMaterials();
                garage.previewVehicle->initTuning(garage.previewVehicleConfig, garage.previewTuning);
                garage.currentStats = garage.previewTuning.computeVehicleStats();
                garage.upgradeStats = garage.currentStats;
            }
        }, WidgetFlags::TRANSIENT, rw.getTexture(), 0.f, [i,this](bool isSelected) -> ImageButtonInfo {
            //bool buttonEnabled = garage.driver->credits >= g_vehicles[i]->price || i == garage.driver->vehicleIndex;
            bool buttonEnabled = true;
            auto v = vehicles[i];
            return ImageButtonInfo{
                buttonEnabled,
                v == garage.previewVehicle, 0, 0,
                v == garage.driver->getVehicleData() ? "OWNED" : tmpStr("%i", v->price), true };
        });

        if (i == 0)
        {
            selectedWidget = w;
        }
    }
}