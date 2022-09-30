#pragma once

#include "Vertex.h"

#include "../util/OBJLoader.h"

#include <cstdint>
#include <string>
#include <vector>

struct Model
{
  std::vector<Vertex> _vertices;
  std::vector<std::uint32_t> _indices;

  bool loadFromObj(const std::string& objPath, const std::string& mtlPath)
  {
    OBJData objData;
    if (!OBJLoader::loadFromFile(objPath, mtlPath, objData)) {
      printf("Could not load OBJ from %s and %s\n", objPath.c_str(), mtlPath.c_str());
      return false;
    }

    if (objData._indices.empty()) {
      printf("No indices in obj file!\n");
      return false;
    }

    _vertices.resize(objData._vertices.size());
    _indices.resize(objData._indices.size());
    for (unsigned int i = 0; i < objData._indices.size(); ++i) {
      Vertex& vertex = _vertices[objData._indices[i]];

      vertex.pos.x = objData._vertices[3 * objData._indices[i] + 0];
      vertex.pos.y = objData._vertices[3 * objData._indices[i] + 1];
      vertex.pos.z = objData._vertices[3 * objData._indices[i] + 2];

      vertex.color.r = objData._colors[3 * objData._indices[i] + 0];
      vertex.color.g = objData._colors[3 * objData._indices[i] + 1];
      vertex.color.b = objData._colors[3 * objData._indices[i] + 2];

      vertex.normal.x = objData._normals[3 * objData._indices[i] + 0];
      vertex.normal.y = objData._normals[3 * objData._indices[i] + 1];
      vertex.normal.z = objData._normals[3 * objData._indices[i] + 2];

      _indices.emplace_back(static_cast<std::uint32_t>(objData._indices[i]));
    }

    return true;
  }
};