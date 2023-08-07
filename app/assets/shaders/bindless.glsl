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
};

struct MeshInfo
{
  uint vertexOffset; // in "number" of vertices, not bytes
  uint indexOffset; // in "number" of indices, not bytes
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

layout(std430, set = 0, binding = 2) readonly buffer RenderableBuffer {
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