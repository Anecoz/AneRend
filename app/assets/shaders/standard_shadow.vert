#version 450

#extension GL_GOOGLE_include_directive : enable
#include "scene_ubo.glsl"

struct Renderable
{
  mat4 transform;
  vec4 bounds;
  vec4 tint;
  uint firstMeshId;
  uint numMeshIds;
  uint _visible;
};

struct TranslationId
{
  uint renderableId;
  uint meshId;
};

layout(std430, set = 0, binding = 2) readonly buffer RenderableBuffer {
  Renderable renderables[];
} renderableBuffer;

layout(std430, set = 1, binding = 0) buffer TranslationBuffer {
  TranslationId ids[];
} translationBuffer;

layout(location = 0) in vec3 inPosition;

layout(location = 0) out vec3 fragPositionWorld;

void main() {
  uint renderableId = translationBuffer.ids[gl_InstanceIndex].renderableId;
  mat4 model = renderableBuffer.renderables[renderableId].transform;
  gl_Position = ubo.directionalShadowMatrixProj * ubo.directionalShadowMatrixView * model * vec4(inPosition, 1.0);
  fragPositionWorld = vec3(model * vec4(inPosition, 1.0));
}