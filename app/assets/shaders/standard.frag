#version 450

layout(binding = 0) uniform UniformBufferObject {
  mat4 view;
  mat4 proj;
  mat4 shadowMatrix[24];
  vec4 cameraPos;
  vec4 lightDir;
  vec4 lightPos[4];
  vec4 lightColor[4];
} ubo;

layout(binding = 1) uniform samplerCube shadowMap[4];

layout(location = 0) in vec3 fragColor;
layout(location = 1) flat in vec3 fragNormal;
layout(location = 2) in vec3 fragPosition;

layout(location = 0) out vec4 outColor;

#define NUM_LIGHTS 4

float inShadow() {
  float total = 0.0;
  for (int i = 0; i < NUM_LIGHTS; ++i) {
    vec3 lightVec = fragPosition - ubo.lightPos[i].xyz;

    float sampledDepth = texture(shadowMap[i], lightVec).r;

    float testDepth = length(lightVec) / 50.0;
    if (sampledDepth < testDepth) {
      total += 1.0;
    }
  }

  return total/NUM_LIGHTS;
}

void main() {
  vec3 ambientColor = vec3(1.0, 1.0, 1.0);
  vec3 diffuseColor = fragColor;
  vec3 specColor = vec3(1.0, 1.0, 1.0);

  vec3 normal = normalize(fragNormal);

  //float shadow = inShadow();

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
  vec3 result = (ambient + diffuse) * diffuseColor;
  outColor = vec4(result, 1.0);
}