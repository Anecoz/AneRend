#version 450

layout(set = 0, binding = 0) uniform UniformBufferObject {
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

struct Renderable
{
  mat4 transform;
  vec4 bounds;
  uint meshId;
  uint _visible;
};

layout(std430, set = 0, binding = 2) readonly buffer RenderableBuffer {
  Renderable renderables[];
} renderableBuffer;

layout(std430, set = 1, binding = 0) buffer TranslationBuffer {
  uint ids[];
} translationBuffer;

layout(location = 0) in vec3 inPosition;

layout(location = 0) out vec3 fragPositionWorld;

void main() {
  uint renderableId = translationBuffer.ids[gl_InstanceIndex];
  mat4 model = renderableBuffer.renderables[renderableId].transform;
  gl_Position = ubo.directionalShadowMatrix * model * vec4(inPosition, 1.0);
  fragPositionWorld = vec3(model * vec4(inPosition, 1.0));
}