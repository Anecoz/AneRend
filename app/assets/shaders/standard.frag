#version 450

layout(set = 0, binding = 0) uniform UniformBufferObject {
  mat4 view;
  mat4 proj;
  mat4 invViewProj;
  mat4 directionalShadowMatrixProj;
  mat4 directionalShadowMatrixView;
  mat4 shadowMatrix[24];
  vec4 cameraPos;
  vec4 lightDir;
  vec4 lightPos[4];
  vec4 lightColor[4];
  vec4 viewVector;
  float time;
} ubo;

layout(location = 0) in vec3 fragColor;
layout(location = 1) flat in vec3 fragNormal;

layout(location = 0) out vec4 outCol0;
layout(location = 1) out vec4 outCol1;

void main() {
  vec3 normal = normalize(fragNormal);
  float metallic = 0.3;
  float roughness = 0.5;

  outCol0 = vec4(normal, fragColor.x);
  outCol1 = vec4(fragColor.yz, metallic, roughness);
}