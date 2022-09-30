#version 450

layout(binding = 1) uniform sampler2D texSampler;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec3 fragPosition;

layout(location = 0) out vec4 outColor;

void main() {
  vec3 lightDir = vec3(0.7, 0.7, 0.7);
  vec3 cameraPos = vec3(5.0, 5.0, 5.0);

  vec3 ambientColor = vec3(1.0, 1.0, 1.0);
  vec3 diffuseColor = fragColor;
  vec3 specColor = vec3(0.5, 0.5, 0.5);

  vec3 normal = normalize(fragNormal);
  vec3 lightDirNorm = -normalize(lightDir);

  // Ambient
  float ambientStrength = 0.1;
  vec3 ambient = ambientStrength * ambientColor;

  // Diffuse
  float diff = max(dot(normal, lightDirNorm), 0.0);
  vec3 diffuse = diff * diffuseColor;

  // Specular
  vec3 viewDir = normalize(cameraPos - fragPosition);
  vec3 reflectDir = reflect(-lightDirNorm, normal);  
  float spec = pow(max(dot(viewDir, reflectDir), 0.0), 324);
  vec3 specular = 0.1 * spec * specColor;

  vec3 result = (ambient + diffuse + specular);

  outColor = vec4(result, 1.0);
}