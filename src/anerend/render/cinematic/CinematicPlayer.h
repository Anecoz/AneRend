#pragma once

namespace render::asset { struct Cinematic; }
namespace render::scene { class Scene; }
namespace render { class Camera; }

namespace render::cinematic {

class CinematicPlayer
{
public:
  CinematicPlayer(const asset::Cinematic* cinematic, scene::Scene* scene, Camera* camera);
  CinematicPlayer() = default;
  ~CinematicPlayer() = default;

  CinematicPlayer(const CinematicPlayer&) = delete;
  CinematicPlayer(CinematicPlayer&&);
  CinematicPlayer& operator=(const CinematicPlayer&) = delete;
  CinematicPlayer& operator=(CinematicPlayer&&);

  void update(double delta);

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

  const asset::Cinematic* _cinematic = nullptr;
  scene::Scene* _scene = nullptr;
  Camera* _camera = nullptr;
  double _currentTime = 0.0;
  bool _finished = false;
};

}