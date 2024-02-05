#version 450

#extension GL_GOOGLE_include_directive : enable
#include "scene_ubo.glsl"

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;

layout(location = 0) out vec3 fragColor;

layout(push_constant) uniform constants {
  mat4 model;
  vec4 tint;
} pc;

void main() {
  gl_Position = ubo.proj * ubo.view * pc.model * vec4(inPosition, 1.0);
  fragColor = pc.tint.xyz * inColor;
}