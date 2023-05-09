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

layout(push_constant) uniform constants {
  mat4 model;
} pushConstants;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec2 inUV;
layout(location = 4) in vec4 inTangent;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec3 fragPos;
layout(location = 3) out vec2 fragUV;
layout(location = 4) out flat uint fragMeshId;
layout(location = 5) out vec3 fragTangent;
layout(location = 6) out mat3 fragTBN;

void main() {
  uint renderableId = translationBuffer.ids[gl_InstanceIndex].renderableId;
  mat4 model = renderableBuffer.renderables[renderableId].transform;

  gl_Position = ubo.proj * ubo.view * model * vec4(inPosition, 1.0);
  fragColor = inColor * renderableBuffer.renderables[renderableId].tint.rgb;
  fragNormal =  mat3(model) * inNormal;
  fragPos = (model * vec4(inPosition, 1.0)).xyz;
  fragUV = inUV;
  fragMeshId = translationBuffer.ids[gl_InstanceIndex].meshId;

  vec3 bitangentL = cross(inNormal, inTangent.xyz);
  vec3 T = normalize(mat3(model) * inTangent.xyz);
  vec3 B = normalize(mat3(model) * bitangentL);
  vec3 N = normalize(mat3(model) * inNormal);
  fragTBN = mat3(T, B, N);
  fragTangent = inTangent.xyz;
}