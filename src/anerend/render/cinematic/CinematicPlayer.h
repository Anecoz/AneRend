#pragma once

#include "../asset/Cinematic.h"

namespace render::scene { class Scene; }
namespace render::asset { class AssetCollection; }
namespace render { class Camera; }

namespace render::cinematic {

class CinematicPlayer
{
public:
  CinematicPlayer(asset::Cinematic cinematic, asset::AssetCollection* assColl, scene::Scene* scene, Camera* camera);
  CinematicPlayer() = default;
  ~CinematicPlayer() = default;

  CinematicPlayer(const CinematicPlayer&) = delete;
  CinematicPlayer(CinematicPlayer&&);
  CinematicPlayer& operator=(const CinematicPlayer&) = delete;
  CinematicPlayer& operator=(CinematicPlayer&&);

  void update(double delta);

  void setCurrentTime(double time) { _currentTime = time; _dirty = true; }
  double getCurrentTime() const { return _currentTime; }

  void play();
  void pause();
  void stop();
  bool finished() const;

private:
  enum class State {
    Playing,
    Paused,
    Stopped
  } _state = State::Stopped;

  asset::Cinematic _cinematic;
  scene::Scene* _scene = nullptr;
  asset::AssetCollection* _assColl = nullptr;
  Camera* _camera = nullptr;
  double _currentTime = 0.0;
  bool _finished = false;
  bool _dirty = false;
};

}