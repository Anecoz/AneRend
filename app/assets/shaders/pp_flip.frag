#version 450

layout(binding = 0) uniform sampler2D inputTex;

layout(location = 0) in vec2 fragTexCoords;

layout(location = 0) out vec4 outColor;

void main() {
  vec4 val = texture(inputTex, vec2(fragTexCoords.x, 1.0 - fragTexCoords.y));
  outColor = vec4(val.r, val.g, val.b, 1.0);
}