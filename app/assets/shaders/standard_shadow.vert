#version 450

layout(binding = 0) uniform UniformBufferObject {
  mat4 shadowMatrix;
} ubo;

struct Renderable
{
  mat4 transform;
  vec4 bounds;
  uint meshId;
};

layout(std430, set = 0, binding = 1) readonly buffer RenderableBuffer {
  Renderable renderables[];
} renderableBuffer;

layout(std430, set = 0, binding = 2) buffer TranslationBuffer {
  uint ids[];
} translationBuffer;

layout(push_constant) uniform constants {
  mat4 shadowMatrix;
} pushConstants;

layout(location = 0) in vec3 inPosition;

void main() {
  uint renderableId = translationBuffer.ids[gl_InstanceIndex];
  mat4 model = renderableBuffer.renderables[renderableId].transform;
  gl_Position = pushConstants.shadowMatrix * model * vec4(inPosition, 1.0);
}