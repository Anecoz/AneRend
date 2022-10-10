#version 450

layout(binding = 0) uniform UniformBufferObject {
  mat4 shadowMatrix;
} ubo;

layout(location = 0) in vec3 inPosition;

layout(location = 3) in mat4 model;

void main() {
  gl_Position = ubo.shadowMatrix * model * vec4(inPosition, 1.0);
}