#pragma once

#include "../DeserialisedSceneData.h"

#include <cstdint>
#include <thread>
#include <filesystem>
#include <future>
#include <memory>

namespace render::scene { class Scene; }

namespace render::scene::internal
{

struct SceneSerializer
{
  SceneSerializer() = default;
  ~SceneSerializer();

  void serialize(const Scene& scene, const std::filesystem::path& path);
  std::future<DeserialisedSceneData> deserialize(const std::filesystem::path& path);

private:
  const std::uint8_t _currentVersion = 1;
  std::thread _serialisationThread;
  std::thread _deserialisationThread;
};

}