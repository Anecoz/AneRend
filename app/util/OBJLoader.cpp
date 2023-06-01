#include "OBJLoader.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#include <stdio.h>
#include <iostream>
#include <algorithm>
#include <unordered_map>

bool OBJLoader::loadFromFile(
  const std::string& objPath,
  const std::string& mtlDir,
  std::vector<render::Vertex>& outVerts,
  std::vector<uint32_t>& outIndices)
{
	tinyobj::attrib_t attrib;
  std::vector<tinyobj::shape_t> shapes;
  std::vector<tinyobj::material_t> materials;
  std::string warn, err;

  printf("OBJ: Loading OBJ model from %s\n", objPath.c_str());
  tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, objPath.c_str(), mtlDir.c_str());

  if (!warn.empty()) {
    printf("OBJ: Warning: %s\n", warn.c_str());
  }
  if (!err.empty()) {
    printf("OBJ: Error: %s\n", err.c_str());
  }

  if (shapes.empty()) {
    printf("OBJ: No shapes loaded, doing nothing\n");
    return false;
  }

  printf("OBJ: Loading %zu shapes\n", shapes.size());

  std::unordered_map<render::Vertex, uint32_t> uniqueVertices{};

  for (int shape = 0; shape < shapes.size(); ++shape) {
    for (int i = 0; i < shapes[shape].mesh.indices.size(); ++i) {
      tinyobj::index_t idx = shapes[shape].mesh.indices[i];

      render::Vertex vert{};

      vert.pos = {
        attrib.vertices[3 * idx.vertex_index + 0],
        attrib.vertices[3 * idx.vertex_index + 1],
        attrib.vertices[3 * idx.vertex_index + 2],
        0.0
      };

      vert.normal = {
        attrib.normals[3 * idx.normal_index + 0],
        attrib.normals[3 * idx.normal_index + 1],
        attrib.normals[3 * idx.normal_index + 2],
        0.0
      };

      if (idx.texcoord_index != -1) {
        vert.uv = {
          attrib.texcoords[2 * idx.texcoord_index + 0],
          1.0f - attrib.texcoords[2 * idx.texcoord_index + 1],
          0.0f,
          0.0f
        };
      }

      if (!shapes[shape].mesh.material_ids.empty()) {
        int materialId = shapes[shape].mesh.material_ids[i / 3];

        vert.color = {
          materials[materialId].diffuse[0],
          materials[materialId].diffuse[1],
          materials[materialId].diffuse[2],
          1.0f
        };
      }
      else {
        vert.color = {
          1.0f,
          1.0f,
          1.0f,
          1.0f
        };
      }

      if (uniqueVertices.count(vert) == 0) {
        uniqueVertices[vert] = static_cast<uint32_t>(outVerts.size());
        outVerts.emplace_back(vert);
      }

      outIndices.emplace_back(uniqueVertices[vert]);
    }
  }
  return true;
}