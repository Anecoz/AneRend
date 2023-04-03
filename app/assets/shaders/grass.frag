#version 450

layout(set = 0, binding = 0) uniform UniformBufferObject {
  mat4 view;
  mat4 proj;
  mat4 directionalShadowMatrixProj;
  mat4 directionalShadowMatrixView;
  mat4 shadowMatrix[24];
  vec4 cameraPos;
  vec4 lightDir;
  vec4 lightPos[4];
  vec4 lightColor[4];
  vec4 viewVector;
  float time;
} ubo;

layout(set = 1, binding = 1) uniform sampler2D shadowMap;

layout(location = 0) in vec3 fragNormal;
layout(location = 1) in float fragT;
layout(location = 2) in vec3 fragPosition;
layout(location = 3) in flat float fragBladeHash;
layout(location = 4) in vec4 fragShadowPos;

layout(location = 0) out vec4 outColor;

const vec3 col0 = vec3(0.0, 154.0/255.0, 23.0/255.0);
const vec3 col1 = vec3(65.0/255.0, 152.0/255.0, 10.0/255.0);

float inShadow() {
  vec3 projCoords = fragShadowPos.xyz / fragShadowPos.w;

  if (projCoords.x > 1.0 || projCoords.x < -1.0 ||
      projCoords.z > 1.0 || projCoords.z < -1.0 ||
      projCoords.y > 1.0 || projCoords.y < -1.0) {
    return 0;
  }

  vec2 shadowMapCoord = projCoords.xy * 0.5 + 0.5;

  float depth = projCoords.z;
  float sampledDepth = texture(shadowMap, shadowMapCoord.xy).r;
  if (depth > sampledDepth) {
    return 1;
  }
  return 0;
}

void main() {
  vec3 ambientColor = vec3(1.0, 1.0, 1.0);
  vec3 diffuseColor = fragBladeHash * col0 + (1.0 - fragBladeHash) * col1;
  vec3 specColor = vec3(1.0, 1.0, 1.0);

  diffuseColor *= fragT;

  vec3 normal = normalize(fragNormal);
  float shadow = inShadow();

  // Ambient
  float ambientStrength = 0.3;
  vec3 ambient = ambientStrength * ambientColor;

  // Diffuse
  float diff = max(dot(normal, ubo.lightDir.xyz), 0.0);
  vec3 diffuse = diff * vec3(1.0, 1.0, 1.0);

  // Specular 
  vec3 lightDirNorm = normalize(ubo.lightDir.xyz);
  vec3 viewDir = normalize(vec3(ubo.cameraPos) - fragPosition);
  vec3 reflectDir = reflect(lightDirNorm, normal);  
  float spec = pow(max(dot(viewDir, reflectDir), 0.0), 16);
  vec3 specular = 1.0 * spec * specColor;

  vec3 result = (ambient + diffuse + specular) * diffuseColor * clamp(1.0 - shadow, 0.3, 1.0);
  outColor = vec4(result, 1.0);
}