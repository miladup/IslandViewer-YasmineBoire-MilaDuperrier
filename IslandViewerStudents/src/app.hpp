#pragma once

#include "raylib.h"
#include "glm/glm.hpp"
#include <vector>

struct ImageGenerationParameters
{
    int noiseSeed{0};
    float noiseScale{4.0f};
    int resolution{256};

    // fbm
    int octaves{5};
    float lacunarity{2.0f};
    float gain{0.5f};

    // masque radial
    float maskPower{2.0f};
    float waterLevel{0.28f};
    float sandLevel{0.30f};

    // palettes de couleurs
    int selectedPalette{0};
};

struct PointsGenerationParameters
{
    float minDistance{0.05f};
    int maxAttempts{30};

    bool allowInWater{false};
    bool allowOnMountains{false};
};

struct IslandPalette {
    const char* name;
    glm::vec3 deepOcean;
    glm::vec3 coastWater;
    glm::vec3 water;
    glm::vec3 sand;
    glm::vec3 grass;
    glm::vec3 lightRock;
};

struct AppContext
{
    Camera camera{};

    // Store the heightmap as a raylib Image, which is easy to sample from CPU side when generating object positions.
    Image heightmapImage{};

    // This is the image we use for texturing the terrain. It can be the same as heightmapImage, but it doesn't have to be (for example, you could use a color image with RGB channels representing different material types instead of height).
    Image image{};

    // The generated texture from the image, stored here so we can easily bind it when generating the model.
    Texture2D texture{};

    glm::vec3 terrainSize{16.0f, 5.0f, 16.0f};

    // The generated terrain mesh and model.
    Mesh mesh{};
    Model model{};

    std::vector<glm::vec3> objectPositions{};

    // A simple cube mesh and material we use to draw objects on the terrain.
    Mesh cube{};
    Material cubeMaterial{};
    float cubeScale{0.1f};
    // float noiseScale{4.0f};

    // Parameters for object positions generation
    PointsGenerationParameters pointsGenerationParameters;

    // Parameters for island generation
    ImageGenerationParameters imageGenerationParameters;

    Model biomeModels[4];
    Model waterModels[4];
    Model mountainModels[4];
    bool modelsLoaded{false};
};

Matrix getTerrainCenteringMatrix(AppContext const &context);
float sampleHeightmap(AppContext const &context, float u, float v);
void unload(AppContext &context);
void regenerateMeshFromImage(AppContext &context);