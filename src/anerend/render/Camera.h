#pragma once

#include "Frustum.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

namespace render {
  
enum class ProjectionType
{
  Perspective,
  Orthogonal
};

class Camera
{
public:
  Camera() = default;
  Camera(glm::vec3 initialPosition, ProjectionType type);
  Camera& operator=(const Camera&) = default;
  ~Camera() = default;

  void update(double delta);
  void updateFrustum();
  void updateViewMatrix();

  void setPosition(const glm::vec3& posIn);
  void setYawPitchRoll(const glm::vec3& ypr); // radians
  void setViewMatrix(const glm::mat4& matrix);
  void setProjection(const glm::mat4& matrix, float near, float far);
  void setRotationMatrix(const glm::mat4& matrix);

  float getFar() const { return _far; }
  float getNear() const { return _near; }
  glm::vec3 getPosition() const { return _position; }
  glm::vec3 getYPR() const { return { _yaw, _pitch, _roll }; } // radians
  const glm::mat4& getProjection() const { return _projection; }
  const glm::mat4& getCamMatrix() const { return _cameraMatrix; }
  glm::mat4 getCombined() const { return _projection * _cameraMatrix; }
  glm::vec3 getForward() const { return _forward; }

  Frustum& getFrustum() { return _frustum; }
  bool insideFrustum(const glm::vec3& point) const;
  bool insideFrustum(const Box3D& box) const;

  bool enabled() const { return _enabled; }
  void enable(bool val);

private:
  void freelookUpdate(double delta);
  void handleFreelookInput(double delta);

  Frustum _frustum;

  glm::vec2 _prevFreelookMousePos;
  bool _firstMouse;
  double _yaw;
  double _pitch;
  double _roll;

  bool _enabled;

  glm::mat4 _projection;
  glm::mat4 _cameraMatrix;
  glm::mat4 _rotation;
  glm::vec3 _position;
  glm::vec3 _forward;
  glm::vec3 _right;
  glm::vec3 _up;

  float _far;
  float _near;
  double _speed = 2.7; // avg running speed human female
  double _sensitivity = 0.002;
};

}