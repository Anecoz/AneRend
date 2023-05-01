#pragma once

#include "../render/Vertex.h"

#include <string>
#include <vector>

class OBJLoader
{
public:
	OBJLoader() = default;
	~OBJLoader() = default;

	static bool loadFromFile(
    const std::string& objPath,
    const std::string& mtlDir,
    std::vector<render::Vertex>& outVerts,
    std::vector<uint32_t>& outIndices);
};

