#version 450

#extension GL_EXT_nonuniform_qualifier : enable

layout(set = 0, binding = 0) uniform UniformBufferObject {
  mat4 view;
  mat4 proj;
  mat4 invProj;
  mat4 invViewProj;
  mat4 directionalShadowMatrixProj;
  mat4 directionalShadowMatrixView; 
  mat4 shadowMatrix[24];
  vec4 cameraPos;
  vec4 lightDir;
  vec4 viewVector;
  float time;
  uint screenWidth;
  uint screenHeight;
} ubo;

layout(set = 0, binding = 5) uniform sampler2D textures[];

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec3 fragPos;
layout(location = 3) in vec2 fragUV;
layout(location = 4) flat in int fragMetallicIndex;
layout(location = 5) flat in int fragRoughnessIndex;
layout(location = 6) flat in int fragAlbedoIndex;
layout(location = 7) flat in int fragNormalIndex;
layout(location = 8) flat in mat3 fragModelMtx;

layout(location = 0) out vec4 outCol0;
layout(location = 1) out vec4 outCol1;
//layout(location = 2) out vec4 outCol2;

void main() {
  vec3 normal = normalize(fragNormal);
  float metallic = 0.3;
  float roughness = 0.5;
  vec3 color = fragColor;

  vec2 uv = vec2(fragUV.x, 1.0 - fragUV.y);

  if (fragMetallicIndex != -1) {
    metallic = texture(textures[nonuniformEXT(fragMetallicIndex)], uv).r;
  }
  if (fragRoughnessIndex != -1) {
    roughness = texture(textures[nonuniformEXT(fragRoughnessIndex)], uv).r;
  }
  if (fragAlbedoIndex != -1) {
    color = texture(textures[nonuniformEXT(fragAlbedoIndex)], uv).rgb;
  }
  /*if (fragNormalIndex != -1) {
    normal = normalize(fragModelMtx * (texture(textures[nonuniformEXT(fragNormalIndex)], uv).rgb * 2.0 - 1.0));
  }*/

  outCol0 = vec4(normal, color.x);
  outCol1 = vec4(color.yz, metallic, roughness);
  //outCol2 = vec4(fragPos, 0.0);
}