#pragma once

#include <bitsery/bitsery.h>
#include <bitsery/adapter/buffer.h>
#include <bitsery/traits/vector.h>
#include <bitsery/traits/string.h>

#include <glm/glm.hpp>

#include <cstdint>
#include <fstream>
#include <filesystem>
#include <string>
#include <vector>

namespace logic {

struct AneditConfig
{
  const std::uint8_t currentVersion = 1;

  std::filesystem::path _scenePath;
  glm::vec3 _lastCamPos;

  void saveToPath(std::filesystem::path p)
  {
    std::ofstream file(p, std::ios::binary);
    if (file.bad()) {
      printf("Could not open path to serialize config: %s!\n", p.string().c_str());
      file.close();
      return;
    }

    std::vector<std::uint8_t> bytes;
    auto byteSize = (uint32_t)bitsery::quickSerialization<bitsery::OutputBufferAdapter<std::vector<std::uint8_t>>>(bytes, *this);

    file.write((const char*)&currentVersion, 1);
    file.write((const char*)bytes.data(), byteSize);
    file.close();

    printf("Wrote config to %s\n", p.string().c_str());
  }

  void readFromPath(std::filesystem::path p)
  {
    std::ifstream file(p, std::ios::binary | std::ios::ate);

    if (file.bad()) {
      printf("Could not open config file for deserialisation (%s)!\n", p.string().c_str());

      file.close();
      return;
    }

    auto fileSize = static_cast<uint32_t>(file.tellg());
    file.seekg(0);

    std::uint8_t deserialisedVersion;
    file.read((char*)&deserialisedVersion, 1);

    if (deserialisedVersion != currentVersion) {
      printf("Wrong version when reading config!\n");
      file.close();
      return;
    }

    std::vector<std::uint8_t> bytes(fileSize - 1);
    file.seekg(1);
    file.read((char*)bytes.data(), bytes.size());

    auto state = bitsery::quickDeserialization<bitsery::InputBufferAdapter<std::vector<uint8_t>>>
      ({ bytes.begin(), bytes.size() }, *this);

    if (!state.second) {
      printf("Failed deserialisation of config %s\n", p.string().c_str());
      file.close();
      return;
    }
  }

};

}


namespace bitsery {

  template <typename S>
  void serialize(S& s, glm::vec3& v)
  {
    s.value4b(v.x);
    s.value4b(v.y);
    s.value4b(v.z);
  }

  template <typename S>
  void serialize(S& s, logic::AneditConfig& conf)
  {
    auto string = conf._scenePath.string();
    s.text1b(string, 40);
    conf._scenePath = std::filesystem::path(string);

    s.object(conf._lastCamPos);
  }

}