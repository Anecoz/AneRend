#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : enable

struct Renderable
{
  mat4 transform;
  vec4 bounds;
  vec4 tint;
  uint firstMeshId;
  uint numMeshIds;
  uint visible;
};

struct Light 
{
  vec4 worldPos; // w is range
  vec4 color; // w is enabled or not
};

struct Cluster
{
  vec4 minVs;
  vec4 maxVs;
};

struct MeshMaterialInfo
{
  int metallicTexIndex;
  int roughnessTexIndex;
  int normalTexIndex;
  int albedoTexIndex;
  int metallicRoughnessTexIndex;
  int emissiveTexIndex;
  float baseColFacR;
  float baseColFacG;
  float baseColFacB;
  float emissiveFactorR;
  float emissiveFactorG;
  float emissiveFactorB;
  //vec4 baseColFactor;
};

struct MeshInfo
{
  uint vertexOffset; // in "number" of vertices, not bytes
  uint indexOffset; // in "number" of indices, not bytes
  uint64_t blasRef;
};

struct PackedVertex
{
  vec4 d0;
  vec4 d1;
  vec4 d2;
  vec4 d3;
};

struct Vertex
{
  vec3 pos;
  vec3 color;
  vec3 normal;
  vec4 tangent;
  vec2 uv;
};

struct IrradianceProbe {
  vec4 pos;
  vec4 irr0; //n.r, n.g, n.b, sum-of-weights
  vec4 irr1; //e.r, e.g, e.b, sum-of-weights
  vec4 irr2; //s.r, s.g, s.b, sum-of-weights
  vec4 irr3; //w.r, w.g, w.b, sum-of-weights
  vec4 irr4; //u.r, u.g, u.b, sum-of-weights
  vec4 irr5; //d.r, d.g, d.b, sum-of-weights
};

struct SurfaceData
{
  vec3 normal;
  vec3 color;
  float metallic;
  float roughness;
  vec3 emissive;
};

Vertex unpackVertex(PackedVertex packedVertex)
{
  Vertex outVtx;
  outVtx.pos = packedVertex.d0.xyz;
  outVtx.color = vec3(packedVertex.d0.w, packedVertex.d1.xy);
  outVtx.normal = vec3(packedVertex.d1.zw, packedVertex.d2.x);
  outVtx.tangent = vec4(packedVertex.d2.yzw, packedVertex.d3.x);
  outVtx.uv = vec2(packedVertex.d3.yz);
  return outVtx;
}

layout(set = 0, binding = 1) uniform sampler2D windForceTex;

layout(std430, set = 0, binding = 2) buffer RenderableBuffer {
  Renderable renderables[];
} renderableBuffer;

layout(std430, set = 0, binding = 3) readonly buffer LightBuffer {
  Light lights[];
} lightBuffer;

layout(std430, set = 0, binding = 4) readonly buffer ClusterBuffer {
  Cluster clusters[];
} clusterBuffer;

layout(std430, set = 0, binding = 5) readonly buffer MeshMaterialBuffer {
  MeshMaterialInfo infos[];
} materialBuffer;

layout(std430, set = 0, binding = 6) readonly buffer IndexBuffer {
  uint indices[];
} indexBuffer;

layout(std430, set = 0, binding = 7) readonly buffer VertexBuffer {
  PackedVertex vertices[];
} vertexBuffer;

layout(std430, set = 0, binding = 8) readonly buffer MeshBuffer {
  MeshInfo meshes[];
} meshBuffer;

//layout(set = 0, binding = 9) uniform accelerationStructureEXT tlas;

/*layout(std430, set = 0, binding = 10) buffer IrradianceProbeBuffer {
  IrradianceProbe probes[];
} irradianceProbeBuffer;*/

layout(set = 0, binding = 10) uniform sampler2D textures[];

SurfaceData getSurfaceDataFromMat(MeshMaterialInfo matInfo, vec2 uv, vec3 inNormal, mat3 inTBN, vec3 inTangent, vec3 inColor)
{
  SurfaceData outData;

  outData.normal = inNormal;
  outData.metallic = 0.1;
  outData.roughness = 0.9;
  outData.color = inColor;
  outData.emissive = vec3(0.0);

  if (matInfo.metallicTexIndex != -1) {
    outData.metallic = texture(textures[nonuniformEXT(matInfo.metallicTexIndex)], uv).r;
  }
  if (matInfo.roughnessTexIndex != -1) {
    outData.roughness = texture(textures[nonuniformEXT(matInfo.roughnessTexIndex)], uv).r;
  }
  if (matInfo.albedoTexIndex != -1) {
    vec4 samp = texture(textures[nonuniformEXT(matInfo.albedoTexIndex)], uv);
    /*if (samp.a < 0.1) {
      discard;
    }*/
    outData.color = vec3(matInfo.baseColFacR * samp.r, matInfo.baseColFacG * samp.g, matInfo.baseColFacB * samp.b);
    //color = samp.rgb;
  }
  if (matInfo.metallicRoughnessTexIndex != -1) {
    vec4 samp = texture(textures[nonuniformEXT(matInfo.metallicRoughnessTexIndex)], uv);
    outData.metallic = samp.b;
    outData.roughness = samp.g;
  }
  if (matInfo.normalTexIndex != -1 && inTangent.xyz != vec3(0.0f)) {
    outData.normal = normalize(texture(textures[nonuniformEXT(matInfo.normalTexIndex)], uv).rgb * 2.0 - 1.0);
    outData.normal = normalize(inTBN * outData.normal);
  }
  if (matInfo.emissiveTexIndex != -1) {
    outData.emissive = texture(textures[nonuniformEXT(matInfo.emissiveTexIndex)], uv).rgb;
    outData.emissive.r *= matInfo.emissiveFactorR;
    outData.emissive.g *= matInfo.emissiveFactorG;
    outData.emissive.b *= matInfo.emissiveFactorB;
  }

  return outData;
}

// Converts a color from sRGB gamma to linear light gamma
vec4 toLinear(vec4 sRGB)
{
  bvec3 cutoff = lessThan(sRGB.rgb, vec3(0.04045));
  vec3 higher = pow((sRGB.rgb + vec3(0.055)) / vec3(1.055), vec3(2.4));
  vec3 lower = sRGB.rgb / vec3(12.92);

  return vec4(mix(higher, lower, cutoff), sRGB.a);
}

bool cullBitSet(Renderable rend)
{
  return bitfieldExtract(rend.visible, 1, 1) > 0;
}

uint setCullBit(Renderable rend, bool val)
{
  uint insert = val ? 1 : 0;
  return bitfieldInsert(rend.visible, insert, 1, 1);
}

bool visibleBitSet(Renderable rend)
{
  return bitfieldExtract(rend.visible, 0, 1) > 0;
}

uint setVisibleBit(Renderable rend, bool val)
{
  uint insert = val ? 1 : 0;
  return bitfieldInsert(rend.visible, insert, 0, 1);
}