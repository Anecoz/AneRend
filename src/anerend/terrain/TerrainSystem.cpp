#include "TerrainSystem.h"

#include "../render/scene/Scene.h"
#include "../component/Components.h"
#include "../util/TangentGenerator.h"
#include "../util/TextureHelpers.h"

namespace terrain {

TerrainSystem::TerrainSystem(render::scene::Scene* scene)
  : _scene(scene)
{
  _observer.connect(_scene->registry().getEnttRegistry(), entt::collector.update<component::Terrain>());
}

void TerrainSystem::setScene(render::scene::Scene* scene)
{
  _scene = scene;
  _observer.connect(_scene->registry().getEnttRegistry(), entt::collector.update<component::Terrain>());
}

void TerrainSystem::update()
{
  for (auto entity : _observer) {
    auto uuid = _scene->registry().reverseLookup(entity);

    auto& terrainComp = _scene->registry().getComponent<component::Terrain>(uuid);
    auto& transComp = _scene->registry().getComponent<component::Transform>(uuid);

    // If no renderable, start generating a model
    if (!_scene->registry().hasComponent<component::Renderable>(uuid)) {
      if (terrainComp._heightMap) {
        generateModel(uuid);
        _scene->registry().patchComponent<component::Terrain>(uuid);
      }
    }
    else {
      // TODO: How handle raytracing material for terrain? The rchit shaders want a material index to shade their hitpoints...
      auto& rendComp = _scene->registry().getComponent<component::Renderable>(uuid);

      if (rendComp._materials.empty() && terrainComp._baseMaterials[0]) {
        rendComp._materials.emplace_back(terrainComp._baseMaterials[0]);
        _scene->registry().patchComponent<component::Renderable>(uuid);
      }
    }

    // Lock transform to tile index
    auto pos = glm::vec3(
      (float)terrainComp._tileIndex.x * (float)render::scene::Tile::_tileSize,
      0.0f,
      (float)terrainComp._tileIndex.y * (float)render::scene::Tile::_tileSize);
    transComp._localTransform = glm::translate(glm::mat4(1.0f), pos);
    _scene->registry().patchComponent<component::Transform>(uuid);
  }

  _observer.clear();
}

namespace {

float convertUint16ToFloat(std::uint16_t v, float scale = 2.0f)
{
  float zeroToOne = (float)v / (float)65535;

  return zeroToOne * scale;
}

inline glm::vec3 posFromPixel(
  const std::uint16_t* data,
  float vertStepMeters,
  unsigned w, unsigned h,
  unsigned x, unsigned y)
{
  float xPos = (float)x * vertStepMeters;
  float zPos = (float)y * vertStepMeters;

  auto val = data[y * w + x];
  float yPos = convertUint16ToFloat(val);

  return {xPos, yPos, zPos};
}

inline glm::vec3 calcNormal(
  const glm::vec3& p0,
  const glm::vec3& p1,
  const glm::vec3& p2)
{
  return glm::normalize(glm::cross(p1 - p0, p2 - p0));
}

inline glm::vec3 calcAvgNormal(
  const std::uint16_t* data,
  float vertStepMeters,
  unsigned w, unsigned h,
  unsigned x, unsigned y,
  const glm::vec3& posAtXY)
{
  // Sample a bunch of gradients around (x,y) and average them
  // Check for edge cases, first case is we're somewhere in the middle and the most common case
  if (x > 0 && x < w - 1 &&
      y > 0 && y < h - 1) {

    auto p00 = posFromPixel(data, vertStepMeters, w, h, x - 1, y - 1);
    auto p10 = posFromPixel(data, vertStepMeters, w, h, x - 0, y - 1);
    //auto p20 = posFromPixel(data, vertStepMeters, w, h, x + 1, y - 1);
    auto p01 = posFromPixel(data, vertStepMeters, w, h, x - 1, y - 0);
    auto p21 = posFromPixel(data, vertStepMeters, w, h, x + 1, y - 0);
    //auto p02 = posFromPixel(data, vertStepMeters, w, h, x - 1, y + 1);
    auto p12 = posFromPixel(data, vertStepMeters, w, h, x - 0, y + 1);
    auto p22 = posFromPixel(data, vertStepMeters, w, h, x + 1, y + 1);

    glm::vec3 normal;
    normal += (calcNormal(posAtXY, p00, p01));
    normal += (calcNormal(posAtXY, p10, p00));
    normal += (calcNormal(posAtXY, p21, p10));
    normal += (calcNormal(posAtXY, p22, p21));
    normal += (calcNormal(posAtXY, p12, p22));
    normal += (calcNormal(posAtXY, p01, p12));

    normal /= 6.0f;
    return glm::normalize(normal);
  }

  // TODO: Fix
  return { 0.0f, 1.0f, 0.0f };
}

}

void TerrainSystem::generateModel(util::Uuid& node)
{
  auto& terrainComp = _scene->registry().getComponent<component::Terrain>(node);
  auto* tex = _scene->getTexture(terrainComp._heightMap);

  // TODO: Do R8 version
  if (tex->_format != render::asset::Texture::Format::R16_UNORM) {
    printf("Unsupported height map format!\n");
    return;
  }

  if (tex->_width != tex->_height) {
    printf("Only support square heightmaps!\n");
    return;
  }

  unsigned gridWidthMeters = render::scene::Tile::_tileSize;
  unsigned gridHeightMeters = render::scene::Tile::_tileSize;

  unsigned w = tex->_width;
  unsigned h = tex->_height;
  unsigned vertScale = w / gridWidthMeters;
  float vertStepMeters = 1.0f / (float)vertScale;
  auto* p = reinterpret_cast<const std::uint16_t*>(tex->_data[0].data());

  // Generate verts
  render::asset::Model model{};
  model._name = "TerrainModel";

  render::asset::Mesh mesh{};
  mesh._minPos = glm::vec3(0.0f, 0.0f, 0.0f);
  mesh._maxPos = glm::vec3((float)gridWidthMeters, 0.0f, (float)gridWidthMeters);

  // Convert each pixel in height map to a vertex
  mesh._vertices.resize(w * h);
  for (unsigned x = 0; x < w; ++x) {
    for (unsigned y = 0; y < h; ++y) {
      render::Vertex vert{};
      vert.jointIds = { -1, -1, -1, -1 };

      vert.pos = posFromPixel(p, vertStepMeters, w, h, x, y);
      vert.normal = calcAvgNormal(p, vertStepMeters, w, h, x, y, vert.pos);
      vert.uv = { vert.pos.x, vert.pos.z }; // Sampler needs to be repeating

      if (vert.pos.y > mesh._maxPos.y) {
        mesh._maxPos.y = vert.pos.y;
      }

      mesh._vertices[y * w + x] = std::move(vert);
    }
  }

  // Create indices to create the triangles
  mesh._indices.reserve((w - 1) * (w - 1) * 2 * 3); // Assume square...
  for (unsigned x = 0; x < w - 1; ++x) {
    for (unsigned y = 0; y < h - 1; ++y) {
      // Each point corresponds to a square consisting of two triangles.

      // Triangle 1.
      {
        mesh._indices.emplace_back((y + 0) * w + (x + 0));
        mesh._indices.emplace_back((y + 1) * w + (x + 0));
        mesh._indices.emplace_back((y + 1) * w + (x + 1));
      }

      // Triangle 2.
      {
        mesh._indices.emplace_back((y + 0) * w + (x + 1));
        mesh._indices.emplace_back((y + 0) * w + (x + 0));
        mesh._indices.emplace_back((y + 1) * w + (x + 1));
      }

    }
  }

  printf("Generated heightmap. %zu verts and %zu indices\n", mesh._vertices.size(), mesh._indices.size());

  model._meshes.emplace_back(std::move(mesh));
  util::TangentGenerator::generateTangents(model._meshes.back());

  auto modelId = _scene->addModel(std::move(model));

  // Add renderable component
  auto& rendComp = _scene->registry().addComponent<component::Renderable>(node);
  rendComp._model = modelId;
  rendComp._boundingSphere = glm::vec4(0.0f, 0.0f, 0.0f, (float)gridWidthMeters);
  rendComp._visible = true;
  rendComp._name = "Terrain";
  _scene->registry().patchComponent<component::Renderable>(node);

  // Generate a default blend mask
  auto blendTex = util::TextureHelpers::createTextureRGBA8(w, h, glm::u8vec4(255, 0, 0, 0));
  blendTex._name = "TerrainBlendTex";
  
  auto blendId = _scene->addTexture(std::move(blendTex));
  terrainComp._blendMap = blendId;
}

}