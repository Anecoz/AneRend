#version 450

layout(binding = 0) uniform UniformBufferObject {
  mat4 view;
  mat4 proj;
  mat4 directionalShadowMatrix;
  mat4 shadowMatrix[24];
  vec4 cameraPos;
  vec4 lightDir;
  vec4 lightPos[4];
  vec4 lightColor[4];
  float time;
} ubo;

layout(location = 0) in vec3 fragNormal;

layout(location = 0) out vec4 outColor;

void main() {
  vec3 ambientColor = vec3(1.0, 1.0, 1.0);
  vec3 diffuseColor = vec3(0.0, 1.0, 0.0);
  vec3 specColor = vec3(1.0, 1.0, 1.0);

  vec3 normal = normalize(fragNormal);

  // Ambient
  float ambientStrength = 0.1;
  vec3 ambient = ambientStrength * ambientColor;

  float diff = max(dot(normal, -ubo.lightDir.xyz), 0.0);
  vec3 diffuse = diff * vec3(1.0, 1.0, 1.0);
  vec3 result = (ambient + diffuse) * diffuseColor;
  outColor = vec4(result, 1.0);
}