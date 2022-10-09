#version 450

layout(binding = 0) uniform UniformBufferObject {
  mat4 view;
  mat4 proj;
  mat4 shadowMatrix;
  vec4 cameraPos;
} ubo;

layout(binding = 1) uniform sampler2D shadowMap;

layout(location = 0) in vec3 fragColor;
layout(location = 1) flat in vec3 fragNormal;
layout(location = 2) in vec3 fragPosition;
layout(location = 3) in vec4 fragShadowPos;

layout(location = 0) out vec4 outColor;

int inShadow() {
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
  vec3 lightDir = vec3(0.7, 0.7, 0.7);

  vec3 ambientColor = vec3(1.0, 1.0, 1.0);
  vec3 diffuseColor = fragColor;
  vec3 specColor = vec3(0.5, 0.5, 0.5);

  vec3 normal = normalize(fragNormal);
  vec3 lightDirNorm = normalize(lightDir);

  int shadow = inShadow();

  // Ambient
  float ambientStrength = 0.1;
  vec3 ambient = ambientStrength * ambientColor;

  if (shadow == 1) {
    ambient *= 0.5;
  }

  // Diffuse
  float diff = max(dot(normal, lightDirNorm), 0.2);
  vec3 diffuse = diff * diffuseColor;

  if (shadow == 1) {
    diffuse *= 0.2;
  }

  // Specular
  vec3 viewDir = normalize(vec3(ubo.cameraPos) - fragPosition);
  vec3 reflectDir = reflect(-lightDirNorm, normal);  
  float spec = pow(max(dot(viewDir, reflectDir), 0.0), 324);
  vec3 specular = 0.1 * spec * specColor;

  vec3 result = (ambient + diffuse + specular);

  outColor = vec4(result, 1.0);
}