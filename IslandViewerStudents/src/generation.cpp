#include "generation.hpp"

#include "noise.hpp"
#include "raylib.h"

#include "utils/raylibUtils.hpp"
#include <algorithm>
#include <cmath>

std::vector<glm::vec2> generate2DPositions(PointsGenerationParameters const &params, AppContext const &context)
{
    float const r{params.minDistance};
    int const k{params.maxAttempts};

    float const cellSize{r / std::sqrt(2.0f)};
    int const gridW{static_cast<int>(std::ceil(1.0f / cellSize))};
    int const gridH{static_cast<int>(std::ceil(1.0f / cellSize))};
    std::vector<int> grid(gridW * gridH, -1);

    auto gridCoord = [&](glm::vec2 const &p) -> glm::ivec2
    {
        return {static_cast<int>(p.x / cellSize), static_cast<int>(p.y / cellSize)};
    };
    auto gridIndex = [&](int gx, int gy) -> int
    {
        return gy * gridW + gx;
    };

    auto isFarEnough = [&](glm::vec2 const &candidate, std::vector<glm::vec2> const &positions) -> bool
    {
        glm::ivec2 const gc{gridCoord(candidate)};
        for (int dy{-2}; dy <= 2; ++dy)
            for (int dx{-2}; dx <= 2; ++dx)
            {
                int const nx{gc.x + dx}, ny{gc.y + dy};
                if (nx < 0 || nx >= gridW || ny < 0 || ny >= gridH)
                    continue;
                int const idx{grid[gridIndex(nx, ny)]};
                if (idx == -1)
                    continue;
                if (glm::distance(candidate, positions[idx]) < r)
                    return false;
            }
        return true;
    };

    auto isOnIsland = [&](glm::vec2 const &p) -> bool
    {
        return sampleHeightmap(context, p.x, p.y) > context.imageGenerationParameters.waterLevel;
    };

    std::vector<glm::vec2> positions{};
    std::vector<int> activeList{};

    {
        glm::vec2 startPoint{};
        bool found{false};
        for (int attempt{0}; attempt < 10000 && !found; ++attempt)
        {
            startPoint = {
                static_cast<float>(GetRandomValue(0, INT_MAX)) / static_cast<float>(INT_MAX),
                static_cast<float>(GetRandomValue(0, INT_MAX)) / static_cast<float>(INT_MAX)};
            if (isOnIsland(startPoint))
                found = true;
        }
        if (!found)
            return positions;

        positions.push_back(startPoint);
        activeList.push_back(0);
        glm::ivec2 const gc{gridCoord(startPoint)};
        grid[gridIndex(gc.x, gc.y)] = 0;
    }

    while (!activeList.empty())
    {
        int const listIdx{GetRandomValue(0, static_cast<int>(activeList.size()) - 1)};
        glm::vec2 const base{positions[activeList[listIdx]]};
        bool foundCandidate{false};

        for (int attempt{0}; attempt < k; ++attempt)
        {
            float const angle{
                static_cast<float>(GetRandomValue(0, INT_MAX)) / static_cast<float>(INT_MAX) * 2.0f * static_cast<float>(M_PI)};
            float const radius{
                r + static_cast<float>(GetRandomValue(0, INT_MAX)) / static_cast<float>(INT_MAX) * r};

            glm::vec2 const candidate{
                base.x + radius * std::cos(angle),
                base.y + radius * std::sin(angle)};

            if (candidate.x < 0.0f || candidate.x > 1.0f ||
                candidate.y < 0.0f || candidate.y > 1.0f)
                continue;
            if (!isOnIsland(candidate))
                continue;
            if (!isFarEnough(candidate, positions))
                continue;

            int const newIdx{static_cast<int>(positions.size())};
            positions.emplace_back(candidate);
            activeList.emplace_back(newIdx);
            glm::ivec2 const gc{gridCoord(candidate)};
            grid[gridIndex(gc.x, gc.y)] = newIdx;
            foundCandidate = true;
            break;
        }

        if (!foundCandidate)
            activeList.erase(activeList.begin() + listIdx);
    }

    return positions;
}

void generateObjectsPositions(AppContext &context)
{
    std::vector<glm::vec2> const positions{generate2DPositions(context.pointsGenerationParameters, context)};

    context.objectPositions.clear();
    context.objectPositions.reserve(positions.size());
    for (glm::vec2 const &p : positions)
    {
        context.objectPositions.emplace_back(
            p.x,
            p.y,
            sampleHeightmap(context, p.x, p.y));
    }
}

