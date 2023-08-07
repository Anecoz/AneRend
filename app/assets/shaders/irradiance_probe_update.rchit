#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_nonuniform_qualifier : enable

#include "bindless.glsl"
#include "bindless_tlas.glsl"
#include "scene_ubo.glsl"
#include "octahedron_helpers.glsl"
#include "pbr_light.glsl"

struct hitPayload
{
  vec3 irradiance;
  vec3 hitPos;
  float shadow;
};

layout(location = 0) rayPayloadInEXT hitPayload payload;
layout(location = 1) rayPayloadEXT hitPayload shadowPayload;
hitAttributeEXT vec2 bary;

layout(set = 1, binding = 2) uniform sampler2D probeTex;

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
  float tMax     = 100.0;

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
  vec3 tangent = v0.tangent.xyz * barycentrics.x + v1.tangent.xyz * barycentrics.y + v2.tangent.xyz * barycentrics.z;

  float metallic = 0.3;
  float roughness = 0.5;

  if (matInfo.metallicTexIndex != -1) {
    metallic = texture(textures[nonuniformEXT(matInfo.metallicTexIndex)], uv).r;
  }
  if (matInfo.roughnessTexIndex != -1) {
    roughness = texture(textures[nonuniformEXT(matInfo.roughnessTexIndex)], uv).r;
  }
  if (matInfo.albedoTexIndex != -1) {
    vec4 samp = texture(textures[nonuniformEXT(matInfo.albedoTexIndex)], uv);
    /*if (samp.a < 0.1) {
      discard;
    }*/
    color = samp.rgb;
  }
  if (matInfo.metallicRoughnessTexIndex != -1) {
    vec4 samp = texture(textures[nonuniformEXT(matInfo.metallicRoughnessTexIndex)], uv);
    metallic = samp.b;
    roughness = samp.g;
  }
  if (matInfo.normalTexIndex != -1 && tangent != vec3(0.0f)) {
    vec3 bitangentL = cross(normal, tangent);
    vec3 T = normalize(mat3(gl_ObjectToWorldEXT) * tangent);
    vec3 B = normalize(mat3(gl_ObjectToWorldEXT) * bitangentL);
    //vec3 N = normalize(mat3(gl_ObjectToWorldEXT) * normal);
    mat3 TBN = mat3(T, B, normal);

    normal = normalize(texture(textures[nonuniformEXT(matInfo.normalTexIndex)], uv).rgb * 2.0 - 1.0);
    normal = normalize(TBN * normal);
  }

  Light dummyLight;

  payload.hitPos = pos;

  if (shadowPayload.shadow < 0.5) {
    // Only calculate direct lighting if we're not in shadow
    payload.irradiance = calcLight(
      normal,
      color,
      metallic,
      roughness,
      pos,
      dummyLight,
      1);
  }

  // Always calculate indirect light
  payload.irradiance += calcIndirectDiffuseLight(
    normal,
    color,
    metallic,
    roughness,
    pos,
    gl_ObjectRayOriginEXT,
    probeTex);
}