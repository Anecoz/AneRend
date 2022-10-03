#pragma once

#include "Box3D.h"

#include <glm/glm.hpp>

namespace render {

class Frustum
{
  public:
    //! TYPEDEF/ENUMS:
    enum Plane { Right, Left, Bottom, Top, Front, Back };
    enum { A, B, C, D };

    //! CTOR/DTOR:
    Frustum();
    virtual ~Frustum();

    //! SERVICES:
    void transform(const glm::mat4& proj, const glm::mat4& view);
    void normalize(Plane plane);

    //! CULLING:
    enum Visibility { Completly, Partially, Invisible };
    Visibility isInside(const glm::vec3& point) const;
    Visibility isInside(const Box3D& box) const;

    //! ACCESSORS:
    glm::vec4 getPlane(Plane plane) const;

  private:
    //! MEMBERS:
    double m_data[6][4];
};

////////////////////////////////////////////////////////////////////////////////
// Frustum inline implementation:
////////////////////////////////////////////////////////////////////////////////
/*----------------------------------------------------------------------------*/
inline Frustum::Frustum() {
}
/*----------------------------------------------------------------------------*/
inline Frustum::~Frustum() {
}

/*----------------------------------------------------------------------------*/
inline glm::vec4 Frustum::getPlane(Plane plane) const {
  return glm::vec4(m_data[plane][A], m_data[plane][B], m_data[plane][C], m_data[plane][D]);
}
}