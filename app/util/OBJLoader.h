#pragma once

#include <string>
#include <vector>

struct OBJData
{
  std::vector<float> _vertices;
  std::vector<float> _normals;
  std::vector<float> _colors;
  std::vector<int> _indices;
};

class OBJLoader
{
public:
	OBJLoader() = default;
	~OBJLoader() = default;

	static bool loadFromFile(const std::string& objPath, const std::string& mtlDir, OBJData& outData);
};

