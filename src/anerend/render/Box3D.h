#pragma once

#include <glm/glm.hpp>

namespace render {

class Box3D
{
public:
  Box3D();
  Box3D(const glm::vec3& min, const glm::vec3& max);

  glm::vec3 getMin() const;
  glm::vec3 getMax() const;

  // Extends the box if the value passed is less than min or greater than max
  void extend(const glm::vec3& val);

private:
  glm::vec3 _min;
  glm::vec3 _max;
};
}