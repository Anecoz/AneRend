#include "CinematicPlayer.h"

#include "../asset/Cinematic.h"
#include "../asset/AssetCollection.h"
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
    // If we couldn't find a keyframe past the time, just snap it to the last one.
    kf0 = &vec[sz - 2];
    kf1 = &vec.back();

    return { kf0, kf1, 1.0 };
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

  // Always transform
  out._comps._trans._localTransform = util::interp::lerpTransforms(
    kf0._comps._trans._localTransform,
    kf1._comps._trans._localTransform,
    factor,
    kf0._easing);

  if (kf0._comps._light) {
    // Lerp color for now
    double easingFactor = util::interp::ease(factor, kf0._easing);
    auto& light0 = kf0._comps._light.value();
    auto& light1 = kf1._comps._light.value();

    component::Light lightOut;
    lightOut._color = (1.0f - float(easingFactor)) * light0._color+ (float)easingFactor * light1._color;

    out._comps._light = std::move(lightOut);
  }
  // Animator
  if (kf0._comps._animator) {
    double easingFactor = util::interp::ease(factor, kf0._easing);

    // Just snap this to whatever kf0 says
    out._comps._animator = kf0._comps._animator;

    // But do blend playback multiplier
    out._comps._animator.value()._playbackMultiplier = 
      (1.0f - float(easingFactor)) * kf0._comps._animator.value()._playbackMultiplier + 
      (float)easingFactor * kf1._comps._animator.value()._playbackMultiplier;
  }

  return out;
}

render::asset::MaterialKeyframe lerp(const render::asset::MaterialKeyframe& kf0, const render::asset::MaterialKeyframe& kf1, double factor)
{
  render::asset::MaterialKeyframe out;
  double easingFactor = util::interp::ease(factor, kf0._easing);

  // Emissive
  out._emissive = (1.0f - float(easingFactor)) * kf0._emissive + (float)easingFactor * kf1._emissive;

  // Base col
  out._baseColFactor = (1.0f - float(easingFactor)) * kf0._baseColFactor + (float)easingFactor * kf1._baseColFactor;

  return out;
}

}

CinematicPlayer::CinematicPlayer(asset::Cinematic cinematic, asset::AssetCollection* assColl, scene::Scene* scene, Camera* camera)
  : _cinematic(std::move(cinematic))
  , _scene(scene)
  , _assColl(assColl)
  , _camera(camera)
{}

CinematicPlayer::CinematicPlayer(CinematicPlayer&& rhs)
{
  std::swap(_cinematic, rhs._cinematic);
  std::swap(_scene, rhs._scene);
  std::swap(_assColl, rhs._assColl);
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
    std::swap(_assColl, rhs._assColl);
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

  // Go through keyframes and find closest one to current time
  {
    // Camera
    auto [kf0, kf1, factor] = findClosestKfs<asset::CameraKeyframe>(_cinematic._camKeyframes, _currentTime);

    if (!kf0 || !kf1) {
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
    for (auto& v : _cinematic._nodeKeyframes) {
      auto [kf0, kf1, factor] = findClosestKfs<asset::NodeKeyframe>(v, _currentTime);

      if (!kf0 || !kf1) {
        return;
      }

      if (kf0 && kf1) {
        auto lerped = lerp(*kf0, *kf1, factor);

        auto* node = _scene->getNode(kf0->_id);

        // Always transform component
        auto& transComp = _scene->registry().getComponent<component::Transform>(kf0->_id);
        transComp._localTransform = lerped._comps._trans._localTransform;
        _scene->registry().patchComponent<component::Transform>(kf0->_id);

        if (_scene->registry().hasComponent<component::Light>(kf0->_id)) {
          auto& lightComp = _scene->registry().getComponent<component::Light>(kf0->_id);
          lightComp = lerped._comps._light.value();
          _scene->registry().patchComponent<component::Light>(kf0->_id);
        }
        if (_scene->registry().hasComponent<component::Animator>(kf0->_id)) {
          auto& animatorComp = _scene->registry().getComponent<component::Animator>(kf0->_id);
          animatorComp = lerped._comps._animator.value();
          _scene->registry().patchComponent<component::Animator>(kf0->_id);
        }
      }
    }

  }

  {
    // materials
    for (auto& v : _cinematic._materialKeyframes) {
      auto [kf0, kf1, factor] = findClosestKfs<asset::MaterialKeyframe>(v, _currentTime);

      if (!kf0 || !kf1) {
        return;
      }

      if (kf0 && kf1) {
        auto lerped = lerp(*kf0, *kf1, factor);

        //auto matCopy = *_scene->getMaterial(kf0->_id);
        auto matCopy = _assColl->getMaterialBlocking(kf0->_id);

        matCopy._emissive = lerped._emissive;
        matCopy._baseColFactor = lerped._baseColFactor;

        //_scene->updateMaterial(std::move(matCopy));
        _assColl->updateMaterial(std::move(matCopy));
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
  if (_currentTime >= _cinematic._maxTime) {
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