#include "Box3D.h"

#include <limits>

namespace render {

Box3D::Box3D()
  : _min(std::numeric_limits<double>::max(), std::numeric_limits<double>::max(), std::numeric_limits<double>::max())
  , _max(std::numeric_limits<double>::min(), std::numeric_limits<double>::min(), std::numeric_limits<double>::min())
{}

Box3D::Box3D(const glm::vec3& min, const glm::vec3& max)
  : _min(min)
  , _max(max)
{}

glm::vec3 Box3D::getMin() const
{
  return _min;
}

glm::vec3 Box3D::getMax() const
{
  return _max;
}

void Box3D::extend(const glm::vec3& val)
{
  // Is value less in any dimension than current min?
  if (val.x < _min.x) {
    _min.x = val.x;
  }
  if (val.y < _min.y) {
    _min.y = val.y;
  }
  if (val.z < _min.z) {
    _min.z = val.z;
  }

  // Same for max
  if (val.x > _max.x) {
    _max.x = val.x;
  }
  if (val.y > _max.y) {
    _max.y = val.y;
  }
  if (val.z > _max.z) {
    _max.z = val.z;
  }
}
}