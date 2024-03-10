#include "EditTerrainGUI.h"

#include "../../logic/AneditContext.h"
#include "../../tool/ImageManipulator.h"

#include <render/scene/Scene.h>
#include <render/asset/AssetCollection.h>

#include <glm/glm.hpp>
#include <imgui.h>

namespace gui {

EditTerrainGUI::EditTerrainGUI()
  : IGUI()
{}

EditTerrainGUI::~EditTerrainGUI()
{}

void EditTerrainGUI::immediateDraw(logic::AneditContext* c)
{
  auto id = c->getFirstSelection();

  if (!id) {
    return;
  }

  bool changed = false;
  auto& terrainComp = c->scene().registry().getComponent<component::Terrain>(id);

  std::string baseMat0 = "Mat0: ";
  std::string baseMat1 = "Mat1: ";
  std::string baseMat2 = "Mat2: ";
  std::string baseMat3 = "Mat3: ";

  if (terrainComp._baseMaterials[0]) {
    baseMat0 += c->assetCollection().getMaterialBlocking(terrainComp._baseMaterials[0])._name;
  }
  if (terrainComp._baseMaterials[1]) {
    baseMat1 += c->assetCollection().getMaterialBlocking(terrainComp._baseMaterials[1])._name;
  }
  if (terrainComp._baseMaterials[2]) {
    baseMat2 += c->assetCollection().getMaterialBlocking(terrainComp._baseMaterials[2])._name;
  }
  if (terrainComp._baseMaterials[3]) {
    baseMat3 += c->assetCollection().getMaterialBlocking(terrainComp._baseMaterials[3])._name;
  }
#if 0
  if (auto* mat = c->scene().getMaterial(terrainComp._baseMaterials[0])) {
    baseMat0 += mat->_name;
  }
  if (auto* mat = c->scene().getMaterial(terrainComp._baseMaterials[1])) {
    baseMat1 += mat->_name;
  }
  if (auto* mat = c->scene().getMaterial(terrainComp._baseMaterials[2])) {
    baseMat2 += mat->_name;
  }
  if (auto* mat = c->scene().getMaterial(terrainComp._baseMaterials[3])) {
    baseMat3 += mat->_name;
  }
#endif

  ImGui::Text("Tile index");
  if (ImGui::InputInt2("##tileindex", (int*)&terrainComp._tileIndex)) {
    changed = true;
  }

  ImGui::Separator();
  // Heightmap
  {
    ImGui::Text("Heightmap");

    if (ImGui::BeginDragDropTarget()) {
      if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("texture_id")) {
        std::array<std::uint8_t, 16> arr{};
        std::memcpy(arr.data(), payload->Data, sizeof(util::Uuid));

        terrainComp._heightMap = util::Uuid(arr);
        changed = true;
      }

      ImGui::EndDragDropTarget();
    }

    if (terrainComp._heightMap) {
      float texFactor = 0.75f;
      auto maxRegion = ImGui::GetWindowContentRegionMax();
      ImVec2 texSize{ texFactor * maxRegion.x , texFactor * maxRegion.x };
      auto* texId = c->getImguiTexId(terrainComp._heightMap);
      if (!texId) {
        // Request it so that it's hopefully in cache next time.
        c->assetCollection().getTexture(terrainComp._heightMap, [](render::asset::Texture) {});
      }
      else {
        ImGui::Image((ImTextureID)c->getImguiTexId(terrainComp._heightMap), texSize);
      }
    }

    if (ImGui::InputFloat("MPP", &terrainComp._mpp)) {
      changed = true;
    }
  }

  ImGui::Separator();
  // Base materials
  {
    ImGui::Text("Base materials");

    ImGui::Text(baseMat0.c_str());
    if (ImGui::BeginDragDropTarget()) {
      if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("material_id")) {
        std::array<std::uint8_t, 16> arr{};
        std::memcpy(arr.data(), payload->Data, sizeof(util::Uuid));

        terrainComp._baseMaterials[0] = util::Uuid(arr);
        changed = true;
      }

      ImGui::EndDragDropTarget();
    }

