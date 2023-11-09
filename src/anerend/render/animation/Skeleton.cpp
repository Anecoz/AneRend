#include "Skeleton.h"

namespace render::anim {

namespace {

void calcGlobalTransformForParent(Joint* joint)
{
  if (joint->_parent == nullptr) {
    joint->_globalTransform = joint->_localTransform;
  }
  else {
    joint->_globalTransform = joint->_parent->_globalTransform * joint->_localTransform;
  }

  for (auto child : joint->_children) {
    calcGlobalTransformForParent(child);
  }
}

}

void Skeleton::calcGlobalTransforms()
{
  calcGlobalTransformForParent(&_joints[0]);
}

void Skeleton::reset()
{
  for (auto& joint : _joints) {
    joint._localTransform = joint._originalLocalTransform;
  }
}

}