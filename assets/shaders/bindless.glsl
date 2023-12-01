#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : enable

struct Renderable
{
  mat4 transform;
  vec4 bounds;
  vec4 tint;
  uint modelOffset;
  uint numMeshes;
  uint skeletonOffset;
  uint visible;
  uint firstMaterialIndex;
  uint dynamicModelOffset; // Only for ray-tracing: First dynamic mesh id (numMeshId applies here aswell)
};

struct Light 
{
  vec4 worldPos; // w is range
  vec4 color; // w is enabled or not
};

struct PointLightShadowCube
{
  mat4 shadowMatrices[6]; // Front, back, up, down, right, left where "front" is +x
};

struct Cluster
{
  vec4 minVs;
  vec4 maxVs;
};

struct MaterialInfo
{
  vec4 baseColFac; // w unused
  vec4 emissive; // factor or RGB depending on if emissive texture present
  ivec4 bindlessIndices; // metallicRoughness, albedo, normal, emissive
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
  vec4 d4;
  ivec4 d5;
};

struct Vertex
{
  vec3 pos;
  vec3 color;
  vec3 normal;
  vec4 tangent;
  ivec4 jointIds;
  vec4 jointWeights;
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
  outVtx.jointWeights = vec4(packedVertex.d3.yzw, packedVertex.d4.x);
  outVtx.uv = vec2(packedVertex.d4.yz);
  outVtx.jointIds = ivec4(packedVertex.d5.x);
  return outVtx;
}

PackedVertex packVertex(Vertex v)
{
  PackedVertex outVtx;
  outVtx.d0 = vec4(v.pos, v.color.r);
  outVtx.d1 = vec4(v.color.gb, v.normal.xy);
  outVtx.d2 = vec4(v.normal.z, v.tangent.xyz);
  outVtx.d3 = vec4(v.tangent.w, v.jointWeights.xyz);
  outVtx.d4 = vec4(v.jointWeights.w, v.uv.xy, 0.0);
  outVtx.d5 = ivec4(v.jointIds);
  return outVtx;
}

layout(set = 0, binding = 1) uniform sampler2D windForceTex;

layout(std430, set = 0, binding = 2) buffer RenderableBuffer {
  Renderable renderables[];
} renderableBuffer;

layout(std430, set = 0, binding = 3) readonly buffer LightBuffer {
  Light lights[];
} lightBuffer;

layout(set = 0, binding = 4) uniform PointLightShadowCubeUBO {
  PointLightShadowCube cube[4];
} pointLightShadowCubeUBO;

layout(std430, set = 0, binding = 5) readonly buffer ClusterBuffer {
  Cluster clusters[];
} clusterBuffer;

layout(std430, set = 0, binding = 6) readonly buffer MaterialBuffer {
  MaterialInfo infos[];
} materialBuffer;

layout(std430, set = 0, binding = 7) readonly buffer RenderableMatIndexBuffer {
  uint indices[];
} rendMatIndexBuffer;

layout(std430, set = 0, binding = 8) readonly buffer RenderableModelBuffer {
  uint indices[];
} rendModelBuffer;

layout(std430, set = 0, binding = 9) readonly buffer IndexBuffer {
  uint indices[];
} indexBuffer;

layout(std430, set = 0, binding = 10) buffer VertexBuffer {
  PackedVertex vertices[];
} vertexBuffer;

layout(std430, set = 0, binding = 11) readonly buffer MeshBuffer {
  MeshInfo meshes[];
} meshBuffer;

// Tlas is binding 12

layout(std430, set = 0, binding = 13) readonly buffer SkeletonBuffer {
  mat4 joints[];
} skeletonBuffer;

layout(set = 0, binding = 14) uniform sampler2D textures[];

SurfaceData getSurfaceDataFromMat(MaterialInfo matInfo, vec2 uv, vec3 inNormal, mat3 inTBN, vec3 inTangent, vec3 inColor)
{
  SurfaceData outData;

  outData.normal = inNormal;
  outData.metallic = 0.1;
  outData.roughness = 0.9;
  outData.color = inColor;
  outData.emissive = vec3(0.0);

  int metRoughIdx = matInfo.bindlessIndices[0];
  int albedoIdx = matInfo.bindlessIndices[1];
  int normalIdx = matInfo.bindlessIndices[2];
  int emissiveIdx = matInfo.bindlessIndices[3];

  if (albedoIdx != -1) {
    vec4 samp = texture(textures[nonuniformEXT(albedoIdx)], uv);
    /*if (samp.a < 0.1) {
      discard;
    }*/
    outData.color = vec3(matInfo.baseColFac.r * samp.r, matInfo.baseColFac.g * samp.g, matInfo.baseColFac.b * samp.b);
    //color = samp.rgb;
  }
  else {
    // If no albedo texture, but baseColFactor is present, use the factor as linear RGB values
    if (matInfo.baseColFac.r + matInfo.baseColFac.g + matInfo.baseColFac.b > 0.1) {
      outData.color = matInfo.baseColFac.rgb;
    }
  }
  if (metRoughIdx != -1) {
    vec4 samp = texture(textures[nonuniformEXT(metRoughIdx)], uv);
    outData.metallic = samp.b;
    outData.roughness = samp.g;
  }
  if (normalIdx != -1 && inTangent.xyz != vec3(0.0f)) {
    outData.normal = normalize(texture(textures[nonuniformEXT(normalIdx)], uv).rgb * 2.0 - 1.0);
    outData.normal = normalize(inTBN * outData.normal);
  }
  if (emissiveIdx != -1) {
    outData.emissive = texture(textures[nonuniformEXT(emissiveIdx)], uv).rgb;
    outData.emissive.r *= matInfo.emissive.r;
    outData.emissive.g *= matInfo.emissive.g;
    outData.emissive.b *= matInfo.emissive.b;
  }
  else {
    outData.emissive = matInfo.emissive.rgb * matInfo.emissive.w;
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