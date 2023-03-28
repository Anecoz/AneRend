#version 450

layout(location = 0) in vec3 fragPositionWorld;

layout(set = 0, binding = 0) uniform UniformBufferObject {
  mat4 view;
  mat4 proj;
  mat4 directionalShadowMatrix;
  mat4 shadowMatrix[24];
  vec4 cameraPos;
  vec4 lightDir;
  vec4 lightPos[4];
  vec4 lightColor[4];
  float time;
} ubo;

void main() {
  //gl_FragDepth = distance(pushConstants.camPosition.xyz, fragPositionWorld) / 50.0;
}