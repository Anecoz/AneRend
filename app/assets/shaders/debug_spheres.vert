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
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 inNormal;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec3 fragPos;

mat4 buildTranslation(mat4 model)
{
  vec3 delta = vec3(model[3][0], model[3][1], model[3][2]);

  return mat4(
        vec4(1.0, 0.0, 0.0, 0.0),
        vec4(0.0, 1.0, 0.0, 0.0),
        vec4(0.0, 0.0, 1.0, 0.0),
        vec4(delta, 1.0));
}

void main() {
  uint renderableId = translationBuffer.ids[gl_InstanceIndex].renderableId;
  mat4 model = renderableBuffer.renderables[renderableId].transform;

  // Only use the translation part from the model matrix
  mat4 trans = buildTranslation(model);

  // Scale position by the radius
  vec3 sphereBoundCenter =  renderableBuffer.renderables[renderableId].bounds.xyz;
  float radius =  renderableBuffer.renderables[renderableId].bounds.w;
  vec3 pos = inPosition * radius + sphereBoundCenter;

  gl_Position = ubo.proj * ubo.view * trans * vec4(pos, 1.0);
  fragColor = inColor * renderableBuffer.renderables[renderableId].tint.rgb;
  fragNormal = inNormal;
  fragPos = (trans * vec4(pos, 1.0)).xyz;
}