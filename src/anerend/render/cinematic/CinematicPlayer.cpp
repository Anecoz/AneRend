#include "CinematicPlayer.h"

#include "../asset/Cinematic.h"
#include "../Camera.h"
#include "../scene/Scene.h"
#include "../../util/Interpolation.h"

namespace render::cinematic {

namespace {

std::tuple<const asset::Cinematic::Keyframe*, const asset::Cinematic::Keyframe*, double> findClosestKfs(const render::asset::Cinematic* cinematic, double time)
{
  auto sz = cinematic->_keyframes.size();
  const asset::Cinematic::Keyframe* kf0 = nullptr;
  const asset::Cinematic::Keyframe* kf1 = nullptr;

  bool found = false;
  for (std::size_t i = 0; i < sz; ++i) {
    if (cinematic->_keyframes[i]._time > time) {
      kf0 = &cinematic->_keyframes[i - 1];
      kf1 = &cinematic->_keyframes[i];
      found = true;
      break;
    }
  }

  if (!found) {
    kf0 = &cinematic->_keyframes[sz - 2];
    kf1 = &cinematic->_keyframes.back();
  }

  double factor = (time - kf0->_time) / (kf1->_time - kf0->_time);

  return { kf0, kf1, factor };
}

render::asset::CameraKeyframe lerp(const render::asset::CameraKeyframe& kf0, const render::asset::CameraKeyframe& kf1, double factor)
{
  //auto view = util::interp::lerpTransforms(kf0._view, kf1._view, factor, util::interp::Easing::InOutSine);

  render::asset::CameraKeyframe out;
  double easingFactor = util::interp::easeInOutSine(factor);

  // Position
  float y = (1.0f - float(factor)) * kf0._pos.y + (float)factor * kf1._pos.y;
  out._pos = (1.0f - float(easingFactor)) * kf0._pos + (float)easingFactor * kf1._pos;
  out._pos.y = y;

  // Quat
  out._orientation = glm::slerp(kf0._orientation, kf1._orientation, (float)easingFactor);
  //out._orientation = kf0._orientation;

  return out;
}

}

CinematicPlayer::CinematicPlayer(const asset::Cinematic* cinematic, scene::Scene* scene, Camera* camera)
  : _cinematic(cinematic)
  , _scene(scene)
  , _camera(camera)
{}

CinematicPlayer::CinematicPlayer(CinematicPlayer&& rhs)
{
  std::swap(_cinematic, rhs._cinematic);
  std::swap(_scene, rhs._scene);
  std::swap(_camera, rhs._camera);
  std::swap(_state, rhs._state);
  std::swap(_currentTime, rhs._currentTime);
  std::swap(_finished, rhs._finished);
}

CinematicPlayer& CinematicPlayer::operator=(CinematicPlayer&& rhs)
{
  if (this != &rhs) {
    std::swap(_cinematic, rhs._cinematic);
    std::swap(_scene, rhs._scene);
    std::swap(_camera, rhs._camera);
    std::swap(_state, rhs._state);
    std::swap(_currentTime, rhs._currentTime);
    std::swap(_finished, rhs._finished);
  }

  return *this;
}

void CinematicPlayer::update(double delta)
{
  if (_state != State::Playing) {
    return;
  }

  if (_cinematic->_keyframes.size() <= 1) {
    printf("Only one keyframe in cinematic, won't play\n");
    _finished = true;
    return;
  }

  // Go through keyframes and find closest one to current time
  auto [kf0, kf1, factor] = findClosestKfs(_cinematic, _currentTime);

  // TODO: Find 2 keyframes so that we can lerp between them

  if (!kf0 || !kf1) {
    printf("No kf for cinematic found!\n");
    return;
  }

  if (kf0->_camKF && kf1->_camKF) {
    // TODO: Use params instead and construct proj matrix here
    auto lerped = lerp(kf0->_camKF.value(), kf1->_camKF.value(), factor);

    _camera->setPosition(lerped._pos);
    _camera->setYawPitchRoll(kf1->_camKF.value()._ypr);
    _camera->setRotationMatrix(glm::toMat4(lerped._orientation));
  }

#if 0
  // For now only transforms of nodes, and no lerp
  for (auto& nodeKF : kf->_nodeKFs) {
    auto& tComp = _scene->registry().getComponent<component::Transform>(nodeKF._id);
    tComp = nodeKF._comps._trans;
    _scene->registry().patchComponent<component::Transform>(nodeKF._id);
  }

  for (auto& animKF : kf->_animatorKFs) {
    auto animator = animKF._animator;
    animator._id = animKF._id;
    _scene->updateAnimator(std::move(animator));
  }

  for (auto& matKF : kf->_materialKFs) {
    auto mat = *_scene->getMaterial(matKF._id);
    mat._emissive = matKF._emissive;
    mat._baseColFactor = matKF._baseColFactor;
    _scene->updateMaterial(std::move(mat));
  }
#endif

  _currentTime += delta;

  // TODO: What do? Just done?
  auto maxTime = _cinematic->_keyframes.back()._time;
  if (_currentTime >= maxTime) {
    _finished = true;
    stop();
  }
}

void CinematicPlayer::play()
{
  _state = State::Playing;
}

void CinematicPlayer::pause()
{
  _state = State::Paused;
}

void CinematicPlayer::stop()
{
  _currentTime = 0.0;
  _state = State::Stopped;
}

bool CinematicPlayer::finished() const
{
  return _finished;
}

}