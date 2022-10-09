#version 450

layout(binding = 0) uniform UniformBufferObject {
  mat4 shadowMatrix;
} ubo;

layout(push_constant) uniform constants {
  mat4 model;
} pushConstants;

layout(location = 0) in vec3 inPosition;

void main() {
  gl_Position = ubo.shadowMatrix * pushConstants.model * vec4(inPosition, 1.0);
}