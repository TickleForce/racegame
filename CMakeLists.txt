cmake_minimum_required(VERSION 3.14)
message("Build type: " ${CMAKE_BUILD_TYPE})

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)

include_directories(
    external
    external/sdl2
    external/sdl2/SDL2
    external/physx/physx/include
    external/physx/pxshared/include
    external/stb
    external/imgui
)
if(WIN32)
    link_directories(game external/sdl2/lib/x64/)
endif()
project(game)
add_executable(game src/main.cpp src/vorbis_decode.cpp src/imgui.cpp external/glad/glad.cpp)
target_compile_definitions(game PRIVATE "$<$<CONFIG:DEBUG>:_DEBUG>")
if(UNIX)
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D_GLIBCXX_DEBUG")
    target_link_libraries(game general SDL2 GL dl pthread)
    foreach(LIB_NAME
        libPhysXCooking_static_64.a
        libPhysXVehicle_static_64.a
        libPhysXExtensions_static_64.a
        libPhysX_static_64.a
        libPhysXPvdSDK_static_64.a
        libPhysXCharacterKinematic_static_64.a
        libPhysXFoundation_static_64.a
        libPhysXCommon_static_64.a)
        target_link_libraries(game debug ${CMAKE_SOURCE_DIR}/external/physx/physx/bin/linux.clang/checked/${LIB_NAME})
        target_link_libraries(game optimized ${CMAKE_SOURCE_DIR}/external/physx/physx/bin/linux.clang/release/${LIB_NAME})
    endforeach()
elseif(WIN32)
    target_link_libraries(game general opengl32 SDL2 SDL2main)
    foreach(LIB_NAME
        PhysXExtensions_static_64.lib
        PhysXCharacterKinematic_static_64.lib
        PhysXCommon_64.lib
        PhysXFoundation_64.lib
        PhysXCooking_64.lib
        PhysX_64.lib
        PhysXVehicle_static_64.lib
        PhysXPvdSDK_static_64.lib)
        target_link_libraries(game debug ${CMAKE_SOURCE_DIR}/external/physx/physx/bin/win.x86_64.vc141.md/debug/${LIB_NAME})
        target_link_libraries(game optimized ${CMAKE_SOURCE_DIR}/external/physx/physx/bin/win.x86_64.vc141.md/release/${LIB_NAME})
    endforeach()
endif()

if(UNIX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti -fno-exceptions")
elseif(MSVC)
    #foreach(flag_var
    #    CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
    #    CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)
    #    if(${flag_var} MATCHES "/MD")
    #        string(REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
    #    endif (${flag_var} MATCHES "/MD")
    #endforeach (flag_var)
endif()

add_custom_command(TARGET game POST_BUILD COMMAND
    ${CMAKE_COMMAND} -E copy $<TARGET_FILE:game> "${CMAKE_SOURCE_DIR}/bin/")
