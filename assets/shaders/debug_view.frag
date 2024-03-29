#version 450

#extension GL_EXT_nonuniform_qualifier : enable

layout(location = 0) in vec2 fragTexCoords;
layout(location = 0) out vec4 outColor;

layout(push_constant) uniform constants {
  uint samplerId;
  float lod;
} pushConstants;

layout(set = 1, binding = 0) uniform sampler2D textures[];

void main() {
  vec4 val = textureLod(textures[pushConstants.samplerId], fragTexCoords, pushConstants.lod);

  outColor = vec4(val);
}
