#include "OBJLoader.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "../tiny_obj_loader.h"

#include <stdio.h>
#include <iostream>
#include <algorithm>

bool OBJLoader::loadFromFile(const std::string& objPath, const std::string& mtlDir, OBJData& outData)
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

  std::vector<int> indices;
  std::vector<float> normals;
  std::vector<float> colors;
  colors.resize(attrib.vertices.size());
  normals.resize(attrib.vertices.size());
  for (int shape = 0; shape < shapes.size(); ++shape) {
    for (int i = 0; i < shapes[shape].mesh.indices.size(); ++i) {
      tinyobj::index_t idx = shapes[shape].mesh.indices[i];
      indices.emplace_back(idx.vertex_index);

      normals[3 * idx.vertex_index + 0] = attrib.normals[3 * idx.normal_index + 0];
      normals[3 * idx.vertex_index + 1] = attrib.normals[3 * idx.normal_index + 1];
      normals[3 * idx.vertex_index + 2] = attrib.normals[3 * idx.normal_index + 2];

      if (!shapes[shape].mesh.material_ids.empty()) {
        int materialId = shapes[shape].mesh.material_ids[i/3];    

        colors[3 * idx.vertex_index + 0] = materials[materialId].diffuse[0];
        colors[3 * idx.vertex_index + 1] = materials[materialId].diffuse[1];
        colors[3 * idx.vertex_index + 2] = materials[materialId].diffuse[2];
      }
      else {
        colors[3 * idx.vertex_index + 0] = 1.0;
        colors[3 * idx.vertex_index + 1] = 1.0;
        colors[3 * idx.vertex_index + 2] = 1.0;
      }
    }
  }

  outData._vertices = std::move(attrib.vertices);
  outData._normals = std::move(normals);
  outData._colors = std::move(colors);
  outData._indices = std::move(indices);
  return true;
}