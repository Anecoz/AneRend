#pragma once

#include "IGUI.h"

#include <util/Uuid.h>
#include <util/Interpolation.h>

#include <render/asset/Cinematic.h>

#include <cstdint>
#include <unordered_map>

namespace gui {

class EditCinematicGUI : public IGUI
{
public:
  EditCinematicGUI();
  ~EditCinematicGUI();

  void immediateDraw(logic::AneditContext* c) override final;
private:
  enum SelectedKeyframeType
  {
    None,
    Camera,
    Node,
    Material
  } _selectedKFType = None;

  std::size_t _selectedIndex = 0;
  util::Uuid _selectedNode;
  util::Uuid _selectedMat;

  bool verifyNodesExist(render::asset::Cinematic& cinematic, logic::AneditContext* c);

  void fillCamKeys(render::asset::Cinematic& cinematic);
  void fillMatKeys(render::asset::Cinematic& cinematic, logic::AneditContext* c);
  void fillNodeKeys(render::asset::Cinematic& cinematic, logic::AneditContext* c);

  std::int32_t translateTimeToKey(const float& time);
  float translateKeyToTime(const std::int32_t& key);

  void addCameraKeyframePressed(render::asset::Cinematic& cinematic, logic::AneditContext* c);
  void addNodeKeyframePressed(render::asset::Cinematic& cinematic, util::Uuid node, logic::AneditContext* c);
  void addMaterialKeyframePressed(render::asset::Cinematic& cinematic, util::Uuid material, logic::AneditContext* c);

  bool nodeDroppedInList(util::Uuid node, render::asset::Cinematic& cinematic, logic::AneditContext* c);
  bool materialDroppedInList(util::Uuid material, render::asset::Cinematic& cinematic, logic::AneditContext* c);

  void playPressed(util::Uuid cinematicId, logic::AneditContext* c);
  void pausePressed(util::Uuid cinematicId, logic::AneditContext* c);
  void stopPressed(util::Uuid cinematicId, logic::AneditContext* c);

  bool drawEasingCombo(util::interp::Easing& easing);
  bool drawCameraKeyframeEditor(render::asset::CameraKeyframe& kf, logic::AneditContext* c);
  bool drawNodeKeyframeEditor(render::asset::NodeKeyframe& kf, logic::AneditContext* c);
  bool drawMatKeyframeEditor(render::asset::MaterialKeyframe& kf, logic::AneditContext* c);

  float _time = 0.0f;

  void recalculateMaxTime(render::asset::Cinematic& cinematic);

  std::vector<std::int32_t> _cachedCamKeys;

  std::unordered_map<util::Uuid, std::vector<std::int32_t>> _cachedMatKeys;
  std::unordered_map<util::Uuid, std::string> _matNames;
  std::unordered_map<util::Uuid, std::size_t> _matIndices;

  std::unordered_map<util::Uuid, std::vector<std::int32_t>> _cachedNodeKeys;
  std::unordered_map<util::Uuid, std::string> _nodeNames;
  std::unordered_map<util::Uuid, std::size_t> _nodeIndices;

  util::Uuid _cachedId;

  bool _forceSelectionClear = false;
};

}