    ImGui::Text(baseMat1.c_str());
    if (ImGui::BeginDragDropTarget()) {
      if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("material_id")) {
        std::array<std::uint8_t, 16> arr{};
        std::memcpy(arr.data(), payload->Data, sizeof(util::Uuid));

        terrainComp._baseMaterials[1] = util::Uuid(arr);
        changed = true;
      }

      ImGui::EndDragDropTarget();
    }

    ImGui::Text(baseMat2.c_str());
    if (ImGui::BeginDragDropTarget()) {
      if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("material_id")) {
        std::array<std::uint8_t, 16> arr{};
        std::memcpy(arr.data(), payload->Data, sizeof(util::Uuid));

        terrainComp._baseMaterials[2] = util::Uuid(arr);
        changed = true;
      }

      ImGui::EndDragDropTarget();
    }

    ImGui::Text(baseMat3.c_str());
    if (ImGui::BeginDragDropTarget()) {
      if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("material_id")) {
        std::array<std::uint8_t, 16> arr{};
        std::memcpy(arr.data(), payload->Data, sizeof(util::Uuid));

        terrainComp._baseMaterials[3] = util::Uuid(arr);
        changed = true;
      }

      ImGui::EndDragDropTarget();
    }
  }

  ImGui::Separator();
  // Blendmap
  {
    ImGui::Text("Blendmap");

    if (ImGui::BeginDragDropTarget()) {
      if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("texture_id")) {
        std::array<std::uint8_t, 16> arr{};
        std::memcpy(arr.data(), payload->Data, sizeof(util::Uuid));

        terrainComp._blendMap = util::Uuid(arr);
        changed = true;
      }

      ImGui::EndDragDropTarget();
    }

    if (terrainComp._blendMap) {
      float texFactor = 0.75f;
      auto maxRegion = ImGui::GetWindowContentRegionMax();
      ImVec2 texSize{ texFactor * maxRegion.x , texFactor * maxRegion.x };
      ImGui::Image((ImTextureID)c->getImguiTexId(terrainComp._blendMap), texSize);
    }
  }

  ImGui::Separator();
  // Vegmap
  {
    ImGui::Text("Vegetationmap");

    if (ImGui::BeginDragDropTarget()) {
      if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("texture_id")) {
        std::array<std::uint8_t, 16> arr{};
        std::memcpy(arr.data(), payload->Data, sizeof(util::Uuid));

        terrainComp._vegetationMap = util::Uuid(arr);
        changed = true;
      }

      ImGui::EndDragDropTarget();
    }

    if (terrainComp._vegetationMap) {
      float texFactor = 0.75f;
      auto maxRegion = ImGui::GetWindowContentRegionMax();
      ImVec2 texSize{ texFactor * maxRegion.x , texFactor * maxRegion.x };
      ImGui::Image((ImTextureID)c->getImguiTexId(terrainComp._vegetationMap), texSize);
    }
  }

  if (changed) {
    c->scene().registry().patchComponent<component::Terrain>(id);
  }

  // Do painting
  ImGui::Separator();
  ImGui::Checkbox("Paint material", &_paintingMat);
  ImGui::Checkbox("Paint vegetation", &_paintingVeg);
  ImGui::Checkbox("Erase", &_eraser);
  ImGui::InputFloat("Opacity", &_paintOpacity);
  ImGui::InputInt("Paint idx", &_paintMatIndex);
  ImGui::InputScalar("Brush size", ImGuiDataType_U32, &_brush._radius);
  ImGui::InputScalar("Brush falloff", ImGuiDataType_U32, &_brush._falloff);

  bool overViewport = !ImGui::GetIO().WantCaptureMouse;

  if (_paintingMat && ImGui::IsMouseDown(ImGuiMouseButton_Left) && overViewport) {
    // Find if we're currently over the selected terrain
    auto worldPos = c->latestWorldPosition();
    auto ts = render::scene::Tile::_tileSize;
    auto ti = terrainComp._tileIndex;

    int wTiX = (int)worldPos.x / (int)ts;
    int wTiZ = (int)worldPos.z / (int)ts;

    if (wTiX != ti.x || wTiZ != ti.y) {
      return;
    }

    // Calc normalized tile coordinate
    float u = (worldPos.x - (float)ts * (float)ti.x) / (float)ts;
    float v = (worldPos.z - (float)ts * (float)ti.y) / (float)ts;

    //auto blendTex = *c->scene().getTexture(terrainComp._blendMap);
    auto blendTex = c->assetCollection().getTextureBlocking(terrainComp._blendMap);

    glm::u8vec4 val = { 0, 0, 0, 0 };
    val[_paintMatIndex] = _eraser ? 0 : (uint8_t)(_paintOpacity * 255.0f);

    tool::ImageManipulator::PaintMask mask{ false, false, false, false };
    if (_paintMatIndex == 0) {
      mask._r = true;
    }
    else if (_paintMatIndex == 1) {
      mask._g = true;
    }
    else if (_paintMatIndex == 2) {
      mask._b = true;
    }
    else if (_paintMatIndex == 3) {
      mask._a = true;
    }

    tool::ImageManipulator imip(_brush);
    //imip.paint8Bit(blendTex, (unsigned)(blendTex._width * u), (unsigned)(blendTex._height * v), val, std::move(mask), !_eraser);
    imip.paintRGBA8(blendTex, (int)(blendTex._width* u), (int)(blendTex._height* v), [idx = _paintMatIndex](glm::u8vec4* val, float falloff) {

      auto old = (*val)[idx];
      auto newVal = (std::uint8_t)(255 * falloff);
      if (old < newVal) {
        (*val)[idx] = newVal;

        if (falloff > 0.999f) {
          // Set all other to 0
          for (int i = 0; i < 4; ++i) {
            if (i != idx) {
              (*val)[i] = 0;
            }
          }
        }
        else {
          // Fade
          float remaining = 1.0f - falloff;
          // Prepass to determine how many channels can share 1.0 - falloff
          int count = 0;
          for (int i = 0; i < 4; ++i) {
            if (i != idx) {
              auto tmp = (*val)[i];
              if (tmp > 0) {
                count++;
              }
            }
          }

          remaining = remaining / (float)count;
          for (int i = 0; i < 4; ++i) {
            if (i != idx) {
              auto tmp = (*val)[i];
              if (tmp > 0) {
                (*val)[i] = (std::uint8_t)(255 * remaining);
              }
            }
          }
        }

      }
    });

    //c->scene().updateTexture(std::move(blendTex));
    c->assetCollection().updateTexture(std::move(blendTex));
  }

  if (_paintingVeg && ImGui::IsMouseDown(ImGuiMouseButton_Left) && overViewport) {
    // Find if we're currently over the selected terrain
    auto worldPos = c->latestWorldPosition();
    auto ts = render::scene::Tile::_tileSize;
    auto ti = terrainComp._tileIndex;

    int wTiX = (int)worldPos.x / (int)ts;
    int wTiZ = (int)worldPos.z / (int)ts;

    if (wTiX != ti.x || wTiZ != ti.y) {
      return;
    }

    // Calc normalized tile coordinate
    float u = (worldPos.x - (float)ts * (float)ti.x) / (float)ts;
    float v = (worldPos.z - (float)ts * (float)ti.y) / (float)ts;

    //auto vegTex = *c->scene().getTexture(terrainComp._vegetationMap);
    auto vegTex = c->assetCollection().getTextureBlocking(terrainComp._vegetationMap);

    tool::ImageManipulator imip(_brush);
    imip.paintR8(vegTex, (int)(vegTex._width* u), (int)(vegTex._height* v),
      [e = _eraser](uint8_t* val, float falloff) {
        *val = e ? 0: 255;
      });

    //c->scene().updateTexture(std::move(vegTex));
    c->assetCollection().updateTexture(std::move(vegTex));
  }
}

}