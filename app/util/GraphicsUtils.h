#pragma once

#include "../render/Vertex.h"

#include <cstdlib>
#include <random>
#include <vector>

#define PI           3.14159265358979323846  /* pi */

namespace graphicsutil {

// Counter clockwise
// Vulkan NDC is
// x -> right
// y -> down
// z -> in toward screen
static std::vector<render::Vertex> createScreenQuad(float widthPercent, float heightPercent) {
  float depth = 0.1f; // Vulkan NDC depth range is 0,1, be close to camera...
  glm::vec4 color {1.0f, 1.0f, 1.0f, 1.0f}; // don't care
  glm::vec4 normal {0.0f, 0.0f, 0.0f, 0.0f}; // don't care

  float x = -1.0f + widthPercent * 2.0f;
  float y = -1.0f + heightPercent * 2.0f;

  return { 
    {{-1.0f, -1.0f, depth, 0.0f}, color, normal, {0.0f, 0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f, 0.0f}},
    {{-1.0f, y, depth, 0.0f}, color, normal, {0.0f, 0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f, 0.0f}},
    {{x, -1.0f, depth, 0.0f}, color, normal, {0.0f, 0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 0.0f}},

    {{x, -1.0f, depth, 0.0f}, color, normal, {0.0f, 0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 0.0f}},
    {{-1.0f, y, depth, 0.0f}, color, normal, {0.0f, 0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f, 0.0f}},
    {{x, y, depth, 0.0f}, color, normal, {0.0f, 0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 0.0f, 0.0f}}
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
  glm::vec4 color{ 1.0f };

  if (randomColor) {
    color = glm::vec4(graphicsutil::randomColor(), 0.0f);
  }

  glm::vec4 normal{ 0.0f, 1.0f, 0.0f, 0.0f };
  glm::vec4 tex{ 0.0f, 0.0f, 0.0f, 0.0f };

  std::vector<std::uint32_t> indices{
    //Top
    2, 7, 6,
    2, 3, 7,

    //Bottom
    0, 4, 5,
    0, 5, 1,

    //Left
    0, 2, 6,
    0, 6, 4,

    //Right
    1, 7, 3,
    1, 5, 7,

    //Front
    0, 3, 2,
    0, 1, 3,

    //Back
    4, 6, 7,
    4, 7, 5
  };

  glm::vec4 tangent{ 0.0f, 0.0f, 0.0f, 0.0f };
  // pos, color, normal, tangent, tex
  std::vector<render::Vertex> verts{
    {{-0.5, -0.5, 0.5, 0.0f}, color, normal, tangent, tex}, // 0
    {{0.5, -0.5, 0.5, 0.0f}, color, normal, tangent, tex},//1
    {{-0.5, 0.5, 0.5, 0.0f}, color, normal, tangent, tex},//2
    {{0.5, 0.5, 0.5, 0.0f}, color, normal, tangent, tex},//3
    {{-0.5, -0.5, -0.5, 0.0f}, color, normal, tangent, tex},//4
    {{0.5, -0.5, -0.5, 0.0f}, color, normal, tangent, tex},//5
    {{-0.5, 0.5, -0.5, 0.0f}, color, normal, tangent, tex},//6
    {{0.5, 0.5, -0.5, 0.0f}, color, normal, tangent, tex}//7
  };

  *vertOut = verts;
  *indOut = indices;
}

static void createSphere(float radius, std::vector<render::Vertex>* vertOut, std::vector<std::uint32_t>* indOut, bool randomColor = true)
{
  // From http://www.songho.ca/opengl/gl_sphere.html

  glm::vec4 color{ 1.0f };
  if (randomColor) {
    color = glm::vec4(graphicsutil::randomColor(), 1.0f);
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
      vert.pos = { x, y, z, 0.0f };

      // normalized vertex normal (nx, ny, nz)
      nx = x * lengthInv;
      ny = y * lengthInv;
      nz = z * lengthInv;
      vert.normal = { nx, ny, nz, 0.0f };

      // vertex tex coord (s, t) range between [0, 1]
      s = (float)j / sectorCount;
      t = (float)i / stackCount;
      vert.uv = { s, t, 0.0f, 0.0f };

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