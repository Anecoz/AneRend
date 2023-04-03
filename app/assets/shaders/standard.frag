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

layout(location = 0) in vec3 fragColor;
layout(location = 1) flat in vec3 fragNormal;
layout(location = 2) in vec3 fragPosition;
layout(location = 3) in vec4 fragShadowPos;

layout(location = 0) out vec4 outColor;

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
  vec3 diffuseColor = fragColor;
  vec3 specColor = vec3(1.0, 1.0, 1.0);

  vec3 normal = normalize(fragNormal);

  float shadow = inShadow();

  // Ambient
  float ambientStrength = 0.1;
  vec3 ambient = ambientStrength * ambientColor;

  // Per light
  /*float lightPower = 40.0;
  vec3 totalLight = vec3(0.0, 0.0, 0.0);
  for (int i = 0; i < NUM_LIGHTS; ++i) {
    vec3 lightDir =  ubo.lightPos[i].xyz - fragPosition;
    float dist = length(lightDir);
    dist = dist * dist;
    lightDir = normalize(lightDir);

    // Diffuse
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * diffuseColor * ubo.lightColor[i].xyz * lightPower / dist;*/

    // Specular
    /*vec3 viewDir = normalize(vec3(ubo.cameraPos) - fragPosition);
    vec3 reflectDir = reflect(-lightDirNorm, normal);  
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 324);
    vec3 specular = 0.1 * spec * specColor;*/

    /*vec3 result = ambient + diffuse * clamp(1.0 - shadow, 0.3, 1.0);
    totalLight = totalLight + result;
  }*/

  float diff = max(dot(normal, -ubo.lightDir.xyz), 0.0);
  vec3 diffuse = diff * vec3(1.0, 1.0, 1.0);
  vec3 result = (ambient + diffuse) * diffuseColor * clamp(1.0 - shadow, 0.3, 1.0);
  outColor = vec4(result, 1.0);
}