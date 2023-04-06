#version 450

layout(set = 1, binding = 0) uniform sampler2D shadowMap;

layout(location = 0) in vec2 fragTexCoords;

layout(location = 0) out vec4 outColor;

void main() {
  vec4 val = texture(shadowMap, fragTexCoords);
  outColor = vec4(val);
}