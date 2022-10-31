#version 450

layout(location = 0) in vec3 fragPositionWorld;

layout(push_constant) uniform constants {
  mat4 shadowMatrix;
  vec4 camPosition;
} pushConstants;

void main() {
  gl_FragDepth = distance(pushConstants.camPosition.xyz, fragPositionWorld) / 50.0;
}