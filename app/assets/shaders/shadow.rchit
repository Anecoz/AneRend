#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_GOOGLE_include_directive : enable

#include "bindless.glsl"

struct hitPayload
{
  float hitValue;
};

layout(location = 0) rayPayloadInEXT hitPayload prd;
//hitAttributeEXT vec2 bary;

void main()
{
  // Attempt to get the albedo of the hit mesh
  /*MeshInfo meshInfo = meshBuffer.meshes[gl_InstanceCustomIndexEXT];
  //MeshMaterialInfo matInfo = materialBuffer.infos[gl_InstanceCustomIndexEXT];

  uint i0 = indexBuffer.indices[meshInfo.indexOffset + 3 * gl_PrimitiveID + 0];
  uint i1 = indexBuffer.indices[meshInfo.indexOffset + 3 * gl_PrimitiveID + 1];
  uint i2 = indexBuffer.indices[meshInfo.indexOffset + 3 * gl_PrimitiveID + 2];

  Vertex v0 = vertexBuffer.vertices[meshInfo.vertexOffset + i0];
  Vertex v1 = vertexBuffer.vertices[meshInfo.vertexOffset + i1];
  Vertex v2 = vertexBuffer.vertices[meshInfo.vertexOffset + i2];

  const vec3 barycentrics = vec3(1.0f - bary.x - bary.y, bary.x, bary.y);
  vec3 color = v0.color.xyz * barycentrics.x + v1.color.xyz * barycentrics.y + v2.color.xyz * barycentrics.z;
  vec3 pos = v0.pos.xyz * barycentrics.x + v1.pos.xyz * barycentrics.y + v2.pos.xyz * barycentrics.z;

  // Just some weird test
  prd.hitValue = v0.color.xyz;*/

  prd.hitValue = 0.0f;
}