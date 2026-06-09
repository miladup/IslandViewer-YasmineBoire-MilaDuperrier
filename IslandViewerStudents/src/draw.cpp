#include "draw.hpp"

#include "app.hpp"

#include "generation.hpp"

#include "imgui.h"
#include "raylib.h"
#include "raymath.h"

void draw3DScene(AppContext &context)
{
    ClearBackground(RAYWHITE);

    BeginMode3D(context.camera);

    Matrix const terrainCentering{getTerrainCenteringMatrix(context)};
    Vector3 const terrainCenterOffset{terrainCentering.m12, terrainCentering.m13, terrainCentering.m14};

    DrawModel(context.model, terrainCenterOffset, 1.0f, WHITE);
    drawCubes(context, terrainCentering);
    DrawGrid(20, 1.0f);

    EndMode3D();
}

void drawCubes(AppContext const &context, Matrix const &terrainCentering)
{
    if (context.objectPositions.empty())
    {
        return;
    }

    float const cubeHalfHeight{0.5f * context.cubeScale}; 

    for (glm::vec3 const &pos : context.objectPositions)
    {
        float height = pos.z;
        int currentTheme = context.imageGenerationParameters.selectedPalette;

        Vector3 objectPos = {
            pos.x * context.terrainSize.x,
            pos.z * context.terrainSize.y,
            pos.y * context.terrainSize.z
        };

        objectPos = Vector3Transform(objectPos, terrainCentering);

        if (!context.modelsLoaded)
        {
            Color backupColor = (height < 0.30f) ? BLUE : ((height > 0.55f) ? GRAY : GREEN);
            objectPos.y += cubeHalfHeight; 
            DrawCube(objectPos, context.cubeScale, context.cubeScale, context.cubeScale, backupColor);
            continue; 
        }

        Model const* modelToDraw = nullptr;

        if (height < 0.30f)
        {
            modelToDraw = &context.waterModels[currentTheme];
        }
        else if (height > 0.55f)
        {
            modelToDraw = &context.mountainModels[currentTheme];
        }
        else
        {
            modelToDraw = &context.biomeModels[currentTheme];
        }

        if (modelToDraw != nullptr && modelToDraw->meshCount > 0)
        {
            float displayScale = context.cubeScale; 
            DrawModelEx(*modelToDraw, objectPos, Vector3{0.0f, 1.0f, 0.0f}, 0.0f, Vector3{displayScale, displayScale, displayScale}, WHITE);
        }
        else
        {
            Color backupColor = (height < 0.30f) ? BLUE : ((height > 0.55f) ? GRAY : GREEN);
            objectPos.y += cubeHalfHeight; 
            DrawCube(objectPos, context.cubeScale, context.cubeScale, context.cubeScale, backupColor);
        }
    }
}

void drawImGui(AppContext &context)
{
    if (ImGui::Button("Generate random positions"))
    {
        generateObjectsPositions(context);
    }

    auto &imgParams = context.imageGenerationParameters;

    if (ImGui::CollapsingHeader("Island generation", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Text("Fractal noise (FBM)");

        if (ImGui::SliderInt("Seed", &imgParams.noiseSeed, 0, 1000))
        {
            generateHeightmap(context);
            regenerateMeshFromImage(context);
        }

        if (ImGui::SliderFloat("Scale", &imgParams.noiseScale, 0.01f, 10.0f))
        {
            generateHeightmap(context);
            regenerateMeshFromImage(context);
        }

        if (ImGui::SliderInt("Octaves", &imgParams.octaves, 1, 8))
        {
            generateHeightmap(context);
            regenerateMeshFromImage(context);
        }

        if (ImGui::SliderFloat("Lacunarity", &imgParams.lacunarity, 1.0f, 4.0f, "%.2f"))
        {
            generateHeightmap(context);
            regenerateMeshFromImage(context);
        }

        if (ImGui::SliderFloat("Gain", &imgParams.gain, 0.0f, 1.0f, "%.2f"))
        {
            generateHeightmap(context);
            regenerateMeshFromImage(context);
        }

        ImGui::Separator();
        ImGui::Text("Island Theme");

        const char* paletteNames[] = { "Tropical", "Volcanic", "Arctic", "Desert" };

        if (ImGui::Combo("Terrain Theme", &context.imageGenerationParameters.selectedPalette, paletteNames, 4))
        {
            generateHeightmap(context); 
            UpdateTexture(context.texture, context.image.data);
        }

        ImGui::Separator();
        ImGui::Text("Radial Mask");

        if (ImGui::SliderFloat("Mask Power", &imgParams.maskPower, 0.5f, 6.0f, "%.1f"))
        {
            generateHeightmap(context);
            regenerateMeshFromImage(context);
        }
    }

    if (ImGui::CollapsingHeader("Objects", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::SliderFloat("Cube Scale", &context.cubeScale, 0.01f, 1.0f);

        auto& spawnParams = context.pointsGenerationParameters;
        
        if (ImGui::Checkbox("Allow in Water", &spawnParams.allowInWater))
        {
            generateObjectsPositions(context);
        }
        
        if (ImGui::Checkbox("Allow on Mountains", &spawnParams.allowOnMountains))
        {
            generateObjectsPositions(context);
        }
    }
}

void drawRaylibUI(AppContext &context)
{
    int screenWidth{GetScreenWidth()};

    float wanted_size{400.f};
    float scale_factor{wanted_size / std::max(context.texture.width, context.texture.height)};
    float const preview_x{screenWidth - wanted_size - 20.f};
    float const preview_y{20.f};
    float const preview_w{context.texture.width * scale_factor};
    float const preview_h{context.texture.height * scale_factor};
    // DrawTexture(context.texture, screenWidth - context.texture.width - 20, 20, WHITE);
    DrawTextureEx(context.texture, {preview_x, preview_y}, 0.0f, scale_factor, WHITE);
    DrawRectangleLines(screenWidth - wanted_size - 20, 20, wanted_size, wanted_size, GREEN);

    // draw positions on top of the heightmap
    for (auto const &pos : context.objectPositions)
    {
        // Remap normalized coordinates [0..1] to the preview image in screen space.
        float const px{preview_x + Clamp(pos.x, 0.0f, 1.0f) * preview_w};
        float const py{preview_y + Clamp(pos.y, 0.0f, 1.0f) * preview_h};

        DrawCircleV({px, py}, 2.0f, RED);
    }

    DrawFPS(10, 10);
}