#include "editor_camera.h"
#include "../input.h"
#include "../game.h"
#include "../imgui.h"

void EditorCamera::update(f32 deltaTime, RenderWorld* rw)
{
    bool isKeyboardHandled = ImGui::GetIO().WantCaptureKeyboard;

    if (!isKeyboardHandled)
    {
        f32 right = (f32)g_input.isKeyDown(KEY_D) - (f32)g_input.isKeyDown(KEY_A);
        f32 up = (f32)g_input.isKeyDown(KEY_S) - (f32)g_input.isKeyDown(KEY_W);
        Vec3 moveDir = (right != 0.f || up != 0.f)
            ? normalize(Vec3(right, up, 0.f)) : Vec3(0, 0, 0);
        Vec3 forward(-lengthdir(cameraYaw, 1.f), 0.f);
        Vec3 sideways(-lengthdir(cameraYaw + PI / 2.f, 1.f), 0.f);

        cameraVelocity += (((forward * moveDir.y) + (sideways * moveDir.x)) * (deltaTime * (120.f + cameraDistance * 1.5f)));
    }
    cameraTarget += cameraVelocity * deltaTime;
    cameraVelocity = smoothMove(cameraVelocity, Vec3(0, 0, 0), 7.f, deltaTime);

    if (g_input.isMouseButtonPressed(MOUSE_RIGHT))
    {
        lastMousePosition = g_input.getMousePosition();
    }
    else if (g_input.isMouseButtonDown(MOUSE_RIGHT))
    {
        cameraYawSpeed = (((lastMousePosition.x) - g_input.getMousePosition().x) / g_game.windowWidth * 2.f) * (1.f / deltaTime);
        cameraPitchSpeed = (((lastMousePosition.y) - g_input.getMousePosition().y) / g_game.windowHeight * 2.f) * (1.f / deltaTime);
        lastMousePosition = g_input.getMousePosition();
    }

    cameraYaw += cameraYawSpeed * deltaTime;
    cameraYawSpeed = smoothMove(cameraYawSpeed, 0, 8.f, deltaTime);
    cameraPitch = clamp(cameraPitch + cameraPitchSpeed * deltaTime, -1.5f, 1.5f);
    cameraPitchSpeed = smoothMove(cameraPitchSpeed, 0, 8.f, deltaTime);

    if (!ImGui::GetIO().WantCaptureMouse && !g_input.isKeyDown(KEY_LCTRL)
            && g_input.getMouseScroll() != 0)
    {
        zoomSpeed = g_input.getMouseScroll() * (cameraDistance * 0.01f);
    }
    cameraDistance = clamp(cameraDistance - zoomSpeed, 3.f, 250.f);
    zoomSpeed = smoothMove(zoomSpeed, 0.f, 10.f, deltaTime);

    Vec3 cameraDir(
            cosf(cameraYaw) * cosf(cameraPitch),
            sinf(cameraYaw) * cosf(cameraPitch),
            sinf(cameraPitch));
    cameraFrom = cameraTarget - cameraDir * cameraDistance;
    rw->setViewportCamera(0, cameraFrom, cameraTarget, nearDistance, farDistance, 53.f);
    camera = rw->getCamera(0);
}

Vec3 EditorCamera::getMouseRay(RenderWorld* rw) const
{
    Vec2 mousePos = g_input.getMousePosition();
    Vec3 rayDir = screenToWorldRay(mousePos,
            Vec2(g_game.windowWidth, g_game.windowHeight), camera.view, camera.projection);
    return rayDir;
}
