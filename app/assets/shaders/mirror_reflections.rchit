#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_nonuniform_qualifier : enable

#include "bindless.glsl"
#include "bindless_tlas.glsl"
#include "octahedron_helpers.glsl"
#include "pbr_light.glsl"

struct hitPayload
{
  vec3 irradiance;
  float dist;
  float shadow;
};

layout(location = 0) rayPayloadInEXT hitPayload payload;
layout(location = 1) rayPayloadEXT hitPayload shadowPayload;
hitAttributeEXT vec2 bary;

layout(set = 1, binding = 3) uniform sampler2D probeTex;

void main()
{
  // Find the irradiance of this particular ray hit and load it in the payload.
  MeshInfo meshInfo = meshBuffer.meshes[gl_InstanceCustomIndexEXT];

  uint i0 = indexBuffer.indices[meshInfo.indexOffset + 3 * gl_PrimitiveID + 0];
  uint i1 = indexBuffer.indices[meshInfo.indexOffset + 3 * gl_PrimitiveID + 1];
  uint i2 = indexBuffer.indices[meshInfo.indexOffset + 3 * gl_PrimitiveID + 2];

  Vertex v0 = unpackVertex(vertexBuffer.vertices[meshInfo.vertexOffset + i0]);
  Vertex v1 = unpackVertex(vertexBuffer.vertices[meshInfo.vertexOffset + i1]);
  Vertex v2 = unpackVertex(vertexBuffer.vertices[meshInfo.vertexOffset + i2]);

  const vec3 barycentrics = vec3(1.0f - bary.x - bary.y, bary.x, bary.y);
  vec3 pos = v0.pos.xyz * barycentrics.x + v1.pos.xyz * barycentrics.y + v2.pos.xyz * barycentrics.z;
  pos = vec3(gl_ObjectToWorldEXT * vec4(pos, 1.0));

  // But first, send a shadow ray to check if we are in direct light.
  shadowPayload.shadow = 1.0;
  vec3 shadowDir = normalize(-ubo.lightDir.xyz);

  uint  rayFlags = 
    gl_RayFlagsOpaqueEXT |   // All geometry is considered opaque
    gl_RayFlagsTerminateOnFirstHitEXT | // We don't care about finding _closest_ hit, just if _something_ is hit
    gl_RayFlagsSkipClosestHitShaderEXT; // Skip running the closest hit shader at the end, since miss shader is enough
  float tMin     = 0.1;
  float tMax     = 50.0;

  traceRayEXT(tlas, // acceleration structure
          rayFlags,       // rayFlags
          0xFF,           // cullMask
          0,              // sbtRecordOffset
          0,              // sbtRecordStride
          0,              // missIndex
          pos,            // ray origin
          tMin,           // ray min range
          shadowDir,      // ray direction
          tMax,           // ray max range
          1               // payload (location = 1)
  );

  //payload.irradiance = shadowPayload.shadow < 0.5 ? vec3(0.0) : vec3(1.0);

  // proceed to calculate our radiance
  MeshMaterialInfo matInfo = materialBuffer.infos[gl_InstanceCustomIndexEXT];
  vec2 uv = v0.uv.xy * barycentrics.x + v1.uv.xy * barycentrics.y + v2.uv.xy * barycentrics.z;
  vec3 normal = normalize(v0.normal.xyz * barycentrics.x + v1.normal.xyz * barycentrics.y + v2.normal.xyz * barycentrics.z);
  normal = normalize(vec3(normal.xyz * gl_WorldToObjectEXT));
  vec3 color = v0.color.xyz * barycentrics.x + v1.color.xyz * barycentrics.y + v2.color.xyz * barycentrics.z;
  color = toLinear(vec4(color, 1.0)).rgb;
  //vec3 tangent = v0.tangent.xyz * barycentrics.x + v1.tangent.xyz * barycentrics.y + v2.tangent.xyz * barycentrics.z;

  /*vec3 bitangentL = cross(normal, tangent);
  vec3 T = normalize(mat3(gl_ObjectToWorldEXT) * tangent);
  vec3 B = normalize(mat3(gl_ObjectToWorldEXT) * bitangentL);
  mat3 TBN = mat3(T, B, normal);*/

  //SurfaceData surfData = getSurfaceDataFromMat(matInfo, uv, normal, TBN, tangent, color);
  SurfaceData surfData = getSurfaceDataFromMat(matInfo, uv, normal, mat3(0.0), vec3(0.0), color);

  Light dummyLight;

  payload.dist = distance(pos, gl_WorldRayOriginEXT);

  if (shadowPayload.shadow < 0.5) {
    // Only calculate direct lighting if we're not in shadow
    payload.irradiance = calcLight(
      surfData.normal,
      surfData.color,
      surfData.metallic,
      surfData.roughness,
      pos,
      gl_WorldRayOriginEXT,
      dummyLight,
      1,
      true);
  }

  // Always calculate indirect light
  if (ubo.ddgiEnabled == 1) {
    payload.irradiance += calcIndirectDiffuseLight(
      surfData.normal,
      surfData.color,
      surfData.metallic,
      surfData.roughness,
      pos,
      gl_ObjectRayOriginEXT,
      probeTex);
  }

  payload.irradiance += surfData.emissive;

  //payload.irradiance = vec3(uv.x, uv.y, 0.0);
  //payload.irradiance = normal;
  //payload.irradiance = color;
  //payload.irradiance = pos;
}