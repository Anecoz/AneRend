#version 450

#extension GL_GOOGLE_include_directive : enable
#include "scene_ubo.glsl"
#include "bindless.glsl"

struct TranslationId
{
  uint renderableId;
  uint meshId;
};

layout(std430, set = 1, binding = 0) buffer TranslationBuffer {
  TranslationId ids[];
} translationBuffer;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in ivec4 joints;
layout(location = 2) in vec4 jointWeights;

void main() {
  uint renderableId = translationBuffer.ids[gl_InstanceIndex].renderableId;
  uint skeletonOffset = renderableBuffer.renderables[renderableId].skeletonOffset;

  vec3 pos = inPosition;
  if (joints[0] != -1) {
    pos = vec3(
      jointWeights[0] * skeletonBuffer.joints[skeletonOffset + joints[0]] * vec4(inPosition, 1.0) +
      jointWeights[1] * skeletonBuffer.joints[skeletonOffset + joints[1]] * vec4(inPosition, 1.0) +
      jointWeights[2] * skeletonBuffer.joints[skeletonOffset + joints[2]] * vec4(inPosition, 1.0) +
      jointWeights[3] * skeletonBuffer.joints[skeletonOffset + joints[3]] * vec4(inPosition, 1.0));
  }

  mat4 model = renderableBuffer.renderables[renderableId].transform;
  gl_Position = ubo.directionalShadowMatrixProj * ubo.directionalShadowMatrixView * model * vec4(pos, 1.0);
}