#version 450

#extension GL_GOOGLE_include_directive : enable
#include "bindless.glsl"

struct TranslationId
{
  uint renderableId;
  uint meshId;
};

layout(std430, set = 1, binding = 0) buffer TranslationBuffer {
  TranslationId ids[];
} translationBuffer;

layout(push_constant) uniform constants {
  mat4 shadowMatrix;
  vec4 lightParams; // pos + range
} pc;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in ivec4 joints;
layout(location = 2) in vec4 jointWeights;

layout(location = 0) out vec3 fragPos;
layout(location = 1) flat out vec3 fragLightPos;
layout(location = 2) flat out float fragRange;

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

  fragPos = (model * vec4(pos, 1.0)).xyz;
  fragRange = pc.lightParams.w;
  fragLightPos = pc.lightParams.xyz;
  gl_Position = pc.shadowMatrix * model * vec4(pos, 1.0);
}