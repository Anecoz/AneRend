#version 450

layout(set = 0, binding = 0) uniform UniformBufferObject {
  mat4 view;
  mat4 proj;
  mat4 invProj;
  mat4 invViewProj;
  mat4 directionalShadowMatrixProj;
  mat4 directionalShadowMatrixView;
  mat4 shadowMatrix[24];
  vec4 cameraPos;
  vec4 lightDir;
  vec4 viewVector;
  float time;
  uint screenWidth;
  uint screenHeight;
} ubo;

struct Renderable
{
  mat4 transform;
  vec4 bounds;
  vec4 tint;
  uint meshId;
  uint _visible;
};

layout(std430, set = 0, binding = 2) readonly buffer RenderableBuffer {
  Renderable renderables[];
} renderableBuffer;

layout(std430, set = 1, binding = 0) buffer TranslationBuffer {
  uint ids[];
} translationBuffer;

layout(push_constant) uniform constants {
  mat4 model;
} pushConstants;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 inNormal;

layout(location = 0) out vec3 fragColor;
layout(location = 1) flat out vec3 fragNormal;

void main() {
  uint renderableId = translationBuffer.ids[gl_InstanceIndex];
  mat4 model = renderableBuffer.renderables[renderableId].transform;

  gl_Position = ubo.proj * ubo.view * model * vec4(inPosition, 1.0);
  fragColor = inColor * renderableBuffer.renderables[renderableId].tint.rgb;
  fragNormal = mat3(model) * inNormal;
}