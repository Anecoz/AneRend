#version 450

layout(binding = 0) uniform sampler2D inputTex;

layout(location = 0) in vec2 fragTexCoords;

layout(location = 0) out vec4 outColor;

void main() {
  vec4 val = texture(inputTex, fragTexCoords);
  outColor = vec4(val.b, val.g, val.r, 1.0);
}