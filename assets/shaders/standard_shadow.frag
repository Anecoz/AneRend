#version 450

layout(location = 0) in vec3 fragPos;
layout(location = 1) flat in vec3 fragLightPos;
layout(location = 2) flat in float fragRange;

void main() {
  gl_FragDepth = distance(fragLightPos, fragPos) / fragRange;
}