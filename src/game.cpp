#include "game.h"
#include "datafile.h"
#include "vehicle_data.h"
#include <chrono>
#include <iostream>

void Game::initPhysX()
{
    physx.foundation = PxCreateFoundation(PX_PHYSICS_VERSION, physx.allocator, physx.errorCallback);
    physx.pvd = PxCreatePvd(*physx.foundation);
    PxPvdTransport* transport = PxDefaultPvdSocketTransportCreate("127.0.0.1", 5425, 10);
    physx.pvd->connect(*transport, PxPvdInstrumentationFlag::eALL);

    physx.physics = PxCreatePhysics(PX_PHYSICS_VERSION, *physx.foundation, PxTolerancesScale(), true, physx.pvd);
    physx.cooking = PxCreateCooking(PX_PHYSICS_VERSION, *physx.foundation, PxCookingParams(PxTolerancesScale()));

    const u32 PHYSICS_THREADS = 0;
    physx.dispatcher = PxDefaultCpuDispatcherCreate(PHYSICS_THREADS);

    PxInitVehicleSDK(*physx.physics);
    PxVehicleSetBasisVectors(PxVec3(0, 0, 1), PxVec3(1, 0, 0));
    PxVehicleSetUpdateMode(PxVehicleUpdateMode::eVELOCITY_CHANGE);
    const f32 POINT_REJECT_ANGLE = PxPi / 4.0f;
    const f32 NORMAL_REJECT_ANGLE = PxPi / 4.0f;
    PxVehicleSetSweepHitRejectionAngles(POINT_REJECT_ANGLE, NORMAL_REJECT_ANGLE);
    PxVehicleSetMaxHitActorAcceleration(80.f);
}

void Game::run()
{
#ifndef NDEBUG
    print("Debug mode\n");
#endif

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) != 0)
    {
        FATAL_ERROR("SDL_Init Error: ", SDL_GetError())
    }

    window = renderer.initWindow("The Game", config.resolutionX, config.resolutionY);

    input.init();
    initPhysX();
    resources.load();
    initVehicleData();

    state.drivers = {
        Driver(true, true, true, &racecar, 0, 0),
        Driver(true, true, false, &sportscar, 1, 0),
        Driver(false, false, false, &car, 2),
        Driver(false, false, false, &car, 8),
        Driver(false, false, false, &sportscar, 4),
        Driver(false, false, false, &cubevan, 3),
        Driver(false, false, false, &sportscar, 6),
        Driver(false, false, false, &sportscar, 7),
    };

    changeScene("world.Scene");

    deltaTime = 1.f / (f32)game.config.maxFPS;
    SDL_Event event;
    bool shouldExit = false;
    while(true)
    {
        auto frameStartTime = std::chrono::high_resolution_clock::now();
        input.onFrameBegin();

        while(SDL_PollEvent(&event) != 0)
        {
            input.handleEvent(event);
            if(event.type == SDL_QUIT)
            {
                shouldExit = true;
            }
        }

        if (input.isKeyPressed(KEY_F4))
        {
            config.fullscreen = !config.fullscreen;
            SDL_SetWindowFullscreen(window, config.fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
        }

        if(shouldExit || input.isKeyPressed(KEY_ESCAPE))
        {
            break;
        }

        i32 w, h;
        SDL_GL_GetDrawableSize(window, &w, &h);
        if(w != (i32)windowWidth || h != (i32)windowHeight)
        {
            windowWidth = w;
            windowHeight = h;
        }

        currentScene->onUpdate(deltaTime);
        renderer.render(deltaTime);
        input.onFrameEnd();

        frameIndex = (frameIndex + 1) % MAX_BUFFERED_FRAMES;

        using seconds = std::chrono::duration<f64, std::ratio<1>>;
        if (!game.config.vsync)
        {
            f64 minFrameTime = 1.0 / (f64)game.config.maxFPS;
            auto frameEndTime = frameStartTime + seconds(minFrameTime);
            while (std::chrono::high_resolution_clock::now() < frameEndTime)
            {
                _mm_pause();
            }
        }

        const f32 maxDeltaTime = 1.f / 30.f;
        deltaTime = glm::min((f32)seconds(std::chrono::high_resolution_clock::now() - frameStartTime).count(), maxDeltaTime);
    }

    SDL_Quit();
}

void Game::changeScene(const char* sceneName)
{
    if (currentScene)
    {
        currentScene->onEnd();
    }
    print("Loading scene: ", sceneName, '\n');
    currentScene.reset(new Scene(sceneName));
    currentScene->onStart();
}
