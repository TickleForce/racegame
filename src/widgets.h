#pragma once

#include "gui.h"
#include "2d.h"
#include "font.h"

#define DEFINE_CALLBACK(className, name) \
    void(*name ## Callback)(void*) = nullptr; \
    void* name ## CallbackData = nullptr; \
    template <typename _F_> \
    className* name(_F_ const& cb) { \
        name ## CallbackData = (void*)widgetBuffer.write<_F_>(cb); \
        name ## Callback = [](void* t){ (*(_F_*)t)(); }; \
        return this; \
    }

#define INVOKE_CALLBACK(name) \
    if (name ## Callback) { name ## Callback(name ## CallbackData); }

namespace gui
{
    const Vec4 COLOR_OUTLINE_SELECTED = Vec4(1.f, 0.6f, 0.05f, 1.f);
    const Vec4 COLOR_OUTLINE_NOT_SELECTED = Vec4(Vec3(0.8f), 1.f);
    const Vec4 COLOR_OUTLINE_DISABLED = Vec4(0.f);
    const Vec4 COLOR_BG_WIDGET = Vec4(Vec3(0), 0.95f);
    const Vec4 COLOR_BG_WIDGET_SELECTED = Vec4(Vec3(0.1f), 0.8f);
    const Vec4 COLOR_BG_WIDGET_DISABLED = Vec4(Vec3(0.06f), 0.8f);
    const Vec4 COLOR_BG_PANEL = Vec4(Vec3(0), 0.65f);
    const Vec4 COLOR_TEXT = Vec4(1);

    struct Stack : public Widget
    {
        Stack() : Widget("Stack") {}
    };

    struct FontDescription
    {
        const char* fontName;
        u32 fontSize;
    };

    struct Text : public Widget
    {
        Font* font;
        const char* text;
        Vec4 color;

        Text(FontDescription const& fontDescription, const char* text, Vec4 color=Vec4(1.0))
            : Widget("Text"), text(text), color(color)
        {
            font = &g_res.getFont(fontDescription.fontName, (u32)(fontDescription.fontSize * guiScale));
        }

        virtual void computeSize(Constraints const& constraints) override
        {
            desiredSize = font->stringDimensions(text);
#if 0
            computedSize.x = clamp(desiredSize.x, constraints.minWidth, constraints.maxWidth);
            computedSize.y = clamp(desiredSize.y, constraints.minHeight, constraints.maxHeight);
#else
            computedSize = desiredSize;
#endif

            assert(childFirst == nullptr);
            assert(constraints.maxWidth > 0);
            assert(constraints.maxHeight > 0);
        }

        virtual void render(GuiContext& ctx, RenderContext& rtx) override
        {
            ui::text(font, text, computedPosition, color.rgb, rtx.alpha * color.a);
        }
    };

    struct Alpha : public Widget
    {
        f32 alpha;

        Alpha(f32 alpha) : Widget("Alpha"), alpha(alpha) {}

        virtual void render(GuiContext& ctx, RenderContext& rtx) override
        {
            RenderContext r = rtx;
            r.alpha *= alpha;
            Widget::render(ctx, r);
        }
    };

    struct Image : public Widget
    {
        Texture* tex;
        Vec4 color;
        bool preserveAspectRatio = true;
        bool flip = false;

        Image(Texture* tex, bool preserveAspectRatio=true, bool flip=false, Vec4 const& color=Vec4(1))
            : Widget("Image"), tex(tex), preserveAspectRatio(preserveAspectRatio), flip(flip),
              color(color) {}

        virtual void computeSize(Constraints const& constraints) override
        {
            if (desiredSize.x == 0.f)
            {
                desiredSize.x = floor((f32)tex->width * guiScale);
            }
            if (desiredSize.y == 0.f)
            {
                desiredSize.y = floor((f32)tex->height * guiScale);
            }

            if (preserveAspectRatio)
            {
                if (desiredSize.x < constraints.minWidth)
                {
                    f32 aspectRatio = (f32)tex->height / (f32)tex->width;
                    desiredSize.x = constraints.minWidth;
                    desiredSize.y = constraints.minWidth * aspectRatio;
                }
                if (desiredSize.x > constraints.maxWidth)
                {
                    f32 aspectRatio = (f32)tex->height / (f32)tex->width;
                    desiredSize.x = constraints.maxWidth;
                    desiredSize.y = constraints.maxWidth * aspectRatio;
                }
                if (desiredSize.y < constraints.minHeight)
                {
                    f32 aspectRatio = (f32)tex->width / (f32)tex->height;
                    desiredSize.y = constraints.minHeight;
                    desiredSize.x = constraints.minHeight * aspectRatio;
                }
                if (desiredSize.y > constraints.maxHeight)
                {
                    f32 aspectRatio = (f32)tex->width / (f32)tex->height;
                    desiredSize.y = constraints.maxHeight;
                    desiredSize.x = constraints.maxHeight * aspectRatio;
                }
                computedSize.x = desiredSize.x;
                computedSize.y = desiredSize.y;
            }
            else
            {
                computedSize.x = clamp(desiredSize.x, constraints.minWidth, constraints.maxWidth);
                computedSize.y = clamp(desiredSize.y, constraints.minHeight, constraints.maxHeight);
            }

            assert(childFirst == nullptr);
        }

        virtual void render(GuiContext& ctx, RenderContext& rtx) override
        {
            Vec4 color(1.f);
            if (flip)
            {
                ui::rectUVBlur(0, tex, computedPosition, computedSize, Vec2(0,1), Vec2(1,0),
                        color, rtx.alpha);
            }
            else
            {
                ui::rectBlur(0, tex, computedPosition, computedSize, color, rtx.alpha);
            }
        }
    };

    struct Border : public Widget
    {
        Insets borders;
        Vec4 color;

        Border(Insets const& borders, Vec4 color)
            : Widget("Border"), borders(borders), color(color)
        {
            this->borders.left   = floorf(borders.left   * guiScale);
            this->borders.right  = floorf(borders.right  * guiScale);
            this->borders.top    = floorf(borders.top    * guiScale);
            this->borders.bottom = floorf(borders.bottom * guiScale);
        }

        virtual void computeSize(Constraints const& constraints) override
        {
            Vec2 maxDim(0, 0);

            if (childFirst)
            {
                f32 w = desiredSize.x > 0.f
                    ? clamp(desiredSize.x, constraints.minWidth, constraints.maxWidth)
                    : constraints.maxWidth;

                f32 h = desiredSize.y > 0.f
                    ? clamp(desiredSize.y, constraints.minHeight, constraints.maxHeight)
                    : constraints.maxHeight;

                Constraints childConstraints;
                childConstraints.minWidth = 0.f;
                childConstraints.maxWidth = w - (borders.left + borders.right);
                childConstraints.minHeight = 0.f;
                childConstraints.maxHeight = h - (borders.top + borders.bottom);

                for (Widget* child = childFirst; child; child = child->neighbor)
                {
                    child->computeSize(childConstraints);
                    maxDim = max(maxDim, child->computedSize);
                }
            }

            computedSize.x = desiredSize.x > 0.f
                ? clamp(desiredSize.x, constraints.minWidth, constraints.maxWidth)
                : maxDim.x + (borders.left + borders.right);
            computedSize.y = desiredSize.y > 0.f
                ? clamp(desiredSize.y, constraints.minHeight, constraints.maxHeight)
                : maxDim.y + (borders.top + borders.bottom);
        }

        virtual void layout(GuiContext& ctx) override
        {
            for (Widget* child = childFirst; child; child = child->neighbor)
            {
                child->positionWithinParent = child->desiredPosition + Vec2(borders.left, borders.top);
                child->computedPosition = computedPosition + child->positionWithinParent;
                child->layout(ctx);
            }
        }

        virtual void render(GuiContext& ctx, RenderContext& rtx) override
        {
            if (borders.top != 0.f)
            {
                ui::rectBlur(0, nullptr, computedPosition,
                        Vec2(computedSize.x, borders.top), color, rtx.alpha);
            }
            if (borders.bottom != 0.f)
            {
                ui::rectBlur(0, nullptr, computedPosition + Vec2(0, computedSize.y - borders.bottom),
                        Vec2(computedSize.x, borders.bottom), color, rtx.alpha);
            }
            if (borders.left != 0.f)
            {
                ui::rectBlur(0, nullptr, computedPosition,
                        Vec2(borders.left, computedSize.y), color, rtx.alpha);
            }
            if (borders.right != 0.f)
            {
                ui::rectBlur(0, nullptr, computedPosition + Vec2(computedSize.x - borders.right, 0),
                        Vec2(borders.right, computedSize.y), color, rtx.alpha);
            }
            Widget::render(ctx, rtx);
        }
    };

    struct Outline : public Widget
    {
        Insets borders;
        Vec4 color;

        Outline(Insets const& borders, Vec4 color)
            : Widget("Outline"), borders(borders), color(color)
        {
            this->borders.left   = floorf(borders.left   * guiScale);
            this->borders.right  = floorf(borders.right  * guiScale);
            this->borders.top    = floorf(borders.top    * guiScale);
            this->borders.bottom = floorf(borders.bottom * guiScale);
        }

        virtual void render(GuiContext& ctx, RenderContext& rtx) override
        {
            Widget::render(ctx, rtx);

            if (borders.top != 0.f)
            {
                ui::rectBlur(0, nullptr, computedPosition,
                        Vec2(computedSize.x, borders.top), color, rtx.alpha);
            }
            if (borders.bottom != 0.f)
            {
                ui::rectBlur(0, nullptr, computedPosition + Vec2(0, computedSize.y - borders.bottom),
                        Vec2(computedSize.x, borders.bottom), color, rtx.alpha);
            }
            if (borders.left != 0.f)
            {
                ui::rectBlur(0, nullptr, computedPosition,
                        Vec2(borders.left, computedSize.y), color, rtx.alpha);
            }
            if (borders.right != 0.f)
            {
                ui::rectBlur(0, nullptr, computedPosition + Vec2(computedSize.x - borders.right, 0),
                        Vec2(borders.right, computedSize.y), color, rtx.alpha);
            }
        }
    };

    struct Container : public Widget
    {
        Insets padding;
        Insets margin; // TODO: remove this if it is never used
        Vec4 backgroundColor = Vec4(0);
        HAlign halign = HAlign::LEFT;
        VAlign valign = VAlign::TOP;

        Container() : Widget("Container") {}
        Container(Insets const& padding, Insets const& margin = {},
                Vec4 const& backgroundColor = Vec4(0), HAlign halign = HAlign::LEFT,
                VAlign valign = VAlign::TOP)
            : Widget("Container"), padding(padding), margin(margin),
              backgroundColor(backgroundColor), halign(halign), valign(valign)
        {
            adjustScale();
        }
        Container(Vec4 const& backgroundColor)
            : Widget("Container"), backgroundColor(backgroundColor) {}

        void adjustScale()
        {
            padding.left   = floorf(padding.left   * guiScale);
            padding.right  = floorf(padding.right  * guiScale);
            padding.top    = floorf(padding.top    * guiScale);
            padding.bottom = floorf(padding.bottom * guiScale);

            margin.left   = floorf(margin.left   * guiScale);
            margin.right  = floorf(margin.right  * guiScale);
            margin.top    = floorf(margin.top    * guiScale);
            margin.bottom = floorf(margin.bottom * guiScale);
        }

        virtual void computeSize(Constraints const& constraints) override
        {
            f32 horizontalInset = padding.left + padding.right + margin.left + margin.right;
            f32 verticalInset = padding.top + padding.bottom + margin.top + margin.bottom;

            Vec2 maxDim(0, 0);
            if (childFirst)
            {
                f32 w = desiredSize.x > 0.f
                    ? clamp(desiredSize.x, constraints.minWidth, constraints.maxWidth)
                    : constraints.maxWidth;

                f32 h = desiredSize.y > 0.f
                    ? clamp(desiredSize.y, constraints.minHeight, constraints.maxHeight)
                    : constraints.maxHeight;

                Constraints childConstraints;
                childConstraints.minWidth = 0.f;
                childConstraints.maxWidth = max(w - horizontalInset, 0.f);
                childConstraints.minHeight = 0.f;
                childConstraints.maxHeight = max(h - verticalInset, 0.f);

                for (Widget* child = childFirst; child; child = child->neighbor)
                {
                    child->computeSize(childConstraints);
                    maxDim = max(maxDim, child->computedSize);
                }
            }

            computedSize.x = desiredSize.x > 0.f
                ? clamp(desiredSize.x, constraints.minWidth, constraints.maxWidth)
                : maxDim.x + horizontalInset;
            computedSize.y = desiredSize.y > 0.f
                ? clamp(desiredSize.y, constraints.minHeight, constraints.maxHeight)
                : maxDim.y + verticalInset;
        }

        virtual void layout(GuiContext& ctx) override
        {
            for (Widget* child = childFirst; child; child = child->neighbor)
            {
                Vec2 childPos(0, 0);

                if (halign == HAlign::LEFT)
                {
                    childPos.x = margin.left + padding.left;
                }
                else if (halign == HAlign::RIGHT)
                {
                    childPos.x =
                        computedSize.x - child->computedSize.x - margin.right - padding.right;
                }
                else if (halign == HAlign::CENTER)
                {
                    childPos.x = computedSize.x * 0.5f - child->computedSize.x * 0.5f;
                }

                if (valign == VAlign::TOP)
                {
                    childPos.y = margin.top + padding.top;
                }
                else if (valign == VAlign::BOTTOM)
                {
                    childPos.y =
                        computedSize.y - child->computedSize.y - margin.bottom - padding.bottom;
                }
                else if (valign == VAlign::CENTER)
                {
                    childPos.y = computedSize.y * 0.5f - child->computedSize.y * 0.5f;
                }

                child->positionWithinParent = child->desiredPosition + childPos;
                child->computedPosition = computedPosition + child->positionWithinParent;
                child->layout(ctx);
            }
        }

        virtual void render(GuiContext& ctx, RenderContext& rtx) override
        {
            if (backgroundColor.a > 0.f)
            {
                ui::rectBlur(0, nullptr, computedPosition + Vec2(margin.left, margin.top),
                    computedSize - Vec2(margin.left + margin.right, margin.top + margin.bottom),
                    backgroundColor, rtx.alpha);
            }
            Widget::render(ctx, rtx);
        }
    };

    struct Row : public Widget
    {
        f32 itemSpacing;
        f32 totalWidthOfChildren;
        HAlign halign;
        VAlign valign;

        Row(f32 itemSpacing=0.f, HAlign halign = HAlign::LEFT, VAlign valign = VAlign::TOP)
            : Widget("Row"), itemSpacing(itemSpacing * guiScale), halign(halign), valign(valign) {}

        virtual void computeSize(Constraints const& constraints) override
        {
            Constraints childConstraints;
            childConstraints.minWidth = 0.f;
            childConstraints.maxWidth = constraints.maxWidth;
            childConstraints.minHeight = 0.f;
            childConstraints.maxHeight = constraints.maxHeight;

            totalWidthOfChildren = 0.f;
            f32 maxHeight = 0.f;
            for (Widget* child = childFirst; child; child = child->neighbor)
            {
                child->computeSize(childConstraints);
                totalWidthOfChildren += child->computedSize.x + itemSpacing;
                maxHeight = max(maxHeight, child->computedSize.y);
                assert(child->desiredSize.x != INFINITY);
            }

            computedSize.x = desiredSize.x > 0.f
                ? clamp(desiredSize.x, constraints.minWidth, constraints.maxWidth)
                : totalWidthOfChildren - itemSpacing;
            computedSize.y = desiredSize.y > 0.f
                ? clamp(desiredSize.y, constraints.minHeight, constraints.maxHeight)
                : maxHeight;
        }

        virtual void layout(GuiContext& ctx) override
        {
            f32 offset = 0;
            for (Widget* child = childFirst; child; child = child->neighbor)
            {
                if (halign == HAlign::LEFT)
                {
                    child->positionWithinParent.x = child->desiredPosition.x + offset;
                    offset += child->computedSize.x + itemSpacing;
                }
                else if (halign == HAlign::RIGHT)
                {
                    offset += child->computedSize.x + itemSpacing;
                    child->positionWithinParent.x = computedSize.x - offset + child->desiredPosition.x;
                }
                else if (halign == HAlign::CENTER)
                {
                    child->positionWithinParent.x = 0.5f * (computedSize.x - totalWidthOfChildren)
                        + offset + child->desiredPosition.x;
                    offset += child->computedSize.x + itemSpacing;
                }

                if (valign == VAlign::TOP)
                {
                    child->positionWithinParent.y = child->desiredPosition.y;
                }
                else if (valign == VAlign::BOTTOM)
                {
                    child->positionWithinParent.y = computedSize.y - child->desiredPosition.y;
                }
                else if (valign == VAlign::CENTER)
                {
                    child->positionWithinParent.y =
                        0.5f * (computedSize.y - child->computedSize.y) - child->desiredPosition.y;
                }

                child->computedPosition = computedPosition + child->positionWithinParent;
                child->layout(ctx);
            }
        }
    };

    struct Column : public Widget
    {
        f32 itemSpacing;
        f32 totalHeightOfChildren;
        HAlign halign;
        VAlign valign;

        Column(f32 itemSpacing=0.f, HAlign halign=HAlign::LEFT, VAlign valign=VAlign::TOP)
            : Widget("Column"), itemSpacing(itemSpacing * guiScale), halign(halign), valign(valign) {}

        virtual void computeSize(Constraints const& constraints) override
        {
            Constraints childConstraints;
            childConstraints.minWidth = 0.f;
            childConstraints.maxWidth = constraints.maxWidth;
            childConstraints.minHeight = 0.f;
            childConstraints.maxHeight = constraints.maxHeight;

            totalHeightOfChildren = 0.f;
            f32 maxWidth = 0.f;
            for (Widget* child = childFirst; child; child = child->neighbor)
            {
                child->computeSize(childConstraints);
                totalHeightOfChildren += child->computedSize.y + itemSpacing;
                maxWidth = max(maxWidth, child->computedSize.x);
                assert(child->desiredSize.y != INFINITY);
            }

            computedSize.x = desiredSize.x > 0.f
                ? clamp(desiredSize.x, constraints.minWidth, constraints.maxWidth)
                : maxWidth;
            computedSize.y = desiredSize.y > 0.f
                ? clamp(desiredSize.y, constraints.minHeight, constraints.maxHeight)
                : totalHeightOfChildren - itemSpacing;
        }

        virtual void layout(GuiContext& ctx) override
        {
            f32 offset = 0;
            for (Widget* child = childFirst; child; child = child->neighbor)
            {
                if (halign == HAlign::LEFT)
                {
                    child->positionWithinParent.x = child->desiredPosition.x;
                }
                else if (halign == HAlign::RIGHT)
                {
                    child->positionWithinParent.x = computedSize.x - child->desiredPosition.x;
                }
                else if (halign == HAlign::CENTER)
                {
                    child->positionWithinParent.x =
                        0.5f * (computedSize.x - child->computedSize.x) - child->desiredPosition.x;
                }

                if (valign == VAlign::TOP)
                {
                    child->positionWithinParent.y = child->desiredPosition.y + offset;
                    offset += child->computedSize.y + itemSpacing;
                }
                else if (valign == VAlign::BOTTOM)
                {
                    offset += child->computedSize.y + itemSpacing;
                    child->positionWithinParent.y = computedSize.y - offset + child->desiredPosition.y;
                }
                else if (valign == VAlign::CENTER)
                {
                    child->positionWithinParent.y = 0.5f * (computedSize.y - totalHeightOfChildren)
                        + offset + child->desiredPosition.y;
                    offset += child->computedSize.y + itemSpacing;
                }

                child->computedPosition = computedPosition + child->positionWithinParent;
                child->layout(ctx);
            }
        }
    };

    struct Grid : public Widget
    {
        u32 columns;
        f32 itemSpacing;

        Grid(u32 columns, f32 itemSpacing=0.f)
            : Widget("Grid"), columns(columns), itemSpacing(itemSpacing * guiScale) {}

        Widget* build()
        {
            assert(desiredSize.x > 0.f);
            return this;
        }

        virtual void computeSize(Constraints const& constraints) override
        {
            computedSize.x = clamp(desiredSize.x, constraints.minWidth, constraints.maxWidth);

            Constraints childConstraints;
            childConstraints.minWidth = 0.f;
            childConstraints.maxWidth = (computedSize.x - itemSpacing * (columns - 1)) / columns;
            childConstraints.minHeight = 0.f;
            //childConstraints.maxHeight = constraints.maxHeight;
            childConstraints.maxHeight = childConstraints.maxWidth;

            f32 totalHeight = 0.f;
            f32 maxRowHeight = 0.f;
            u32 rowChildCount = 0;
            for (Widget* child = childFirst; child; child = child->neighbor)
            {
                child->computeSize(childConstraints);
                maxRowHeight = max(maxRowHeight, child->computedSize.y);

                if (rowChildCount == 0)
                {
                    totalHeight += maxRowHeight + itemSpacing;
                }

                if (++rowChildCount == columns)
                {
                    maxRowHeight = 0.f;
                    rowChildCount = 0;
                }

                assert(child->desiredSize.y != INFINITY);
            }

            computedSize.y = desiredSize.y > 0.f
                ? clamp(desiredSize.y, constraints.minHeight, constraints.maxHeight)
                : totalHeight - itemSpacing;
        }

        virtual void layout(GuiContext& ctx) override
        {
            f32 totalHeight = 0.f;
            f32 maxRowHeight = 0.f;
            f32 nextX = 0.f;
            u32 rowChildCount = 0;
            for (Widget* child = childFirst; child; child = child->neighbor)
            {
                child->positionWithinParent = Vec2(nextX, totalHeight);
                child->computedPosition = computedPosition + child->positionWithinParent;
                child->layout(ctx);

                maxRowHeight = max(maxRowHeight, child->computedSize.y);

                nextX += (computedSize.x - itemSpacing * (columns - 1)) / columns + itemSpacing;
                if (++rowChildCount == columns)
                {
                    totalHeight += maxRowHeight + itemSpacing;
                    maxRowHeight = 0.f;
                    rowChildCount = 0;
                    nextX = 0.f;
                }
            }
        }
    };

    struct FlexibleColumn : public Widget { };

    struct FlexibleRow : public Widget { };

    struct ScrollPanel : public Widget
    {
        // TODO: unlink the children that won't be visible, in the earliest pass possible, so they
        // are not processed by later stages
    };

    namespace ButtonFlags
    {
        enum
        {
            BACK         = 1 << 0,
            NO_ACTIVE    = 1 << 1,
            FORCE_ACTIVE = 1 << 2,
        };
    };

    struct Button : public Widget
    {
        Container* bg;
        Outline* outline;
        Alpha* alpha;
        u32 buttonFlags;
        f32 padding;
        const char* name;

        Button(u32 buttonFlags=0, const char* name="Button", f32 padding=10.f)
            : Widget(name), name(name), buttonFlags(buttonFlags), padding(padding) {}

        struct ButtonState
        {
            f32 hoverIntensity = 0.f;
            f32 hoverTimer = 0.f;
            f32 hoverValue = 0.f;
            bool isPressed = false;
        };

        Widget* build()
        {
            pushID(name);

            flags |= WidgetFlags::BLOCK_INPUT | WidgetFlags::SELECTABLE;

            outline = (Outline*)this->add(Outline(Insets(2), {}))->size(desiredSize);
            alpha = (Alpha*)outline->add(Alpha(1.f))->size(desiredSize);
            bg = (Container*)alpha->add(
                    Container(Insets(padding), {}, COLOR_BG_WIDGET, HAlign::CENTER, VAlign::CENTER))
                ->size(desiredSize);
            return bg;
        }

        DEFINE_CALLBACK(Button, onPress);
        DEFINE_CALLBACK(Button, onSelect);
        DEFINE_CALLBACK(Button, onGainedSelect);
        DEFINE_CALLBACK(Button, onLostSelect);

        void press(InputCaptureContext& ctx)
        {
            if (!(buttonFlags & ButtonFlags::NO_ACTIVE))
            {
                getState<ButtonState>(this)->isPressed = true;
            }
            ctx.selectedWidgetStateNode = stateNode;
            INVOKE_CALLBACK(onPress);
            g_audio.playSound(g_res.getSound("click"), SoundType::MENU_SFX);
        }

        virtual bool handleInputEvent(
                InputCaptureContext& ctx, Widget* inputCapture, InputEvent const& ev) override
        {
            if (flags & WidgetFlags::DISABLED)
            {
                return false;
            }

            if (ctx.selectedWidgetStateNode == stateNode)
            {
                if ((ev.type == INPUT_KEYBOARD_PRESSED && ev.keyboard.key == KEY_RETURN)
                    || (ev.type == INPUT_CONTROLLER_BUTTON_PRESSED && ev.controller.button == BUTTON_A))
                {
                    press(ctx);
                    return true;
                }
            }
            if (buttonFlags & ButtonFlags::BACK)
            {
                if (ev.type == INPUT_KEYBOARD_PRESSED && ev.keyboard.key == KEY_ESCAPE)
                {
                    press(ctx);
                    return true;
                }
                else if (ev.type == INPUT_CONTROLLER_BUTTON_PRESSED && ev.controller.button == BUTTON_B)
                {
                    press(ctx);
                    return true;
                }
            }

            return false;
        }

        virtual bool handleMouseInput(InputCaptureContext& ctx, InputEvent& input)
            override
        {
            if (flags & WidgetFlags::DISABLED)
            {
                return false;
            }

            bool isMouseOver =
                pointInRectangle(input.mouse.mousePos, computedPosition, computedPosition + computedSize);

            if (!isMouseOver)
            {
                return false;
            }

            ctx.selectedWidgetStateNode = stateNode;

            if (input.type == INPUT_MOUSE_BUTTON_PRESSED && input.mouse.button == MOUSE_LEFT)
            {
                press(ctx);
            }

            return true;
        }

        virtual void render(GuiContext& ctx, RenderContext& rtx) override
        {
            ButtonState* state = getState<ButtonState>(this);
            RenderContext r = rtx;

            if (buttonFlags & ButtonFlags::FORCE_ACTIVE)
            {
                state->isPressed = true;
            }
            else if (!(flags & WidgetFlags::STATUS_SELECTED))
            {
                state->isPressed = false;
            }

            if (flags & WidgetFlags::DISABLED)
            {
                state->hoverIntensity = 0.f;
                state->hoverTimer = 0.f;
                state->hoverValue = 0.f;

                outline->borders = Insets(2);
                outline->color = COLOR_OUTLINE_DISABLED;

                bg->backgroundColor = COLOR_BG_WIDGET_DISABLED;

                r.alpha *= 0.5f;
            }
            else
            {
                if (flags & WidgetFlags::STATUS_SELECTED)
                {
                    state->hoverIntensity = min(state->hoverIntensity + ctx.deltaTime * 4.f, 1.f);
                    state->hoverTimer += ctx.deltaTime;
                    state->hoverValue = 0.1f + (sinf(state->hoverTimer * 4.f) + 1.f) * 0.5f;

                    outline->borders = Insets(4);
                    outline->color = COLOR_OUTLINE_SELECTED;
                }
                else
                {
                    state->hoverIntensity = max(state->hoverIntensity - ctx.deltaTime * 2.f, 0.f);
                    state->hoverTimer = 0.f;
                    state->hoverValue = max(state->hoverValue - ctx.deltaTime * 2.f, 0.f);

                    outline->borders = Insets(2);
                    outline->color = COLOR_OUTLINE_NOT_SELECTED;
                }

                if (flags & WidgetFlags::FADED)
                {
                    bg->backgroundColor = COLOR_BG_WIDGET_DISABLED;
                    alpha->alpha *= 0.5f;
                }
                else
                {
                    if (state->isPressed)
                    {
                        bg->backgroundColor = COLOR_OUTLINE_SELECTED;
                    }
                    else
                    {
                        bg->backgroundColor = mix(COLOR_BG_WIDGET,
                                COLOR_BG_WIDGET_SELECTED, state->hoverValue * state->hoverIntensity);
                    }
                }
            }
            Widget::render(ctx, r);
        }

        virtual void handleStatus() override
        {
            if (flags & WidgetFlags::STATUS_SELECTED)
            {
                INVOKE_CALLBACK(onSelect);
            }
            if (flags & WidgetFlags::STATUS_GAINED_SELECTED)
            {
                INVOKE_CALLBACK(onGainedSelect);
            }
            if (flags & WidgetFlags::STATUS_LOST_SELECTED)
            {
                INVOKE_CALLBACK(onLostSelect);
            }
        }
    };

    struct TextButton : public Button
    {
        const char* text;
        FontDescription font;
        Texture* icon;

        TextButton(FontDescription const& font, const char* text, u32 buttonFlags=0,
                Texture* icon=nullptr)
            : Button(buttonFlags, "TextButton"), text(text), font(font), icon(icon) {}

        Widget* build()
        {
            pushID(text);
            auto btn = Button::build();
            if (icon)
            {
                auto row = btn->add(Row(20, HAlign::LEFT, VAlign::CENTER));
                const f32 iconSize = 48;
                row->add(Image(icon, false))->size(iconSize, iconSize);
                row->add(Text(font, text));
            }
            else
            {
                btn->add(Text(font, text));
            }
            return btn;
        }
    };

    struct TextEdit : public Widget { };

    struct TextArea : public Widget { };

    struct ColorPicker : public Widget { };

    struct Toggle : public Widget { };

    struct RadioButton : public Widget { };

    struct Slider : public Widget { };

    struct Select : public Widget { };

    struct Tabs : public Widget { };

    struct Window : public Widget {  };

    struct Transform : public Widget
    {
        Mat4 transform;
        bool center;

        Transform(Mat4 const& transform, bool center=true)
            : Widget("Transform"), transform(transform), center(center) {}

        virtual void layout(GuiContext& ctx) override
        {
            if (center)
            {
                Vec2 offset = -(computedPosition + computedSize * 0.5f);
                transform = Mat4::translation(Vec3(-offset, 0.f)) *
                    transform * Mat4::translation(Vec3(offset, 0.f));
            }
            Widget::layout(ctx);
        }

        virtual bool handleMouseInput(InputCaptureContext& ctx, InputEvent& input) override
        {
            // TODO: Make this work for more than just 2D transformations
            Vec2 prevMousePos = input.mouse.mousePos;
            input.mouse.mousePos = (inverse(transform) * Vec4(input.mouse.mousePos, 0.f, 1.f)).xy;
            bool result = Widget::handleMouseInput(ctx, input);
            input.mouse.mousePos = prevMousePos;
            return result;
        }

        virtual void render(GuiContext& ctx, RenderContext& rtx) override
        {
            Mat4 oldTransform = rtx.transform;
            rtx.transform = rtx.transform * transform;
            ui::setTransform(rtx.transform);
            Widget::render(ctx, rtx);
            ui::setTransform(oldTransform);
        }
    };

    struct Animation : public Widget
    {
        f32 animationLength;
        bool isDirectionForward = true;

        struct AnimationState
        {
            f32 animationProgress = 0.f;
            bool finishedBegin = false;
            bool finishedEnd = false;
        };

        Animation(f32 animationLength = 1.f, bool isDirectionForward=true, const char* name="Animation")
            : Widget(name), animationLength(animationLength), isDirectionForward(isDirectionForward) {}

        DEFINE_CALLBACK(Animation, onAnimationEnd);
        DEFINE_CALLBACK(Animation, onAnimationReverseEnd);

        AnimationState* animate()
        {
            auto state = getState<AnimationState>(this);
            if (isDirectionForward)
            {
                state->animationProgress =
                    min(state->animationProgress + g_game.deltaTime * (1.f / animationLength), 1.f);
                if (state->animationProgress == 1.f && !state->finishedBegin)
                {
                    INVOKE_CALLBACK(onAnimationEnd);
                    state->finishedBegin = true;
                }
                if (state->animationProgress < 1.f)
                {
                    state->finishedBegin = false;
                }
            }
            else
            {
                state->animationProgress =
                    max(state->animationProgress - g_game.deltaTime * (1.f / animationLength), 0.f);
                if (state->animationProgress == 0.f && !state->finishedEnd)
                {
                    INVOKE_CALLBACK(onAnimationReverseEnd);
                    state->finishedEnd = true;
                }
                if (state->animationProgress > 0.f)
                {
                    state->finishedEnd = false;
                }
            }
            return state;
        }
    };

    struct SlideAnimation : public Animation
    {
        SlideAnimation(f32 animationLength = 1.f, bool isDirectionForward=true)
            : Animation(animationLength, isDirectionForward, "SlideAnimation") {}

        Widget* build()
        {
            pushID("SlideAnimation");
            return this;
        }

        virtual void layout(GuiContext& ctx) override
        {
            auto state = animate();

            //f32 t = easeInOutBezier(state->animationProgress);
            f32 t = easeInOutParametric(state->animationProgress);
            for (Widget* child = childFirst; child; child = child->neighbor)
            {
                child->positionWithinParent =
                    child->desiredPosition - Vec2(0, (1.f - t) * g_game.windowHeight);
                child->computedPosition = computedPosition + child->positionWithinParent;
                child->layout(ctx);
            }
        }
    };

    struct FadeAnimation : public Animation
    {
        f32 fromAlpha;
        f32 toAlpha;

        FadeAnimation(f32 fromAlpha=0.f, f32 toAlpha=1.f,
                f32 animationLength = 1.f, bool isDirectionForward=true,
                const char* name="FadeAnimation")
            : Animation(animationLength, isDirectionForward, name), fromAlpha(fromAlpha),
              toAlpha(toAlpha) {}

        Widget* build()
        {
            pushID(name);
            return this;
        }

        virtual void layout(GuiContext& ctx) override
        {
            auto state = animate();
            Widget::layout(ctx);
        }

        virtual void render(GuiContext& ctx, RenderContext& rtx) override
        {
            auto state = getState<AnimationState>(this);
            RenderContext r = rtx;
            r.alpha = rtx.alpha * lerp(fromAlpha, toAlpha, state->animationProgress);
            Widget::render(ctx, r);
        }
    };

    struct ScaleAnimation : public Animation
    {
        f32 startScale;
        f32 endScale;
        Transform* transform;

        ScaleAnimation(f32 startScale=0.5f, f32 endScale=1.f,
                f32 animationLength = 1.f, bool isDirectionForward=true)
            : Animation(animationLength, isDirectionForward, "ScaleAnimation"),
              startScale(startScale), endScale(endScale) {}

        Widget* build()
        {
            pushID("ScaleAnimation");
            transform = this->add(Transform(Mat4(1.f)));
            return transform;
        }

        virtual void layout(GuiContext& ctx) override
        {
            auto state = animate();
            f32 t = easeInOutParametric(state->animationProgress);
            Mat4 scale =
                Mat4::scaling(Vec3(Vec2(lerp(startScale, endScale, t)), 1.f));
            transform->transform = scale;
            Widget::layout(ctx);
        }

        virtual void computeSize(Constraints const& constraints) override
        {
            transform->size(desiredSize);
            Widget::computeSize(constraints);
        }
    };

    struct InputCapture : public Widget
    {
        const char* name;
        bool blockInput = false;
        bool hideChildrenWhenInactive = false;
        bool active = false;

        struct InputCaptureState
        {
            bool isEntering = true;
        };

        InputCapture(const char* name=nullptr, bool blockInput=false)
            : Widget("InputCapture"), name(name), blockInput(blockInput)
        {
            flags |= WidgetFlags::INPUT_CAPTURE;
        }

        Widget* build()
        {
            pushID(name ? name : "InputCapture");
            addInputCapture(this);
            active = isActiveInputCapture(this);
            return this;
        }

        bool isEntering() { return getState<InputCaptureState>(this)->isEntering; }

        void setEntering(bool entering) { getState<InputCaptureState>(this)->isEntering = entering;  }

        virtual void layout(GuiContext& ctx) override
        {
            if (!active && hideChildrenWhenInactive)
            {
                return;
            }
            Widget::layout(ctx);
        }

        virtual bool handleInputCaptureEvent(InputCaptureContext& ctx, InputEvent const& ev,
                Widget* selectedWidget) override
        {
            if (blockInput || !isEntering())
            {
                return false;
            }
            return Widget::handleInputCaptureEvent(ctx, ev, selectedWidget);
        }

        virtual bool handleInputEvent(
                InputCaptureContext& ctx, Widget* inputCapture, InputEvent const& ev) override
        {
            if (blockInput || !isEntering())
            {
                return false;
            }
            return Widget::handleInputEvent(ctx, inputCapture, ev);
        }

        virtual bool handleMouseInput(InputCaptureContext& ctx, InputEvent& input) override
        {
            if (!active || blockInput || !isEntering())
            {
                return false;
            }
            return Widget::handleMouseInput(ctx, input);
        }

        virtual void render(GuiContext& ctx, RenderContext& rtx) override
        {
            if (!active && hideChildrenWhenInactive)
            {
                return;
            }
            Widget::render(ctx, rtx);
        }
    };

    struct Clip : public Widget
    {
        Clip() : Widget("Clip") {}

        virtual void render(GuiContext& ctx, RenderContext& rtx) override
        {
            RenderContext r = rtx;
            r.scissorPos = computedPosition;
            r.scissorSize = computedSize;
            ui::setScissor(r.scissorPos, r.scissorSize);
            Widget::render(ctx, r);
            ui::setScissor(rtx.scissorPos, rtx.scissorSize);
        }
    };

    struct Delay : public Widget
    {
        f32 delay;
        const char* name;

        struct DelayState
        {
            f32 timer = 0.f;
        };

        Delay(f32 delay, const char* name) : Widget("Delay"), delay(delay), name(name) {}

        Widget* build()
        {
            pushID(name);
            auto state = getState<DelayState>(this);
            if (state->timer > delay)
            {
                return this;
            }
            return nullWidget;
        }

        virtual void layout(GuiContext& ctx) override
        {
            auto state = getState<DelayState>(this);
            state->timer += ctx.deltaTime;
            Widget::layout(ctx);
        }
    };

    struct PositionDelay : public Widget
    {
        f32 amount;
        f32 delay;
        const char* name;

        struct DelayState
        {
            f32 timer = 0.f;
        };

        PositionDelay(f32 amount, const char* name)
            : Widget("Position Delay"), amount(amount), name(name) {};

        Widget* build()
        {
            pushID(name);
            return this;
        }

        virtual void layout(GuiContext& ctx) override
        {
            auto state = getState<DelayState>(this);
            state->timer += ctx.deltaTime;
            delay = computedPosition.y / ctx.actualScreenSize.y * amount;
            if (state->timer > delay)
            {
                Widget::layout(ctx);
            }
        }

        virtual void render(GuiContext& ctx, RenderContext& rtx) override
        {
            auto state = getState<DelayState>(this);
            if (state->timer > delay)
            {
                Widget::render(ctx, rtx);
            }
        }
    };
};
