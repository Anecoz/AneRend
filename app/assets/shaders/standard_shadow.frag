#version 450

layout(location = 0) in vec3 fragPositionWorld;

void main() {
  //gl_FragDepth = distance(pushConstants.camPosition.xyz, fragPositionWorld) / 50.0;
}