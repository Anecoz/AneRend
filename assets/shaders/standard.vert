#version 450

#extension GL_GOOGLE_include_directive : enable
#include "scene_ubo.glsl"
#include "bindless.glsl"

struct TranslationId
{
  uint renderableIndex;
  uint meshOffset;
};

layout(std430, set = 1, binding = 0) buffer TranslationBuffer {
  TranslationId ids[];
} translationBuffer;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec2 inUV;
layout(location = 4) in vec4 inTangent;
layout(location = 5) in ivec4 joints;
layout(location = 6) in vec4 jointWeights;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec3 fragPos;
layout(location = 3) out vec2 fragUV;
layout(location = 4) out flat uint fragMaterialIdx;
layout(location = 5) out vec3 fragTangent;
layout(location = 6) out mat3 fragTBN;

void main() {
  uint renderableIndex = translationBuffer.ids[gl_InstanceIndex].renderableIndex;
  mat4 model = renderableBuffer.renderables[renderableIndex].transform;
  uint skeletonOffset = renderableBuffer.renderables[renderableIndex].skeletonOffset;

  vec3 pos = inPosition;
  vec3 normal = inNormal;
  if (joints[0] != -1) {
    pos = vec3(
      jointWeights[0] * skeletonBuffer.joints[skeletonOffset + joints[0]] * vec4(inPosition, 1.0) +
      jointWeights[1] * skeletonBuffer.joints[skeletonOffset + joints[1]] * vec4(inPosition, 1.0) +
      jointWeights[2] * skeletonBuffer.joints[skeletonOffset + joints[2]] * vec4(inPosition, 1.0) +
      jointWeights[3] * skeletonBuffer.joints[skeletonOffset + joints[3]] * vec4(inPosition, 1.0));

    normal = vec3(
      jointWeights[0] * skeletonBuffer.joints[skeletonOffset + joints[0]] * vec4(inNormal, 0.0) +
      jointWeights[1] * skeletonBuffer.joints[skeletonOffset + joints[1]] * vec4(inNormal, 0.0) +
      jointWeights[2] * skeletonBuffer.joints[skeletonOffset + joints[2]] * vec4(inNormal, 0.0) +
      jointWeights[3] * skeletonBuffer.joints[skeletonOffset + joints[3]] * vec4(inNormal, 0.0)
    );
  }

  gl_Position = ubo.proj * ubo.view * model * vec4(pos, 1.0);
  fragColor = toLinear(vec4(inColor, 1.0)).rgb * renderableBuffer.renderables[renderableIndex].tint.rgb;
  fragNormal =  normalize(mat3(model) * normal);
  fragPos = (model * vec4(inPosition, 1.0)).xyz;
  fragUV = inUV;

  uint meshOffset = translationBuffer.ids[gl_InstanceIndex].meshOffset;
  uint firstMeshOffset = renderableBuffer.renderables[renderableIndex].modelOffset;
  uint materialOffset = meshOffset - firstMeshOffset;
  fragMaterialIdx = renderableBuffer.renderables[renderableIndex].firstMaterialIndex + materialOffset;

  vec3 bitangentL = cross(normal, inTangent.xyz);
  vec3 T = normalize(mat3(model) * inTangent.xyz);
  vec3 B = normalize(mat3(model) * bitangentL);
  vec3 N = fragNormal;
  fragTBN = mat3(T, B, N);
  fragTangent = inTangent.xyz;

  // If ray tracing is enabled, we need to write our verts to the dynamic mesh buffer (for creating BLASes)
  // But only if we are animated!
  uint dynamicModelOffset = renderableBuffer.renderables[renderableIndex].dynamicModelOffset;
  if (ubo.rtEnabled == 1 && dynamicModelOffset != 0) {
    uint meshIndex = rendModelBuffer.indices[meshOffset];
    uint dynamicIndex = rendModelBuffer.indices[materialOffset + dynamicModelOffset];

    MeshInfo dynamicMeshInfo = meshBuffer.meshes[dynamicIndex];
    MeshInfo meshInfo = meshBuffer.meshes[meshIndex];

    Vertex v;
    v.pos = pos;
    v.color = inColor;
    v.normal = normal;
    v.tangent = inTangent;
    v.jointIds = joints;
    v.jointWeights = jointWeights;
    v.uv = inUV;
    PackedVertex packedV = packVertex(v);

    // Note: gl_VertexIndex includes any vertex offset specified when calling the draw cmd.
    //       i.e., we need to subtract meshInfo.vertexOffset from the index.
    vertexBuffer.vertices[dynamicMeshInfo.vertexOffset + gl_VertexIndex - meshInfo.vertexOffset] = packedV;
  }
}