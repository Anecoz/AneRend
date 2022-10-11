#version 450

layout(binding = 0) uniform UniformBufferObject {
  mat4 view;
  mat4 proj;
  mat4 shadowMatrix;
  vec4 cameraPos;
  vec4 lightDir;
} ubo;

layout(binding = 1) uniform sampler2D shadowMap;

layout(location = 0) in vec3 fragColor;
layout(location = 1) flat in vec3 fragNormal;
layout(location = 2) in vec3 fragPosition;
layout(location = 3) in vec4 fragShadowPos;

layout(location = 0) out vec4 outColor;

float inShadow() {
  vec3 projCoords = fragShadowPos.xyz / fragShadowPos.w;

  if (projCoords.x > 1.0 || projCoords.x < -1.0 ||
      projCoords.z > 1.0 || projCoords.z < 0.0 ||
      projCoords.y > 1.0 || projCoords.y < -1.0) {
    return 0.0;
  }

  vec2 shadowMapCoord = projCoords.xy * 0.5 + 0.5;

  float depth = projCoords.z;
  int stepsPerAxis = 8;
  float total = 0.0;
  for (int x = 0; x < stepsPerAxis; ++x) {
    for (int y = 0; y < stepsPerAxis; ++y) {
      float sampledDepth = texture(shadowMap, vec2(shadowMapCoord + vec2(0.0005 * x, 0.0005 * y))).r;
      if (depth > sampledDepth) {
        total += 1.0;
      }
    }
  }

  return total / float(stepsPerAxis * stepsPerAxis);
}

void main() {
  vec3 lightD = -ubo.lightDir.xyz;
  vec3 ambientColor = vec3(1.0, 1.0, 1.0);
  vec3 diffuseColor = fragColor;
  vec3 specColor = vec3(0.5, 0.5, 0.5);

  vec3 normal = normalize(fragNormal);
  vec3 lightDirNorm = normalize(lightD);

  float shadow = inShadow();

  // Ambient
  float ambientStrength = 0.1;
  vec3 ambient = ambientStrength * ambientColor;

  // Diffuse
  float diff = max(dot(normal, lightDirNorm), 0.2);
  vec3 diffuse = diff * diffuseColor;

  // Specular
  vec3 viewDir = normalize(vec3(ubo.cameraPos) - fragPosition);
  vec3 reflectDir = reflect(-lightDirNorm, normal);  
  float spec = pow(max(dot(viewDir, reflectDir), 0.0), 324);
  vec3 specular = 0.1 * spec * specColor;

  vec3 result = (ambient + diffuse + specular) * clamp(1.0 - shadow, 0.3, 1.0);

  outColor = vec4(result, 1.0);
}