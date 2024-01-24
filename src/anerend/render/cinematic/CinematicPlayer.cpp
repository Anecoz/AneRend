#include "CinematicPlayer.h"

#include "../asset/Cinematic.h"
#include "../Camera.h"
#include "../scene/Scene.h"
#include "../../util/Interpolation.h"

namespace render::cinematic {

namespace {

template <typename T>
std::tuple<const T*, const T*, double> findClosestKfs(const std::vector<T>& vec, double time)
{
  auto sz = vec.size();

  if (sz == 0) {
    return { nullptr, nullptr, 0.0 };
  }

  const T* kf0 = nullptr;
  const T* kf1 = nullptr;

  if (sz == 1) {
    kf0 = &vec[0];

    return { kf0, kf1, 0.0 };
  }

  bool found = false;
  for (std::size_t i = 0; i < sz; ++i) {
    if (vec[i]._time > time) {
      kf0 = &vec[i - 1];
      kf1 = &vec[i];
      found = true;
      break;
    }
  }

  if (!found) {
    kf0 = &vec[sz - 2];
    kf1 = &vec.back();
  }

  double factor = (time - kf0->_time) / (kf1->_time - kf0->_time);

  return { kf0, kf1, factor };
}

render::asset::CameraKeyframe lerp(const render::asset::CameraKeyframe& kf0, const render::asset::CameraKeyframe& kf1, double factor)
{
  render::asset::CameraKeyframe out;
  double easingFactor = util::interp::ease(factor, kf0._easing);

  // Position
  out._pos = (1.0f - float(easingFactor)) * kf0._pos + (float)easingFactor * kf1._pos;

  // Quat
  out._orientation = glm::slerp(kf0._orientation, kf1._orientation, (float)easingFactor);

  return out;
}

render::asset::NodeKeyframe lerp(const render::asset::NodeKeyframe& kf0, const render::asset::NodeKeyframe& kf1, double factor)
{
  render::asset::NodeKeyframe out;

  // Transform comp for now
  out._comps._trans._localTransform = util::interp::lerpTransforms(
    kf0._comps._trans._localTransform,
    kf1._comps._trans._localTransform,
    factor,
    kf0._easing);

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
  if (_state != State::Playing && !_dirty) {
    return;
  }

#if 0
  if (_cinematic->_keyframes.size() <= 1) {
    printf("Only one keyframe in cinematic, won't play\n");
    _finished = true;
    return;
  }
#endif

  // Go through keyframes and find closest one to current time
  {
    // Camera
    auto [kf0, kf1, factor] = findClosestKfs<asset::CameraKeyframe>(_cinematic->_camKeyframes, _currentTime);

    if (!kf0 || !kf1) {
      printf("No kf for cinematic found!\n");
      return;
    }

    if (kf0 && kf1) {
      // TODO: Use params instead and construct proj matrix here
      auto lerped = lerp(*kf0, *kf1, factor);

      _camera->setPosition(lerped._pos);
      _camera->setYawPitchRoll(kf1->_ypr);
      _camera->setRotationMatrix(glm::toMat4(lerped._orientation));
    }
  }

  {
    // nodes
    for (auto& v : _cinematic->_nodeKeyframes) {
      auto [kf0, kf1, factor] = findClosestKfs<asset::NodeKeyframe>(v, _currentTime);

      if (!kf0 || !kf1) {
        printf("No kf for cinematic found!\n");
        return;
      }

      if (kf0 && kf1) {
        auto lerped = lerp(*kf0, *kf1, factor);

        auto* node = _scene->getNode(kf0->_id);

        // Only transform for now
        auto& transComp = _scene->registry().getComponent<component::Transform>(kf0->_id);
        transComp._localTransform = lerped._comps._trans._localTransform;
        _scene->registry().patchComponent<component::Transform>(kf0->_id);
      }
    }

  }

#if 0
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
  if (_currentTime >= _cinematic->_maxTime) {
    _finished = true;
    stop();
  }

  _dirty = false;
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