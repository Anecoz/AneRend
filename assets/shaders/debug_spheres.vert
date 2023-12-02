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
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 inNormal;

layout(location = 0) out vec3 fragColor;

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
  uint meshOffset = translationBuffer.ids[gl_InstanceIndex].meshId;
  mat4 model = renderableBuffer.renderables[renderableId].transform;
  uint meshIndex = rendModelBuffer.indices[meshOffset];
  MeshInfo meshInfo = meshBuffer.meshes[meshIndex];

  // Only use the translation part from the model matrix
  mat4 trans = buildTranslation(model);

  vec4 sphere = constructBoundingSphere(meshInfo);
  vec3 scale = getScale(model);
  sphere.xyz = vec3(model * vec4(sphere.xyz, 1.0));
  sphere.w *= scale.x; // NOTE: Assuming uniform scaling!

  // Scale position by the radius
  vec3 sphereBoundCenter = sphere.xyz;
  float radius = sphere.w;
  vec3 pos = inPosition * radius + sphereBoundCenter;

  gl_Position = ubo.proj * ubo.view * vec4(pos, 1.0);
  fragColor = inColor * renderableBuffer.renderables[renderableId].tint.rgb;
}