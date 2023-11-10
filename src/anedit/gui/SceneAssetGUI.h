#pragma once

#include "IGUI.h"
#include "../render/Identifiers.h"

namespace gui {

class SceneAssetGUI : public IGUI
{
public:
  SceneAssetGUI();
  ~SceneAssetGUI();

  void immediateDraw(logic::AneditContext* c) override final;

private:
  render::MaterialId _selectedMaterial = render::INVALID_ID;
  render::ModelId _selectedModel = render::INVALID_ID;
  render::AnimationId _selectedAnimation = render::INVALID_ID;
  render::AnimatorId _selectedAnimator = render::INVALID_ID;
  render::SkeletonId _selectedSkeleton = render::INVALID_ID;
};

}