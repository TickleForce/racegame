#pragma once

#include "misc.h"
#include "driver.h"
#include "menu.h"
#include "config.h"
#include "buffer.h"
#include <memory>

enum struct GameMode
{
    NONE,
    CHAMPIONSHIP,
    QUICK_RACE
};

class Game
{
    void initPhysX();

public:
    std::unique_ptr<class Renderer> renderer;

    struct
    {
        std::vector<Driver> drivers;
        u32 currentLeague = 0;
        u32 currentRace = 0;
        i32 driverContextIndex = 0;
        GameMode gameMode = GameMode::NONE;
    } state;

    struct
    {
        PxDefaultAllocator allocator;
        PxFoundation* foundation;
        PxPhysics* physics;
        PxDefaultCpuDispatcher* dispatcher;
        PxPvd* pvd;
        PxCooking* cooking;
    } physx;

    Config config;

    u32 windowWidth;
    u32 windowHeight;
    f32 realDeltaTime;
    f32 deltaTime;
    f64 currentTime = 0.0;
    f64 timeDilation = 1.0; // TODO: use this somewhere
    u64 frameCount = 0;
    u32 frameIndex = 0;
    bool shouldExit = false;
    bool isEditing = false;

    f32 recentHighestDeltaTime = FLT_MIN;
    f32 allTimeHighestDeltaTime = FLT_MIN;
    f32 allTimeLowestDeltaTime = FLT_MAX;
    f32 averageDeltaTime = 0.f;
    f32 deltaTimeHistory[300] = { 0 };

    SDL_Window* window = nullptr;
    std::unique_ptr<class Scene> currentScene;
    std::unique_ptr<class Scene> nextScene;
    Menu menu;
    Buffer tempMem = Buffer(megabytes(4), 16);

    void run();
    Scene* changeScene(const char* sceneName);

    void saveGame();
    void loadGame();
} g_game;

template <typename... Args>
char* tstr(Args const&... args)
{
    std::string s = str(args...);
    u8* mem = g_game.tempMem.writeBytes((void*)s.data(), s.size() + 1);
    return reinterpret_cast<char*>(mem);
}
