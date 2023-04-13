#version 450

layout(set = 0, binding = 0) uniform UniformBufferObject {
  mat4 view;
  mat4 proj;
  mat4 invViewProj;
  mat4 directionalShadowMatrixProj;
  mat4 directionalShadowMatrixView;
  mat4 shadowMatrix[24];
  vec4 cameraPos;
  vec4 lightDir;
  vec4 viewVector;
  float time;
} ubo;

layout(set = 1, binding = 0) uniform sampler2D ssaoTex;

layout(location = 0) in vec2 fragTexCoords;

layout(location = 0) out vec4 outColor;

void main() {
  vec2 texelSize = 1.0 / vec2(textureSize(ssaoTex, 0));
    float result = 0.0;
    for (int x = -2; x < 2; ++x) {
      for (int y = -2; y < 2; ++y) {
        vec2 offset = vec2(float(x), float(y)) * texelSize;
        result += texture(ssaoTex, fragTexCoords + offset).r;
      }
    }
    outColor.r = result / (4.0 * 4.0);
}