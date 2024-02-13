#pragma once

#include <glm/glm.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

#include <tuple>

namespace util {

static std::tuple<glm::quat, glm::vec3> getRotAndTrans(const glm::mat4& mat)
{
	glm::vec3 s;
	glm::quat q;
	glm::vec3 t;

	glm::vec3 unused_0;
	glm::vec4 unused_1;

	glm::decompose(mat, s, q, t, unused_0, unused_1);

	return { q, t };
}

static glm::quat getRot(const glm::mat4& mat)
{
	glm::vec3 s;
	glm::quat q;
	glm::vec3 t;

	glm::vec3 unused_0;
	glm::vec4 unused_1;

	glm::decompose(mat, s, q, t, unused_0, unused_1);

	return q;
}

static glm::vec3 getTrans(const glm::mat4& mat)
{
	glm::vec3 s;
	glm::quat q;
	glm::vec3 t;

	glm::vec3 unused_0;
	glm::vec4 unused_1;

	glm::decompose(mat, s, q, t, unused_0, unused_1);

	return t;
}

static glm::vec3 getScale(const glm::mat4& mat)
{
	glm::vec3 s;
	glm::quat q;
	glm::vec3 t;

	glm::vec3 unused_0;
	glm::vec4 unused_1;

	glm::decompose(mat, s, q, t, unused_0, unused_1);

	return s;
}

}