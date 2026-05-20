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
        Matrix const objectTranslation{MatrixTranslate(
            pos.x * context.terrainSize.x,
            pos.z * context.terrainSize.y + cubeHalfHeight,
            pos.y * context.terrainSize.z)};
        Matrix const centeredTranslation{MatrixMultiply(objectTranslation, terrainCentering)};
        Matrix const scale{MatrixScale(context.cubeScale, context.cubeScale, context.cubeScale)};
        Matrix const transform{MatrixMultiply(scale, centeredTranslation)};
        DrawMesh(context.cube, context.cubeMaterial, transform);
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
        ImGui::Text("Radial Mask");

        if (ImGui::SliderFloat("Mask power", &imgParams.maskPower, 0.5f, 6.0f, "%.1f"))
        {
            generateHeightmap(context);
            regenerateMeshFromImage(context);
        }
    }

    if (ImGui::CollapsingHeader("Objects", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::SliderFloat("Cube Scale", &context.cubeScale, 0.01f, 1.0f);
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