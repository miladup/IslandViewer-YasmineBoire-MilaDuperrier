
#include "raylib.h"
#include "rlImGui.h"

#include "app.hpp"
#include "draw.hpp"
#include "generation.hpp"

int main() {
    ChangeDirectory(CMAKE_SOURCE_DIR);

    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(1280, 720, "IMAC island viewer");

    // Force a regular resizable window state at startup.
    ClearWindowState(FLAG_FULLSCREEN_MODE);
    SetWindowMinSize(800, 450);

    int const monitor { GetCurrentMonitor() };
    int const monitor_width { GetMonitorWidth(monitor) };
    int const monitor_height { GetMonitorHeight(monitor) };
    if (monitor_width > 0 && monitor_height > 0) {
        SetWindowSize(monitor_width/2, monitor_height/2);
        SetWindowPosition(monitor_width/4, monitor_height/4);
    }
    SetTargetFPS(30);

    rlImGuiSetup(true);

    AppContext context {};

    context.cube = GenMeshCube(1.0f, 1.0f, 1.0f);
    context.cubeMaterial = LoadMaterialDefault();
    context.cubeMaterial.maps[MATERIAL_MAP_DIFFUSE].color = RED;
    
    // Define our custom camera to look into our 3d world
    context.camera = {
        .position={ 18.0f, 21.0f, 18.0f },
        .target={ 0.0f, 0.0f, 0.0f },
        .up={ 0.0f, 1.0f, 0.0f },
        .fovy=45.0f,
        .projection=CAMERA_PERSPECTIVE
    };

    // loading texture from file exemple, not used for now since we generate the heightmap texture from code, but it can be useful for testing or as a fallback
    // std::filesystem::path path { pathUtils::make_absolute_path("resources/heightmap.png") };
    // context.image = LoadImage(path.string().c_str());     // Load heightmap image (RAM)
    // context.texture = LoadTextureFromImage(context.image);        // Convert image to texture (VRAM)
    // context.mesh = GenMeshHeightmap(context.image, context.terrainSize); // Generate heightmap mesh (RAM and VRAM)
    // context.model = LoadModelFromMesh(context.mesh);                  // Load model from generated mesh
    // context.model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = context.texture; // Set map diffuse texture

    generateHeightmap(context);
    regenerateMeshFromImage(context);
    generateObjectsPositions(context);

    // tropical
    context.biomeModels[0] = LoadModel("resources/biome/tree_palmDetailedTall.obj");
    context.waterModels[0] = LoadModel("resources/water/canoe.obj");
    context.mountainModels[0] = LoadModel("resources/mountains/stone_tallH.obj");

    // volcanic
    context.biomeModels[1] = LoadModel("resources/biome/stump_oldTall.obj");
    context.waterModels[1] = LoadModel("resources/water/stone_smallI.obj");
    context.mountainModels[1] = LoadModel("resources/mountains/stone_tallE.obj");

    // arctic
    context.biomeModels[2] = LoadModel("resources/biome/tree_pineTallB_detailed.obj");
    context.waterModels[2] = LoadModel("resources/water/stone_smallI_white.obj");
    context.mountainModels[2] = LoadModel("resources/mountains/stone_tallH.obj");

    // desert
    context.biomeModels[3] = LoadModel("resources/biome/cactus_tall.obj");
    context.waterModels[3] = LoadModel("resources/water/stone_smallFlatC.obj");
    context.mountainModels[3] = LoadModel("resources/mountains/stone_tallE_brown.obj");

    context.modelsLoaded = true;

    while (!WindowShouldClose())    // Detect window close button or ESC key
    {
        UpdateCamera(&context.camera, CAMERA_ORBITAL);
        BeginDrawing();

        ClearBackground(RAYWHITE);
        draw3DScene(context);
        drawRaylibUI(context);

        rlImGuiBegin();
        drawImGui(context);
        rlImGuiEnd();

        EndDrawing();
    }

    unload(context);

    // biomes
    for (int i = 0; i < 4; i++) {
        if (context.biomeModels[i].meshCount > 0) {
            UnloadModel(context.biomeModels[i]);
        }
    }

    // water
    for (int i = 0; i < 4; i++) {
        if (context.waterModels[i].meshCount > 0) {
            UnloadModel(context.waterModels[i]);
        }
    }

    // mountains
    for (int i = 0; i < 4; i++) {
        if (context.mountainModels[i].meshCount > 0) {
            UnloadModel(context.mountainModels[i]);
        }
    }
    
    rlImGuiShutdown();
    CloseWindow();
    return 0;
}