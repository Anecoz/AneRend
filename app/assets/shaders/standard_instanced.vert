#version 450

layout(binding = 0) uniform UniformBufferObject {
  mat4 view;
  mat4 proj;
  mat4 shadowMatrix[24];
  vec4 cameraPos;
  vec4 lightDir;
  vec4 lightPos[4];
  vec4 lightColor[4];
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in mat4 model;

layout(location = 0) out vec3 fragColor;
layout(location = 1) flat out vec3 fragNormal;
layout(location = 2) out vec3 fragPosition;
layout(location = 3) out vec4 fragShadowPos;

void main() {
  fragPosition = vec3(model * vec4(inPosition, 1.0));
  fragShadowPos = ubo.shadowMatrix[0] * vec4(fragPosition, 1.0);

  gl_Position = ubo.proj * ubo.view * model * vec4(inPosition, 1.0);
  fragColor = inColor;
  fragNormal = mat3(model) * inNormal;
}