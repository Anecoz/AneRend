#version 450

#extension GL_GOOGLE_include_directive : enable
#include "scene_ubo.glsl"

layout(location = 0) in vec3 fragNormal;
layout(location = 1) in float fragT;
layout(location = 2) in vec3 fragPos;
layout(location = 3) in flat float fragBladeHash;

layout(location = 0) out vec4 outColor0;
layout(location = 1) out vec4 outColor1;
layout(location = 2) out vec4 outColor2;

const vec3 col0 = vec3(0.0, 154.0/255.0, 23.0/255.0) * 0.5;
const vec3 col1 = vec3(65.0/255.0, 152.0/255.0, 10.0/255.0) * 0.5;

void main() {
  vec3 diffuseColor = fragBladeHash * col0 + (1.0 - fragBladeHash) * col1 * fragT;

  vec3 normal = -normalize(fragNormal);
  float metallic = 0.0;
  float roughness = 0.8;

  outColor0 = vec4(normal, diffuseColor.x);
  outColor1 = vec4(diffuseColor.yz, metallic, roughness);
  outColor2 = vec4(0.0);

}