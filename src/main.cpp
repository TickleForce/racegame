// unity build

#include "math.cpp"
#include "game.cpp"
#include "threadpool.cpp"
#include "scene.cpp"
#include "renderer.cpp"
#include "batcher.cpp"
#include "datafile.cpp"
#include "resources.cpp"
#include "material.cpp"
#include "texture.cpp"
#include "vehicle.cpp"
#include "vehicle_data.cpp"
#include "vehicle_physics.cpp"
#include "font.cpp"
#include "track_graph.cpp"
//#include "motion_grid.cpp"
#include "particle_system.cpp"
#include "audio.cpp"
#include "driver.cpp"
#include "mesh.cpp"
#include "model.cpp"
#include "terrain.cpp"
#include "track.cpp"
#include "spline.cpp"
#include "dynamic_buffer.cpp"
#include "decal.cpp"
#include "menu.cpp"
#include "gui.cpp"
#include "weapon.cpp"
#include "entities/projectile.cpp"
#include "entities/mine.cpp"
#include "entities/flash.cpp"
#include "entities/static_mesh.cpp"
#include "entities/static_decal.cpp"
#include "entities/start.cpp"
#include "entities/booster.cpp"
#include "entities/oil.cpp"
#include "entities/glue.cpp"
#include "entities/entities.cpp"

#include "editor/editor_camera.cpp"
#include "editor/resource_manager.cpp"
#include "editor/track_editor.cpp"
#include "editor/model_editor.cpp"

#define STB_RECT_PACK_IMPLEMENTATION
#include <stb_rect_pack.h>
#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define STB_INCLUDE_LINE_GLSL
#define STB_INCLUDE_IMPLEMENTATION
#include <stb_include.h>
#define STB_DXT_IMPLEMENTATION
#include <stb_dxt.h>
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include <stb_image_resize.h>

int main(int argc, char** argv)
{
    g_game.run();
    return EXIT_SUCCESS;
}
