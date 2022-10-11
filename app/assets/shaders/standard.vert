#version 450

layout(binding = 0) uniform UniformBufferObject {
  mat4 view;
  mat4 proj;
  mat4 shadowMatrix;
  vec4 cameraPos;
  vec4 lightDir;
} ubo;

layout(push_constant) uniform constants {
  mat4 model;
} pushConstants;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 inNormal;

layout(location = 0) out vec3 fragColor;
layout(location = 1) flat out vec3 fragNormal;
layout(location = 2) out vec3 fragPosition;
layout(location = 3) out vec4 fragShadowPos;

void main() {
  fragPosition = vec3(pushConstants.model * vec4(inPosition, 1.0));
  fragShadowPos = ubo.shadowMatrix * vec4(fragPosition, 1.0);
  gl_Position = ubo.proj * ubo.view * pushConstants.model * vec4(inPosition, 1.0);
  fragColor = inColor;
  fragNormal = mat3(pushConstants.model) * inNormal;
}