float sampleHeightmap(AppContext const &context, float u, float v)
{
    if (!context.heightmapImage.data || context.heightmapImage.width <= 0 || context.heightmapImage.height <= 0)
        return 0.0f;

    int const px = std::clamp(static_cast<int>(u * static_cast<float>(context.heightmapImage.width - 1)), 0, context.heightmapImage.width - 1);
    int const py = std::clamp(static_cast<int>(v * static_cast<float>(context.heightmapImage.height - 1)), 0, context.heightmapImage.height - 1);

    // If the heightmap is in R32 format, we can directly read the height value as a float.
    if (context.heightmapImage.format == PIXELFORMAT_UNCOMPRESSED_R32)
    {
        float const *heightData = static_cast<float const *>(context.heightmapImage.data);
        int const idx = py * context.heightmapImage.width + px;
        return std::clamp(heightData[idx], 0.0f, 1.0f);
    }

    // Otherwise, we assume it's in a color format and we read the red channel as height (with normalization from [0..255] to [0..1]).
    Color const c = GetImageColor(context.heightmapImage, px, py);
    return static_cast<float>(c.r) / 255.0f;
}

extern int g_numOctaves;
extern float g_lacunarity;
extern float g_gain;

void generateHeightmap(AppContext &context)
{
    if (context.texture.id > 0)
    {
        UnloadTexture(context.texture);
        context.texture = {};
    }

    if (context.image.data)
    {
        UnloadImage(context.image);
        context.image = {};
    }

    if (context.heightmapImage.data)
    {
        UnloadImage(context.heightmapImage);
        context.heightmapImage = {};
    }

    g_numOctaves = context.imageGenerationParameters.octaves;
    g_lacunarity = context.imageGenerationParameters.lacunarity;
    g_gain = context.imageGenerationParameters.gain;

    int const resolution = std::max(1, context.imageGenerationParameters.resolution);

    context.heightmapImage = GenImageFromNoiseFunction<float>(resolution, resolution, PIXELFORMAT_UNCOMPRESSED_R32,
                                                              [&](glm::vec2 const &p) -> float
                                                              {
                                                                  float noiseValue = octaveNoise(p * context.imageGenerationParameters.noiseScale,
                                                                                                 [&](glm::vec2 const &p) -> float
                                                                                                 {
                                                                                                     return perlinNoiseSeeded(p, context.imageGenerationParameters.noiseSeed);
                                                                                                 }) *
                                                                                         0.5f +
                                                                                     0.5f;

                                                                  glm::vec2 center(0.5f, 0.5f);
                                                                  float distanceToCenter = glm::distance(p, center);
                                                                  float normalizedDistance = distanceToCenter / 0.5f;
                                                                  float mask = 1.0f - std::pow(std::clamp(normalizedDistance, 0.0f, 1.0f), context.imageGenerationParameters.maskPower);
                                                                  float finalHeight = noiseValue * mask;

                                                                  if (finalHeight < context.imageGenerationParameters.waterLevel)
                                                                  {
                                                                      return context.imageGenerationParameters.waterLevel;
                                                                  }

                                                                  return finalHeight;
                                                              });

    // exemple conversion from heightmap to color image
    context.image = TransformImage<float, Color>(context.heightmapImage, [&](float const &v, int const, int const)
                                                 {
                                                     glm::vec3 water = {70.0f, 130.0f, 180.0f};
                                                     glm::vec3 sand  = {238.0f, 214.0f, 175.0f};
                                                     glm::vec3 grass = {34.0f, 139.0f, 34.0f};
                                                     glm::vec3 lightRock = {180.0f, 180.0f, 180.0f};

                                                     if (v < 0.28f)
                                                     {
                                                         return color_from({70, 130, 180});
                                                     }
                                                     else if (v < 0.30f)
                                                     {
                                                         float t = (v - 0.28f) / (0.30f - 0.28f); // water to sand
                                                         glm::vec3 mixed = glm::mix(water, sand, t);
                                                         return color_from({ (unsigned char)mixed.x, (unsigned char)mixed.y, (unsigned char)mixed.z });
                                                     }
                                                     else if (v < 0.55f)
                                                     {
                                                         float t = (v - 0.30f) / (0.55f - 0.30f); // sand to grass
                                                         glm::vec3 mixed = glm::mix(sand, grass, t);
                                                         return color_from({ (unsigned char)mixed.x, (unsigned char)mixed.y, (unsigned char)mixed.z });
                                                     }
                                                     else 
                                                     {
                                                        float t = std::clamp((v - 0.55f) / (0.70f - 0.55f), 0.0f, 1.0f); // grass to rock
                                                        glm::vec3 mixed = glm::mix(grass, lightRock, t);
                                                        return color_from({ (unsigned char)mixed.x, (unsigned char)mixed.y, (unsigned char)mixed.z });
                                                     } }, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);

    context.texture = LoadTextureFromImage(context.image);
    if (context.model.meshCount > 0)
    {
        context.model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = context.texture;
    }
}