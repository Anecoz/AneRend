#pragma once

#include "../render/Vertex.h"

#include <cstdlib>
#include <random>
#include <vector>

#define PI           3.14159265358979323846f  /* pi */

namespace util {

// Counter clockwise
// Vulkan NDC is
// x -> right
// y -> down
// z -> in toward screen
static std::vector<render::Vertex> createScreenQuad(float widthPercent, float heightPercent) {
  float depth = 0.1f; // Vulkan NDC depth range is 0,1, be close to camera...
  glm::vec3 color {1.0f}; // don't care
  glm::vec3 normal {0.0f}; // don't care
  glm::i16vec4 joints{ -1, -1, -1, -1 };
  glm::vec4 weights{ 0.0f, 0.0f, 0.0f, 0.0f };

  float x = -1.0f + widthPercent * 2.0f;
  float y = -1.0f + heightPercent * 2.0f;

  return { 
    {{-1.0f, -1.0f, depth}, color, normal, {0.0f, 0.0f, 0.0f, 0.0f}, weights, {0.0f, 0.0f}, 0.0f, joints},
    {{-1.0f, y, depth}, color, normal, {0.0f, 0.0f, 0.0f, 0.0f}, weights, {0.0f, 1.0f}, 0.0f, joints},
    {{x, -1.0f, depth}, color, normal, {0.0f, 0.0f, 0.0f, 0.0f}, weights, {1.0f, 0.0f}, 0.0f, joints},

    {{x, -1.0f, depth}, color, normal, {0.0f, 0.0f, 0.0f, 0.0f}, weights,  {1.0f, 0.0f}, 0.0f, joints},
    {{-1.0f, y, depth}, color, normal, {0.0f, 0.0f, 0.0f, 0.0f}, weights, {0.0f, 1.0f}, 0.0f, joints},
    {{x, y, depth}, color, normal, {0.0f, 0.0f, 0.0f, 0.0f}, weights, {1.0f, 1.0f}, 0.0f, joints}
  };
}

static glm::vec3 randomColor()
{
  std::random_device dev;
  std::mt19937 rng(dev());
  std::uniform_real_distribution<> col(0.0, 1.0);

  return glm::vec3(col(rng), col(rng), col(rng));
}

static void createUnitCube(std::vector<render::Vertex>* vertOut, std::vector<std::uint32_t>* indOut, bool randomColor = true) {
  glm::vec3 color{ 1.0f };

  if (randomColor) {
    color = util::randomColor();
  }

  glm::vec3 normal{ 0.0f, 1.0f, 0.0f};
  glm::vec2 tex{ 0.0f};
  glm::vec4 tangent{ 0.0f, 0.0f, 0.0f, 0.0f };
  glm::i16vec4 joints{ -1, -1, -1, -1 };
  glm::vec4 weights{ 0.0f, 0.0f, 0.0f, 0.0f };
  float pad = 0.0f;

  glm::vec3 up{ 0.0, 1.0, 0.0 };
  glm::vec3 down = -up;
  glm::vec3 right{ 1.0, 0.0, 0.0 };
  glm::vec3 left = -right;
  glm::vec3 forward{ 0.0, 0.0, 1.0 };
  glm::vec3 back = -forward;

  std::vector<render::Vertex> verts{
    // top
    { { -0.5,  0.5, -0.5 }, color, up, tangent, weights, tex, pad, joints },
    { { -0.5,  0.5,  0.5 }, color, up, tangent, weights, tex, pad, joints },
    { {  0.5,  0.5, -0.5 }, color, up, tangent, weights, tex, pad, joints },
    { {  0.5,  0.5, -0.5 }, color, up, tangent, weights, tex, pad, joints },
    { { -0.5,  0.5,  0.5 }, color, up, tangent, weights, tex, pad, joints },
    { {  0.5,  0.5,  0.5 }, color, up, tangent, weights, tex, pad, joints },
    // right
    { {  0.5,  0.5, -0.5 }, color, right, tangent, weights, tex, pad, joints },
    { {  0.5,  0.5,  0.5 }, color, right, tangent, weights, tex, pad, joints },
    { {  0.5, -0.5, -0.5 }, color, right, tangent, weights, tex, pad, joints },
    { {  0.5,  0.5,  0.5 }, color, right, tangent, weights, tex, pad, joints },
    { {  0.5, -0.5,  0.5 }, color, right, tangent, weights, tex, pad, joints },
    { {  0.5, -0.5, -0.5 }, color, right, tangent, weights, tex, pad, joints },
    //bottom
    { {  0.5, -0.5, -0.5 }, color, down, tangent, weights, tex, pad, joints },
    { {  0.5, -0.5,  0.5 }, color, down, tangent, weights, tex, pad, joints },
    { { -0.5, -0.5,  0.5 }, color, down, tangent, weights, tex, pad, joints },
    { {  0.5, -0.5, -0.5 }, color, down, tangent, weights, tex, pad, joints },
    { { -0.5, -0.5,  0.5 }, color, down, tangent, weights, tex, pad, joints },
    { { -0.5, -0.5, -0.5 }, color, down, tangent, weights, tex, pad, joints },
    //left
    { { -0.5,  0.5,  0.5 }, color, left, tangent, weights, tex, pad, joints },
    { { -0.5, -0.5, -0.5 }, color, left, tangent, weights, tex, pad, joints },
    { { -0.5, -0.5,  0.5 }, color, left, tangent, weights, tex, pad, joints },
    { { -0.5,  0.5,  0.5 }, color, left, tangent, weights, tex, pad, joints },
    { { -0.5,  0.5, -0.5 }, color, left, tangent, weights, tex, pad, joints },
    { { -0.5, -0.5, -0.5 }, color, left, tangent, weights, tex, pad, joints },
    //back
    { { -0.5,  0.5, -0.5 }, color, back, tangent, weights, tex, pad, joints },
    { {  0.5, -0.5, -0.5 }, color, back, tangent, weights, tex, pad, joints },
    { { -0.5, -0.5, -0.5 }, color, back, tangent, weights, tex, pad, joints },
    { { -0.5,  0.5, -0.5 }, color, back, tangent, weights, tex, pad, joints },
    { {  0.5,  0.5, -0.5 }, color, back, tangent, weights, tex, pad, joints },
    { {  0.5, -0.5, -0.5 }, color, back, tangent, weights, tex, pad, joints },
    //front
    { { -0.5,  0.5,  0.5 }, color, forward, tangent, weights, tex, pad, joints },
    { {  0.5, -0.5,  0.5 }, color, forward, tangent, weights, tex, pad, joints },
    { {  0.5,  0.5,  0.5 }, color, forward, tangent, weights, tex, pad, joints },
    { { -0.5,  0.5,  0.5 }, color, forward, tangent, weights, tex, pad, joints },
    { { -0.5, -0.5,  0.5 }, color, forward, tangent, weights, tex, pad, joints },
    { {  0.5, -0.5,  0.5 }, color, forward, tangent, weights, tex, pad, joints },
  };

  std::vector<std::uint32_t> indices{
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
    11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
    21, 22, 23, 24, 25, 26, 27, 28, 29, 30,
    31, 32, 33, 34, 35 };

  *vertOut = verts;
  *indOut = indices;
}

static void createSphere(float radius, std::vector<render::Vertex>* vertOut, std::vector<std::uint32_t>* indOut, bool randomColor = true)
{
  // From http://www.songho.ca/opengl/gl_sphere.html

  glm::vec4 color{ 1.0f };
  if (randomColor) {
    color = glm::vec4(util::randomColor(), 1.0f);
  }

  int sectorCount = 30;
  int stackCount = 30;

  float x, y, z, xy;                              // vertex position
  float nx, ny, nz, lengthInv = 1.0f / radius;    // vertex normal
  float s, t;                                     // vertex texCoord

  float sectorStep = 2 * PI / sectorCount;
  float stackStep = PI / stackCount;
  float sectorAngle, stackAngle;

  for (int i = 0; i <= stackCount; ++i) {
    stackAngle = PI / 2 - i * stackStep;        // starting from pi/2 to -pi/2
    xy = radius * cosf(stackAngle);             // r * cos(u)
    z = radius * sinf(stackAngle);              // r * sin(u)

    // add (sectorCount+1) vertices per stack
    // first and last vertices have same position and normal, but different tex coords
    for (int j = 0; j <= sectorCount; ++j) {
      render::Vertex vert{};

      vert.color = color;

      sectorAngle = j * sectorStep;           // starting from 0 to 2pi

      // vertex position (x, y, z)
      x = xy * cosf(sectorAngle);             // r * cos(u) * cos(v)
      y = xy * sinf(sectorAngle);             // r * cos(u) * sin(v)
      vert.pos = { x, y, z };

      // normalized vertex normal (nx, ny, nz)
      nx = x * lengthInv;
      ny = y * lengthInv;
      nz = z * lengthInv;
      vert.normal = { nx, ny, nz };

      // vertex tex coord (s, t) range between [0, 1]
      s = (float)j / sectorCount;
      t = (float)i / stackCount;
      vert.uv = { s, t };

      vertOut->emplace_back(std::move(vert));
    }
  }

  // Indices
  int k1, k2;
  for (int i = 0; i < stackCount; ++i) {
    k1 = i * (sectorCount + 1);     // beginning of current stack
    k2 = k1 + sectorCount + 1;      // beginning of next stack

    for (int j = 0; j < sectorCount; ++j, ++k1, ++k2) {
      // 2 triangles per sector excluding first and last stacks
      // k1 => k2 => k1+1
      if (i != 0) {
        indOut->emplace_back(k1);
        indOut->emplace_back(k2);
        indOut->emplace_back(k1 + 1);
      }

      // k1+1 => k2 => k2+1
      if (i != (stackCount - 1)) {
        indOut->emplace_back(k1 + 1);
        indOut->emplace_back(k2);
        indOut->emplace_back(k2 + 1);
      }
    }
  }
}

}