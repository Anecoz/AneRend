#version 450

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

struct Renderable
{
  mat4 transform;
  vec4 bounds;
  vec4 tint;
  uint meshId;
  uint _visible;
  int metallicTexIndex;
  int roughnessTexIndex;
  int normalTexIndex;
  int albedoTexIndex;
};

layout(std430, set = 0, binding = 2) readonly buffer RenderableBuffer {
  Renderable renderables[];
} renderableBuffer;

layout(std430, set = 1, binding = 0) buffer TranslationBuffer {
  uint ids[];
} translationBuffer;

layout(push_constant) uniform constants {
  mat4 model;
} pushConstants;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec2 inUV;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec3 fragPos;
layout(location = 3) out vec2 fragUV;
layout(location = 4) flat out int fragMetallicIndex;
layout(location = 5) flat out int fragRoughnessIndex;
layout(location = 6) flat out int fragAlbedoIndex;
layout(location = 7) flat out int fragNormalIndex;
layout(location = 8) flat out mat3 fragModelMtx;

void main() {
  uint renderableId = translationBuffer.ids[gl_InstanceIndex];
  mat4 model = renderableBuffer.renderables[renderableId].transform;

  gl_Position = ubo.proj * ubo.view * model * vec4(inPosition, 1.0);
  fragColor = inColor * renderableBuffer.renderables[renderableId].tint.rgb;
  fragModelMtx = mat3(model);
  fragNormal = fragModelMtx * inNormal;
  fragPos = (model * vec4(inPosition, 1.0)).xyz;
  fragUV = inUV;
  fragMetallicIndex = renderableBuffer.renderables[renderableId].metallicTexIndex;
  fragRoughnessIndex = renderableBuffer.renderables[renderableId].roughnessTexIndex;
  fragAlbedoIndex = renderableBuffer.renderables[renderableId].albedoTexIndex;
  fragNormalIndex = renderableBuffer.renderables[renderableId].normalTexIndex;
}