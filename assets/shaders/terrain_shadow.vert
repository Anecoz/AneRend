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

void main() {
  uint renderableId = translationBuffer.ids[gl_InstanceIndex].renderableId;

  mat4 model = renderableBuffer.renderables[renderableId].transform;
  gl_Position = ubo.directionalShadowMatrixProj * ubo.directionalShadowMatrixView * model * vec4(inPosition, 1.0);
}