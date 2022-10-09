#version 450

layout(binding = 0) uniform UniformBufferObject {
  mat4 view;
  mat4 proj;
  mat4 shadowMatrix;
  vec4 cameraPos;
} ubo;

layout(binding = 1) uniform sampler2D shadowMap;

layout(location = 0) in vec2 fragTexCoords;

layout(location = 0) out vec4 outColor;

void main() {
  float val = texture(shadowMap, vec2(fragTexCoords.x, (1.0 - fragTexCoords.y))).r;
  outColor = vec4(val, val, val, 1.0);
}