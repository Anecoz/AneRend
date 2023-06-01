#extension GL_EXT_ray_tracing : enable

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

struct Vertex
{
  vec4 pos;
  vec4 color;
  vec4 normal;
  vec2 uv;
  vec4 tangent;
};

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
  Vertex vertices[];
} vertexBuffer;

layout(std430, set = 0, binding = 8) readonly buffer MeshBuffer {
  MeshInfo meshes[];
} meshBuffer;

layout(set = 0, binding = 9) uniform accelerationStructureEXT tlas;

layout(set = 0, binding = 10) uniform sampler2D textures[];