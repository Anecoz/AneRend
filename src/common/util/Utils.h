#pragma once

#include <cstdio>
#include <fstream>
#include <functional>
#include <vector>

namespace util {

struct Defer
{
  Defer(std::function<void()> f) : _f(f) {}
  ~Defer() { _f(); }

  std::function<void()> _f;
};

static std::vector<char> readFile(const std::string& filename) 
{
  std::ifstream file(filename, std::ios::ate | std::ios::binary);

  if (!file.is_open()) {
    printf("Could not open file: %s\n", filename.c_str());
    return {};
  }

  size_t fileSize = (size_t) file.tellg();
  std::vector<char> buffer(fileSize);

  file.seekg(0);
  file.read(buffer.data(), fileSize);

  file.close();
  return buffer;
}

}

#define DEFER(f) util::Defer defer([&](){f();})